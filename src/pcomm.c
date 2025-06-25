/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pcomm - command processor for pdos                               */
/*                                                                   */
/*  Note that pcomm is pretty much an ordinary C program, however    */
/*  if it is executed with /p it reads autoexec.bat and if it is     */
/*  executed with /c it runs a single command and then exits.        */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "pos.h"
#include "dostime.h"
#include "memmgr.h"
#include "support.h"

/* In C99 or higher we would just include <stdbool.h>. But, we need to
 * support older C compilers (C89/C90) which don't come with <stdbool.h>
 */
#define true 1
#define false 0
#define bool int

#define MAX_COMMAND_BUF 400
#define DOT_COM_EXTEND 4
#define DOT_EXE_EXTEND 4
#define DOT_BAT_EXTEND 4

/* Written By NECDET COKYAZICI, Public Domain */
void putch(int a);
void bell(void);
void safegets(char *buffer, int size, bool use_history);

/* Command history ring buffer */
#define MAX_COMMAND_HISTORY 32
static int first_command_history = 0;
static int last_command_history = 0;
static char oldbufs[MAX_COMMAND_HISTORY][MAX_COMMAND_BUF];

#define NEXT_HISTORY(idx) ((idx + 1) % MAX_COMMAND_HISTORY)
#define PREVIOUS_HISTORY(idx) ((idx + MAX_COMMAND_HISTORY - 1) % MAX_COMMAND_HISTORY)

static char buf[MAX_COMMAND_BUF];
#ifdef __32BIT__
/* note that this later gets updated to include the full path of
   the executable, so might want to add FILENAME_MAX to the size.
   And use a define for this value of 400 too */
static unsigned char cmdt[MAX_COMMAND_BUF];
#else
static unsigned char cmdt[140];
#endif
#define PATH_MAX 500
static char path[PATH_MAX] = ";" ; /* Used to store path */
static char origpath2[PATH_MAX] = "" ; /* Used to store original path */
static POSEXEC_PARMBLOCK
    parmblock = { 0, cmdt, NULL, NULL };
static size_t len;
static char drive[2] = "A";
static char cwd[65];
#define PROMPT_MAX 256
static char prompt[PROMPT_MAX];
static int singleCommand = 0;
static int primary = 0;
static int term = 0;

/* Name of currently running command (in uppercase) */
static char curCmdName[50];

/* GOTO target label */
static char gotoTarget[32];

/* Flag to trigger GOTO to be taken */
static int executeGoto = 0;

/* Echo each command in batch file */
static int echoFlag = 0;

/* Abort batch file if any command fails */
static int abortFlag = 0;

/* Flag indicating batch file currently running */
static int inBatch = 0;

/* FILE handle for current batch file (used by SAVE command) */
static FILE * currentBatchFp = NULL;

/* PDPCLIB defines this as pointer to environment */
/* If using msvcrt.dll then I assume we need our own */
#ifdef __WIN32__
static
#else
extern
#endif
char *__envptr;

/* Flag set to true if genuine PDOS detected */
static int genuine_pdos = 0;

/* Save the video state to restore it */
static pos_video_info savedVideoState;

static void parseArgs(int argc, char **argv);
static int processInput(bool save_in_history);
static void putPrompt(void);
static int doExec(char *b,char *p);
static int changedisk(int drive);
static int ins_strcmp(char *one, char *two);
static int ins_strncmp(char *one, char *two, size_t len);
static int readBat(char *fnm);
static int exists(char *fnm);
static int isBlankString(char *str);
static void showArgsUnsupportedMsg(void);
static void strtoupper(char *s);
static char *stringTrimBoth(char *s);

/*===== OPTION FACILITY =====*/
/* The "OPTION" command is used to view or set flags which
 * control system behaviour. Some flags may control PCOMM
 * behaviour while other flags control PDOS itself. The
 * options registry stores which options are known.
 */

/* Function signature for option get routine */
typedef int (*optGet)(void);

/* Function signature for option set routine */
typedef void (*optSet)(int flag);

/* An entry in the options registry */
typedef struct optBlock
{
    char key;
    char *name;
    char *description;
    optGet getProc;
    optSet setProc;
} optBlock;

/* Define option macro.
 * Parameters are:
 * key         - single uppercase character which uniquely identifies this
 *               option
 * name        - name of this option (lowercase, unquoted)
 * description - one-line description of what this option does
 */
#define OPTDEF(key,name,description) { key, #name, description, \
    opt_##name##_get, opt_##name##_set }

/* Sentinel record for end of command registry */
#define OPT_REGISTRY_END { 0, NULL, NULL, NULL, NULL }

/* Macro to generate prototypes for option get/set routines */
#define OPTPROTO(name) \
    static int opt_##name##_get(void); \
    static void opt_##name##_set(int)

/* Prototypes for option get/set routines */
OPTPROTO(abort);
OPTPROTO(break);
OPTPROTO(echo);
OPTPROTO(logunimp);
OPTPROTO(verify);

/* The options registry stores definitions of all options. */
static optBlock optRegistry[] =
{
    OPTDEF('A',abort,"Exit batch file on first failing command"),
    OPTDEF('B',break,"Check Ctrl+Break state more frequently"),
    OPTDEF('E',echo,"Echo each command executed in batch file"),
    OPTDEF('U',logunimp,"Log unimplemented DOS calls"),
    OPTDEF('V',verify,"Verify disk writes"),
    OPT_REGISTRY_END
};

/*===== COMMAND REGISTRY =====*/

/* Macro to generate prototypes for command do/help routines */
#define CMDPROTO(name) \
    static int cmd_##name##_run(char *arg); \
    static void cmd_##name##_help(void)

/* Prototypes for command do/help routines */
CMDPROTO(accmap);
CMDPROTO(attrib);
CMDPROTO(cd);
CMDPROTO(cls);
CMDPROTO(copy);
CMDPROTO(date);
CMDPROTO(del);
CMDPROTO(dir);
CMDPROTO(echo);
CMDPROTO(exit);
CMDPROTO(goto);
CMDPROTO(help);
CMDPROTO(keybmap);
CMDPROTO(loadfont);
CMDPROTO(mcd);
CMDPROTO(md);
CMDPROTO(monitor);
CMDPROTO(option);
CMDPROTO(path);
CMDPROTO(pause);
CMDPROTO(peek);
CMDPROTO(poweroff);
CMDPROTO(prompt);
CMDPROTO(ps);
CMDPROTO(rd);
CMDPROTO(reboot);
CMDPROTO(rehash);
CMDPROTO(rem);
CMDPROTO(ren);
CMDPROTO(save);
CMDPROTO(scanmap);
CMDPROTO(scrncap);
CMDPROTO(scrnmap);
CMDPROTO(set);
CMDPROTO(showret);
CMDPROTO(sleep);
CMDPROTO(time);
CMDPROTO(type);
CMDPROTO(unboot);
CMDPROTO(v);
CMDPROTO(ver);

/* Function signature for command implementation functions */
typedef int (*cmdProc)(char *arg);

/* Function signature for command help display functions */
typedef void (*cmdHelp)(void);

/* Defines an internal command */
typedef struct cmdBlock
{
    char *name;
    char *description;
    cmdProc proc;
    cmdHelp help;
}
cmdBlock;

/* Define command macro.
 * Parameters are:
 * name - name of this command (lowercase, unquoted)
 * aliases - empty string "" for no aliases. Otherwise each alias in string
 *           prefixed with pipe character.
 * description - one-line description of command for help system.
 */
#define CMDDEF(name,aliases,description) { #name aliases , description , \
    cmd_##name##_run, cmd_##name##_help }

/* Sentinel record for end of command registry */
#define CMD_REGISTRY_END { NULL, NULL, NULL, NULL }

/* The command registry stores definitions of all internal commands.
 */
static cmdBlock cmdRegistry[] =
{
    CMDDEF(accmap,"","Load an accent map"),
    CMDDEF(attrib,"","View or change file attributes"),
    CMDDEF(cd,"|chdir","Changes the current directory"),
    CMDDEF(cls,"","Clears the screen"),
    CMDDEF(copy,"","Copies files and directories"),
    CMDDEF(date,"","Shows the date"),
    CMDDEF(del,"|erase","Deletes files"),
    CMDDEF(dir,"","Lists files and directories"),
    CMDDEF(echo,"","Displays a message"),
    CMDDEF(exit,"","Exits PCOMM"),
    CMDDEF(goto,"","GOTO label in batch file"),
    CMDDEF(help,"","Provides information about PDOS commands"),
    CMDDEF(keybmap,"","Remaps the keyboard"),
    CMDDEF(loadfont,"","Loads 8 * 14 fonts"),
    CMDDEF(mcd,"","Make new directory and change to it"),
    CMDDEF(md,"|mkdir","Creates new directories"),
    CMDDEF(monitor,"","Enter the system monitor"),
    CMDDEF(option,"","Controls behavioural flags"),
    CMDDEF(path,"","Displays or modifies PATH variable"),
    CMDDEF(pause,"","Wait for user to press any key"),
    CMDDEF(peek,"","Peek at some memory location"),
    CMDDEF(poweroff,"","Powers off the computer"),
    CMDDEF(prompt,"","Displays or modifies PROMPT variable"),
    CMDDEF(ps,"","Shows running processes"),
    CMDDEF(rd,"|rmdir","Removes directories"),
    CMDDEF(reboot,"","Reboots the computer"),
    CMDDEF(rehash,"","Clears the cache of a drive"),
    CMDDEF(rem,"","Comment in a batch file"),
    CMDDEF(ren,"|rename","Renames files and directories"),
    CMDDEF(save,"","Saves user input to file"),
    CMDDEF(scanmap,"","Map scancodes to ASCII"),
    CMDDEF(scrncap,"","Start/stop screen capture"),
    CMDDEF(scrnmap,"","Maps screen output"),
    CMDDEF(set,"","Show/modify environment variables"),
    CMDDEF(showret,"","Show return codes"),
    CMDDEF(sleep,"","Sleep for some seconds"),
    CMDDEF(time,"","Shows the time"),
    CMDDEF(type,"","Reads and displays a text file"),
    CMDDEF(unboot,"","Marks a disk as non-bootable"),
    CMDDEF(v,"","Video control"),
    CMDDEF(ver,"","Displays the current version of PDOS"),
    CMD_REGISTRY_END
};

/*===== PROMPT SYMBOL REGISTRY =====*/

/* Function signature for PROMPT symbol display func */
typedef void (*promptSymProc)(char *prompt, int *index);

/* Macro to generate prototype for PROMPT symbol procedure */
#define PROMPTSYMPROTO(name) \
    static void promptSymProc_##name(char *prompt, int *index)

/* PROMPT symbol procedure prototypes */
PROMPTSYMPROTO(dollar);
PROMPTSYMPROTO(B);
PROMPTSYMPROTO(D);
PROMPTSYMPROTO(E);
PROMPTSYMPROTO(G);
PROMPTSYMPROTO(H);
PROMPTSYMPROTO(L);
PROMPTSYMPROTO(N);
PROMPTSYMPROTO(P);
PROMPTSYMPROTO(Q);
PROMPTSYMPROTO(T);
PROMPTSYMPROTO(V);
PROMPTSYMPROTO(X);
PROMPTSYMPROTO(underscore);
PROMPTSYMPROTO(lbracket);

/* Defines a prompt symbol */
typedef struct promptSym
{
    char symbol;
    char *description;
    promptSymProc proc;
}
promptSym;

/* Define prompt symbol macro.
 * Parameters are:
 * symbol - single uppercase character (quoted)
 * name - name of prompt symbol (unquoted)
 * description - one-line description of prompt symbol for help system.
 */
#define PROMPTSYMDEF(symbol,name,description) { symbol , description , \
    promptSymProc_##name }

/* Sentinel record for end of prompt symbol registry */
#define PROMPT_SYM_REGISTRY_END { 0, NULL, NULL }

/* The prompt symbol registry stores definitions of all supported prompt
 * symbols.
 */
static promptSym promptSymRegistry[] =
{
    PROMPTSYMDEF('$',dollar,"$ (dollar sign)"),
    PROMPTSYMDEF('B',B,"| (pipe)"),
    PROMPTSYMDEF('D',D,"Current date"),
    PROMPTSYMDEF('E',E,"ASCII escape code (decimal 27)"),
    PROMPTSYMDEF('G',G,"> (greater-than sign)"),
    PROMPTSYMDEF('H',H,"ASCII backspace code (decimal 8)"),
    PROMPTSYMDEF('L',L,"< (less-than sign)"),
    PROMPTSYMDEF('N',N,"Current drive"),
    PROMPTSYMDEF('P',P,"Current drive and directory"),
    PROMPTSYMDEF('Q',Q,"= (equal sign)"),
    PROMPTSYMDEF('T',T,"Current time"),
    PROMPTSYMDEF('V',V,"DOS version number"),
    PROMPTSYMDEF('X',X,"Current PID"),
    PROMPTSYMDEF('_',underscore,"CR+LF"),
    PROMPTSYMDEF('[',lbracket,"Run command terminated by ]"),
    PROMPT_SYM_REGISTRY_END
};

/* utility function - check if string is blank */
static bool isBlankString(char *str)
{
    while (*str != 0)
    {
        if (!isspace((unsigned char)*str))
        {
            return false;
        }
        str++;
    }
    return true;
}

/* utility function - show an error if command doesn't accept arguments */
static void showArgsUnsupportedMsg(void)
{
    printf("ERROR: Command '%s' does not accept any arguments\n", curCmdName);
}

/* utility function - show an error if command requires arguments */
static void showArgsRequiredMsg(void)
{
    printf("ERROR: Command '%s' requires arguments, but none supplied\n",
            curCmdName);
}

/* utility function - show an error if command requires genuine PDOS */
static void showGenuineRequiredMsg(void)
{
    printf("ERROR: Command '%s' requires genuine PDOS, which is not detected\n",
            curCmdName);
}

/* utility function - show an error if command argument syntax incorrect */
static void showArgBadSyntaxMsg(char *argName)
{
    printf("ERROR: Command '%s' argument '%s' syntax incorrect\n",
            curCmdName, argName);
}

/* utility function - show an error if command argument unknown */
static void showUnrecognisedArgsMsg(char *args)
{
    printf("ERROR: Unrecognized arguments '%s' to %s command\n",
            args, curCmdName);
}

/* utility macro - fail command if non-blank arguments supplied */
#define CMD_HAS_NO_ARGS(args) \
    do { \
        if (!isBlankString(args)) { \
            showArgsUnsupportedMsg(); \
            return 1; \
        } \
    }  while (0)

/* utility macro - fail command if non-blank arguments missing */
#define CMD_REQUIRES_ARGS(args) \
    do { \
        if (isBlankString(args)) { \
            showArgsRequiredMsg(); \
            return 1; \
        } \
    }  while (0)

/* utility macro - fail command if not genuine PDOS */
#define CMD_REQUIRES_GENUINE() \
    do { \
        if (!genuine_pdos) { \
            showGenuineRequiredMsg(); \
            return 1; \
        } \
    }  while (0)

/* Searches command registry for command with given name */
cmdBlock *findCommand(char *cmdName)
{
    char tmp[128];
    char *p1, *p2;
    int i;
    cmdBlock *block;

    /* Ignore initial whitespace */
    while (isspace((unsigned char)*cmdName))
    {
        cmdName++;
    }

    /* Ignore final whitespace */
    for (i = 0; cmdName[i] != 0; i++)
    {
        if (isspace((unsigned char)(cmdName[i])))
        {
            cmdName[i] = 0;
            break;
        }
    }

    /* Search the registry for the command */
    for (i = 0; cmdRegistry[i].name != NULL; i++)
    {
        block = &(cmdRegistry[i]);
        if (strchr(block->name,'|') == NULL)
        {
            if (ins_strcmp(block->name,cmdName) == 0)
            {
                return block;
            }
        }
        else
        {
            strcpy(tmp,block->name);
            p1 = tmp;
            while (1)
            {
                p2 = strchr(p1,'|');
                if (p2 == NULL)
                {
                    if (ins_strcmp(p1,cmdName) == 0)
                    {
                        return block;
                    }
                    break;
                }
                *p2 = 0;
                p2++;
                if (ins_strcmp(p1,cmdName) == 0)
                {
                    return block;
                }
                p1 = p2;
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    char *sPROMPT;
    char *origpath;
#ifdef USING_EXE
    pdosRun();
#endif
    /* Save whether this is genuine PDOS or not */
    genuine_pdos = (PosGetMagic() == PDOS_MAGIC);
    /* Save video state at startup */
    if (genuine_pdos)
    {
        PosGetVideoInfo(&savedVideoState, sizeof(pos_video_info));
    }
    origpath = getenv("PATH");
    if (origpath != NULL)
    {
        if (strlen(origpath) < PATH_MAX)
        {
            strcpy(path, origpath);
            strcpy(origpath2, origpath);
        }
    }
    if (genuine_pdos)
    {
         PosSetEnv("PATH",path);
         __envptr = PosGetEnvBlock();
    }
    sPROMPT = getenv("PROMPT");
    if (sPROMPT != NULL)
    {
        strncpy(prompt,sPROMPT,PROMPT_MAX-1);
    }
    else
    {
        /* Support colorful prompt.
         * Sometimes I test PCOMM under MS-DOS.
         * And then I get confused about whether I am in PCOMM or COMMAND.COM.
         * Making the default prompt for PCOMM different from that of
         * COMMAND.COM helps avoid that confusion.
         * Also, lets have a colorful prompt, because we can.
         * - Simon Kissane
         */
        if (genuine_pdos)
        {
            strncpy(prompt,"$_$[V SAVE=FG FG=14]$P]$[V RESTORE=FG]",
                    PROMPT_MAX-1);
            PosSetEnv("PROMPT",prompt);
            __envptr = PosGetEnvBlock();
        }
        else
        {
            strncpy(prompt,"$_$P]",PROMPT_MAX-1);
        }
    }
    /* Parse our arguments */
    parseArgs(argc, argv);
    if (singleCommand)
    {
        int rc;

        rc = processInput(false);
        if (genuine_pdos)
        {
             if (origpath != NULL)
             {
                 PosSetEnv("PATH",origpath2);
                 __envptr = PosGetEnvBlock();
             }
        }
        return (rc);
    }
    if (primary)
    {
        if (genuine_pdos) PosSetVideoAttribute(0x0A);
        printf("Welcome to PCOMM\n");
        if (genuine_pdos) PosSetVideoAttribute(savedVideoState.currentAttrib);
        readBat("AUTOEXEC.BAT");
    }
    else
    {
        if (genuine_pdos) PosSetVideoAttribute(0x0A);
        printf("Welcome to PCOMM - EXIT to return\n");
        if (genuine_pdos) PosSetVideoAttribute(savedVideoState.currentAttrib);
    }
    while (!term)
    {
        putPrompt();
        safegets(buf, MAX_COMMAND_BUF, true);

        processInput(true);
    }
    if (genuine_pdos) PosSetVideoAttribute(0x0C);
    printf("Thank you for using PCOMM!\n");
    if (genuine_pdos) PosSetVideoAttribute(savedVideoState.currentAttrib);
    return (0);
}

static void parseArgs(int argc, char **argv)
{
    int x;

    if (argc > 1)
    {
        if ((argv[1][0] == '-') || (argv[1][0] == '/'))
        {
            if ((argv[1][1] == 'C') || (argv[1][1] == 'c'))
            {
                singleCommand = 1;
            }
            if ((argv[1][1] == 'P') || (argv[1][1] == 'p'))
            {
                primary = 1;
            }
        }
    }
    if (singleCommand)
    {
        strcpy(buf, "");
        for (x = 2; x < argc; x++)
        {
            strcat(buf, *(argv + x));
            strcat(buf, " ");
        }
        len = strlen(buf);
        if (len > 0)
        {
            len--;
            buf[len] = '\0';
        }
    }
    return;
}

static int processInput(bool save_in_history)
{
    char *p;
    cmdBlock *block;
    int ret;

    char new_buf[MAX_COMMAND_BUF];
    struct {
        char *name;
        int to_handle_num;
        int saved_handle;
        enum {
            MODE_NONE = 0,
            MODE_TO_FILE,
            MODE_TO_HANDLE,
            MODE_APPEND,
            MODE_FROM_FILE,
            MODE_FROM_HANDLE
        } mode;
    } redirects[10] = {{NULL}};
    int i;
    const char *error_msg = NULL;

    /* Remove newline at end of buffer */
    len = strlen(buf);
    if ((len > 0) && (buf[len - 1] == '\n'))
    {
        len--;
        buf[len] = '\0';
    }

    /* If command line is blank, do nothing */
    if (isBlankString(buf))
    {
        return 0;
    }

    if(save_in_history)
    {
        strcpy(oldbufs[last_command_history], buf);
        last_command_history = NEXT_HISTORY(last_command_history);

        if(last_command_history == first_command_history)
        {
            first_command_history = NEXT_HISTORY(first_command_history);
        }
    }

    /* Labels should be checked early as they skip command redirection. */
    if (buf[0] == ':')
    {
        /* Labels are ignored here */
        return 0;
    }

    {
#define SKIP_SPACES(a) do { while (*(a) == ' ') (a)++; } while (0)
        char *src, *dst;

        src = buf;
        SKIP_SPACES (src);

        dst = new_buf;
        while (*src) {
            int handle_num = -1;

            if (*src == '^') {
                src++;
                goto copy;
            }

            if (isdigit (*src)
                && (src[1] == '<' || src[1] == '>')) {
                handle_num = *src - '0';
                src++;
            }

            if (*src == '<' || *src == '>') {
                if (*src == '<') {
                    if (handle_num == -1) handle_num = 0;
                    redirects[handle_num].mode = MODE_FROM_FILE;
                    if (src[1] == '&') {
                        redirects[handle_num].mode = MODE_FROM_HANDLE;
                        *src = '\0';
                        src++;
                    }
                } else {
                    if (handle_num == -1) handle_num = 1;
                    redirects[handle_num].mode = MODE_TO_FILE;
                    if (src[1] == '&') {
                        redirects[handle_num].mode = MODE_TO_HANDLE;
                        *src = '\0';
                        src++;
                    } else if (src[1] == '>') {
                        redirects[handle_num].mode = MODE_APPEND;
                        *src = '\0';
                        src++;
                    }
                }

                *src = '\0';
                src++;

                SKIP_SPACES (src);

                if (redirects[handle_num].mode == MODE_TO_HANDLE
                    || redirects[handle_num].mode == MODE_FROM_HANDLE) {
                    long lhandle;

                    lhandle = strtol (src, &p, 10);
                    if (lhandle == 0 && src == p) {
                        fprintf (stderr,
                             "The syntax of the command is incorrect.\n");
                        return 1;
                    }

                    redirects[handle_num].to_handle_num = (int)lhandle;
                } else {
                    for (p = src;
                         *p && *p != '>' && *p != '<' && *p != ' ';
                         p++) {}
                    if (p == src) {
                        fprintf (stderr,
                                 "The syntax of the command is incorrect.\n");
                        return 1;
                    }

                    redirects[handle_num].name = src;
                }

                src = p;
                if (*src == ' ') {
                    *src = '\0';
                    src++;
                    SKIP_SPACES (src);
                }
            } else {
        copy:
                *dst = *src;
                src++;
                dst++;
            }
        }

        *dst = '\0';
    }

    /* Redirection without a command is not permitted. */
    if (!new_buf[0]) {
        fprintf (stderr, "The syntax of the command is incorrect.\n");
        return 1;
    }

    /* C library does not know handles are being manipulated,
     * so it needs to be told to flush buffers.
     */
    fflush (stdout);
    fflush (stderr);

    /* Errors that occur during redirection cannot be reported immediately
     * as stderr might have been already redirected.
     */
    for (i = 0; i < sizeof (redirects) / sizeof (redirects[0]); i++) {
        if (redirects[i].mode) {
            int temp_handle;

            if (genuine_pdos) {
                printf ("Command redirection is not "
                        "supported on genuine PDOS.\n");
                break;
            }

            switch (redirects[i].mode) {
                case MODE_TO_FILE:
                    if (PosCreatFile (redirects[i].name, 0, &temp_handle)) {
                        error_msg = "Failed to create file.";
                        goto end;
                    }
                    break;

                case MODE_TO_HANDLE:
                    temp_handle = redirects[i].to_handle_num;
                    break;

                case MODE_APPEND:
                    error_msg = "'>>' is not yet supported.";
                    goto end;

                case MODE_FROM_FILE:
                    /* '<' does not provide the file directly,
                     * it "type"s it and provides the output.
                     * (So '<' can be done with same file
                     *  multiple times for one command.)
                     * It could be probably implemented using
                     * Int 21/AH=5Ah CREATE TEMPORARY FILE
                     * but that is not yet in pos.c.
                     */
                    error_msg = "'<' is not yet supported.";
                    goto end;

                case MODE_FROM_HANDLE:
                    error_msg = "'<&' is not yet supported.";
                    goto end;
            }

            /* Handles 3-9 are undefined, so they should not be redirected
             * but they still should create a file when '>' is used.
             */
            if (i < 3) {
                redirects[i].saved_handle = PosDuplicateFileHandle (i);

                if (PosForceDuplicateFileHandle (temp_handle, i)) {
                    error_msg = "Failed to duplicate handle.";
                    goto end;
                }
            }

            if (redirects[i].mode != MODE_TO_HANDLE) {
                PosCloseFile (temp_handle);
            }
        }
    }

    len = strlen(new_buf);
    /* Split command and arguments */
    p = strchr(new_buf, ' ');
    if (p != NULL)
    {
        *p++ = '\0';
    }
    else
    {
        p = new_buf + len;
    }
    len -= (size_t)(p - new_buf);

    /* Check for special case syntax first */
    if (ins_strncmp(new_buf, "cd.", 3) == 0)
    {
        ret = cmd_cd_run(new_buf + 2);
    }
    else if (ins_strncmp(new_buf, "cd\\", 3) == 0)
    {
        ret = cmd_cd_run(new_buf + 2);
    }
    else if ((strlen(new_buf) == 2) && (new_buf[1] == ':'))
    {
        ret = changedisk(new_buf[0]);
    }
    else
    {
        /* Look for internal command in registry */
        block = findCommand(new_buf);
        if (block != NULL)
        {
            /* Command was found, call implementation if present */
            if (block->proc == NULL)
            {
                printf("ERROR: No implementation found for command '%s'\n",
                        new_buf);
                ret = 1;
            }
            else
            {
                /* Populate curCmdName (for use in error messages) */
                strcpy(curCmdName, new_buf);
                strtoupper(curCmdName);
                /* Call implementation */
                ret = block->proc(p);
            }
        }
        else
        {
            /* No internal command found, presume external command */
            ret = doExec(new_buf,p);
        }
    }

end:
    fflush (stdout);
    fflush (stderr);

    for (i = 0; i < sizeof (redirects) / sizeof (redirects[0]); i++) {
        if (redirects[i].mode) {
            if (genuine_pdos) break;

            if (redirects[i].saved_handle) {
                if (PosForceDuplicateFileHandle (redirects[i].saved_handle,
                                                 i)) {
                    error_msg = "Failed to restore handle.";
                }
                PosCloseFile (redirects[i].saved_handle);
            }
        }
    }

    if (error_msg) {
        fprintf (stderr, "%s\n", error_msg);
        ret = 1;
    }

    return ret;
}

/* $$ - prints dollar sign */
static void promptSymProc_dollar(char *prompt, int *index)
{
    fputc('$', stdout);
}

/* $B - prints pipe symbol */
static void promptSymProc_B(char *prompt, int *index)
{
    fputc('|', stdout);
}

/* $D - prints current date */
static void promptSymProc_D(char *prompt, int *index)
{
    unsigned int y, m, d, dw;
    PosGetSystemDate(&y,&m,&d,&dw);
    printf("%04u-%02u-%02u", y, m, d);
}

/* $E - prints escape character */
static void promptSymProc_E(char *prompt, int *index)
{
    fputc(27, stdout);
}

/* $G - prints greater than sign */
static void promptSymProc_G(char *prompt, int *index)
{
    fputc('>', stdout);
}

/* $H - prints backspace */
static void promptSymProc_H(char *prompt, int *index)
{
    fputc('\b', stdout);
}

/* $L - prints less-than sign */
static void promptSymProc_L(char *prompt, int *index)
{
    fputc('<', stdout);
}

/* $N - prints current drive letter */
static void promptSymProc_N(char *prompt, int *index)
{
    fputc(PosGetDefaultDrive() + 'A', stdout);
}

/* $P - prints current drive and directory */
static void promptSymProc_P(char *prompt, int *index)
{
    PosGetCurDir(0, cwd);
    printf("%c:\\%s", 'A' + PosGetDefaultDrive(), cwd);
}

/* $Q - prints equal sign */
static void promptSymProc_Q(char *prompt, int *index)
{
    fputc('=', stdout);
}

/* $T - prints time */
static void promptSymProc_T(char *prompt, int *index)
{
    unsigned int hr, min, sec, hund;
    PosGetSystemTime(&hr,&min,&sec,&hund);
    printf("%02u:%02u:%02u", hr, min, sec);
}

/* $V - prints DOS version number */
static void promptSymProc_V(char *prompt, int *index)
{
    int ver, major, minor;
    ver = PosGetDosVersion();
    minor = (ver & 0xFF);
    major = ((ver>>8)&0xFF);
    printf("DOS %d.%d", major, minor);
}

/* $X - print current PID */
static void promptSymProc_X(char *prompt, int *index)
{
    printf("%X",PosGetCurrentProcessId());
}

/* $_ - prints underscore symbol */
static void promptSymProc_underscore(char *prompt, int *index)
{
    fputs("\r\n", stdout);
}

/* Runs command */
static int runCmdLine(char *cmdLine)
{
    int rc;
    char savebuf[MAX_COMMAND_BUF];

    strcpy(savebuf, buf);
    strcpy(buf, cmdLine);
    rc = processInput(false);
    strcpy(buf, savebuf);
    return rc;
}

/* $[ - run command terminated by ] */
static void promptSymProc_lbracket(char *prompt, int *index)
{
    char cbuf[MAX_COMMAND_BUF];
    int j = 0;
    while (prompt[*index] != 0 && prompt[*index] != ']')
    {
        cbuf[j++] = prompt[*index];
        (*index)++;
    }
    cbuf[j] = 0;
    fflush(stdout);
    if (runCmdLine(cbuf+1) != 0)
    {
        printf("\n\n");
    }
    fflush(stdout);
}

static void putPromptSymbol(char ch, char *prompt, int *index)
{
    int i;
    ch = toupper((unsigned char)ch);
    for (i = 0; promptSymRegistry[i].symbol != 0; i++)
    {
        if (promptSymRegistry[i].symbol == ch)
        {
            promptSymRegistry[i].proc(prompt, index);
            return;
        }
    }
    /* Unrecognised symbols are ignored */
}

static void putPrompt(void)
{
    int i;

    /* Process each PROMPT symbol */
    for (i = 0; prompt[i] != 0; i++)
    {
        /* Character not part of prompt symbol, output as-is */
        if (prompt[i] != '$')
        {
            fputc(prompt[i], stdout);
        }

        /* Character beginning of prompt symbol, call output routine */
        else if (prompt[i+1] != 0)
        {
            i++;
            putPromptSymbol(prompt[i], prompt, &i);
        }
    }
    fflush(stdout);
    return;
}

static void errorBadArgs(void)
{
    printf("ERROR: Bad arguments to %s command\n", curCmdName);
}

static void showError(int ret)
{
    char *msg = NULL;
    /* No error, nothing to display */
    if (ret == POS_ERR_NO_ERROR)
    {
        return;
    }
    /* Don't try to get error message if not genuine PDOS,
       will return rubbish */
    if (genuine_pdos)
    {
        /* Get error message */
        msg = PosGetErrorMessageString(ret);
    }
    /* NULL means unknown error */
    if (msg == NULL)
    {
        printf("ERROR: Operation failed due to error code %d "
                "(no message available)\n", ret);
    }
    /* Otherwise display the message */
    else
    {
        printf("ERROR: Operation failed due to error %s (error code %d)\n",
                msg, ret);
    }
}

static int cmd_exit_run(char *options)
{
    int doTSR = 0;

    /* Argument parsing code - /TSR only allowed option at present */
    if (!isBlankString(options))
    {
        options = stringTrimBoth(options);
        if (ins_strcmp(options,"/TSR") == 0)
        {
            doTSR = 1;
        }
        else
        {
            errorBadArgs();
            return 1;
        }
    }

    /* Handle exit with /TSR flag */
    if (doTSR)
    {
        if (primary)
        {
            printf("ERROR: EXIT /TSR not allowed from primary shell\n");
            return 1;
        }
        /* -1 is PDOS extension, means don't free any memory */
        PosTerminateAndStayResident(0, -1);
        printf("ERROR: PosTerminateAndStayResident failed\n");
        return 1;
    }

    /* Handle non-TSR exit */
#ifdef CONTINUOUS_LOOP
    primary = 0;
#endif
    if (!primary)
    {
        term = 1;
    }
    return 0;
}

static int cmd_echo_run(char *msg)
{
    printf("%s\n", msg);
    return 0;
}

static int cmd_poweroff_run(char *ignored)
{
    CMD_HAS_NO_ARGS(ignored);
    PosPowerOff();
    /* if we return from PosPowerOff(), we know it failed */
    printf("ERROR: Power off failed\n");
    return 1;
}

static int cmd_reboot_run(char *ignored)
{
    CMD_HAS_NO_ARGS(ignored);
    PosReboot();
    /* if we return from PosReboot(), we know it failed */
    printf("ERROR: Reboot failed\n");
    return 1;
}

static int cmd_rehash_run(char *drive)
{
    CMD_REQUIRES_ARGS(drive);
    PosRehash((toupper(drive[0])) - 'A');
    printf("cache hopefully cleared\n");
    return (0);
}

static int cmd_type_run(char *file)
{
    FILE *fp;
    int p = 0, l = 0, count = 1, pa = 1;
    CMD_REQUIRES_ARGS(file);

    /* Collect optional /L and /P arguments */
    file = stringTrimBoth(file);
    for (;;)
    {
        if (ins_strncmp(file,"/L ",3) == 0)
        {
            file += 3;
            l = 1;
            file = stringTrimBoth(file);
            continue;
        }
        if (ins_strncmp(file,"/P ",3) == 0)
        {
            file += 3;
            p = 1;
            file = stringTrimBoth(file);
            continue;
        }
        break;
    }

    /* Ensure we have an actual file name (not just /L /P options) */
    CMD_REQUIRES_ARGS(file);

    /* Open the file */
    fp = fopen(file, "r");

    if (fp != NULL)
    {
        while (fgets(buf, MAX_COMMAND_BUF, fp) != NULL)
        {
            /* Print line numbers if requested by /L */
            if (l)
            {
                printf("%d ", count);
            }

            /* Print the line */
            fputs(buf, stdout);

            /* Increment line number count */
            count++;

            /* Process paging */
            pa++;
            if (pa == 24)
            {
                pa = 1;
                if (p)
                {
                    cmd_pause_run("");
                }
            }
        }
        fclose(fp);
    }

    else
    {
       printf("file not found: %s\n", file);
       return 1;
    }

    return 0;
}

static int cmd_del_run(char *fnm)
{
    int ret;
    DTA *dta;
    CMD_REQUIRES_ARGS(fnm);

    dta = PosGetDTA();
    ret = PosFindFirst(fnm,0x10);

    if(ret == 2)
    {
        printf("File not found \n");
        printf("No Files Deleted \n");
        return 1;
    }
    while(ret == 0)
    {
        PosDeleteFile(dta->file_name);
        ret = PosFindNext();
    }
    return 0;
}

static int cmd_scrncap_run(char *drive)
{
    int drivnum;

    CMD_REQUIRES_ARGS(drive);

    if (ins_strcmp(drive, "off") == 0)
    {
        drivnum = -1;
    }
    else
    {
        drivnum = (int)strtol(drive, NULL, 16);
    }
    PosScrncap(drivnum);
    return (0);
}

static int cmd_monitor_run(char *ignored)
{
    CMD_HAS_NO_ARGS(ignored);
    PosMonitor();
    return 0;
}

static int cmd_dir_run(char *pattern)
{
    DTA *dta;
    int ret;
    char *p;
    time_t tt;
    struct tm *tms;

    dta = PosGetDTA();
    if (isBlankString(pattern))
    {
        p = "*.*";
    }
    else
    {
        p = pattern;
    }
    ret = PosFindFirst(p, 0x10);
    if(ret == 2)
    {
        printf("No files found\n");
        return 1;
    }
    if (ret != 0 && ret != POS_ERR_NO_MORE_FILES) {
        showError(ret);
        return 1;
    }
    while (ret == 0)
    {
        char dirdisp[6] = "";

        if ((dta->attrib & FILE_ATTR_DIRECTORY) != 0)
        {
            strcpy(dirdisp, "<DIR>");
        }
        tt = dos_to_timet(dta->file_date,dta->file_time);
        tms = localtime(&tt);
        printf("%-13s %9ld %5.5s %04d-%02d-%02d %02d:%02d:%02d %s\n",
               dta->file_name,
               dta->file_size,
               dirdisp,
               tms->tm_year + 1900,
               tms->tm_mon + 1,
               tms->tm_mday,
               tms->tm_hour,
               tms->tm_min,
               tms->tm_sec,
               /* We only print LFN if genuine PDOS. So if testing under
                * some DOS that doesn't support LFNs, we don't print garbage.
                * (Actually maybe check should be if current DOS supports
                * LFNs, i.e. INT 21,71, but PDOS doesn't implement that yet.)
                */
               genuine_pdos ? (char *)dta->lfn : "");
        ret = PosFindNext();
    }
    return 0;
}

#ifdef __32BIT__
#define PDOS_FLAVOR "PDOS-32"
#else
#define PDOS_FLAVOR "PDOS-16"
#endif

/* Trim whitespace from end of string */
static char *stringTrimRight(char *s)
{
    while (strlen(s) > 0 && isspace((unsigned char)(s[strlen(s)-1])))
    {
        s[strlen(s)-1] = 0;
    }
    return s;
}

/* Trim whitespace from start of string */
static char *stringTrimLeft(char *s)
{
    while (*s != 0 && isspace((unsigned char)*s))
    {
        s++;
    }
    return s;
}

/* Trim whitespace from both ends of string */
static char *stringTrimBoth(char *s)
{
    return stringTrimLeft(stringTrimRight(s));
}

static void errorBatchOnly(void)
{
    printf("ERROR: Command %s allowed in batch file only\n", curCmdName);
}

static void errorBadDosVersion(void)
{
    printf("ERROR: Bad version number in VER DOS=\n");
}

/* Left shift by 6 bits converts units of paragraphs (16 bytes)
 * to KB (1024 bytes). */
#define PARAS_TO_KB(paras) ((paras)>>6)

static int cmd_ver_run(char *arg)
{
    int ver, major, minor;
    MEMMGRSTATS stats;
    pos_video_info videoInfo;
    unsigned long ticks;

    /* Only supported param is DOS=x.yy to set DOS version */
    if (!isBlankString(arg))
    {
        /* Trim whitespace */
        arg = stringTrimBoth(arg);

        /* Ensure we have DOS= argument (other args not supported */
        if (ins_strncmp(arg, "DOS=", 4) != 0)
        {
            errorBadArgs();
            return 1;
        }
        /* Strip the "DOS=" prefix */
        arg += 4;
        /* Expect start of major version */
        if (!isdigit((unsigned char)*arg))
        {
            errorBadDosVersion();
            return 1;
        }
        /* Parse major version */
        major = strtol(arg, &arg, 10);
        /* Expect dot (.) */
        if (arg[0] != '.' || !isdigit((unsigned char)(arg[1])))
        {
            errorBadDosVersion();
            return 1;
        }
        /* Skip dot */
        arg++;
        /* Parse minor version */
        minor = strtol(arg, &arg, 10);
        /* Arg should now point to NUL. If not, there is junk after minor */
        if (arg[0] != 0)
        {
            errorBadDosVersion();
            return 1;
        }
        /* Okay, we have major and minor, construct version */
        ver = ((minor&0xFF)<<8) | (major&0xFF);
        /* Set DOS version */
        PosSetDosVersion(ver);
        /* Read back and compare to confirm it worked */
        ver = PosGetDosVersion();
        if ((major != ((ver>>8)&0xFF)) || (minor != (ver & 0xFF)))
        {
            printf("ERROR: Setting DOS version failed\n");
            return 1;
        }
        printf("SUCCESS: Set reported DOS version to %d.%d\n", major, minor);
        return 0;
    }

    ver = PosGetDosVersion();
    minor = (ver & 0xFF);
    major = ((ver>>8)&0xFF);
    printf("PCOMM for " PDOS_FLAVOR "\n");
    printf("Reporting DOS version %d.%d\n", major, minor);
    printf("PCOMM built at %s %s\n", __DATE__, __TIME__);
    printf("Booted from drive %c\n", (PosGetBootDrive() - 1 + 'A'));
    printf("Current process is %04X\n", PosGetCurrentProcessId());

    /* Test if this is genuine PDOS and not something else like MS-DOS.
     * No point calling PDOS extension APIs if not really PDOS, they will
     * return meaningless values.
     */
    if (!genuine_pdos)
    {
        printf("WARNING: PDOS not detected, "
                "PCOMM running under another DOS\n");
        printf("Some features of PCOMM may not work.\n");
        return 0;
    }
    printf("Detected genuine " PDOS_FLAVOR "\n");

    /* Get and display memory allocation statistics */
    PosGetMemoryManagementStats(&stats);
    /* Weird printf bug, only first two %d work, the third becomes 0.
     * Workaround is split printf calls up */
    printf("%lu KB memory free in %ld blocks",
           (unsigned long)PARAS_TO_KB(stats.totalFree), stats.countFree);
    printf(" (largest free block %lu KB)\n",
           (unsigned long)PARAS_TO_KB(stats.maxFree));
    printf("%lu KB memory allocated in %ld blocks",
           (unsigned long)PARAS_TO_KB(stats.totalAllocated),
           stats.countAllocated);
    printf(" (largest allocated block %lu KB)\n",
           (unsigned long)PARAS_TO_KB(stats.maxAllocated));
    printf("%lu KB memory total in %ld blocks",
           (unsigned long)PARAS_TO_KB(stats.totalAllocated+stats.totalFree),
           stats.countAllocated+stats.countFree);
    printf(" (largest block %lu KB)\n",
           (unsigned long)PARAS_TO_KB(stats.maxAllocated > stats.maxFree ?
                                      stats.maxAllocated : stats.maxFree));
    /* Show video subsystem info */
    PosGetVideoInfo(&videoInfo, sizeof(pos_video_info));
    printf("Video mode %Xh (%ux%u), page %u\n",
            videoInfo.mode,
            videoInfo.cols,
            videoInfo.rows,
            videoInfo.page);
    /* Show tick count */
    ticks = PosGetClockTickCount();
    printf("Tick count %lu\n", ticks);
    return 0;
}

static int cmd_path_run(char *s)
{
    if (isBlankString(s))
    {
        char *t;

        t = path;

        if (*t == ';')
        {
            t++;
        }

        if (*t == '\0')
        {
            printf("No Path defined\n");
        }

        else
        {
            printf("%s\n", t);
        }
    }
    else
    {
        if (*s == ';')
        {
            strcpy(path, s);
        }
        else
        {
            strcpy(path, ";");
            strcat(path, s);
        }
        if (genuine_pdos)
        {
             PosSetEnv("PATH",path);
             __envptr = PosGetEnvBlock();
        }
    }
    return 0;
}

static int cmd_prompt_run(char *s)
{
     if (isBlankString(s))
     {
        printf("Current PROMPT is %s\n", prompt);
     }

     else
     {
        strncpy(prompt, s, PROMPT_MAX-1);
        if (genuine_pdos)
        {
            PosSetEnv("PROMPT",prompt);
            __envptr = PosGetEnvBlock();
        }
     }
     return 0;
}

static int doExec(char *b,char *p)
{
    char *s;
    char *t;
    size_t ln;
    size_t ln2;
    size_t ln3;
    size_t ln4;
    int err;
    char tempbuf[FILENAME_MAX];
    char *z;

    s = path;
    ln = strlen(p);
    ln3 = strlen(b);
#ifndef __32BIT__
    if (ln >= 0x7e)
    {
        if (!genuine_pdos)
        {
            fprintf(stderr, "command too long for non-genuine PDOS\n");
            return (1);
        }
        /* reconstitute temporarily */
        z = b + ln3;
        *z = ' ';
        PosSetEnv("CMDLINE", b);
        __envptr = PosGetEnvBlock();
        *z = '\0';
        cmdt[0] = 0x7f;
        memcpy(cmdt + 1, p, 0x7e);
    }
    else
    {
        cmdt[0] = ln;
        memcpy(cmdt + 1, p, ln);
        memcpy(cmdt + ln + 1, "\r", 2);
    }
#endif
    err = 0;
    while(*s != '\0')
    {
        t = strchr(s, ';');

        if (t == NULL)
        {
            t = s + strlen(s);
        }

        ln2 = t - s;
        memcpy(tempbuf, s ,ln2);

        if (ln2 != 0)
        {
            tempbuf[ln2++] = '\\';
        }

        ln4 = ln2 + ln3;
        if (ln4 > (FILENAME_MAX - DOT_COM_EXTEND) ||
            ln4 > (FILENAME_MAX - DOT_EXE_EXTEND) ||
            ln4 > (FILENAME_MAX - DOT_BAT_EXTEND))    
        {
            err = 1;
            break;
        }
        strcpy(tempbuf + ln2, b);
        ln2 += ln3;
        strcpy(tempbuf + ln2, ".com");

        if (exists(tempbuf))
        {
#ifdef __32BIT__
            strcpy(cmdt, tempbuf);
            strcat(cmdt, " ");
            strcat(cmdt, p);
#endif
            PosExec(tempbuf, &parmblock);
            break;
        }

        strcpy(tempbuf + ln2 ,".exe");

        if (exists(tempbuf))
        {
#ifdef __32BIT__
            strcpy(cmdt, tempbuf);
            strcat(cmdt, " ");
            strcat(cmdt, p);
#endif
            PosExec(tempbuf, &parmblock);
            break;
        }

        strcpy(tempbuf + ln2 ,".bat");

        if (exists(tempbuf))
        {
            readBat(tempbuf);
            break;
        }

        s = t;

        if (*s == ';')
        {
            s++;
        }
    }

    if (*s == '\0' || err == 1)
    {
        printf("Command '%s' not found\n", b);
        return 1;
    }

    return PosGetReturnCode();
}

static int cmd_copy_run(char *b)
{
    char bbb[512];
    char *src;
    char *dest;
    FILE *fsrc;
    FILE *fdest;
    int bytes_read = 0;
    int rv = 0;
    unsigned long total_read = 0, verify_read = 0;
    CMD_REQUIRES_ARGS(b);

    src = b;
    dest = strchr(b,' ');

    if (dest == NULL)
    {
        printf("\n Two Filenames required");
        return 1;
    }

    *dest++ ='\0';

    printf("Source is %s \n",src);
    printf("Dest is %s \n",dest);

    fsrc = fopen(src,"rb");

    if (fsrc != NULL)
    {
        fdest = fopen(dest,"wb");

        if (fdest != NULL)
        {
            while((bytes_read = fread(bbb,1,sizeof(bbb),fsrc)) != 0)
            {
                total_read += bytes_read;
                fwrite(bbb,1,bytes_read,fdest);
            }

            fclose(fdest);
        }

        else
        {
            printf("Destination %s file invalid\n",dest);
            rv = 1;
        }

        fclose(fsrc);
        printf("Copied %lu bytes\n", total_read);
        /* Verify copied file */
        if (PosGetVerifyFlag())
        {
            printf("Verifying copied file...\n");
            fdest = fopen(dest, "rb");
            if (fdest == NULL)
            {
                printf(
                  "ERROR: failed opening destination file for verification\n");
                return 1;
            }
            while((bytes_read = fread(bbb,1,sizeof(bbb),fdest)) != 0)
            {
                verify_read += bytes_read;
            }
            fclose(fdest);
            if (verify_read == total_read)
            {
                printf("SUCCESS: Read %lu bytes from destination file\n",
                        verify_read);
            }
            else
            {
                printf(
                     "ERROR: Read %lu bytes from destination (expected %lu)\n",
                        verify_read, total_read);
                return 1;
            }
        }
    }

    else
    {
        printf("Source file %s not found \n",src);
        rv = 1;
    }

    return rv;
}

static int cmd_cd_run(char *to)
{
    int ret;
    int d;

    /* No arguments means to print current drive and directory */
    if (isBlankString(to))
    {
        d = PosGetDefaultDrive();
        drive[0] = d + 'A';
        PosGetCurDir(0, cwd);
        printf("%s:\\%s\n", drive, cwd);
        return 0;
    }

    ret = PosChangeDir(to);
    if (ret != POS_ERR_NO_ERROR)
    {
        showError(ret);
        return 1;
    }
    return 0;
}

static int changedisk(int drive)
{
    int selected;
    PosSelectDisk(toupper(drive) - 'A');
    selected = PosGetDefaultDrive() + 'A';
    if (selected != toupper(drive))
    {
        printf("ERROR: Failed changing to drive %c:\n", toupper(drive));
        return 1;
    }
    return 0;
}

/*
 * **** HELP ROUTINES ***
 */
static void cmd_exit_help(void)
{
    printf("EXIT [/TSR]\n");
    printf("/TSR = Terminate and Stay Resident\n");
    printf("(Note, PCOMM is not a real TSR, and so keeping it resident\n");
    printf("after exit serves little purpose. However, it does help in\n");
    printf("testing the TSR functionality of PDOS\n");
}

static void cmd_accmap_help(void)
{
    printf("ACCMAP [accent key] [filename]\n");
    printf("allows you to provide a 256-byte file\n");
    printf("that is used to provide an accent key and what it should map to\n");
    printf("if the accent maps to itself, a single accent character will\n");
    printf("be produced. If the mapping is invalid, 2 will be generated.\n");
}

static void cmd_attrib_help(void)
{
    printf("ATTRIB filename {{+|-}{A|H|R|S}}*\n");
    printf("A = Archive\n");
    printf("H = Hidden\n");
    printf("R = Read-Only\n");
    printf("S = System\n");
}

static void cmd_path_help(void)
{
    printf("PATH variable provides a list of directories\n");
    printf("which are searched when looking for executables.\n\n");
    printf("PATH\n");
    printf("PATH [path]\n");
    printf("PATH [;]\n");
}

static void cmd_prompt_help(void)
{
    int i;
    promptSym *sym;
    printf("PROMPT variable stores the PCOMM prompt.\n");
    printf("PROMPT\n");
    printf("PROMPT [prompt]\n\n");
    printf("Supported PROMPT symbols are:\n");
    for (i = 0; promptSymRegistry[i].symbol != 0; i++)
    {
        sym = &(promptSymRegistry[i]);
        printf("$%c %s\n", sym->symbol, sym->description);
    }
}

static void cmd_type_help(void)
{
    printf("TYPE [/L] [/P] [path]\n");
    printf("/L = print line numbers\n");
    printf("/P = pause after each screenful\n");
}

static void cmd_dir_help(void)
{
    printf("DIR\n");
}

static void cmd_v_help(void)
{
    printf("V STATUS\n");
    printf("    Shows video information\n");
    printf("V {SAVE|RESTORE}={MODE|PAGE|COLOR|POS}\n");
    printf("    Push/pop video settings\n");
    printf("V {FG|BG}={0-15}\n");
    printf("    Set foreground/background color\n");
    printf("V {ROW|COL|PAGE}=n\n");
    printf("    Set row/column/page\n");
    printf("V MODE=x\n");
    printf("    Set video mode (hexadecimal)\n");
    printf("V FONT=[8x8|8x14|8x16]\n");
    printf("    Use named font in text mode\n");
    printf("Multiple options can be combined in a single invocation\n");
    printf("If no options supplied, STATUS is the default\n");
}

static void cmd_ver_help(void)
{
    printf("VER\n");
    printf("VER DOS=x.yy\n\n");
    printf("Can be used to display PDOS/PCOMM version.\n");
    printf("Can also be used to change reported DOS version.\n");
    printf("For example:\n");
    printf("\tVER DOS=6.22\n");
    printf("Changes reported DOS version to 6.22\n");
}

static void cmd_echo_help(void)
{
    printf("ECHO [message]\n");
}

static void cmd_cd_help(void)
{
    printf("CD [path]\n");
}

static void cmd_cls_help(void)
{
    printf("CLS\n");
}

static void cmd_reboot_help(void)
{
    printf("REBOOT\n");
}

static void cmd_rehash_help(void)
{
    printf("REHASH [drive]\n");
    printf("clear cache of a drive\n");
    printf("useful if you used the bios to bypass the OS "
           "to change the drive\n");
    printf("e.g. rehash c\n");
}

static void cmd_poweroff_help(void)
{
    printf("POWEROFF\n");
}

static void cmd_del_help(void)
{
    printf("DEL [path]\n");
}

static void cmd_scrncap_help(void)
{
    printf("SCRNCAP [drive] [off]\n");
    printf("drive is in hex. First hard disk is 80\n");
    printf("It will be wiped. Please don't use this command\n");
}

static void cmd_monitor_help(void)
{
    printf("MONITOR\n");
}

static void cmd_copy_help(void)
{
    printf("COPY [path1] [path2]\n");
}

static void cmd_help_help(void)
{
    printf("HELP\n");
    printf("HELP [command]\n");
}

static void cmd_md_help(void)
{
    printf("MD [path]\n");
}

static void cmd_mcd_help(void)
{
    printf("MCD [path]\n");
}

static void cmd_rd_help(void)
{
    printf("RD [path]\n");
}

static void cmd_ren_help(void)
{
    printf("REN [path] [name]\n");
}

static void cmd_date_help(void)
{
    printf("DATE\n\n");
    printf("Limitations (may be removed in future versions):\n");
    printf("- No arguments supported at present\n");
    printf("- No facility to change the date\n");
}

static void cmd_time_help(void)
{
    printf("TIME\n\n");
    printf("Limitations (may be removed in future versions):\n");
    printf("- No arguments supported at present\n");
    printf("- No facility to change the time\n");
}

static void cmd_rem_help(void)
{
    printf("REM [any arguments ignored]\n");
}

static void cmd_pause_help(void)
{
    printf("PAUSE\n");
}

static void cmd_peek_help(void)
{
    printf("PEEK location [#][quantity]\n");
#ifdef __32BIT__
    printf("location is flat 32-bit hex memory address, e.g. 46C\n");
#else
    printf("location is segment:offset hex memory address, e.g. 40:6C\n");
#endif
    printf("optional quantity is how many bytes to dump:\n");
    printf("B = dump byte\n");
    printf("W = dump 16-bit word\n");
    printf("D = dump 32-bit double word\n");
    printf("# will trigger decimal output, default is hex\n");
}

static void cmd_save_help(void)
{
    printf("SAVE filename [options...]\n\n");
    printf("Writes user input to a file.\n");
    printf("Options:\n");
    printf("/Dstr - Terminate input using string str\n");
    printf("/I    - Interactive: read from user, not from the batch file\n");
    printf("/O    - Overwrite existing file\n");
    printf("/Q    - Quiet mode, suppress all prompts/messages\n");
}

static void cmd_set_help(void)
{
    printf("SET\n\n");
    printf("SET NAME=[VALUE]\n\n");
    printf("Called without arguments, will print current environment\n");
    printf("Pass NAME=VALUE to set variable NAME to that VALUE\n");
    printf("Leave the VALUE blank to delete that environment variable\n");
}

static void cmd_showret_help(void)
{
    printf("SHOWRET [on|off]\n");
    printf("shows return codes from programs\n");
}

static void cmd_keybmap_help(void)
{
    printf("KEYBMAP [off|filename]\n");
    printf("allows you to provide a 256-byte file\n");
    printf("that is used to change keyboard hex codes into some\n");
    printf("new hex code - especially for use with non-US keyboards\n");
}

static void cmd_loadfont_help(void)
{
    printf("LOADFONT [startchar] [filename]\n");
    printf("allows you to provide a file containing 8 * 14 fonts, ie\n");
    printf("14 bytes per character, to be loaded at the given hex character\n");
    printf("e.g. loadfont 0x80 file.dat\n");
}

static void cmd_scanmap_help(void)
{
    printf("SCANMAP [off|filename] [shift|ctrl]\n");
    printf("allows you to provide a 256-byte file\n");
    printf("that is used to change keyboard scan codes into some\n");
    printf("new hex code - especially for use with non-US keyboards\n");
    printf("3 tables are supported - normal, shifted and ctrl pressed\n");
}

static void cmd_scrnmap_help(void)
{
    printf("SCRNMAP [off|filename]\n");
    printf("allows you to provide a 256-byte file\n");
    printf("that is used to change screen hex codes into some\n");
    printf("new hex code - especially for use with non-US countries\n");
    printf("attempting to use US bioses\n");
}

static void cmd_sleep_help(void)
{
    printf("SLEEP seconds\n");
}

static void cmd_option_help(void)
{
    printf("OPTION {{+-}X}*\n\n");
    printf("where X is an option character\n");
    printf("+X sets option X, -X clears option X\n");
    printf("Call with no arguments for current state of options\n");
}

static void cmd_unboot_help(void)
{
    printf("UNBOOT X:\n\n");
    printf("Marks disk X: as non-bootable\n");
    printf("It does this by rewriting the boot sector to\n");
    printf("remove the 55AA boot signature.\n");
}

static void cmd_goto_help(void)
{
    printf("GOTO [/16 | /32] label\n\n");
    printf("Transfer batch file execution to :label\n");
    printf("Option /16 means only take branch if running PDOS-16\n");
    printf("Option /32 means only take branch if running PDOS-32\n");
}

static void cmd_ps_help(void)
{
    printf("PS\n\n");
}

/* Modify string in-place to be all upper case */
static void strtoupper(char *s)
{
    while (*s != 0)
    {
        *s = toupper((unsigned char)*s);
        s++;
    }
}

/* Assumption as to number of lines before we print
 * "Press any key to continue...". Actually we should call some API to find
 * out screen dimensions and calculate dynamically. But for now, we will just
 * assume a standard 25 line display. The paging offset then is a bit less
 * than 25 to take into account the 3 line header, etc.
 */
#define HELP_PAGE_AMOUNT 19

/* cmd_help_run function and condition for it originally written by Alica
 * Okano. Rewritten to use table-driven design by Simon Kissane.
 */
static int cmd_help_run(char *cmd)
{
    int i = 0;
    int pageAt = HELP_PAGE_AMOUNT;
    int maxCmdNameLen = 0, curCmdNameLen = 0;
    cmdBlock *block = NULL;
    char *pipe = NULL;
    char tmp[128];
    if (isBlankString(cmd))
    {
        printf("Use HELP [command] to obtain more information about\n");
        printf("specific command.\n\n");

        /* Find max command name length. Used to construct columns. */
        for (i = 0; cmdRegistry[i].name != NULL; i++)
        {
            block = &(cmdRegistry[i]);
            curCmdNameLen = strlen(block->name);
            if (curCmdNameLen > maxCmdNameLen)
                maxCmdNameLen = curCmdNameLen;
        }

        /* Print each line */
        for (i = 0; cmdRegistry[i].name != NULL; i++)
        {
            block = &(cmdRegistry[i]);

            /* Allocate temporary buffer to modify name */
            memset(tmp, 0, 128);
            strcpy(tmp,block->name);

            /* Strip aliases */
            pipe = strchr(tmp,'|');
            if (pipe != NULL)
            {
                while (*pipe != 0)
                {
                    *pipe = 0;
                    pipe++;
                }
            }

            /* Make upper case */
            strtoupper(tmp);

            /* Left pad with spaces */
            while (strlen(tmp) < maxCmdNameLen)
                tmp[strlen(tmp)] = ' ';
            tmp[strlen(tmp)] = ' ';

            /* Print help line */
            printf("%s %s.\n", tmp, block->description);

            /* Page the screen if necessary */
            if (i >= pageAt && cmdRegistry[i+1].name != NULL)
            {
                cmd_pause_run("");
                pageAt += HELP_PAGE_AMOUNT;
            }
        }
    }
    else
    {
        /* Lookup command, report error if not found */
        block = findCommand(cmd);
        if (block == NULL)
        {
            printf("ERROR: No such command \"%s\"\n",cmd);
            return 1;
        }

        /* Allocate temporary buffer to modify name */
        memset(tmp, 0, 128);
        strcpy(tmp,block->name);

        /* Strip aliases */
        pipe = strchr(tmp,'|');
        if (pipe != NULL)
        {
            while (*pipe != 0)
            {
                *pipe = 0;
                pipe++;
            }
        }

        /* Make upper case */
        strtoupper(tmp);

        /* Print header line */
        printf("%s - %s\n\n", tmp,
                (block->description != NULL ? block->description :
                 "(no description)"));

        /* Print aliases */
        if (strchr(block->name,'|') != NULL)
        {
            memset(tmp, 0, 128);
            strcpy(tmp,block->name);
            pipe = strchr(tmp,'|');
            pipe++;
            for (i = 0; pipe[i] != 0; i++)
            {
                pipe[i] = toupper((unsigned char)(pipe[i]));
                if (pipe[i] == '|')
                {
                    pipe[i] = ' ';
                }
            }
            printf("Aliases: %s\n\n", pipe);
        }

        /* Print detailed help */
        if (block->help != NULL)
        {
            (block->help)();
        }
    }

    return 0;
}

static int cmd_md_run(char *dnm)
{
    int ret;
    CMD_REQUIRES_ARGS(dnm);

    ret = PosMakeDir(dnm);
    if (ret != POS_ERR_NO_ERROR)
    {
        showError(ret);
        return 1;
    }
    return 0;
}

static int cmd_mcd_run(char *dnm)
{
    int rc = 0;
    CMD_REQUIRES_ARGS(dnm);
    rc = cmd_md_run(dnm);
    rc = cmd_cd_run(dnm);
    return rc;
}

static int cmd_rd_run(char *dnm)
{
    int ret;
    CMD_REQUIRES_ARGS(dnm);

    ret = PosRemoveDir(dnm);
    if (ret != POS_ERR_NO_ERROR)
    {
        showError(ret);
        return 1;
    }
    return 0;
}

static int cmd_ren_run(char *src)
{
    int ret;
    char *dest;
    CMD_REQUIRES_ARGS(src);

    dest = strchr(src, ' ');
    if (*src == '\0' || dest == NULL)
    {
        printf("Required Parameter Missing\n");
        return 1;
    }

    *dest = '\0';
    dest++;

    ret = PosRenameFile(src, dest);
    if (ret != POS_ERR_NO_ERROR)
    {
        showError(ret);
        return 1;
    }
    return 0;
}

static int ins_strcmp(char *one, char *two)
{
    while (toupper((unsigned char)*one) == toupper((unsigned char)*two))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
    }
    if (toupper((unsigned char)*one) < toupper((unsigned char)*two))
    {
        return (-1);
    }
    return (1);
}

static int ins_strncmp(char *one, char *two, size_t len)
{
    size_t x = 0;

    if (len == 0) return (0);
    while ((x < len)
           && (toupper((unsigned char)*one) == toupper((unsigned char)*two)))
    {
        if (*one == '\0')
        {
            return (0);
        }
        one++;
        two++;
        x++;
    }
    if (x == len) return (0);
    return (toupper((unsigned char)*one) - toupper((unsigned char)*two));
}

static int readBat(char *fnm)
{
    FILE *fp;
    int rc = 0;

    inBatch = 1;

    fp = fopen(fnm, "r");
    if (fp != NULL)
    {
        currentBatchFp = fp;
        while (fgets(buf, MAX_COMMAND_BUF, fp) != NULL)
        {
            /* If GOTO target set, scan for matching label */
            if (*gotoTarget != 0)
            {
                if (ins_strncmp(buf, gotoTarget, strlen(gotoTarget)) == 0)
                {
                    /* Found gotoTarget, clear it */
                    gotoTarget[0] = 0;
                }
                /* We don't echo or execute while doing GOTO scan */
                continue;

            }
            /* If OPTION +E print the command before running it */
            if (echoFlag)
            {
                putPrompt();
                printf("%s\n", buf);
            }
            rc = processInput(false);
            currentBatchFp = fp; /* In case we ran some other batch file */
            /* Abort if OPTION +A and batch file command failed */
            if (abortFlag && rc != 0)
            {
                printf("ERROR: Aborting batch file due to command failure\n");
                rc = 1;
                break;
            }
            /* If executeGoto=1, reopen file to commence GOTO execution.
             * Basically, on every GOTO, we rewind file to start
             */
            if (executeGoto)
            {
                fclose(fp);
                fp = fopen(fnm, "r");
                currentBatchFp = fp;
                executeGoto = 0;
            }
        }
        fclose(fp);
        if (*gotoTarget != 0)
        {
            printf("ERROR: GOTO target '%s' not found\n", gotoTarget);
            gotoTarget[0] = 0;
            rc = 1;
        }
    }
    else
    {
        rc = 1;
    }

    inBatch = 0;
    currentBatchFp = NULL;
    return rc;
}

static int exists(char *fnm)
{
    FILE *fp;

    fp = fopen(fnm,"rb");

    if (fp != NULL)
    {
        fclose(fp);
        return 1;
    }

    return 0;
}

/* Written By NECDET COKYAZICI, Public Domain */


void putch(int a)
{
    putchar(a);
    fflush(stdout);
}


void bell(void)
{
    putch('\a');
}

#define ESC 0x1b
#define UP_ARROW (72 << 8)
#define LEFT_ARROW (75 << 8)
#define RIGHT_ARROW (77 << 8)
#define DOWN_ARROW (80 << 8)
void safegets(char *buffer, int size, bool use_history)
{
    int a;
    int i = 0;
    int j;
    int history_cursor = -1;
    int pos = 0;

    setvbuf(stdin, NULL, _IONBF, 0);
    while (1)
    {

        a = getchar();
        if (a == ESC)
        {
            a = getchar();
            if (a != ESC)
            {
                if (a == '[')
                {
                    a = getchar();
                    if (a == 'A')
                    {
                        a = UP_ARROW;
                    }
                    else if (a == 'B')
                    {
                        a = DOWN_ARROW;
                    }
                    else if (a == 'C')
                    {
                        a = RIGHT_ARROW;
                    }
                    else if (a == 'D')
                    {
                        a = LEFT_ARROW;
                    }
                }
            }
        }
#if 0
        a = PosGetCharInputNoEcho();
        if(!a)
            a = PosGetCharInputNoEcho() << 8;
#endif
        if(use_history)
        {

            if (a == UP_ARROW || a == DOWN_ARROW)
            {

                if(a == UP_ARROW)
                {
                    if(history_cursor == -1 && first_command_history != last_command_history)
                    history_cursor = PREVIOUS_HISTORY(last_command_history);

                    else if(history_cursor != first_command_history)
                    history_cursor = PREVIOUS_HISTORY(history_cursor);
                }
                if(a == DOWN_ARROW)
                {
                    if(history_cursor == -1) continue;

                    if(history_cursor == PREVIOUS_HISTORY(last_command_history))
                    history_cursor = -1;

                    else
                    history_cursor = NEXT_HISTORY(history_cursor);
                }

                /* Delete all characters on the screen */
                for(j = pos; j < i; ++ j) putch(' ');
                for(j = 0; j < i; ++ j) putch('\b');

                for(j = 0; j < i; ++ j) putch(' ');
                for(j = 0; j < i; ++ j) putch('\b');

                if(history_cursor == -1)
                {
                    i = 0;
                }
                else
                {
                    i = strlen(oldbufs[history_cursor]);
                    /* Put the new command onto the screen and into the buffer */
                    for(j = 0; j < i; ++ j)
                    {
                        char ch = oldbufs[history_cursor][j];
                        putch(ch);
                        buffer[j] = ch;
                    }

                }
                buffer[i] = '\0';
                pos = i;
                continue;
            }
            if (a == LEFT_ARROW) {
                
                if (i > 0) i--;
                
                if (pos > 0) {
                    putch('\b');
                    pos--;
                }
                continue;
            }
            if (a == RIGHT_ARROW) {
                if (i < MAX_COMMAND_BUF - 1) i++;
                if (pos < i) {
                    putch(buffer[pos]);
                    pos++;
                }
                continue;
            }
            if (!(a & 0xff)) continue;
        }

        if (i == size)
        {
            buffer[size] = '\0';

            if ((a == '\n') || (a == '\r'))
            {
                setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
                return;
            }

            bell();
        }

        /* TODO: tab completions */
        if (a == '\t')
            continue;

        if (i < size)
        {
            if ((a == '\n') || (a == '\r'))
            {
                putch('\n');

                buffer[i] = '\0';

                setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
                return;
            }


            if (a == '\b')
            {

                if (pos == 0)
                    continue;

                else
                    pos--;

                i--;
                memmove(buffer+pos, buffer+pos+1, i-pos);
                putch('\b');
            }
            else if (!iscntrl((unsigned char)a))
            {
                /*Allow the user to enter (MAX_COMMAND_BUF - 1) characters.*/
                if (pos < MAX_COMMAND_BUF - 1) {
                    putch(a);
                    memmove(buffer+pos+1, buffer+pos, i-pos+1);
                    buffer[pos] = a;
                    pos++;
                    i++;
                }
            }
            else
            {
                bell();

                continue;
            }

            for (j = pos;j < i;j++) putch(buffer[j]);
            if (a == '\b') {
                putch(' ');
                putch('\b');
            }
            for (j = i - 1;j >= pos;j--) putch('\b');
        }
        else bell();

    }

}

static int cmd_date_run(char *ignored)
{
    unsigned int y, m, d, dw;
    CMD_HAS_NO_ARGS(ignored);
    PosGetSystemDate(&y,&m,&d,&dw);
    printf("%04d-%02d-%02d\n", y, m, d);
    return 0;
}

static int cmd_time_run(char *ignored)
{
    unsigned int hr, min, sec, hund;
    CMD_HAS_NO_ARGS(ignored);
    PosGetSystemTime(&hr,&min,&sec,&hund);
    printf("%02u:%02u:%02u\n", hr, min, sec);
    return 0;
}

static int cmd_rem_run(char *ignored)
{
    /* Nothing to do for comment */
    return 0;
}

static int cmd_pause_run(char *ignored)
{
    CMD_HAS_NO_ARGS(ignored);
    printf("Press any key to continue . . .\n");
    PosDirectCharInputNoEcho();
    return 0;
}

static int cmd_peek_run(char *arg)
{
    void *ptr;
    char addr[10];
    char quantity = 'B';
    int decimal = 0;
#ifdef __32BIT__
    unsigned long loc;
#else
    unsigned int seg, off;
#endif
    CMD_REQUIRES_ARGS(arg);
    arg = stringTrimBoth(arg);
    if (!isxdigit((unsigned char)(arg[0])))
    {
        printf("ERROR: PEEK command requires valid hex memory location\n");
        return 1;
    }
#ifdef __32BIT__
    loc = strtoul(arg,&arg,16);
    ptr = (void*)loc;
    sprintf(addr,"%08lX", loc);
#else
    seg = (unsigned int)strtoul(arg,&arg,16);
    if (arg[0] != ':' || !isxdigit((unsigned char)(arg[1])))
    {
        printf("ERROR: PEEK command needs SEGMENT:OFFSET address\n");
        return 1;
    }
    arg++;
    off = (unsigned int)strtoul(arg,&arg,16);
    ptr = MK_FP(seg,off);
    sprintf(addr,"%04X:%04X", seg, off);
#endif
    arg = stringTrimBoth(arg);
    if (arg[0] == '#')
    {
        decimal = 1;
        arg++;
        arg = stringTrimBoth(arg);
    }
    switch (toupper((unsigned char)(arg[0])))
    {
        case 0:
        case 'B':
            quantity = 'B';
            break;
        case 'W':
            quantity = 'W';
            break;
        case 'D':
            quantity = 'D';
            break;
        default:
            printf("ERROR: PEEK command illegal quantity '%c'\n", arg[0]);
            return 1;
    }
    if (arg[0] != 0)
        arg++;
    if (arg[0] != 0)
    {
        printf("ERROR: Junk at end of PEEK command\n");
        return 1;
    }
    switch (quantity)
    {
        case 'B':
        {
            unsigned char *b = ptr;
            printf(decimal ? "%s = %u\n" : "%s = %02X\n",
                    addr, (unsigned int)(*b));
            break;
        }
        case 'W':
        {
            unsigned short *w = ptr;
            printf(decimal ? "%s = %u\n" : "%s = %02X\n",
                    addr, (unsigned int)(*w));
            break;
        }
        case 'D':
        {
            unsigned long *d = ptr;
            unsigned long v = *d;
#ifdef __32BIT__
            printf(decimal ? "%s = %lu\n" : "%s = %08lX\n", addr, v);
#else
            if (decimal)
            {
                printf("%s = %lu\n", addr, v);
            }
            else
            {
                unsigned int hi = (unsigned int)(v >> 16UL);
                unsigned int lo = (unsigned int)(v);
                printf("%s = %04X%04X\n", addr, hi, lo);
            }
#endif
            break;
        }
    }
    return 0;
}

static int cmd_set_run(char *arg)
{
    char *env = (char*)__envptr;
    size_t len;
    char *name, *value;
    int ret, i;
    unsigned short footerCount;
    bool empty;

    /* If argument passed, we have something to set NAME=VALUE */
    if (!isBlankString(arg))
    {
        CMD_REQUIRES_GENUINE();
        name = stringTrimBoth(arg);
        value = strchr(name,'=');
        /* No equal sign is taken to mean UNSET */
        if (value != NULL)
        {
            *value = 0;
            value++;
            value = stringTrimBoth(value);
            if (*value == 0)
            {
                /* Nothing but blanks after = sign means UNSET */
                value = NULL;
            }
        }
        strtoupper(name);
        ret = PosSetEnv(name,value);
        if (ret != POS_ERR_NO_ERROR)
        {
            showError(ret);
            return 1;
        }
        __envptr = PosGetEnvBlock();
        return 0;
    }

    /* No argument was passed, time to display environment */
    if (env == NULL)
    {
        return 0;
    }
    /* Empty environment must contain two NUL bytes */
    empty = env[0] == 0 && env[1] == 0;
    for (;;)
    {
        len = strlen(env);
        if (len == 0)
        {
            env++;
            break;
        }
        printf("%s\n", env);
        env += (len + 1);
    }
    if (empty)
        env++;
    /* Display the environment footers. There should only be one, which is
     * ARGV[0]. But more could be added in future.
     */
    footerCount = *((unsigned short *)env);
    env += sizeof(unsigned short);
    for (i = 0; i < footerCount; i++)
    {
        printf("; %s\n", env);
        env += strlen(env) + 1;
    }
    return 0;
}

static int cmd_showret_run(char *arg)
{
    int flag;

    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    if (strcmp(arg, "on") == 0)
    {
        flag = 1;
    }
    else if (strcmp(arg, "off") == 0)
    {
        flag = 0;
    }
    else
    {
        printf("ERROR: SHOWRET command argument not valid (on/off)\n");
        return 1;
    }
    PosShowret(flag);
    return 0;
}

static int cmd_keybmap_run(char *arg)
{
    int flag;

    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    if (strcmp(arg, "off") == 0)
    {
        PosKeyboardMap(NULL);
    }
    else
    {
        FILE *fp;
        char buf[257];
        int cnt;

        fp = fopen(arg, "rb");
        if (fp == NULL)
        {
            printf("can't open %s for reading\n", arg);
            return 1;
        }
        cnt = fread(buf, 1, MAX_COMMAND_BUF, fp);
        fclose(fp);
        if (cnt != 256)
        {
            printf("file is not exactly 256 bytes in size\n");
            return 1;
        }
        PosKeyboardMap(buf);
    }
    return 0;
}

static int cmd_loadfont_run(char *arg)
{
    int flag;
    int startch;
    char fonts[14 * 256];
    int cnt;
    char *p;
    FILE *fp;

    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    p = strchr(arg, ' ');
    if (p == NULL)
    {
        printf("need 2 parameters\n");
        return 0;
    }
    p++;
    startch = strtol(arg, NULL, 0);
    fp = fopen(p, "rb");
    if (fp == NULL)
    {
        printf("couldn't open %s\n", p);
        return 0;
    }
    cnt = fread(fonts, 1, sizeof fonts, fp);
    if ((cnt % 14) != 0)
    {
        printf("file must be a multiple of 14\n");
        fclose(fp);
        return 0;
    }
    cnt /= 14;
    PosLoadFonts(startch, cnt, 14, fonts, 0);
    fclose(fp);
    return 0;
}

static int cmd_scanmap_run(char *arg)
{
    int flag;

    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    if (strcmp(arg, "off") == 0)
    {
        PosScancodeMap(NULL, 0);
    }
    else
    {
        FILE *fp;
        char buf[257];
        int cnt;
        char *p;
        int type;

        p = strchr(arg, ' ');
        if (p == NULL)
        {
            type = 0;
        }
        else
        {
            *p++ = '\0';
            if (strcmp(p, "shift") == 0)
            {
                type = 1;
            }
            else if (strcmp(p, "ctrl") == 0)
            {
                type = 2;
            }
            else
            {
                printf("unknown map type %s\n", p);
                return (1);
            }
        }
        fp = fopen(arg, "rb");
        if (fp == NULL)
        {
            printf("can't open %s for reading\n", arg);
            return 1;
        }
        cnt = fread(buf, 1, MAX_COMMAND_BUF, fp);
        fclose(fp);
        if (cnt != 256)
        {
            printf("file is not exactly 256 bytes in size\n");
            return 1;
        }
        PosScancodeMap(buf, type);
    }
    return 0;
}

static int cmd_accmap_run(char *arg)
{
    int flag;

    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    {
        FILE *fp;
        char buf[257];
        int cnt;
        char *p;
        int accent_key;

        p = strchr(arg, ' ');
        if (p == NULL)
        {
            printf("accent key and filename required\n");
            return (0);
        }
        else
        {
            accent_key = arg[0];
            *p++ = '\0';
        }
        fp = fopen(p, "rb");
        if (fp == NULL)
        {
            printf("can't open %s for reading\n", p);
            return 1;
        }
        cnt = fread(buf, 1, MAX_COMMAND_BUF, fp);
        fclose(fp);
        if (cnt != 256)
        {
            printf("file is not exactly 256 bytes in size\n");
            return 1;
        }
        PosAccentMap(accent_key, buf);
    }
    return 0;
}

static int cmd_scrnmap_run(char *arg)
{
    int flag;

    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    if (strcmp(arg, "off") == 0)
    {
        PosScreenMap(NULL);
    }
    else
    {
        FILE *fp;
        char buf[257];
        int cnt;

        fp = fopen(arg, "rb");
        if (fp == NULL)
        {
            printf("can't open %s for reading\n", arg);
            return 1;
        }
        cnt = fread(buf, 1, MAX_COMMAND_BUF, fp);
        fclose(fp);
        if (cnt != 256)
        {
            printf("file is not exactly 256 bytes in size\n");
            return 1;
        }
        PosScreenMap(buf);
    }
    return 0;
}

static int cmd_sleep_run(char *arg)
{
    unsigned long seconds;
    CMD_REQUIRES_ARGS(arg);
    CMD_REQUIRES_GENUINE();
    arg = stringTrimBoth(arg);
    if (!isdigit((unsigned char)(arg[0])))
    {
        printf("ERROR: SLEEP command argument not valid seconds count\n");
        return 1;
    }
    seconds = strtol(arg, &arg, 10);
    if (arg[0] != 0)
    {
        printf("ERROR: SLEEP command argument not valid seconds count\n");
        return 1;
    }
    PosSleep(seconds);
    return 0;
}

/* Find option in options registry */
static optBlock *findOption(char optKey)
{
    optBlock *cur;
    int i;
    for (i = 0; optRegistry[i].name != NULL; i++)
    {
        cur = &(optRegistry[i]);
        if (cur->key == optKey)
        {
            return cur;
        }
    }
    return NULL;
}

static int opt_logunimp_get(void)
{
    /* If not PDOS, no such flag, just always return 0=off */
    if (!genuine_pdos)
    {
        return 0;
    }
    return PosGetLogUnimplemented();
}

static void opt_logunimp_set(int flag)
{
    /* Only try set flag if PDOS, API doesn't exist otherwise */
    if (genuine_pdos)
    {
        PosSetLogUnimplemented(!!flag);
    }
}

static int opt_verify_get(void)
{
    return PosGetVerifyFlag();
}

static void opt_verify_set(int flag)
{
    PosSetVerifyFlag(!!flag);
}

static int opt_break_get(void)
{
    return PosGetBreakFlag();
}

static void opt_break_set(int flag)
{
    PosSetBreakFlag(!!flag);
}

static int opt_echo_get(void)
{
    return echoFlag;
}

static void opt_echo_set(int flag)
{
    echoFlag = !!flag;
}

static int opt_abort_get(void)
{
    return abortFlag;
}

static void opt_abort_set(int flag)
{
    abortFlag = !!flag;
}

static int cmd_option_run(char *arg)
{
    optBlock *cur;
    int i, set;
    char key;
    int flag;

    /* No args -> display current options */
    if (isBlankString(arg))
    {
        for (i = 0; optRegistry[i].name != NULL; i++)
        {
            cur = &(optRegistry[i]);
            flag = (cur->getProc)();
            printf("%c%c %s\n", flag ? '+' : '-', cur->key, cur->description);
        }
        return 0;
    }

    /* Process args */
    for (i = 0; arg[i] != 0; i++)
    {
        /* Skip whitespace */
        if (isspace((unsigned char)(arg[i])))
        {
            continue;
        }

        /* Expect + or - */
        if (arg[i] != '+' && arg[i] != '-')
        {
            printf("ERROR: Bad syntax in OPTION command (expected + or -)\n");
            return 1;
        }

        /* Note which one we got */
        set = (arg[i] == '+');

        /* Next character must be option key */
        i++;
        if (arg[i] == 0 || !isalpha((unsigned char)(arg[i])))
        {
            printf("ERROR: Bad syntax in OPTION command "
                    "(expected option character)\n");
            return 1;
        }

        /* Find option with that key */
        key = toupper((unsigned char)(arg[i]));
        cur = findOption(key);
        if (cur == NULL)
        {
            printf("ERROR: Unknown option %c%c\n", (set ? '+' : '-'), key);
            return 1;
        }

        /* Perform the set */
        cur->setProc(set);

        /* Confirm new value set */
        if (cur->getProc() != set)
        {
            printf("ERROR: Option %c%c failed to take effect\n",
                    (set ? '+' : '-'), key);
            return 1;
        }

        /* Success message */
        printf("Set option %c%c (%s)\n", (set ? '+' : '-'), key,
                cur->description);
    }
    return 0;
}

static void bootSectorAccessError(int drive, int rc, int writing)
{
    printf("ERROR: Error %d %s boot sector of drive %c:\n",
            rc, writing ? "writing" : "reading", (drive + 'A'));
}

static int cmd_unboot_run(char *arg)
{
    int drive, rc, i;
    unsigned char buf[512];

    /* In theory, command should work on MS-DOS. In practice, crashes. */
    CMD_REQUIRES_GENUINE();
    CMD_REQUIRES_ARGS(arg);
    arg = stringTrimBoth(arg);

    /* Validate parameters - X: is drive to make unbootable */
    if (!isalpha((unsigned char)(arg[0])) || arg[1] != ':' || arg[2] != 0)
    {
            errorBadArgs();
            return 1;
    }
    drive = toupper((unsigned char)(arg[0])) - 'A';

    /* Read the boot sector */
    rc = PosAbsoluteDiskRead(drive, 0, 1, buf);
    if (rc != 0)
    {
        bootSectorAccessError(drive, rc, 0);
        return 1;
    }

    /* If disk is already non-bootable, nothing to do */
    if (buf[510] != 0x55 || buf[511] != 0xAA)
    {
        printf("\n");
        printf("UNBOOT: Disk %c: already not bootable, nothing to do\n",
                (drive + 'A'));
        return 0;
    }

    /* Modify boot sector to make it unbootable */
    buf[510] = 0x12;
    buf[511] = 0x34;

    /* For unfathomable reasons, VirtualBox will try to boot a floppy in
     * drive A even if it lacks the 55AA boot signature. For it to consider
     * the disk unbootable, the first and second 16-bit words have to be
     * identical. (In November 2017, it was changed to the first and third
     * words instead.) So, to convince VirtualBox that the disk can't be
     * booted, we will overwrite the first ten bytes of a floppy boot sector
     * with 0xE9. MS-DOS will still consider the disk valid (first byte is E9),
     * while VirtualBox will consider the floppy non-bootable. Note that since
     * this corrupts the OEM name, DOS may ignore the BPB. On a floppy with
     * standard geometry, that doesn't really matter. (We won't do this on
     * hard disks, where it matters more.)
     */
    if (drive < 2)
    {
        for (i = 0; i < 0xB; i++)
        {
            buf[i] = 0xE9;
        }
    }

    /* Write the modified boot sector */
    rc = PosAbsoluteDiskWrite(drive, 0, 1, buf);
    if (rc != 0)
    {
        bootSectorAccessError(drive, rc, 1);
        return 1;
    }

    /* To confirm it was written correctly, read it back in */
    memset(buf, 0, 512);
    rc = PosAbsoluteDiskRead(drive, 0, 1, buf);
    if (rc != 0)
    {
        bootSectorAccessError(drive, rc, 0);
        return 1;
    }
    if (buf[510] != 0x12 || buf[511] != 0x34)
    {
        printf("ERROR: UNBOOT boot sector verification failed.\n");
        return 1;
    }

    /* Print SUCCESS message then done. */
    printf("SUCCESS: Disk in drive %c: made unbootable.\n", (drive + 'A'));
    return 0;
}

static int cmd_goto_run(char *arg)
{
    int is16 = 0, is32 = 0;

    CMD_REQUIRES_ARGS(arg);
    if (!inBatch)
    {
        errorBatchOnly();
        return 1;
    }
    arg = stringTrimBoth(arg);

    if (ins_strncmp(arg,"/16 ", 4)==0)
    {
        is16 = 1;
        arg += 4;
    }
    else if (ins_strncmp(arg,"/32 ", 4)==0)
    {
        is32 = 1;
        arg += 4;
    }
    arg = stringTrimBoth(arg);
    if (isBlankString(arg))
    {
        errorBadArgs();
        return 1;
    }

    /* Respect /16 and /32 args */
#ifdef __32BIT__
    if (is16)
    {
        return 0;
    }
#else
    if (is32)
    {
        return 0;
    }
#endif

    /* Actually perform the GOTO */
    gotoTarget[0] = ':';
    strcpy(gotoTarget + 1, arg);
    executeGoto = 1;
    return 0;
}

static char *describeProcStatus(PDOS_PROCSTATUS status)
{
    switch (status)
    {
        case PDOS_PROCSTATUS_LOADED:
            return "LOADED    ";
        case PDOS_PROCSTATUS_ACTIVE:
            return "ACTIVE    ";
        case PDOS_PROCSTATUS_CHILDWAIT:
            return "CHILDWAIT ";
        case PDOS_PROCSTATUS_TSR:
            return "TSR       ";
        case PDOS_PROCSTATUS_SUSPENDED:
            return "SUSPENDED ";
        case PDOS_PROCSTATUS_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN   ";
    }
}

static int cmd_ps_run(char *ignored)
{
    unsigned long pid = 0;
    int rc;
    PDOS_PROCINFO info;
    MEMMGRSTATS stats;

    CMD_HAS_NO_ARGS(ignored);

    printf("     PID   PARENT STATUS            USED  #BLOCKS NAME\n");
    for (;;)
    {
        rc = PosProcessGetInfo(pid, &info, sizeof(PDOS_PROCINFO));
        if (rc != POS_ERR_NO_ERROR)
        {
            showError(rc);
            return 1;
        }
        printf("%8lX ", info.pid);
        if (info.ppid == 0)
            printf("         ");
        else
            printf("%8lX ", info.ppid);
        printf("%s ",describeProcStatus(info.status));

        PosProcessGetMemoryStats(info.pid,&stats);
        printf("%8lu KB ", (unsigned long)PARAS_TO_KB(stats.totalAllocated));
        printf("% 8ld ", stats.countAllocated);
        printf("%s\n",info.exeName);
        if (info.nextPid == 0)
            break;
        pid = info.nextPid;
    }
    return 0;
}

static int cmd_cls_run(char *ignored)
{
    CMD_HAS_NO_ARGS(ignored);
    PosClearScreen();
    return 0;
}

/* Splits first word from input. Inserts a NUL byte at the first space
 * character after a non-space character.
 */
static char *splitFirstWord(char *str)
{
    /* Skip any initial whitespace */
    while (*str != 0 && isspace((unsigned char)*str))
        str++;
    /* Now skip all non-whitespace */
    while (*str != 0 && !isspace((unsigned char)*str))
        str++;
    /* Have we reached a NUL? - then nothing after initial token */
    if (*str == 0)
        return str;
    /* Not a NUL, must be a space - zero it out then move forward */
    *str = 0;
    str++;
    /* Strip any whitespace before second token */
    while (*str != 0 && isspace((unsigned char)*str))
        str++;
    /* Return start of second token */
    return str;

}

static bool isDelimiterLine(char *line, char *delimiter)
{
    int len = strlen(delimiter);
    /* If delimiter not at start, not a delimiter line */
    if (ins_strncmp(line, delimiter, len) != 0)
        return false;
    /* Skip past the delimiter */
    line += len;
    /* Allow whitespace between delimiter and end of line */
    while (isspace((unsigned char)*line))
        line++;
    /* Not a delimiter line if non-whitespace after delimiter */
    return *line == 0;
}

static int cmd_save_run(char *arg)
{
    char *fileName = NULL;    /* Holds the file name */
    bool interactive = false; /* Option /I - Interactive mode */
    bool overwrite = false;   /* Option /O - Overwrite */
    bool append = false;      /* Option /A - Append */
    bool quiet = false;       /* Option /Q - Quiet Mode */
    char *delimiter = NULL;   /* Option /D... - Set Delimiter */
    bool exists = false;      /* Does file exist? */
    int attrs;                /* Attributes of existing file */
    FILE *fh;                 /* File to which we will write */
    int lineCount = 0;        /* How many lines processed so far */
    char line[MAX_COMMAND_BUF];    /* Buffer for read lines */

    /* Strip out the file name */
    CMD_REQUIRES_ARGS(arg);
    arg = stringTrimBoth(arg);
    fileName = arg;
    arg = splitFirstWord(arg);

    /* Process the options */
    while (*arg != 0)
    {
        /* Skip whitespace */
        if (isspace((unsigned char)*arg))
        {
            arg++;
            continue;
        }
        /* Option /I - Interactive */
        if (ins_strncmp(arg,"/I",2) == 0) {
            interactive = 1;
            arg += 2;
            continue;
        }
        /* Option /O - Overwrite */
        if (ins_strncmp(arg,"/O",2) == 0) {
            overwrite = 1;
            arg += 2;
            continue;
        }
	/* Option /A - Append */
	if (ins_strncmp(arg,"/A",2) == 0) {
            append=true;
	    arg += 2;
	    continue;
        }
        /* Option /Q - Quiet */
        if (ins_strncmp(arg,"/Q",2) == 0) {
            quiet = 1;
            arg += 2;
            continue;
        }
        /* Option /Dx - Set Delimiter to x */
        if (ins_strncmp(arg,"/D",2) == 0) {
            arg += 2;
            /* Delimiter starts after /D */
            delimiter = arg;
            /* End delimiter with a NUL */
            while (true)
            {
                if (*arg == 0)
                    break;
                if (isspace((unsigned char)*arg)) {
                    *arg = 0;
                    arg++;
                    break;
                }
                arg++;
            }
            continue;
        }
        /* Something else */
        showUnrecognisedArgsMsg(arg);
        return 1;
    }

    /* Validate delimiter */
    if (delimiter != NULL && *delimiter == 0)
    {
        printf("ERROR: /D option requires argument\n");
        return 1;
    }

    /* If no delimiter, use the default */
    if (delimiter == NULL)
    {
        delimiter = ".";
    }

    /* In batch file, if interactive mode not requested, turn on quiet */
    if (inBatch && !interactive)
        quiet = true;

    /* Check if the file already exists */
    exists = PosGetFileAttributes(fileName,&attrs) == POS_ERR_NO_ERROR;

    /* If file already exists, and not overwrite mode, raise error */
    if (exists && !overwrite)
    {
        printf("ERROR: File '%s' exists but /O not specified\n",
                fileName);
        return 1;
    }

    /* Open the file */
    fh = fopen(fileName, append?"a":"w");
    if (!fh)
    {
        printf("ERROR: Failed to open file '%s' for writing/appending\n", fileName);
        return 1;
    }

    /* If not quiet mode, display the instructions */
    if (!quiet)
    {
        printf("Enter lines to write to file '%s'\n", fileName);
        printf("Terminate input with %s on a line by itself\n", delimiter);
    }

    /* Now we start looping, reading in the lines */
    for (;;)
    {
        /* If not quiet mode, show the prompt */
        if (!quiet)
        {
            printf("%d: ", lineCount+1);
            fflush(stdout);
        }

        /* Read one line */
        if (inBatch && !interactive)
        {
            if (fgets(line, sizeof line, currentBatchFp) == NULL)
            {
                printf("ERROR: Unexpected end of batch file during SAVE\n");
                fclose(fh);
                return 1;
            }
        }
        else
        {
            safegets(line, sizeof line, false);
        }

        /* If delimiter line, exit loop */
        if (isDelimiterLine(line, delimiter))
            break;

        /* Send out that line */
        fprintf(fh, "%s\n", line);

        /* Increment line counter */
        lineCount++;
    }

    /* Close the file */
    fflush(fh);
    fclose(fh);

    /* Success message if not quiet mode */
    if (!quiet)
    {
        printf("SUCCESS: Wrote %d lines to file '%s'\n",
                lineCount, fileName);
    }

    /* Report success */
    return 0;
}

static void showVideoSetError(char *attr, int n, bool hex)
{
    printf("ERROR: Setting video %s=", attr);
    printf(hex ? "%X" : "%d", n);
    printf(" failed\n");
} 

static int cmd_v_run(char *arg)
{
    pos_video_info videoInfo;
    int n;
    char *token;
    int ret;

    CMD_REQUIRES_GENUINE();
    PosGetVideoInfo(&videoInfo, sizeof(pos_video_info));
    arg = stringTrimBoth(arg);
    if (*arg == 0)
    {
        /* No args, print video mode status */
        arg = "STATUS";
    }

    for (;;)
    {
        if (isBlankString(arg))
            break;
        token = arg;
        arg = splitFirstWord(arg);
        if (ins_strcmp("STATUS",token)==0)
        {
            printf("V MODE=%X PAGE=%u FG=%u BG=%u ROW=%u COL=%u\n",
                    videoInfo.mode,
                    videoInfo.page,
                    videoInfo.currentAttrib & 0xf,
                    videoInfo.currentAttrib >> 4,
                    videoInfo.row,
                    videoInfo.col
            );
            printf("    ROWS=%u COLS=%u CSTART=%u CEND=%u\n",
                    videoInfo.rows,
                    videoInfo.cols,
                    videoInfo.cursorStart,
                    videoInfo.cursorEnd
            );
            continue;
        }
        if (ins_strncmp("FG=",token,3)==0)
        {
            token += 3;
            if (!isdigit((unsigned char)*token))
            {
                showArgBadSyntaxMsg("FG=");
                return 1;
            }
            n = strtol(token,&token,10);
            if (*token != 0 || n < 0 || n > 15)
            {
                showArgBadSyntaxMsg("FG=");
                return 1;
            }
            videoInfo.currentAttrib &= 0xf0;
            videoInfo.currentAttrib |= n;
            PosSetVideoAttribute(videoInfo.currentAttrib);
            continue;
        }
        if (ins_strncmp("ROW=",token,4)==0)
        {
            token += 4;
            if (!isdigit((unsigned char)*token))
            {
                showArgBadSyntaxMsg("ROW=");
                return 1;
            }
            n = (int)strtol(token,&token,10);
            if (*token != 0 || n < 0)
            {
                showArgBadSyntaxMsg("ROW=");
                return 1;
            }
            PosGetVideoInfo(&videoInfo, sizeof(pos_video_info));
            PosMoveCursor(n, videoInfo.col);
            PosGetVideoInfo(&videoInfo, sizeof(pos_video_info));
            if (videoInfo.row != n)
            {
                showVideoSetError("ROW",n,false);
                return 1;
            }
            continue;
        }
        if (ins_strncmp("COL=",token,4)==0)
        {
            token += 4;
            if (!isdigit((unsigned char)*token))
            {
                showArgBadSyntaxMsg("COL=");
                return 1;
            }
            n = strtol(token,&token,10);
            if (*token != 0 || n < 0)
            {
                showArgBadSyntaxMsg("COL=");
                return 1;
            }
            PosGetVideoInfo(&videoInfo, sizeof(pos_video_info));
            PosMoveCursor(videoInfo.row, n);
            PosGetVideoInfo(&videoInfo, sizeof(pos_video_info));
            if (videoInfo.col != n)
            {
                showVideoSetError("COL",n,false);
                return 1;
            }
            continue;
        }
        if (ins_strncmp("BG=",token,3)==0)
        {
            token += 3;
            if (!isdigit((unsigned char)*token))
            {
                showArgBadSyntaxMsg("BG=");
                return 1;
            }
            n = strtol(token,&token,10);
            if (*token != 0 || n < 0 || n > 15)
            {
                showArgBadSyntaxMsg("BG=");
                return 1;
            }
            videoInfo.currentAttrib &= 0x0f;
            videoInfo.currentAttrib |= (n<<4);
            PosSetVideoAttribute(videoInfo.currentAttrib);
            continue;
        }
        if (ins_strncmp("MODE=",token,5)==0)
        {
            token += 5;
            if (!isxdigit((unsigned char)*token))
            {
                showArgBadSyntaxMsg("MODE=");
                return 1;
            }
            n = strtol(token,&token,16);
            if (*token != 0 || n < 0 || n > 255)
            {
                showArgBadSyntaxMsg("MODE=");
                return 1;
            }
            videoInfo.mode = n;
            if (PosSetVideoMode(videoInfo.mode) != 0)
            {
                printf("ERROR: Setting video MODE=%lX failed\n", n);
                showVideoSetError("MODE",n,true);
                return 1;
            }
            continue;
        }
        if (ins_strncmp("PAGE=",token,5)==0)
        {
            token += 5;
            if (!isdigit((unsigned char)*token))
            {
                showArgBadSyntaxMsg("PAGE=");
                return 1;
            }
            n = strtol(token,&token,10);
            if (*token != 0 || n < 0 || n > 4)
            {
                showArgBadSyntaxMsg("PAGE=");
                return 1;
            }
            videoInfo.page = n;
            if (PosSetVideoPage(videoInfo.page) != 0)
            {
                showVideoSetError("PAGE",n,false);
                return 1;
            }
            continue;
        }
        if (ins_strcmp("SAVE=MODE",token) == 0)
        {
            savedVideoState.mode = videoInfo.mode;
            continue;
        }
        if (ins_strcmp("RESTORE=MODE",token) == 0)
        {
            if (PosSetVideoMode(savedVideoState.mode) != 0)
            {
                showVideoSetError("MODE",savedVideoState.mode,true);
                return 1;
            }
            continue;
        }
        if (ins_strcmp("SAVE=PAGE",token) == 0)
        {
            savedVideoState.page = videoInfo.page;
            continue;
        }
        if (ins_strcmp("RESTORE=PAGE",token) == 0)
        {
            if (PosSetVideoPage(savedVideoState.page) != 0)
            {
                showVideoSetError("PAGE",savedVideoState.page,false);
                return 1;
            }
            continue;
        }
        if (ins_strcmp("SAVE=POS",token) == 0)
        {
            savedVideoState.row = videoInfo.row;
            savedVideoState.col = videoInfo.col;
            continue;
        }
        if (ins_strcmp("RESTORE=POS",token) == 0)
        {
            PosMoveCursor(savedVideoState.row,savedVideoState.col);
            continue;
        }
        if (ins_strcmp("SAVE=FG",token) == 0)
        {
            savedVideoState.currentAttrib &= 0xf0;
            savedVideoState.currentAttrib |= (videoInfo.currentAttrib&0x0f);
            continue;
        }
        if (ins_strcmp("RESTORE=FG",token) == 0)
        {
            videoInfo.currentAttrib &= 0xf0;
            videoInfo.currentAttrib |= (savedVideoState.currentAttrib&0x0f);
            PosSetVideoAttribute(videoInfo.currentAttrib);
            continue;
        }
        if (ins_strcmp("SAVE=BG",token) == 0)
        {
            savedVideoState.currentAttrib &= 0x0f;
            savedVideoState.currentAttrib |= (videoInfo.currentAttrib&0xf0);
            continue;
        }
        if (ins_strcmp("RESTORE=BG",token) == 0)
        {
            videoInfo.currentAttrib &= 0x0f;
            videoInfo.currentAttrib |= (savedVideoState.currentAttrib&0xf0);
            PosSetVideoAttribute(videoInfo.currentAttrib);
            continue;
        }
        if (ins_strncmp("FONT=",token,5)==0)
        {
            token += 5;
            if (isBlankString(token))
            {
                showArgBadSyntaxMsg("FONT=");
                return 1;
            }
            ret = PosSetNamedFont(token);
            if (ret != POS_ERR_NO_ERROR)
            {
                showError(ret);
                return 1;
            }
            /* Clear screen after setting font */
            PosClearScreen();
            continue;
        }
        showUnrecognisedArgsMsg(token);
        return 1;
    }
    return 0;
}

static int getAttrMask(char attr)
{
    switch (toupper((unsigned char)attr))
    {
        case 'R':
            return FILE_ATTR_READONLY;
        case 'H':
            return FILE_ATTR_HIDDEN;
        case 'S':
            return FILE_ATTR_SYSTEM;
        case 'A':
            return FILE_ATTR_ARCHIVE;
        default:
            return 0;
    }
}

static int cmd_attrib_run(char *arg)
{
    char *fileName;
    int rc;
    int attrs;
    int mask;

    CMD_REQUIRES_ARGS(arg);
    arg = stringTrimBoth(arg);
    fileName = arg;
    arg = splitFirstWord(arg);
    rc = PosGetFileAttributes(fileName,&attrs);
    if (rc != POS_ERR_NO_ERROR)
    {
        showError(rc);
        return 1;
    }
    for (;;)
    {
        arg = stringTrimBoth(arg);
        if (*arg == 0)
            break;
        if (arg[0] != '+' && arg[0] != '-')
        {
            showUnrecognisedArgsMsg(arg);
            return 1;
        }
        mask = getAttrMask(arg[1]);
        if (mask == 0)
        {
            showUnrecognisedArgsMsg(arg);
            return 1;
        }
        attrs &= ~mask;
        if (arg[0] == '+')
            attrs |= mask;
        rc = PosSetFileAttributes(fileName,attrs);
        if (rc != POS_ERR_NO_ERROR)
        {
            showError(rc);
            return 1;
        }
        arg += 2;
    }
    printf("%c%c%c%c%c %s\n",
        ((attrs&FILE_ATTR_ARCHIVE)!=0)?'A':'-',
        ((attrs&FILE_ATTR_DIRECTORY)!=0)?'D':'-',
        ((attrs&FILE_ATTR_HIDDEN)!=0)?'H':'-',
        ((attrs&FILE_ATTR_READONLY)!=0)?'R':'-',
        ((attrs&FILE_ATTR_SYSTEM)!=0)?'S':'-',
        fileName);
    return 0;
}
