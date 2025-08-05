/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  stdio.h - stdio header file                                      */
/*                                                                   */
/*********************************************************************/

#ifndef __STDIO_INCLUDED
#define __STDIO_INCLUDED

/* Perhaps should copy these definitions in instead */
#include <stdarg.h>

#if defined(__PDPCLIB_DLL) && !defined(__WATCOMC__) \
    && !defined(__NODECLSPEC__)
#define __PDPCLIB_HEADFUNC __declspec(dllexport)
#endif

#ifndef __PDPCLIB_HEADFUNC
#define __PDPCLIB_HEADFUNC
#endif

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
#if defined(__64BIT__)
#ifdef __SUBC__
typedef unsigned int size_t;
#else

#if defined(__LONG64__)
typedef unsigned long size_t;
#else
typedef unsigned long long size_t;
#endif

#endif
#elif (defined(__MVS__) \
    || defined(__MF32__) \
    || defined(__CMS__) || defined(__VSE__) || defined(__SMALLERC__) \
    || defined(__ARM__) \
    || defined(__SZ4__))
typedef unsigned long size_t;
#elif (defined(__MSDOS__) || defined(__DOS__) || defined(__POWERC) \
    || defined(__WIN32__) || defined(__AMIGA__) || defined(__EFI__) \
    || defined(__gnu_linux__) || defined(__ATARI__) \
    || defined(__OS2__) || defined(__32BIT__) || defined(__PDOS386__))
typedef unsigned int size_t;
#endif
#endif

/*
    What we have is an internal buffer, which is 8 characters
    longer than the actually used buffer.  E.g. say BUFSIZ is
    512 bytes, then we actually allocate 520 bytes.  The first
    2 characters will be junk, the next 2 characters set to NUL,
    for protection against some backward-compares.  The fourth-last
    character is set to '\n', to protect against overscan.  The
    last 3 characters will be junk, to protect against memory
    violation.  intBuffer is the internal buffer, but everyone refers
    to fbuf, which is actually set to the &intBuffer[4].  Also,
    szfbuf is the size of the "visible" buffer, not the internal
    buffer.  The reason for the 2 junk characters at the beginning
    is to align the buffer on a 4-byte boundary.

    Here is what memory would look like after an fwrite of "ABC"
    to an MVS LRECL=80, RECFM=F dataset:

    intbuffer: x'50000'
    fbuf:      x'50004'
    upto:      x'50007'
    endbuf:    x'58004'

    x'50004' = x'C1'
    x'50005' = x'C2'
    x'50006' = x'C3'
*/

typedef struct
{
#if ((defined(__OS2__) && !defined(__16BIT__)) \
    || defined(__32BIT__) || defined(__PDOS386__))
    unsigned long hfile;  /* OS/2 file handle */
#elif (defined(__MSDOS__) || defined(__DOS__) || defined(__POWERC) \
    || defined(__ZPDOSGPB__) \
    || defined(__SMALLERC__) || defined(__OS2__))
    int hfile; /* dos file handle */
#elif defined(__WIN32__) || defined(__AMIGA__) || defined(__EFI__)
    void *hfile;
#elif defined(__gnu_linux__) || defined(__ARM__) || defined(__ATARI__)
    int hfile;
#endif
#if defined(__EFIBIOS__)
    int block;
    unsigned long sector;
#endif
#if defined(__ZPDOSGPB__)
    unsigned int devnum;
    unsigned int devtype;
    unsigned long sector;
#endif

#if defined(__OS2__)
#ifdef __16BIT__
    unsigned int kbdfile;
#else
    unsigned long kbdfile;
#endif
#endif

#if (defined(__MVS__) || defined(__CMS__) || defined(__VSE__))
    void *hfile;
    void *asmbuf;
    int recfm;
    int true_recfm;
    int style;
    int lrecl;
    int blksize;
    char ddname[9];
    char pdsmem[9];
    int reallyu;    /* 1 = this is really a RECFM=U file */
    int reallyt;    /* 1 = this is really a text file */
    int dynal;      /* 1 = this file was dynamically allocated */
    int line_buf;   /* 1 = we are forcing line buffering */
    int blocks_per_track; /* used for fast seeking */
    int done_first;
#endif
#if defined(__VSE__)
    int vse_punch;  /* 1 = this is writing to a VSE library */
    char *vselupto; /* where we are up to in the internal buffer */
    char *vselend; /* end of the internal buffer */
#endif
    int quickBin;  /* 1 = do DosRead NOW!!!! */
    int quickText; /* 1 = quick text mode */
    int textMode; /* 1 = text mode, 0 = binary mode */
    int intFno;   /* internal file number */
    unsigned long bufStartR; /* buffer start represents, e.g. if we
        have read in 3 buffers, each of 512 bytes, and we are
        currently reading from the 3rd buffer, then the first
        character in the buffer would be 1024, so that is what is
        put in bufStartR. */
    int justseeked; /* 1 = last operation was a seek */
    char *fbuf;     /* file buffer - this is what all the routines
                       look at. */
    size_t szfbuf;  /* size of file buffer (the one that the routines
                       see, and the user allocates, and what is actually
                       read in from disk) */
    char *upto;     /* what character is next to read from buffer */
    char *endbuf;   /* pointer PAST last character in buffer, ie it
                       points to the '\n' in the internal buffer */
    int errorInd;   /* whether an error has occurred on this file */
    int eofInd;     /* whether EOF has been reached on this file */
    int ungetCh;    /* character pushed back, -1 if none */
    int bufTech;    /* buffering technique, _IOFBF etc */
    char *intBuffer; /* internal buffer */
    int noNl;       /* When doing gets, we don't copy NL */
    int mode;       /* __READ_MODE or __WRITE_MODE */
    int update;     /* Is file update (read + write)? */
    int theirBuffer; /* Is the buffer supplied by them? */
    int permfile;   /* Is this stdin/stdout/stderr? */
    int isopen;     /* Is this file open? */
    char modeStr[4]; /* 2nd parameter to fopen */
    int tempch; /* work variable for putc */
    int istemp; /* opened by tmpfile? */
    int runnum; /* which level in start.c opened this file */
} FILE;

typedef unsigned long fpos_t;

#define NULL ((void *)0)
#define FILENAME_MAX 260

#ifdef __CC64OS__
/* hack for file leak and no cleanup */
#define FOPEN_MAX 25600
#else
#define FOPEN_MAX 256
#endif

#define _IOFBF 0
/* _IOLBF behaves the same as _IOFBF in Windows msvcrt.dll
 * but in PDPCLIB it behaves differently,
 * so it is intentionally set to incompatible value.
 */
/*#define _IOLBF 64*/
#define _IOLBF 2
#define _IONBF 4


#ifndef BUFSIZ

/*#define BUFSIZ 409600*/
/* #define BUFSIZ 8192 */
/*#define BUFSIZ 5120*/

#if defined(__MVS__) || defined(__VSE__)
/* set it to maximum possible LRECL to simplify processing */
/* also add in room for a RDW and dword align it just to be
   on the safe side */
#define BUFSIZ 32768
#elif defined(__ZPDOSGPB__)
#define BUFSIZ 512
#elif defined(__CMS__)
/* similar considerations for CMS as MVS */
#define BUFSIZ 65544
#else
#define BUFSIZ 6144
#endif
/* #define BUFSIZ 10 */
/* #define BUFSIZ 512 */

#endif


#define EOF -1
#define L_tmpnam FILENAME_MAX
#define TMP_MAX 25
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define __NFILE (FOPEN_MAX - 3)
#define __READ_MODE 0
#define __WRITE_MODE 1

#define __RECFM_F 0
#define __RECFM_V 1
#define __RECFM_U 2

#if 0
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

extern FILE *__userFiles[__NFILE];
#endif

__PDPCLIB_HEADFUNC FILE **__gtin(void);
__PDPCLIB_HEADFUNC FILE **__gtout(void);
__PDPCLIB_HEADFUNC FILE **__gterr(void);

#define __stdin (*(__gtin()))
#define __stdout (*(__gtout()))
#define __stderr (*(__gterr()))


/* the W64SHELL should replace CC64OS */

#if (defined(__WIN32__) && !defined(__STATIC__)) || defined(__CC64OS__) \
    || defined(__W64SHELL__) || defined(__W32EMUL__) || defined(__W32DLL__) \
    || defined(__GENSHELL__)
/* For Windows stdin, stdout and stderr macros
 * are implemented using an array FILE _iob[]
 * where the first three members
 * are stdin, stdout and stderr.
 * In this array each member has size 32 bytes (32-bit) or 48 bytes (64-bit),
 * so __DUMMYFILE is used instead
 * and the pointers are converted
 * using __INTFILE macro. */
#if defined(__64BIT__)
#define __DUMMYFILE_SIZE 48
#elif defined(__ARM__)
#define __DUMMYFILE_SIZE 32
#else
#define __DUMMYFILE_SIZE 32
#endif

typedef struct
{
    char filler[__DUMMYFILE_SIZE];
} __DUMMYFILE;

#if !defined(__PDPCLIB_DLL) && !defined(__CC64OS__)
#if !defined(__SUBC__) && !defined(__NODECLSPEC__) && !defined(__CC64__) \
    && !defined(__M68K__) \
    && !defined(__W64SHELL__) && !defined(__W32EMUL__) \
    && !defined(__GENSHELL__) && !defined(__ARM__) \
    && !(defined(__WIN32__) && defined(__64BIT__))

__declspec(dllimport) __DUMMYFILE _iob[3];

#else

#if defined(__CC64__) || defined(__W64SHELL__) || defined(__ARM__) \
    || defined(__M68K__) \
    || (defined(__WIN32__) && defined(__64BIT__))
extern __DUMMYFILE *__iob_func(void);
#elif defined(__64BIT__)
extern __DUMMYFILE *_imp__iob;
#else
extern __DUMMYFILE *_imp___iob;
#endif

#endif
#endif

#if !defined(__SUBC__) && !defined(__NODECLSPEC__) && \
    !(defined(__CC64__) && !defined(__PDPCLIB_DLL)) && \
    !defined(__CC64OS__) && !defined(__W64SHELL__) && \
    !defined(__W32EMUL__) && !defined(__GENSHELL__) && \
    !defined(__M68K__) && \
    !defined(__ARM__) && \
    !(defined(__WIN32__) && defined(__64BIT__))
#define stdin ((FILE *) &(_iob[0]))
#define stdout ((FILE *) &(_iob[1]))
#define stderr ((FILE *) &(_iob[2]))
#else

#if defined(__CC64__) || defined(__W64SHELL__) || defined(__ARM__) \
    || defined(__M68K__) \
    || (defined(__WIN32__) && defined(__64BIT__))
#if defined(__CC64OS__) || defined(__W64SHELL__) || defined(__ARM__)
extern __PDPCLIB_HEADFUNC __DUMMYFILE _iob[3];
extern __PDPCLIB_HEADFUNC __DUMMYFILE *__iob_func(void);
#endif
#define stdin ((FILE *) __iob_func())
#define stdout ((FILE *) (__iob_func() + 1))
#define stderr ((FILE *) (__iob_func() + 2))
#elif defined(__64BIT__)
#define stdin ((FILE *) &(_imp__iob[0]))
#define stdout ((FILE *) &(_imp__iob[1]))
#define stderr ((FILE *) &(_imp__iob[2]))
#else
#define stdin ((FILE *) &(_imp___iob[0]))
#define stdout ((FILE *) &(_imp___iob[1]))
#define stderr ((FILE *) &(_imp___iob[2]))
#endif

#endif

#define __INTFILE(f) (((f) == stdin) ? __stdin : \
                      ((f) == stdout) ? __stdout : \
                      ((f) == stderr) ? __stderr : \
                      f)
#else
#define stdin (*(__gtin()))
#define stdout (*(__gtout()))
#define stderr (*(__gterr()))

#define __INTFILE(f) (f)
#endif

#if defined(__VSE__)
extern FILE *__stdpch;
#endif

__PDPCLIB_HEADFUNC int printf(const char *format, ...);
__PDPCLIB_HEADFUNC FILE *fopen(const char *filename, const char *mode);
__PDPCLIB_HEADFUNC int fclose(FILE *stream);
__PDPCLIB_HEADFUNC size_t fread(void *ptr, size_t size, size_t nmemb,
    FILE *stream);
__PDPCLIB_HEADFUNC size_t fwrite(const void *ptr, size_t size, size_t nmemb,
    FILE *stream);
__PDPCLIB_HEADFUNC int fputc(int c, FILE *stream);
__PDPCLIB_HEADFUNC int fputs(const char *s, FILE *stream);
__PDPCLIB_HEADFUNC int fprintf(FILE *stream, const char *format, ...);
__PDPCLIB_HEADFUNC int vfprintf(FILE *stream, const char *format, va_list arg);
__PDPCLIB_HEADFUNC int vprintf(const char *format, va_list arg);
__PDPCLIB_HEADFUNC int remove(const char *filename);
__PDPCLIB_HEADFUNC int rename(const char *old, const char *newnam);
__PDPCLIB_HEADFUNC int sprintf(char *s, const char *format, ...);
__PDPCLIB_HEADFUNC int vsprintf(char *s, const char *format, va_list arg);
__PDPCLIB_HEADFUNC char *fgets(char *s, int n, FILE *stream);
__PDPCLIB_HEADFUNC int ungetc(int c, FILE *stream);
__PDPCLIB_HEADFUNC int fgetc(FILE *stream);
__PDPCLIB_HEADFUNC int fseek(FILE *stream, long offset, int whence);
__PDPCLIB_HEADFUNC long ftell(FILE *stream);
__PDPCLIB_HEADFUNC int fsetpos(FILE *stream, const fpos_t *pos);
__PDPCLIB_HEADFUNC int fgetpos(FILE *stream, fpos_t *pos);
__PDPCLIB_HEADFUNC void rewind(FILE *stream);
__PDPCLIB_HEADFUNC void clearerr(FILE *stream);
__PDPCLIB_HEADFUNC void perror(const char *s);
__PDPCLIB_HEADFUNC int setvbuf(FILE *stream, char *buf, int mode, size_t size);
__PDPCLIB_HEADFUNC int setbuf(FILE *stream, char *buf);
__PDPCLIB_HEADFUNC FILE *freopen(const char *filename, const char *mode,
    FILE *stream);
__PDPCLIB_HEADFUNC int fflush(FILE *stream);
__PDPCLIB_HEADFUNC char *tmpnam(char *s);
__PDPCLIB_HEADFUNC FILE *tmpfile(void);
__PDPCLIB_HEADFUNC int fscanf(FILE *stream, const char *format, ...);
__PDPCLIB_HEADFUNC int scanf(const char *format, ...);
__PDPCLIB_HEADFUNC int sscanf(const char *s, const char *format, ...);
__PDPCLIB_HEADFUNC char *gets(char *s);
__PDPCLIB_HEADFUNC int puts(const char *s);

#ifndef __POWERC
__PDPCLIB_HEADFUNC int getchar(void);
__PDPCLIB_HEADFUNC int putchar(int c);
__PDPCLIB_HEADFUNC int getc(FILE *stream);
__PDPCLIB_HEADFUNC int putc(int c, FILE *stream);
__PDPCLIB_HEADFUNC int feof(FILE *stream);
__PDPCLIB_HEADFUNC int ferror(FILE *stream);
#endif

#if !defined(__W64SHELL__) && !defined(__W32EMUL__) \
    && !(defined(__WIN32__) && !defined(__STATIC__)) \
    && !defined(__PDOSGEN__) \
    && !defined(__GENSHELL__)

#define getchar() (getc(stdin))
#define putchar(c) (putc((c), stdout))
#define getc(stream) (fgetc((stream)))
#define putc(c, stream) \
  ( \
    ((stream)->tempch = (c)), \
    ((stream)->quickBin = 0), \
    ((stream)->quickText = 0), \
    (((stream)->tempch == '\n') \
        || (stream)->justseeked \
        || (((stream)->upto + 1) >= (stream)->endbuf)) ? \
        (fputc((stream)->tempch, (stream))) : \
        (*(stream)->upto++ = (stream)->tempch) \
  )

#define feof(stream) ((stream)->eofInd)
#define ferror(stream) ((stream)->errorInd)
#endif

#if defined(__PDOSGEN__)
#include <__os.h>

#define printf __os->Xprintf
#undef stdin
#define stdin __os->Xstdin
#undef stdout
#define stdout __os->Xstdout
#undef stderr
#define stderr __os->Xstderr
#define fopen __os->Xfopen
#define fseek __os->Xfseek
#define fread __os->Xfread
#define fclose __os->Xfclose
#define fwrite __os->Xfwrite
#define fgets __os->Xfgets
#define fgetc __os->Xfgetc
#define fputc __os->Xfputc
#define fflush __os->Xfflush
#define remove __os->Xremove
#define fputs __os->Xfputs
#define fprintf __os->Xfprintf
#define tmpnam __os->Xtmpnam
#define vfprintf __os->Xvfprintf
#define ungetc __os->Xungetc
#define vsprintf __os->Xvsprintf
#define sprintf __os->Xsprintf
#define ftell __os->Xftell
#define perror __os->Xperror
#define rewind __os->Xrewind
#define sscanf __os->Xsscanf
#define rename __os->Xrename
#define clearerr __os->Xclearerr
#define setvbuf __os->Xsetvbuf
#define puts __os->Xputs
#define vprintf __os->Xvprintf
#define ferror __os->Xferror
#define putc __os->Xputc
#define getc __os->Xgetc
#define feof __os->Xfeof
#define putchar __os->Xputchar
#define getchar __os->Xgetchar
#define tmpfile __os->Xtmpfile
#endif

#endif /* __STDIO_INCLUDED */
