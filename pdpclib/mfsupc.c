/* mainframe support routines written in C */
/* written by Paul Edwards */
/* released to the public domain */

/* routines have double underscore so that they can be
   used as part of a C library. Do a #define if not
   part of a C library */

#include <string.h>

#include <__os.h>

#include "__mvs.h"

extern OS *__pgparm;

#ifdef __BIGFOOT__
void __exita(int rc);
#endif

int __wto(int len, int flags, char *msg)
{
    /* not sure what alignment is required - fullword should be enough */
    union {
        int dummy;
        char buf[84];
    } ubuf;
    regs regsin;
    regs regsout;

    if (len < 0) return (0);
    if (len > 80) len = 80;
    len += 4;
    *(short *)ubuf.buf = len;
    *(short *)(ubuf.buf + 2) = flags;
    memcpy(ubuf.buf + 4, msg, len - 4);
    regsin.r[1] = (int)ubuf.buf;
    __svc(35, &regsin, &regsout);
    return (0);
}


/* return number 6 for testing purposes */

int __ret6(void)
{
    return (6);
}


#ifdef __MVS__

/* C startup code for MVS */

int __mvsrun(void)
{
    __wto(11, 0, "ABC DEF GHI");
    return (__ret6());
}

#endif


#ifdef __VSE__

/* C startup code for VSE */

int __vserun(void)
{
    /* issue a WTO for now */
    /* not sure what alignment a CCW needs */
    union {
        double dummy;
        struct {
            unsigned int addr;
            unsigned short flags;
            unsigned short len;
        } actual;
    } ccw;
    char msg[80];
    /* not sure what alignment a CCB needs */
    union {
        double dummy;
        struct {
            unsigned short residual_count;
            unsigned short communication_bytes;
            unsigned short csw_status_bytes;
            unsigned char logical_unit_class;
            unsigned char logical_unit;
            void *ccw_address;
            unsigned int status_and_csw_ccw;
        } actual;
    } ccb;
    regs regsin;
    regs regsout;

    memcpy(msg, "MADE IT!", 8);
    ccw.actual.addr = (0x09 << 24) | (unsigned int)msg;
    ccw.actual.flags = 0;
    ccw.actual.len = 8;
    
    ccb.actual.residual_count = 0;
    ccb.actual.communication_bytes = 0;
    ccb.actual.csw_status_bytes = 0;
    ccb.actual.logical_unit_class = 0;
    /* logical units are:
       0 = sysrdr
       1 = sysipt
       2 = syspch
       3 = syslst
       4 = syslog
       5 = syslnk
       6 = sysres
       7 = sysslb
       8 = sysrlb
       9 = sysuse
       10 = sysrec
       11 = sysclb
       12 = sysvis
       13 = syscat
       and also sys000 is 0, sys001 is 1 etc, I think */
    ccb.actual.logical_unit = 4; /* SYSLOG */
    ccb.actual.ccw_address = &ccw;
    ccb.actual.status_and_csw_ccw = 0;

    /* excp ccb */
    regsin.r[1] = (int)&ccb;
    __svc(0, &regsin, &regsout);

    /* wait ccb */
    /* so long as the top bit is 0, the EXCP was accepted and
       thus it is OK to wait on it - I think */
    if ((ccb.actual.csw_status_bytes & 0x8000U) == 0)
    {
        regsin.r[1] = (int)&ccb;
        __svc(7, &regsin, &regsout);
    }

    return (0);
}

#endif


#ifdef __BIGFOOT__

/* C startup code for Linux Bigfoot */

int __bigrun(void)
{
    regs regsin;
    regs regsout;
    int fh;
    int rc;

#ifdef LOGSHELL
    /* standard handles are not available when a login shell */
    /* we need to open the TTY instead */

    /* 2 = O_RDWR */
    fh = __open("/dev/console", 2);

    /* duplicate the returned handle into the standard handles */
    rc = __dup2(fh, 0);
    if (rc == -1) __write(fh, "fail0\n", 6);
    rc = __dup2(fh, 1);
    if (rc == -1) __write(fh, "fail1\n", 6);
    rc = __dup2(fh, 2);
    if (rc == -1) __write(fh, "fail2\n", 6);
#endif

#if 0
    /* 1 = stdout */
    __write(1, "Hi There\n", 9);
#endif

    __start(0);

    __exita(5);

    return (__ret6());
}

#endif


int __svc(int svcnum, regs *regsin, regs *regsout)
{
    int ret;

    if (__pgparm == 0)
    {
        ret = __svcrl(svcnum, regsin, regsout);
#if 0
        if (regsin->r[7] == 0x14)
        {
            for (;;) ;
        }
#endif
        return (ret);
    }
    else
    {
        return (__pgparm->Xservice(svcnum, regsin, regsout));
    }
}

#ifdef __BIGFOOT__

/* syscall numbers can be found here:
https://github.com/linas/i370-linux-2.2.1/blob/master/include/asm-i370/unistd.h
*/

int __write(int fd, void *buf, int len)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 4; /* write syscall */
    regsin.r[5] = fd; /* e.g. 1 */
    regsin.r[6] = (int)buf;
    regsin.r[7] = len;
    return (__svc(0, &regsin, &regsout));
}

int __close(int fd)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 57; /* close syscall */
    regsin.r[5] = fd;
    return (__svc(0, &regsin, &regsout));
}

int __rename(char *old, char *new)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 38; /* rename syscall */
    regsin.r[5] = (int)old;
    regsin.r[6] = (int)new;
    return (__svc(0, &regsin, &regsout));
}

int __dup2(int old, int new)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 63; /* dup2 syscall */
    regsin.r[5] = old;
    regsin.r[6] = new;
    return (__svc(0, &regsin, &regsout));
}

int __seek(int fd, int pos, int how)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 62; /* lseek syscall */
    regsin.r[5] = fd;
    regsin.r[6] = pos;
    regsin.r[7] = how;
    return (__svc(0, &regsin, &regsout));
}

int __remove(char *path)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 10; /* unlinnk syscall */
    regsin.r[5] = (int)path;
    return (__svc(0, &regsin, &regsout));
}

int __open(char *path, int flags)
{
    int fh;
    regs regsin;
    regs regsout;

    regsin.r[1] = 5; /* open syscall */
    regsin.r[5] = (int)path;
    regsin.r[6] = flags; /* O_WRONLY etc */
    regsin.r[7] = 0x1A4; /* 0644 - permissions in case create */
    fh = __svc(0, &regsin, &regsout);
    return (fh);
}

int __ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 54; /* ioctl syscall */
    regsin.r[5] = fd;
    regsin.r[6] = cmd;
    regsin.r[7] = arg;
    return (__svc(0, &regsin, &regsout));
}

int __getpid(void)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 20; /* getpid syscall */
    return (__svc(0, &regsin, &regsout));
}

int __read(int fd, void *buf, int len)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 3; /* read syscall */
    regsin.r[5] = fd;
    regsin.r[6] = (int)buf;
    regsin.r[7] = len;
    return (__svc(0, &regsin, &regsout));
}

void __exita(int rc)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 1; /* exit syscall */
    regsin.r[5] = rc;
    __svc(0, &regsin, &regsout);
    return;
}

void *__mmap(void *a, int b, int prot, int flags, int fd,
             long offset, long offset2)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 90; /* mmap syscall */
    regsin.r[5] = (int)a;
    regsin.r[6] = b;
    regsin.r[7] = prot;
    regsin.r[8] = flags;
    regsin.r[9] = fd;
    regsin.r[10] = offset;
    regsin.r[11] = offset2;
    return ((void *)__svc(0, &regsin, &regsout));
}

int __munmap(void *a, size_t b)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 91; /* munmap syscall */
    regsin.r[5] = (int)a;
    regsin.r[6] = b;
    return (__svc(0, &regsin, &regsout));
}


int __clone(int x, char *y, char *z, char *a, char *b)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 120; /* clone syscall */
    regsin.r[5] = x;
    regsin.r[6] = (int)y;
    regsin.r[7] = (int)z;
    regsin.r[8] = (int)a;
    regsin.r[9] = (int)b;
    return (__svc(0, &regsin, &regsout));
}

/* we don't have waitid available, only waitpid */
/* should probably change this */
int __waitid(int a, long pid, void *c, int d, void *e)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 7; /* waitpid syscall */
    regsin.r[5] = (int)pid;
    regsin.r[6] = NULL;
    regsin.r[7] = (int)d;
    return (__svc(0, &regsin, &regsout));
}

/* this has not been done properly */
int __execve(char *a, char **oldargv, void *c)
{
    regs regsin;
    regs regsout;
    static char *argv[2];

    argv[0] = "prog";
    argv[1] = NULL;

    regsin.r[1] = 11; /* execve syscall */
    regsin.r[5] = (int)oldargv[2];
    regsin.r[6] = (int)argv;
    regsin.r[7] = (int)c;
    return (__svc(0, &regsin, &regsout));
}

int __getdents(unsigned int fd, void *dirent, int count)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 141; /* getdents syscall */
    regsin.r[5] = fd;
    regsin.r[6] = (int)dirent;
    regsin.r[7] = count;
    return (__svc(0, &regsin, &regsout));
}

int __chdir(const char *filename)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 12; /* chdir syscall */
    regsin.r[5] = (int)filename;
    return (__svc(0, &regsin, &regsout));
}

int __mkdir(const char *filename, int mode)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 39; /* mkdir syscall */
    regsin.r[5] = (int)filename;
    regsin.r[6] = mode;
    return (__svc(0, &regsin, &regsout));
}

int __rmdir(const char *filename)
{
    regs regsin;
    regs regsout;

    regsin.r[1] = 40; /* rmdir syscall */
    regsin.r[5] = (int)filename;
    return (__svc(0, &regsin, &regsout));
}

#if 0
int __time(void);
#endif

#endif
