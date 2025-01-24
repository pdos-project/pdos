/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  mfterm - terminal mainly for mainframe                           */
/*                                                                   */
/*********************************************************************/

/* Probably want to have a generic protocol for creating
   TCP/IP packets, that doesn't rely on timeouts. So send
   an XOFF before sending either an XOFF as data, or an
   XON as a TCP/IP control. But the TCP/IP packets themselves
   are still created externally (via qemu or whatever). So
   this is not implementing SLIP/PPP (my understanding) */

/* Note that the EBCDIC ANSI protocol works in that mfterm
   solicits input from the keyboard, but won't interpret an
   ANSI X3.64 escape sequence. It just passes it to the
   mainframe. The mainframe hasn't yet completed a
   recognized escape sequence so it solicits more input.
   Hercules/380 sees the read request and sends an XON
   before waiting. That then wakes up mfterm. This protocol
   will need to be changed for a real mainframe that supports
   EBCDIC ANSI - z/PDOS will need to send an XON itself. I
   guess either side can send an XON to say "I don't know
   what the situation is - I need data from you because an
   app has done a read request and I'm about to enter into
   a wait - you'll need to wake me up with an XON from your
   end". Another thing is that to avoid the (presumed) issue
   with qemu sending a packet immediately because I only
   write 1 character to the COM port, mfterm could first send
   an XOFF and some intermediate program could recognize that
   as a halt in transmission and buffer characters until I
   send an XON to release it. In this situation the XON would
   not be sent onwards as it is assumed that the other end
   knows what to do with this sequence already. */

/* alternate form of extended - used by zPDT (aka z1090/zOPD) IBM
   hardware emulator */
/* normal extended is z/VM */
/* and the OSA-ICC never got anywhere */
/* Joes's OSA-ICC system should be 0 */
#define ALTEXT 0

/* define this if you need to negotiate past a splash screen */
#define HERCSPLASH 1

/* use XOFF logic to accumulate bytes */
#define XOFFLOGIC

#if 0

Here is a list of control characters. 6 will need to be
requisitioned to support Vietnamese. VISCII used a set of
6 characters that interferes with my use of micro-emacs etc.

ctrl-B is common between ASCII and EBCDIC so we will reserve
that for future use, and the other 6 "AVAILABLE" can be used
for Vietnamese.

I would have used ctrl-B as the menu/mode switch character
here, but there is already prior art to use ctrl-], which is
0x1d, and also common between ASCII and EBCDIC.

0 - ctrl-@ - used by C etc.
1 - ctrl-A - used by micro-emacs
2 - ctrl-B - AVAILABLE
3 - ctrl-C - used to break programs
4 - ctrl-D - used to indicate EOF in Unix
5 - ctrl-E - used by micro-emacs
6 - ctrl-F - ACK - may be AVAILABLE
7 - ctrl-G - used by micro-emacs and to beep
8 - ctrl-H - used as backspace
9 - ctrl-I - tab
A - ctrl-J - newline
B - ctrl-K - used by micro-emacs
C - ctrl-L - used by micro-emacs
D - ctrl-M - carriage return
E - ctrl-N - AVAILABLE
F - ctrl-O - AVAILABLE
10 - ctrl-P - AVAILABLE
11 - ctrl-Q - used for XON
12 - ctrl-R - formfeed, AVAILABLE
13 - ctrl-S - XOFF and used by micro-emacs
14 - ctrl-T - AVAILABLE
15 - ctrl-U - NAK - may be AVAILABLE
16 - ctrl-V - used by micro-emacs, but if page-down works can be made available
17 - ctrl-W - used by nano - may be AVAILABLE
18 - ctrl-X - used by micro-emacs
19 - ctrl-Y - used by micro-emacs
1A - ctrl-Z - used for EOF by MSDOS
1B - ctrl-[ - ESC
1C - ctrl-\ - used by Unix to kill process
1D - ctrl-] - used by Telnet to get prompt
1E - ctrl-^ - AVAILABLE
1F - ctrl-_ - AVAILABLE

#endif


#include <stdio.h>
#include <stdlib.h>

#define XON 0x11
#define XOFF 0x13
#define MENU 0x1d

/* double splash screen on bare lpar */
#define DOUBLE 0

#define IAC 0xff /* telnet */

static int noisy = 0;
static int card = 0;
/* 1057 = EBCDIC ANSI */
/* 3270 = normal 3270 */
/* 3275 = 3270 with EBCDIC ANSI embedded as hex */
/* the extra 5 is for 'E' - the 5th letter of the alphabet */
static int termtype = 1052; /* ASCII ANSI */

static char cardbuf[80];

static char keybbuf[300];

static char luname[50] = "";

static int extend = 0;

/* counter used in TN3270E */
static char buildup[5];

/* global counter of messages sent */
static int globcount = 0;

static void negotiate(FILE *sf);
static void interact(FILE *sf);
static void expect(FILE *sf, unsigned char *buf, size_t buflen);

int febc(int ebc);
int tebc(int local);

int main(int argc, char **argv)
{
    FILE *sf;
    int x = 1;

    if (argc < 2)
    {
        printf("usage: mfterm [-lu=xxx] [-3270|-3275|-1052|-1057]"
               " <serial file>\n");
        printf("establishes a telnet ANSI terminal connection to mainframe\n");
        printf("e.g. mfterm com1:\n");
        printf("1057 is an EBCDIC ANSI terminal\n");
        printf("3275 is an EBCDIC ANSI stream on top of a 3270 data stream\n");
        printf("ctrl-] gets menu when keyboard is active\n");
        printf("-x at beginning does extended 3270\n");
        return (EXIT_FAILURE);
    }
    if (argc > 2)
    {
        if (strncmp(argv[x], "-x", 2) == 0)
        {
            extend = 1;
            x++;
        }
        if (strncmp(argv[x], "-lu=", 4) == 0)
        {
            strcpy(luname, argv[x] + 4);
            x++;
        }
        if (strcmp(argv[x], "-3270") == 0)
        {
            termtype = 3270;
        }
        else if (strcmp(argv[x], "-3275") == 0)
        {
            termtype = 3275;
        }
        else if (strcmp(argv[x], "-1057") == 0)
        {
            termtype = 1057;
        }
        else if (strcmp(argv[x], "-1052") == 0)
        {
            termtype = 1052;
        }
        else
        {
            printf("unknown argument %s\n", argv[x]);
            return (EXIT_FAILURE);
        }
        x++;
    }
    if (x > (argc - 1))
    {
        x--;
    }
    sf = fopen(*(argv + x), "r+b");
    if (sf == NULL)
    {
        printf("failed to open serial file\n");
        return (EXIT_FAILURE);
    }

    negotiate(sf);
    if (termtype != 3270)
    {
        setvbuf(stdin, NULL, _IONBF, 0);
    }
    interact(sf);
    if (termtype != 3270)
    {
        setvbuf(stdin, NULL, _IOLBF, 0);
    }

    fclose(sf);
    return (0);
}

static void negotiate(FILE *sf)
{
    if (extend)
    {
        fseek(sf, 0, SEEK_CUR);
        /* IAC DO TN3270E */
        expect(sf, "\xff\xfd\x28", 3);

#if 1
        fseek(sf, 0, SEEK_CUR);
        printf("writing will 3270e\n");
        /* IAC WILL TN3270E */
        fwrite("\xff\xfb\x28", 1, 3, sf);
#else
        /* note that declining 3270e has it asking for
           terminal type and not accepting my response.
           so we'll try extended (above) instead */
        /* I think that was an unrelated error, so maybe
           try this "easy" option again? */
        /* It was indeed an unrelated error. Unfortunately
           at the end of the "easy" option, the OSA-ICC
           hung, so we're back to doing 3270e */
        fseek(sf, 0, SEEK_CUR);
        printf("writing wont 3270e\n");
        /* IAC WONT TN3270E */
        fwrite("\xff\xfc\x28", 1, 3, sf);
#endif

#ifdef XOFFLOGIC
        fputc(XOFF, sf);
#endif

    }

#if 1
    if (extend)
    {
        fseek(sf, 0, SEEK_CUR);
        /* IAC SB TN3270E SEND DEVICE-TYPE IAC SE */
        expect(sf, "\xff\xfa\x28\x08\x02\xff\xf0", 7);

        fseek(sf, 0, SEEK_CUR);
        printf("sending device type\n");
        /* luname = device-name = network name */
#if 0
        /* IAC SB TN3270E DEVICE-TYPE IS xxx CONNECT yyy IAC SE */
        fwrite("\xff\xfa\x28\x02\x00" "IBM-3278-2" "\x01", 1, 16, sf);
        fwrite(luname, 1, strlen(luname), sf);
        fwrite("\xff\xf0", 1, 2, sf);
#endif
        /* IAC SB TN3270E DEVICE-TYPE REQUEST xxx */
        fwrite("\xff\xfa\x28\x02\x07" "IBM-3278-2", 1, 15, sf);
        fwrite("-E", 1, 2, sf); /* I don't think we need this extended
                                   - try deleting after we match c3270 */
        if (strcmp(luname, "") != 0)
        {
            /* CONNECT yyy */
            fwrite("\x01", 1, 1, sf);
            fwrite(luname, 1, strlen(luname), sf);
        }
        /* IAC SE */
        fwrite("\xff\xf0", 1, 2, sf);
#ifdef XOFFLOGIC
        fputc(XOFF, sf);
#endif




        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        /* this looks like an echo of the luname - TERM - so
           we need to be more flexible with the expect - and
           may as well do an expect of the terminal type the
           same way */
        expect(sf, "\xff\xfa\x28\x02\x04", 5);
        expect(sf, "IBM-3278-2", 10);
        expect(sf, "-E", 2);
        if (strcmp(luname, "") != 0)
        {
            expect(sf, "\x01", 1);
            expect(sf, luname, strlen(luname));
        }
        expect(sf, "xff\xf0", 2);

        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        fwrite("\xff\xfa\x28\x03\x07\x00\x02\x04\xff\xf0", 1, 10, sf);
#ifdef XOFFLOGIC
        fputc(XOFF, sf);
#endif




        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        expect(sf, "\xff\xfa\x28\x03\x07\x02\xff\xf0", 8);

        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        fwrite("\xff\xfa\x28\x03\x04\x02\xff\xf0", 1, 8, sf);
#ifdef XOFFLOGIC
        fputc(XOFF, sf);
#endif




        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        expect(sf, "\x00\x00\x00\x00\x00\x0d\x44\xff\xef", 9);

        fseek(sf, 0, SEEK_CUR);
        /* send a null to get another message */
#ifdef XOFFLOGIC
        fputc(XOFF, sf);
#endif




        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        expect(sf, "\x00\x00\x00\x00\x00\x02\xff\xef", 8);

        printf("ready to write thousands of NULs\n");
        fseek(sf, 0, SEEK_CUR);
        /* don't know */
        fwrite("\x00\x00\x00\x00\x00\x60\x40\x40", 1, 8, sf);
        {
            int x;
            for (x = 0; x < 1930 - 8 - 2; x++)
            {
                fputc(0x00, sf);
            }
        }
        fwrite("\xff\xef", 1, 2, sf);
#ifdef XOFFLOGIC
        fputc(XOFF, sf);
#endif




#if 0
        fseek(sf, 0, SEEK_CUR);
        /* IAC DONT TN3270E IAC DO TERMINAL_TYPE */
        expect(sf, "\xff\xfe\x28\xff\xfd\x18", 6);

        printf("ok, we won't?\n");
        fseek(sf, 0, SEEK_CUR);
        /* IAC WONT TN3270E IAC DO TERMINAL_TYPE */
        expect(sf, "\xff\xfc\x28\xff\xfd\x18", 6);
#endif

#if 0
        fseek(sf, 0, SEEK_CUR);
        /* IAC DONT TN3270E */
        expect(sf, "\xff\xfe\x28", 3);
        /* fall through to expect request to do terminal type */
#endif
    }
#endif

    if (!extend) /* || !ALTEXT) */
    {
    fseek(sf, 0, SEEK_CUR);
    /* IAC DO TERM_TYPE */
    expect(sf, "\xff\xfd\x18", 3);

    fseek(sf, 0, SEEK_CUR);
    /* printf("writing\n"); */
    /* IAC WILL TERM_TYPE */
    fwrite("\xff\xfb\x18", 1, 3, sf);
#ifdef XOFFLOGIC
    fputc(XOFF, sf);
#endif


    fseek(sf, 0, SEEK_CUR);
    /* IAC SB TERM_TYPE SEND IAC SE */
    expect(sf, "\xff\xfa\x18\x01\xff\xf0", 6);
    
    fseek(sf, 0, SEEK_CUR);
    /* printf("writing\n"); */
    /* IAC SB TERM_TYPE IS (ANSI) IAC SE */
    fwrite("\xff\xfa\x18\x00", 1, 4, sf);
    if (HERCSPLASH || (termtype == 3270) || (termtype == 3275))
    {
        printf("writing IBM-3270\n");
        if (1) /* (!HERCSPLASH) */ /* was 1 - was - extend) */
        {
            printf("actually IBM-3278-2\n");
            fwrite("IBM-3278-2", 1, 10, sf);
        }
        else
        {
            fwrite("IBM-3270", 1, 8, sf);
        }
        if (strcmp(luname, "") != 0)
        {
            fputc('@', sf);
            fwrite(luname, 1, strlen(luname), sf);
        }
    }
    else
    {
        fwrite("ANSI", 1, 4, sf);
    }
    fwrite("\xff\xf0", 1, 2, sf);

#ifdef XOFFLOGIC
    fputc(XOFF, sf);
#endif
    }

    if ((termtype == 1052) || (termtype == 1057))
    {
    if (!extend)
    {
    /* the ICC will send 3 separate packets, and
       modem.c won't consolidate them, so this logic
       needs to be split */
#if HERCSPLASH
    fseek(sf, 0, SEEK_CUR);
    /* get fd first, but must respond fb first */
    expect(sf, "\xff\xfd\x19", 3);
    fseek(sf, 0, SEEK_CUR);
    fwrite("\xff\xfb\x19" "\xff\xfd\x19", 6, 1, sf);
#ifdef XOFFLOGIC
    fputc(XOFF, sf);
#endif

    fseek(sf, 0, SEEK_CUR);
    expect(sf, "\xff\xfb\x19" "\xff\xfd\x00", 6);
    fseek(sf, 0, SEEK_CUR);
    fwrite("\xff\xfb\x00" "\xff\xfd\x00", 6, 1, sf);
#ifdef XOFFLOGIC
    fputc(XOFF, sf);
#endif

    fseek(sf, 0, SEEK_CUR);
    expect(sf, "\xff\xfb\x00", 3);

#else
    fseek(sf, 0, SEEK_CUR);
    /* IAC WILL ECHO */
    expect(sf, "\xff\xfb\x01", 3);

    fseek(sf, 0, SEEK_CUR);
    /* printf("writing\n"); */
    /* IAC DO ECHO */
    fwrite("\xff\xfd\x01", 1, 3, sf);
#endif
    }
    } /* !extend */

    printf("looking for splash\n");
#if HERCSPLASH
    fseek(sf, 0, SEEK_CUR);
    /* splash screen? */
    /* expect(sf, "\xf5\x42\x11\x40", 4); */
        {
            int x = 0;
            int c;

            while (1)
            {
                c = fgetc(sf);
                /* printf("c from logo is %02X\n", c); */
                if (c == EOF) break;
                if (c == IAC)
                {
                    x++;
                    c = fgetc(sf);
                    if (c == 0xef)
                    {
                        x++;
                        printf("read and discarded %d bytes of logo data\n", x);
                        break;
                    }
                }
                x++;
            }
        }
#endif

#if DOUBLE
    fseek(sf, 0, SEEK_CUR);
    /* splash screen? */
    /* expect(sf, "\xf5\x42\x11\x40", 4); */
        {
            int x = 0;
            int c;

            while (1)
            {
                c = fgetc(sf);
                /* printf("c from logo is %02X\n", c); */
                if (c == EOF) break;
                if (c == IAC)
                {
                    x++;
                    c = fgetc(sf);
                    if (c == 0xef)
                    {
                        x++;
                        printf("read and discarded %d bytes of logo data\n", x);
                        break;
                    }
                }
                x++;
            }
        }
#endif

    if ((termtype == 3270) || (termtype == 3275))
    {
        fseek(sf, 0, SEEK_CUR);
#if 0
        if (extend)
        {
    expect(sf, "\xff\xfa\x18\x01\xff\xf0", 6);
        }
#endif
        /* do eor and will eor */
        if (extend && ALTEXT)
        {
            expect(sf, "\xff\xfb\x19" "\xff\xfd\x19", 6);
        }
        else
        {
            expect(sf, "\xff\xfd\x19" "\xff\xfb\x19", 6);
        }
        fseek(sf, 0, SEEK_CUR);

        if (extend && ALTEXT)
        {
            fwrite("\xff\xfb\x19" "\xff\xfd\x19", 6, 1, sf);
        }
        else
        {
            fwrite("\xff\xfd\x19" "\xff\xfb\x19", 6, 1, sf);
        }
        fseek(sf, 0, SEEK_CUR);

        printf("binary\n");
        if (extend && ALTEXT)
        {
        expect(sf, "\xff\xfb\x00" "\xff\xfd\x00", 6);
        }
        else
        {
        expect(sf, "\xff\xfd\x00" "\xff\xfb\x00", 6);
        }
        fseek(sf, 0, SEEK_CUR);
        /* and swap these too for non-ALTEXT!!! */
        if (extend && ALTEXT)
        {
        fwrite("\xff\xfb\x00" "\xff\xfd\x00", 6, 1, sf);
        }
        else
        {
        fwrite("\xff\xfd\x00" "\xff\xfb\x00", 6, 1, sf);
        }
#if !ALTEXT
        if (extend)
        {
            fseek(sf, 0, SEEK_CUR);

            /* this stuff seems to be different for each system */
            /* I think when connected directly to z/PDOS you won't
               see any of this. This might only be applicable when
               connected to ZVM. In which case, put it all behind
               a ifdef for ZVM */

#if 0
            /* IAC DO ? IAC WILL ? ? ? IAC EOR_MARK ? IAC EOR_MARK */
#if ALTEXT
            expect(sf, "\x0d\xc7\x11\x40\x40\x1d\x60\x40\xe4", 9);
#else
            /* ignore this i think - this is the old message */
            expect(sf, "\xff\xfd\x00"
                   "\xff\xfb\x00\x0d\xc2\xff\xef\x02\xff\xef", 13);
#endif
#endif

            fseek(sf, 0, SEEK_CUR);
            /* press enter */
            fwrite("\x7d\x5b\xe2\x11\x5b\x61", 6, 1, sf);
            /* insert data before enter here */
            fwrite("\xff\xef", 1, 2, sf);
            
            fseek(sf, 0, SEEK_CUR);
            expect(sf, "\x00", 1);
            return;
        }
#endif
        fseek(sf, 0, SEEK_CUR);
        {
            int x = 0;
            int c;

            while (1)
            {
                c = fgetc(sf);
#if ALTEXT
                printf("c from logo is %02X\n", c);
#endif
                if (c == EOF) break;
                if (c == IAC)
                {
                    x++;
                    c = fgetc(sf);
                    if (c == 0xef)
                    {
                        x++;
                        printf("read and discarded %d bytes of logo data\n", x);
                        break;
                    }
                }
                x++;
            }
        }
    }

    return;
}

static void expect(FILE *sf, unsigned char *buf, size_t buflen)
{
    int c;
    size_t x;

    printf("expect for %d bytes\n", buflen);
    for (x = 0; x < buflen; x++)
    {
        c = fgetc(sf);
        printf("c is ... %x\n", c);
        if (c != buf[x])
        {
            printf("got %x, expected %x\n", c, buf[x]);
            exit(EXIT_FAILURE);
        }
    }
    return;
}

static void interact(FILE *sf)
{
    int c = 0;
    FILE *cf = NULL;
    int cnt;
    int keybcode = 0;
    int ign3275;
    static char *hex = "0123456789ABCDEF";
    int gotxon;

    while (c != EOF)
    {
        fseek(sf, 0, SEEK_CUR);
        cnt = 0;
        ign3275 = 0;
        gotxon = 0;
        while (1)
        {
            /* printf("about to fgetc\n"); */
            c = fgetc(sf);
            if ((termtype == 1057) && extend)
            {
                if (cnt < sizeof buildup)
                {
                    buildup[cnt] = c;
                }
            }
            cnt++;
            /* printf("ccc is %x %d\n", c, cnt++); */
#if HERCSPLASH
            if ((cnt == 1) && (c == 0xf1))
            {
                /* printf("skip f1\n"); */
                continue;
            }
#endif

            if ((termtype == 1052) || (termtype == 1057))
            {
                if ((c == EOF) || (c == XON))
                {
                    if (extend)
                    {
                        if (cnt > sizeof buildup)
                        {
                            /* need to defer to EOR so that we
                               can do the ACK first */
                            gotxon = 1;
                        }
                        continue;
                    }
                    break;
                }
#if HERCSPLASH
                if (c == IAC) {}
                else
#endif
                if (termtype == 1057)
                {
                    c = febc(c);
                    if (c != 0)
                    {
                        fputc(c, stdout);
                    }
                }
                else
                {
                    fputc(c, stdout);
                }
                fflush(stdout);
            }
            /* hercsplash goes through both */
            if (HERCSPLASH || (termtype == 3270) || (termtype == 3275))
            {
                if (c == IAC)
                {
                    c = fgetc(sf);
                    /* EOR is end of transmission */
                    if (c == 0xef)
                    {
                        cnt = 0;
                        ign3275 = 0;
                        
                        /* the 1057 needs to ignore the Hercules/OSA markup */
                        if (termtype != 1057)
                        {
                        /* if the keyboard is unlocked, we need to type
                           something */
                        if (keybcode == 0xc3)
                        {
                            c = XON;
                            /* printf("time for keyboard\n"); */
                            break;
                        }
                        }
                        /* 1057 extended needs to respond to every message */
                        else if (extend)
                        {
                            fseek(sf, 0, SEEK_CUR);
                            buildup[0] = buildup[2];
                            buildup[2] = '\x00';
                            fwrite(buildup, 1, sizeof buildup, sf);
                            fputc(0x00, sf);
                            fwrite("\xff\xef", 1, 2, sf);
#ifdef XOFFLOGIC
                            fputc(XOFF, sf);
#endif
                            if (gotxon)
                            {
                                c = XON;
                                break;
                            }
                            fseek(sf, 0, SEEK_CUR);
                        }
                        /* printf("got EOR, continuing\n"); */
                        continue;
                    }
                    /* ignore any IAC commands */
                    if (c != IAC) continue;
                }
                /* the first character received is x'f1' which is a write
                   command, added by Hercules, not PDOS. That's what the
                   +1 is about */
                /* zPDT seems to send x'01' instead of x'f1'. no idea
                   what the difference is. */
                /* ignore first 6+1 characters */
#if !HERCSPLASH
                if (cnt == 2)
                {
                    keybcode = c;
                    /* printf("keyboard code is %x\n", c); */
                }
                if (cnt <= (6+1))
                {
                    /* printf("ignoring %x\n", c); */
                    continue;
                }
                /* ignore stuff after actual data */
                if (cnt > (1766+1)) continue;
                c = febc(c);
                if (termtype == 3275)
                {
                    char *p;
                    int newc;

                    /* if (c != ' ') printf("myc is %x\n", c); */
                    if (c == 'X')
                    {
                        ign3275 = 1;
                        continue;
                    }
                    if (ign3275) continue;
                    if (c == 0)
                    {
                        ign3275 = 1;
                        continue;
                    }
                    p = strchr(hex, c);
                    if (p == NULL)
                    {
                        ign3275 = 1;
                        continue;
                    }
                    newc = (p - hex) * 16;
                    c = fgetc(sf);
                    if (c == EOF) break;
                    cnt++;
                    c = febc(c);
                    if (c == 0)
                    {
                        ign3275 = 1;
                        continue;
                    }
                    p = strchr(hex, c);
                    if (p == NULL)
                    {
                        ign3275 = 1;
                        continue;
                    }
                    newc += (p - hex);
                    c = newc;
                    /* printf("deciphered %x\n", c); */
                    c = febc(c);
                    /* printf("which is %x\n", c); */
                    if (c == 0)
                    {
                        ign3275 = 1;
                        continue;
                    }
                    fputc(c, stdout);
                    fflush(stdout);
                }
                else
                {
                    if (c != 0)
                    {
                        fputc(c, stdout);
                        fflush(stdout);
                    }
                }
#endif
            }
        }
        if (c == XON)
        {
            fseek(sf, 0, SEEK_CUR);
            if (card)
            {
                fread(cardbuf, 1, sizeof cardbuf, cf);
                if (feof(cf))
                {
                    printf("finished IPL, returning to keyboard\n");
                    fclose(cf);
                    card = 0;
                }
                else
                {
                    fwrite(cardbuf, 1, sizeof cardbuf, sf);
                    continue;
                }
            }
            if (termtype == 3270)
            {
                unsigned char *p;
                char fixed[6] = "\x7d\x5b\xe2\x11\x5b\x61";

                printf("type something\n");
                fgets(keybbuf, sizeof keybbuf, stdin);
                /* printf("you typed x%sx\n", keybbuf); */
                p = strchr(keybbuf, '\n');
                if (p != NULL)
                {
                    *p = '\0';
                }
                p = keybbuf;
                while (*p != '\0')
                {
                    *p = tebc(*p);
                    p++;
                }
                /* I need 6 characters */
                /* that e2 changes */
                /* but I don't think I need that change */
                /* fixed[2] = 0xe1 + strlen(keybbuf); */
                fwrite(fixed, 1, 6, sf);
                fwrite(keybbuf, 1, strlen(keybbuf), sf);
                /* testing revealed i need this too */
                fwrite("\xff\xef", 1, 2, sf);
                continue;
            }
            c = fgetc(stdin);
            /* ctrl-] brings up a menu */
            if ((c != EOF) && (c != MENU))
            {
                if (termtype == 1057)
                {
                    c = tebc(c);
                    if (c != 0)
                    {
                        if (extend)
                        {
                            globcount++;
                            fwrite("\x00\x00\x00\x00", 1, 4, sf);
                            fputc(globcount, sf);
                        }
                        if (HERCSPLASH)
                        {
                            /* printf("writing 7d\n"); */
                            /* fwrite("\x7d", 1, 1, sf); */
                        }
                        /* printf("writing %x\n", c); */
                        fputc(c, sf);
                        /* this is for PDOS */
                        fputc(XON, sf);
                        if (HERCSPLASH)
                        {
                            /* printf("writing eor\n"); */
                            fwrite("\xff\xef", 1, 2, sf);
                        }
#ifdef XOFFLOGIC
                        /* this is for modem */
                        fputc(XOFF, sf);
#endif
                    }
                }
                else if (termtype == 3275)
                {
                    char fixed[6] = "\x7d\x5b\xe2\x11\x5b\x61";

                    fwrite(fixed, 1, 6, sf);
                    c = tebc(c);
                    fputc(tebc(hex[(c >> 4) & 0xf]), sf);
                    fputc(tebc(hex[c & 0xf]), sf);
                    fputc(tebc('X'), sf);
                    fputc(tebc('X'), sf);
                    fwrite("\xff\xef", 1, 2, sf);
                    continue;
                }
                else
                {
                    fputc(c, sf);
                }
                fflush(sf);
            }
            if (c == MENU)
            {
                printf("menu - options are noisy, quiet, card\n");
#if 0
                setvbuf(stdin, NULL, _IOLBF, 0);
                fgets(keybbuf, sizeof keybbuf, stdin);
                setvbuf(stdin, NULL, _IONBF, 0);
                if (strcmp(keybbuf, "noisy\n") == 0) noisy = 1;
                if (strcmp(keybbuf, "quiet\n") == 0) noisy = 0;
                if (strcmp(keybbuf, "card\n") == 0) card = 1;
#endif
                card = 1;
                if (card)
                {
                    cf = fopen("iplcrd.dat", "rb");
                    if (cf == NULL)
                    {
                        printf("failed to open iplcrd.dat\n");
                        card = 0;
                        c = fgetc(stdin);
                        continue;
                    }
                    fread(cardbuf, 1, sizeof cardbuf, cf);
                    fseek(sf, 0, SEEK_CUR);
                    fwrite(cardbuf, 1, sizeof cardbuf, sf);
                    continue;
                }
            }
        }
    }
    return;
}

/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  This program takes an integer as a parameter.  The integer       */
/*  should have a representation of an EBCDIC character.  An integer */
/*  is returned, containing the representation of that character     */
/*  in whatever character set you are using on this system.          */
/*                                                                   */
/*********************************************************************/
int febc(int ebc)
{
  switch (ebc)
  {
    case 0x16 : return('\b');
    case 0x15 : return('\n');
    case 0x25 : return('\n');
    case 0x40 : return(' ');
    case 0x5A : return('!');
    case 0x7f : return('\"');
    case 0x7b : return('#');
    case 0x5b : return('$');
    case 0x6c : return('%');
    case 0x50 : return('&');
    case 0x7d : return('\'');
    case 0x4d : return('(');
    case 0x5d : return(')');
    case 0x5c : return('*');
    case 0x4e : return('+');
    case 0x6b : return(',');
    case 0x60 : return('-');
    case 0x4b : return('.');
    case 0x61 : return('/');
    case 0xf0 : return('0');
    case 0xf1 : return('1');
    case 0xf2 : return('2');
    case 0xf3 : return('3');
    case 0xf4 : return('4');
    case 0xf5 : return('5');
    case 0xf6 : return('6');
    case 0xf7 : return('7');
    case 0xf8 : return('8');
    case 0xf9 : return('9');
    case 0x7a : return(':');
    case 0x5e : return(';');
    case 0x4c : return('<');
    case 0x7e : return('=');
    case 0x6e : return('>');
    case 0x6f : return('?');
    case 0x7c : return('@');
    case 0xc1 : return('A');
    case 0xc2 : return('B');
    case 0xc3 : return('C');
    case 0xc4 : return('D');
    case 0xc5 : return('E');
    case 0xc6 : return('F');
    case 0xc7 : return('G');
    case 0xc8 : return('H');
    case 0xc9 : return('I');
    case 0xd1 : return('J');
    case 0xd2 : return('K');
    case 0xd3 : return('L');
    case 0xd4 : return('M');
    case 0xd5 : return('N');
    case 0xd6 : return('O');
    case 0xd7 : return('P');
    case 0xd8 : return('Q');
    case 0xd9 : return('R');
    case 0xe2 : return('S');
    case 0xe3 : return('T');
    case 0xe4 : return('U');
    case 0xe5 : return('V');
    case 0xe6 : return('W');
    case 0xe7 : return('X');
    case 0xe8 : return('Y');
    case 0xe9 : return('Z');
    case 0xad : return('[');
    case 0xe0 : return('\\');
    case 0xbd : return(']');
    case 0x5f : return('^');
    case 0x6d : return('_');
    case 0x79 : return('`');
    case 0x81 : return('a');
    case 0x82 : return('b');
    case 0x83 : return('c');
    case 0x84 : return('d');
    case 0x85 : return('e');
    case 0x86 : return('f');
    case 0x87 : return('g');
    case 0x88 : return('h');
    case 0x89 : return('i');
    case 0x91 : return('j');
    case 0x92 : return('k');
    case 0x93 : return('l');
    case 0x94 : return('m');
    case 0x95 : return('n');
    case 0x96 : return('o');
    case 0x97 : return('p');
    case 0x98 : return('q');
    case 0x99 : return('r');
    case 0xa2 : return('s');
    case 0xa3 : return('t');
    case 0xa4 : return('u');
    case 0xa5 : return('v');
    case 0xa6 : return('w');
    case 0xa7 : return('x');
    case 0xa8 : return('y');
    case 0xa9 : return('z');
    case 0xc0 : return('{');
    case 0x4f : return('|');
    case 0xd0 : return('}');
    case 0xa1 : return('~');
    case 0x27 : return(0x1b); /* ESC character */
    default   : return(0);
  }
}

/*********************************************************************/
/*                                                                   */
/*  This Program Written By Paul Edwards.                            */
/*  Released to the public domain.                                   */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  This program takes an integer as a parameter.  The integer       */
/*  should have a character in the local character set.  It will be  */
/*  converted into the EBCDIC equivalent.                            */
/*                                                                   */
/*********************************************************************/

int tebc(int local)
{
  switch (local)
  {
    case '\n' : return (0x15);
    case ' '  : return (0x40);
    case '!'  : return (0x5A);
    case '\"' : return (0x7f);
    case '#'  : return (0x7b);
    case '$'  : return (0x5b);
    case '%'  : return (0x6c);
    case '&'  : return (0x50);
    case '\'' : return (0x7d);
    case '('  : return (0x4d);
    case ')'  : return (0x5d);
    case '*'  : return (0x5c);
    case '+'  : return (0x4e);
    case ','  : return (0x6b);
    case '-'  : return (0x60);
    case '.'  : return (0x4b);
    case '/'  : return (0x61);
    case '0'  : return (0xf0);
    case '1'  : return (0xf1);
    case '2'  : return (0xf2);
    case '3'  : return (0xf3);
    case '4'  : return (0xf4);
    case '5'  : return (0xf5);
    case '6'  : return (0xf6);
    case '7'  : return (0xf7);
    case '8'  : return (0xf8);
    case '9'  : return (0xf9);
    case ':'  : return (0x7a);
    case ';'  : return (0x5e);
    case '<'  : return (0x4c);
    case '='  : return (0x7e);
    case '>'  : return (0x6e);
    case '?'  : return (0x6f);
    case '@'  : return (0x7c);
    case 'A'  : return (0xc1);
    case 'B'  : return (0xc2);
    case 'C'  : return (0xc3);
    case 'D'  : return (0xc4);
    case 'E'  : return (0xc5);
    case 'F'  : return (0xc6);
    case 'G'  : return (0xc7);
    case 'H'  : return (0xc8);
    case 'I'  : return (0xc9);
    case 'J'  : return (0xd1);
    case 'K'  : return (0xd2);
    case 'L'  : return (0xd3);
    case 'M'  : return (0xd4);
    case 'N'  : return (0xd5);
    case 'O'  : return (0xd6);
    case 'P'  : return (0xd7);
    case 'Q'  : return (0xd8);
    case 'R'  : return (0xd9);
    case 'S'  : return (0xe2);
    case 'T'  : return (0xe3);
    case 'U'  : return (0xe4);
    case 'V'  : return (0xe5);
    case 'W'  : return (0xe6);
    case 'X'  : return (0xe7);
    case 'Y'  : return (0xe8);
    case 'Z'  : return (0xe9);
    case '['  : return (0xad);
    case '\\' : return (0xe0);
    case ']'  : return (0xbd);
    case '^'  : return (0x5f);
    case '_'  : return (0x6d);
    case '`'  : return (0x79);
    case 'a'  : return (0x81);
    case 'b'  : return (0x82);
    case 'c'  : return (0x83);
    case 'd'  : return (0x84);
    case 'e'  : return (0x85);
    case 'f'  : return (0x86);
    case 'g'  : return (0x87);
    case 'h'  : return (0x88);
    case 'i'  : return (0x89);
    case 'j'  : return (0x91);
    case 'k'  : return (0x92);
    case 'l'  : return (0x93);
    case 'm'  : return (0x94);
    case 'n'  : return (0x95);
    case 'o'  : return (0x96);
    case 'p'  : return (0x97);
    case 'q'  : return (0x98);
    case 'r'  : return (0x99);
    case 's'  : return (0xa2);
    case 't'  : return (0xa3);
    case 'u'  : return (0xa4);
    case 'v'  : return (0xa5);
    case 'w'  : return (0xa6);
    case 'x'  : return (0xa7);
    case 'y'  : return (0xa8);
    case 'z'  : return (0xa9);
    case '{'  : return (0xc0);
    case '|'  : return (0x4f);
    case '}'  : return (0xd0);
    case '~'  : return (0xa1);
    case 0x1b : return (0x27); /* ESC character */
    case 1    : return (1); /* ctrl-A */
    case 2    : return (2); /* ctrl-B */
    case 3    : return (3); /* ctrl-C */
    case 4    : return (0x37); /* ctrl-D */
    case 5    : return (0x2d); /* ctrl-E */
    case 6    : return (0x2e); /* ctrl-F */
    case 7    : return (0x2f); /* ctrl-G */
    case 8    : return (0x16); /* ctrl-H */
    case 9    : return (0x05); /* ctrl-I */
    /* case 0x0A : return (0x15); */ /* ctrl-J */
    case 0x0B : return (0x0B); /* ctrl-K */
    case 0x0C : return (0x0C); /* ctrl-L */
    case 0x0D : return (0x0D); /* ctrl-M */
    case 0x0E : return (0x0E); /* ctrl-N */
    case 0x0F : return (0x0F); /* ctrl-O */
    case 0x10 : return (0x10); /* ctrl-P */
    case 0x11 : return (0x11); /* ctrl-Q */
    case 0x12 : return (0x12); /* ctrl-R */
    case 0x13 : return (0x13); /* ctrl-S */
    case 0x14 : return (0x3C); /* ctrl-T */
    case 0x15 : return (0x3D); /* ctrl-U */
    case 0x16 : return (0x32); /* ctrl-V */
    case 0x17 : return (0x26); /* ctrl-W */
    case 0x18 : return (0x18); /* ctrl-X */
    case 0x19 : return (0x19); /* ctrl-Y */
    case 0x1A : return (0x3F); /* ctrl-Z */
    default   : return (0);
  }
}
