/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  pdos - generic OS                                                */
/*                                                                   */
/*********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <locale.h>
#include <ctype.h>
#include <assert.h>
#include <setjmp.h>

#include <pos.h>

#include <exeload.h>

#include <__os.h>

#include <__memmgr.h>
extern int __mmgid;

#include <fat.h>

#ifdef NEED_BIGFOOT
#include <fasc.h>
#endif

extern int __minstart;

#ifdef __CC64__
extern int __ncallbacks;
#endif

#ifdef EBCDIC
#define CHAR_ESC_STR "\x27"
#else
#define CHAR_ESC_STR "\x1b"
#endif

/*use the FILENAME_MAX in stdio.h*/
#if 0
#define MAX_PATH 260 /* With trailing '\0' included. */
#endif

#define BIOS_SEEK_SET SEEK_SET
#define BIOS_IONBF _IONBF
#define BIOS_IOLBF _IOLBF
#define BIOS_BUFSIZ BUFSIZ

extern OS *bios;
extern int __start(char *p);

static unsigned int currentDrive = 2;

extern int salone;

static int lastcc = 0;

/* do we want auto-translation of text files from ASCII to EBCDIC? */
static int ascii_flag;

extern char *__envptr;

extern int __genstart;
extern int (*__genmain)(int argc, char **argv);

char mycmdline[400];
static char mycmdline2[400];

static int allbios = 0; /* send all files to pseudobios? */

/* if current directory is root directory, this is empty */
static char cwd[FILENAME_MAX] = "";

static char *PosGetCommandLine2(void);

static OS os = { __start, 0, 0, mycmdline, printf, 0, malloc, NULL, NULL,
  fopen, fseek, fread, fclose, fwrite, fgets, strchr,
  strcmp, strncmp, strcpy, strlen, fgetc, fputc,
  fflush, setvbuf,
  PosGetDTA, PosFindFirst, PosFindNext,
  PosGetDeviceInformation, PosSetDeviceInformation,
  ctime, time,
  PosChangeDir, PosMakeDir, PosRemoveDir,
  remove,
  memcpy, strncpy, strcat, 0 /* stderr */, free, abort, memset, fputs, fprintf,
  getenv, memmove, exit, memcmp, _errno, tmpnam, vfprintf, ungetc, vsprintf,
  sprintf, signal, raise, calloc, realloc, atoi, strtol, strtoul, qsort,
  bsearch, localtime, clock, strerror, strrchr, strstr, strpbrk, strspn,
  strcspn, memchr, ftell, abs, setlocale, perror, rewind, strncat, sscanf,
  isalnum, isxdigit, rename, clearerr, _assert, atof,
  isdigit, isalpha, isprint, isspace, tolower, system,
  islower, isupper, atexit, ceil, toupper, iscntrl,
  sin, cos, tan, floor, asin, acos, atan, sinh, cosh, tanh,
  rand, srand, strftime, puts,
  pow, modf, log, log10, atan2, fabs, exp, sqrt,
  strtok, atol, mktime, vprintf, ferror, putc, feof, getc,
  getchar, putchar, PosExec, longjmp,
  0, /* service call */
  PosGetCommandLine2,
  PosGetReturnCode,
  strtod, tmpfile, ispunct,
  0, /* Atari trap1 */
};

static int (*pgastart)(OS *os);

static void *disk;

static DTA origdta;

static int stdin_raw = 0;

#define SECTSZ 512

static unsigned long lba;
static unsigned char sect[SECTSZ];

static FAT fat;
static FATFILE fatfile;

static void *mem_base;


#ifdef __SUBC__
static char myname[1];
#else
static char *myname = "";
#endif

#define MAX_HANDLE 20

static struct {
    int inuse;
    void *fptr;
    FATFILE ff;
} handles[MAX_HANDLE];


static void runexe(char *prog_name);


static void readLogical(void *diskptr, unsigned long sector, void *buf);
static void writeLogical(void *diskptr, unsigned long sector, void *buf);
static void getDateTime(FAT_DATETIME *ptr);
static void convertPath(char *dirname);
static int formatPath(const char *input, char *output);


#ifdef ATARICLONE
static long atariTrap1(short cnt, void *s);
#endif

/* The BIOS C library will call this, and then we call our own C library */
/* Don't rely on the BIOS having a C library capable of breaking down a
   command line buffer, but there will at least be a program name, possibly
   an empty string. Our own C library can break it down though. */

#ifdef __CC64__
$callback
#endif
int biosmain(int argc, char **argv)
{
    if (argc >= 1)
    {
#ifndef __SUBC__
        myname = argv[0];
#endif
    }
    return (__start(0));
}

int main(int argc, char **argv)
{
    unsigned char lbabuf[4];
    int argupto = 1;
    int havedisk = 0;
    char *config = NULL;
    char *pcomm_name = ":pcomm.exe";

    __genstart = 1;
    os.main = &__genmain;
    os.Xstdin = stdin;
    os.Xstdout = stdout;
    os.Xstderr = stderr;

#ifdef ATARICLONE
    os.Xtrap1 = atariTrap1;
#endif

    bios->Xsetvbuf(bios->Xstdin, NULL, _IONBF, 0);
    handles[0].fptr = bios->Xstdin;
    handles[1].fptr = bios->Xstdout;
    handles[2].fptr = bios->Xstderr;
    mem_base = bios->malloc(bios->mem_amt);
    if (mem_base == NULL)
    {
        bios->Xfwrite("failed to do promised malloc\n", 29, 1, bios->Xstdout);
        bios->Xsetvbuf(bios->Xstdin, NULL, _IOLBF, 0);
        return (EXIT_FAILURE);
    }

    /* C library will have done this already */
    /* for some implementations maybe? */
    /* no, I don't think any implementations will do this */
    /* They probably should - zpdos is trying that */
    /* Not clear how anything couldn't */

#ifndef DONT_MM
    memmgrDefaults(&__memmgr);
    memmgrInit(&__memmgr);
#endif
    memmgrSupply(&__memmgr, mem_base, bios->mem_amt);

    /* printf(CHAR_ESC_STR "[2J"); */
    printf("welcome to PDOS-generic\n");
    printf("running as %s\n", argv[0]);
    if (argc < 2)
    {
        printf("must provide disk name as a parameter\n");
        printf("or at least -c config.sys or whatever\n");
        printf("-allbios will send all files to pseudobios to open\n");
        bios->free(mem_base);
        bios->Xsetvbuf(bios->Xstdin, NULL, _IOLBF, 0);
        return (EXIT_FAILURE);
    }
    printf("before printing parm\n");
    printf("argv1 is %s\n", argv[1]);
    if ((strcmp(argv[argupto], "-c") == 0)
        || (strcmp(argv[argupto], "-C") == 0)
       )
    {
        config = argv[argupto + 1];
        argupto += 2;
    }
    if (strcmp(argv[argupto], "-allbios") == 0)
    {
        allbios = 1;
        argupto++;
    }
    __envptr = "COMSPEC=\\COMMAND.EXE\0\0\0"; /* extra 3 NULs to signify no program name */
    if (argc > argupto)
    {
        printf("about to open disk\n");
        /* for (;;) ; */
        disk = bios->Xfopen(argv[argupto], "r+b");
        if (disk == NULL)
        {
            printf("can't open hard disk %s\n", argv[argupto]);
            bios->free(mem_base);
            bios->Xsetvbuf(bios->Xstdin, NULL, _IOLBF, 0);
            return (EXIT_FAILURE);
        }
        printf("done open\n");
        {
            /* we should be able to do this, but for now, we'll read
            an entire sector to unburden the C library */
#if 0
            bios->Xfseek(disk, 0x1be + 0x8, BIOS_SEEK_SET);
            printf("done seek\n");
            bios->Xfread(lbabuf, 1, 4, disk);
            printf("done read\n");
#else
            bios->Xfseek(disk, 0, BIOS_SEEK_SET);
            bios->Xfread(sect, 1, 512, disk);
            /* this is not ideal as the MBR code could contain this */
            /* this is to support drives with just a single VBR, no MBR */
            /* EFI effectively gives us this, and in addition, we need
            to squelch the hidden sectors */
            if ((memcmp(sect + 0x52, "FAT32", 5) == 0)
                || (memcmp(sect + 0x36, "FAT12", 5) == 0)
                || (memcmp(sect + 0x36, "FAT16", 5) == 0))
            {
                memcpy(lbabuf, "\x00\x00\x00\x00", 4);
                memcpy(sect + 11 + 17, "\x00\x00\x00\x00", 4);
            }
            else
            {
                memcpy(lbabuf, sect + 0x1be + 0x8, 4);
            }
#endif
        }
        lba = ((unsigned long)lbabuf[3] << 24)
            | ((unsigned long)lbabuf[2] << 16)
            | (lbabuf[1] << 8)
            | lbabuf[0];
        printf("lba is %lx\n", lba);
        if (lba != 0)
        {
            bios->Xfseek(disk, lba * SECTSZ, BIOS_SEEK_SET);
            bios->Xfread(sect, SECTSZ, 1, disk);
        }
        printf("fat type is %.5s\n", &sect[0x36]);
        fatDefaults(&fat);
        fatInit(&fat, &sect[11], readLogical, writeLogical, disk, getDateTime);
    }


    if (config != NULL)
    {
        if (strchr(config, ':') != NULL)
        {
            FILE *fp;
            char buf[100];
            char *p;

            printf("opening %s\n", config);
            fp = bios->Xfopen(config, "r");
            bios->Xfgets(buf, sizeof buf, fp);
            p = strchr(buf, '=');
            if (p != NULL)
            {
               p++;
               pcomm_name = p;
               p = strchr(pcomm_name, '\n');
               if (p != NULL)
               {
                   *p = '\0';
               }
               strcpy(mycmdline, pcomm_name);
               p = strchr(pcomm_name, ' ');
               if (p != NULL)
               {
                   *p = '\0';
               }
               printf("shell name is %s\n", pcomm_name);
            }
            bios->Xfclose(fp);
        }
    }
    runexe(pcomm_name);
    /* we need to rely on the C library to do the termination when it
       has finished closing files */
#ifndef DONT_MM
    memmgrTerm(&__memmgr);
#endif
    bios->free(mem_base);
    bios->Xsetvbuf(bios->Xstdin, NULL, _IOLBF, 0);
    return (0);
}



#if defined(NEED_VSE) || defined(NEED_MVS) || defined(NEED_BIGFOOT)

#include <mfsup.h>

static int service_call(int svcnum, void *a, void *b)
{
    printf("got service call %d\n", svcnum);
    if ((svcnum == 0) && ascii_flag) /* Linux Bigfoot */
    {
        REGS *regs;
        int func_code;
        int len;
        int x;
        int c;
        char *buf;

        regs = a;
        func_code = regs->r[1];
        if (func_code == 1)
        {
            printf("can't exit (%d) currently\n", regs->r[5]);
        }
        else if (func_code == 5) /* open */
        {
            printf("open isn't doing anything currently\n");
        }
        else if (func_code == 4) /* write */
        {
            /* r[5] has handle - assume stdout for now */
            buf = (char *)regs->r[6];
            len = regs->r[7];
            printf("have len %d to write, first byte %x\n", len, buf[0]);
            printf("will convert to EBCDIC\n");
            for (x = 0; x < len; x++)
            {
                c = fasc(buf[x]);
                putc(c, stdout);
            }
        }
    }
    else if (svcnum == 0) /* VSE */
    {
        REGS *regs;
        CCB *ccb;
        CCW *ccw;
        char *msg;

        regs = a;
        ccb = (CCB *)regs->r[1];
        ccw = ccb->actual.ccw_address;
        msg = (char *)(ccw->actual.addr & 0xffffff);
        printf("len is %d\n", ccw->actual.len);
        printf("msg is %.*s\n", ccw->actual.len, msg);
    }
    else if (svcnum == 35) /* MVS WTO */
    {
        REGS *regs;
        int len;
        char *buf;

        regs = a;
        buf = (char *)regs->r[1];
        len = *(short *)buf;
        len -= 4;
        buf += 4;
        printf("%.*s\n", len, buf);
    }
    return (0);
}
#endif



#ifdef ATARICLONE

extern FILE *__userFiles[__NFILE];

static FILE *handtofile(int handle)
{
    FILE *fp;

    if (handle == 0)
    {
        fp = stdin;
    }
    else if (handle == 1)
    {
        fp = stdout;
    }
    else if (handle == 2)
    {
        fp = stderr;
    }
    else
    {
        fp = __userFiles[handle - 3];
    }
    return (fp);
}

static long atariTrap1(short cnt, void *s_in)
{
    int opcode;

    opcode = *(short *)s_in;
    /* printf("opcode is %d\n", opcode); */
    if (opcode == 64)
    {
        struct { short opcode;
                 short handle;
                 long count;
                 void *buf; } *s = (void *)s_in;

        fwrite(s->buf, 1, s->count, handtofile(s->handle));
    }
    else if (opcode == 72)
    {
        struct { short opcode;
                 long count; } *s = (void *)s_in;

        return ((long)malloc(s->count));
    }
    else if (opcode == 73)
    {
        struct { short opcode;
                 void *buf; } *s = (void *)s_in;

        free(s->buf);
        return (0);
    }
    return (0);
}


int callatr2(unsigned char *basepage, unsigned char *altbase, OS *os, void *st);

static int callatr(char *cmd, OS *os, void *st)
{
    unsigned char basepage[256];
    int rc;

    memset(basepage, 0, sizeof basepage);

    /* there may be an ARGV standard to allow longer
       command lines */
    if (strlen(cmd) <= 124)
    {
        basepage[128] = strlen(cmd);
        strcpy(basepage + 129, cmd);
    }

    rc = callatr2(basepage, (unsigned char *)0xffffffff, os, st);
    return (rc);
}
#endif



/* mycmdline must have been populated first */

static void runexe(char *prog_name)
{
    unsigned char *entry_point;
    unsigned char *p = NULL;
    int ret;
    int old_ascii;
#ifdef __CC64__
    int old_n;
#endif

    if (exeloadDoload(&entry_point, prog_name, &p) != 0)
    {
        printf("failed to load program\n");
        return;
        for (;;) ;
        exit(EXIT_FAILURE);
    }
    pgastart = (void *)entry_point;

    old_ascii = ascii_flag;
    ascii_flag = 0;
#ifdef NEED_DELAY

#ifdef __ARMGEN__
    {
        /* 1, 0 works */
        /* 0, 50000000 works */
        /* 0, 5000000 sometimes fails */
        /* 0, 10000000 works, so double to be hopefully safe */
        /* Although that is apparently enough for a Pinebook Pro,
           it wasn't enough for an armv7 netbook, which continued
           to randomly fail. So 1 second was added */
        static unsigned int val1[2] = {1, 20000000UL};
        unsigned int val2[2];
        printf("sleeping before executing BSS memory\n");
        __nanosleep(val1, val2);
    }
#else
    for (rc = 0; rc < 500; rc++)
    {
        printf("please accept a delay before we execute program "
               "in BSS memory\n");
    }
#endif

#endif


#ifdef NEED_FLUSH
    __cacheflush(mem_base, bios->mem_amt, 0);
#endif


#if defined(NEED_VSE) || defined(NEED_MVS) || defined(NEED_BIGFOOT)
    os.Xservice = service_call;

#if defined(NEED_BIGFOOT)
    if (memcmp(entry_point + 12, "\x50\x47\x43\x58", 4) == 0)
    {
        *(void **)(entry_point + 20) = &os;
        ascii_flag = 1;
    }
#else
    if (memcmp(entry_point + 4, "PGCX", 4) == 0)
    {
        *(void **)(entry_point + 12) = &os;
    }
#endif

#endif

    __mmgid += 256;
    printf("about to call app at address %p\n", pgastart);
    /* printf("first byte is %x\n", *(unsigned char *)pgastart); */

#ifdef __CC64__
    old_n = __ncallbacks;
#endif


#ifdef ATARICLONE
    /* this is not ideal, and will presumably need a
       command.exe test eventually too */
    if (strstr(prog_name, "pcomm") != NULL)
    {
        ret = pgastart(&os);
    }
    else
    {
        ret = callatr(mycmdline, &os, entry_point);
    }
#else

    if (salone)
    {
        __genmain = (void *)pgastart;
        ret = __start(mycmdline);
        salone = 0;
        __genmain = 0;
    }
    else
    {
#ifdef __CC64__
        ret = (*pgastart)(&os);
#else
        ret = pgastart(&os);
#endif
    }
#endif


#ifdef __CC64__
    /* we need to restore the original n_callbacks value,
       otherwise we get leaks, probably because a longjmp
       is bypassing the pop */
    __ncallbacks = old_n;
#endif

    printf("return from app is hex %x\n", ret);
    memmgrFreeId(&__memmgr, __mmgid);
    __mmgid -= 256;
    lastcc = ret;
    ascii_flag = old_ascii;

    free(p);
    return;
}



/* the intention is to replace PosOpenFile/PosCreatFile, PosReadFile etc
   with PosFopen, PosFread etc. Starting with just PosOpenFile. These
   new functions will probably not get interrupt numbers assigned, as
   they are only used internally, and it is expected that apps use
   the now-exported fopen, fread etc */

/* currently a handle is being used instead of a pointer to the
   handle location, but that is expected to be rectified in due course */

void *PosFopen(const char *name, const char *mode)
{
    int ret;
    int x;
    int bios_file = 0;

    /* printf("got request to open %s\n", name); */
    for (x = 3; x < MAX_HANDLE; x++)
    {
        if (!handles[x].inuse) break;
    }
    if (x == MAX_HANDLE)
    {
        return (NULL);
    }
    if (name[0] == ':')
    {
        name++;
        bios_file = 1;
    }
    else if (strchr(name, ':') != NULL)
    {
        /* this allows a device to be opened, but we need better
           logic for when we want to reference a file on an FAT
           drive */
        bios_file = 1;
    }
    else if (allbios)
    {
        bios_file = 1;
    }
    if (bios_file)
    {
        handles[x].fptr = bios->Xfopen(name, mode);
        if (handles[x].fptr != NULL)
        {
            handles[x].inuse = 1;
            return ((void *)(ptrdiff_t)x);
        }
        else
        {
            return (NULL);
        }
    }
    {
        char fullname[FILENAME_MAX];

#if 0
        strcpy(fullname, "");
        if (name[0] == '\\')
        {
            /* if they provide the full path, don't use cwd */
        }
        else if (cwd[0] != '\0')
        {
            strcat(fullname, cwd);
            strcat(fullname, "\\");
        }
        strcat(fullname, name);
#endif
        formatPath(name, fullname);
        ret = fatOpenFile(&fat, fullname + 3, &handles[x].ff);
    }
    if (ret != 0) return (NULL);
    handles[x].inuse = 1;
    return ((void *)(ptrdiff_t)x);
}

int PosOpenFile(const char *name, int mode, int *handle)
{
    int ret;
    int x;
    int bios_file = 0;

    /* printf("got request2 to open %s\n", name); */
    for (x = 3; x < MAX_HANDLE; x++)
    {
        if (!handles[x].inuse) break;
    }
    if (x == MAX_HANDLE)
    {
        return (1);
    }
    if (name[0] == ':')
    {
        name++;
        bios_file = 1;
    }
    else if (strchr(name, ':') != NULL)
    {
        /* this allows a device to be opened, but we need better
           logic for when we want to reference a file on an FAT
           drive */
        bios_file = 1;
    }
    else if (allbios)
    {
        bios_file = 1;
    }
    if (bios_file)
    {
        handles[x].fptr = bios->Xfopen(name, "r+b");
        if (handles[x].fptr != NULL)
        {
            *handle = x;
            handles[x].inuse = 1;
            return (0);
        }
        else
        {
            return (1);
        }
    }
    {
        char fullname[FILENAME_MAX];
        
#if 0
        strcpy(fullname, "");
        if (name[0] == '\\')
        {
            /* if they provide the full path, don't use cwd */
        }
        else if (cwd[0] != '\0')
        {
            strcat(fullname, cwd);
            strcat(fullname, "\\");
        }
        strcat(fullname, name);
#endif
        formatPath(name, fullname);
        ret = fatOpenFile(&fat, fullname + 3, &handles[x].ff);
    }
    if (ret != 0) return (1);
    *handle = x;
    handles[x].inuse = 1;
    return (0);
}

int PosCloseFile(int fno)
{
    /* printf("got request to close %d\n", fno); */
    if (handles[fno].fptr)
    {
        bios->Xfclose(handles[fno].fptr);
        handles[fno].fptr = NULL;
    }
    else
    {
    }
    handles[fno].inuse = 0;
    return (0);
}

int PosCreatFile(const char *name, int attrib, int *handle)
{
    int ret;
    int x;
    int bios_file = 0;

    /* printf("got request to create %s\n", name); */
    for (x = 3; x < MAX_HANDLE; x++)
    {
        if (!handles[x].inuse) break;
    }
    if (x == MAX_HANDLE)
    {
        return (1);
    }
    if (name[0] == ':')
    {
        name++;
        bios_file = 1;
    }
    else if (strchr(name, ':') != NULL)
    {
        /* this allows a device to be opened, but we need better
           logic for when we want to reference a file on an FAT
           drive */
        bios_file = 1;
    }
    else if (allbios)
    {
        bios_file = 1;
    }
    if (bios_file)
    {
        handles[x].fptr = bios->Xfopen(name, "w+b");
        if (handles[x].fptr != NULL)
        {
            *handle = x;
            handles[x].inuse = 1;
            return (0);
        }
        else
        {
            return (1);
        }
    }
    {
        char fullname[FILENAME_MAX];
#if 0 
        strcpy(fullname, "");
        if (name[0] == '\\')
        {
            /* if they provide the full path, don't use cwd */
        }
        else if (cwd[0] != '\0')
        {
            strcat(fullname, cwd);
            strcat(fullname, "\\");
        }
        strcat(fullname, name);
#endif
        formatPath(name, fullname);
        ret = fatCreatFile(&fat, fullname + 3, &handles[x].ff, attrib);
    }
    if (ret != 0) return (1);
    *handle = x;
    handles[x].inuse = 1;
    return (0);
}

int PosReadFile(int fh, void *data, unsigned int bytes, unsigned int *readbytes)
{
    /* printf("got request to read %lu bytes\n", (unsigned long)bytes); */
    if (0) /* (fh < 3) */
    {
        /* we need to use fgets, not fread, because we need a function
           that will terminate at a newline. If the caller is actually
           trying to do an fgets themselves, then we will be reading 1
           less byte here because of the terminating NUL allowance. But
           that doesn't matter either, because if we hit that border
           condition, the caller will just issue another PosReadFile
           until they get the newline character they need.

           Note that that is normal line mode reading from stdin. The
           caller doesn't have the option to wait until the buffer is
           full, so that doesn't need to be considered. And if they are
           doing character-oriented input, then a call to this function
           with a length of 1 will also work when passed to fgets. */
        bios->Xfgets(data, bytes, bios->Xstdin);
        *readbytes = strlen(data);
        /* printf("got %d bytes\n", *readbytes); */
    }
    else
    {
        if (handles[fh].fptr != NULL)
        {
            *readbytes = bios->Xfread(data, 1, bytes, handles[fh].fptr);
            /* printf("got %d bytes from bios\n", *readbytes); */
        }
        else
        {
            fatReadFile(&fat, &handles[fh].ff, data, bytes, readbytes);
        }
    }
    /* printf("read %lu bytes\n", (unsigned long)*readbytes); */
    return (0);
}

int PosWriteFile(int fh,
                 const void *data,
                 unsigned int len,
                 unsigned int *writtenbytes)
{
    if (fh < 3)
    {
        bios->Xfwrite(data, 1, len, bios->Xstdout);
        bios->Xfflush(bios->Xstdout);
    }
    else
    {
        if (handles[fh].fptr != NULL)
        {
            *writtenbytes = bios->Xfwrite(data, 1, len, handles[fh].fptr);
        }
        else
        {
            fatWriteFile(&fat, &handles[fh].ff, data, len, writtenbytes);
        }
    }
    return (0);
}

static int dirCreat(const char *dnm, int attrib)
{
    const char *p;
    int drive;
    int rc;
    char parentname[FILENAME_MAX];
    char *end;
    char *temp;

    p = strchr(dnm, ':');
    if (p == NULL)
    {
        p = dnm;
        drive = currentDrive;
    }
    else
    {
        drive = *(p - 1);
        drive = toupper(drive) - 'A';
        p++;
#if 0
        if (drive < 2)
        {
            accessDisk(drive);
        }
#endif
    }

    if ((p[0] == '\\') || (p[0] == '/'))
    {
        p++;
    }

    memset(parentname,'\0',sizeof(parentname));
    end = strrchr(p, '\\');
    temp = strrchr(p, '/');

    if(!end || (temp > end))
    {
        end = temp;
    }
    if(end)
    {
        strncpy(parentname,p,(end-p));
    }
    /* this only operates on the main FAT drive for now */
    rc = fatCreatDir(&fat, p, parentname, attrib);
    return (rc);
}

/*convert '/' to '\\' of the path*/
static void convertPath(char *dirname)
{
    char* p = dirname;
    while (*p)
    {
        if (*p == '/') *p = '\\';
        ++p;    
    }
}


static int formatPath(const char *input, char *output)
{
    char *p, *q, *k, *z;
    int n, len;
    char temp[FILENAME_MAX];
    
    len = strlen(input);
    strcpy(temp, input);
    convertPath(temp);

    p = temp;
    /*
    e.g. \from\1.txt the function corrects it to
    e.g. c:\from\1.txt.
    */
    if (p[0] == '\\')
    {
        output[0] = 'A' + currentDrive;
        strcpy(output + 1, ":");
        strcat(output, p);
    }
    /*
    The user provides the file name in full format, don't use cwd
    e.g. c:\from\1.txt
    */
    else if((len >= 3) && (memcmp(p + 1, ":\\", 2) == 0))
    {
        strcpy(output, p);
    }
    /*
    The user create at the root directory but misses the '\' e.g. c:1.txt.
    */
    else if (len >= 2 && p[1] == ':' && p[2] != '\\')
    {
        memcpy(output, p, 2);
        memcpy(output + 2, "\\", 2);
        strcat(output, p + 2);
    }
    /*
    The user provides only the <file-name>
    e.g. 1.txt in that case the drive name,'\'
    and currect working directory needs to be
    prepended e.g. c:\from\1.txt.
    */
    else
    {
        /* The user provided '.' or .\test */
        if (p[0] == '.')
        {
            if (p[1] == '\\')
            {
                p += 2;
            }
            else if (p[1] == '\0') 
            {
                p++;
            }
        }
        output[0] = 'A' + currentDrive;
        strcpy(output + 1, ":");
        strcat(output, "\\");
        strcat(output, cwd);
        if(strcmp(cwd, "") != 0)
        {
            strcat(output, "\\");
        }
        strcat(output, p);
    }
    
    /* Checks for '\' or '/' before the null terminator and removes it. */
    p = strchr(output, '\0') - 1;
    if (p[0] == '\\') p[0] = '\0';

    /*parse the '..' in the path*/
    q = strstr(output, "..");
    k = NULL, z = NULL;
    n = 0;
    while (q)
    {
        n++;
        if (n == 1) 
        {
            k = q - 2;
        }
        else 
        {
            k--;
        }
        
        /*loop until found the '\' or the first of string*/
        while (k[0] != '\\' && k[0] != ('A' + currentDrive))
        {
             k--;
        }

        q += 2;
        /*backup the last position of '..' */
        z = q;
        q = strstr(q, "..");
    }
    if (k && z ) strcpy(k, z);
    
    return (0);
}


int PosMakeDir(const char *dirname)
{
    int ret = 0;
    char dname[FILENAME_MAX];

    formatPath(dirname, dname);
    /*the dname is a full path, we need to ignore three characters.*/
    ret = fatCheckDir(&fat, dname + 3);
    if(ret)
    {
        dirCreat(dname, 0);
    }
    
    return (0);
}


int PosChangeDir(const char *dirname)
{
    char temp_dir[FILENAME_MAX] = {0};
    char dname[FILENAME_MAX] = {0};
    const char *start, *end;
    int len = strlen(dirname);
    int result;

    if (dirname[0] == '\0') return(0);

    /*trim space at start and end of dirname*/
    start = dirname;
    end = dirname + len - 1;
    while (1)
    {
        if (*start != ' ' && *end != ' ')
        {
            strncpy(temp_dir, start, (end + 1 - start));
            break;
        }
        if (*start == ' ') ++start;
        if (*end == ' ') --end;
    }

     /*return if cd to root directory*/
    if (len == 1 && temp_dir[0] == '\\' )
    {
        cwd[0] = '\0';
        return (0);
    }
    else if(len == 2 && temp_dir[0] == '.' && temp_dir[1] == '.')
    {
        char *p = strrchr(cwd, '\\');
        if (p) *p = '\0';
        else cwd[0] = '\0';

        return (0);
    }
    
    formatPath(temp_dir, dname);
    result = fatCheckDir(&fat, dname + 3);
    if(!result) {
        strcpy(cwd, dname + 3);
    } 
    else {
        printf("cd %s: No such file or directory\n", temp_dir);
    }

    return (0);
}



/* We should check that the directory is empty before removing it,
   but we are currently not doing so */
   
int PosRemoveDir(const char *dirname)
{
    int ret = 0;
    char dname[FILENAME_MAX];

    formatPath(dirname, dname);
    /*the dname is a full path, we need to ignore three characters.*/
    ret = fatCheckDir(&fat, dname + 3);
    if(!ret) 
    {
        ret = fatDeleteFile(&fat, dname + 2);
    }
    return (ret);   
}


int PosMoveFilePointer(int handle, long offset, int whence, long *newpos)
{
    if (handles[handle].fptr != NULL)
    {
        bios->Xfseek(handles[handle].fptr, offset, SEEK_SET);
        *newpos = offset;
    }
    else
    {
        *newpos = fatSeek(&fat, &handles[handle].ff, offset, whence);
    }
    return (*newpos);
}

void *PosAllocMem(unsigned int size, unsigned int flags)
{
    char *p;

    /* printf("got request to allocate %lu bytes\n",
        (unsigned long)size); */
    p = memmgrAllocate(&__memmgr, size, 0);
    return (p);
}

int PosFreeMem(void *ptr)
{
    memmgrFree(&__memmgr, ptr);
    return (0);
}

int PosGetDeviceInformation(int handle, unsigned int *devinfo)
{
    *devinfo = stdin_raw << 5;
    return (0);
}

int PosSetDeviceInformation(int handle, unsigned int devinfo)
{
/* stay in unbuffered mode */
#if 0
    stdin_raw = ((devinfo & (1 << 5)) != 0);
    if (stdin_raw)
    {
        bios->Xsetvbuf(bios->Xstdin, NULL, BIOS_IONBF, 0);
    }
    else
    {
        bios->Xsetvbuf(bios->Xstdin, NULL, BIOS_IOLBF, BIOS_BUFSIZ);
    }
#endif
    return (0);
}

void PosGetSystemDate(unsigned int *year,
                      unsigned int *month,
                      unsigned int *day,
                      unsigned int *dw)
{
    return;
}

void PosGetSystemTime(unsigned int *hour, unsigned int *minute,
                      unsigned int *second, unsigned int *hundredths)
{
    return;
}

int PosDeleteFile(const char *name)
{
    int ret;
    char fullname[FILENAME_MAX];

    strcpy(fullname, "");
    if (name[0] == '\\')
    {
        /* if they provide the full path, don't use cwd */
    }
    else if (cwd[0] != '\0')
    {
        strcat(fullname, cwd);
        strcat(fullname, "\\");
    }
    strcat(fullname, name);
    ret = fatDeleteFile(&fat, fullname);
    return (ret);
}

int PosRenameFile(const char *old, const char *new)
{
    int ret = 0;
    char f1[FILENAME_MAX];
    char f2[FILENAME_MAX];

    formatPath(old, f1);

    ret = fatCheckFile(&fat, f1 + 3);
    if (ret == POS_ERR_NO_ERROR)
    {
        formatPath(new, f2);
        ret = fatRenameFile(&fat, f1 + 3, f2 + 3);
    }
    return (ret);
}

int PosExec(char *prog, POSEXEC_PARMBLOCK *parmblock)
{
    strcpy(mycmdline, (char *)parmblock->cmdtail);
    strcpy(mycmdline2, (char *)parmblock->cmdtail);
    runexe(prog);
    return (0);
}

int PosGetReturnCode(void)
{
    return (lastcc);
}

void PosTerminate(int rc)
{
    return;
}

char *PosGetCommandLine(void)
{
    return (mycmdline);
}

static char *PosGetCommandLine2(void)
{
    return (mycmdline2);
}

void *PosGetEnvBlock(void)
{
    return (void *)__envptr;
}

void *PosGetDTA(void)
{
    return (&origdta);
}

static int ff_search(void)
{
    DIRENT dirent;
    unsigned int readbytes;

    while (1)
    {
        fatReadFile(&fat, &fatfile, &dirent, sizeof dirent, &readbytes);
        if (readbytes != sizeof dirent) return (1);
        if (dirent.file_name[0] == '\0') return (1);
        if (dirent.file_name[0] == DIRENT_DEL) continue;
        strncpy(origdta.file_name, (char *)dirent.file_name, 11);
        origdta.file_name[11] = '\0';
        break;
    }
    return (0);
}

int PosFindFirst(char *pat, int attrib)
{
    /*make sure the cwd point to current directory.*/
    fatOpenFile(&fat, cwd, &fatfile);
    return (ff_search());
}

int PosFindNext(void)
{
    return (ff_search());
}

unsigned int PosMonitor(void)
{
    printf("no monitor available\n");
    for (;;) ;
}

int PosGetMagic(void)
{
    return (PDOS_MAGIC);
}

static void readLogical(void *diskptr, unsigned long sector, void *buf)
{
    int ret;

    sector += fat.hidden;
    bios->Xfseek(diskptr, sector * SECTSZ, BIOS_SEEK_SET);
    ret = bios->Xfread(buf, SECTSZ, 1, diskptr);
    return;
}

static void writeLogical(void *diskptr, unsigned long sector, void *buf)
{
    int ret;

    sector += fat.hidden;
    bios->Xfseek(diskptr, sector * SECTSZ, BIOS_SEEK_SET);
    ret = bios->Xfwrite(buf, SECTSZ, 1, diskptr);
    return;
}

static void getDateTime(FAT_DATETIME *ptr)
{
    memset(ptr, '\0', sizeof *ptr);
    return;
}
