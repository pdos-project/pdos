/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  time.h - time header file.                                       */
/*                                                                   */
/*********************************************************************/

#ifndef __TIME_INCLUDED
#define __TIME_INCLUDED

#if defined(__PDPCLIB_DLL) && !defined(__WATCOMC__) \
    && !defined(__NODECLSPEC__)
#define __PDPCLIB_HEADFUNC __declspec(dllexport)
#endif

#ifndef __PDPCLIB_HEADFUNC
#define __PDPCLIB_HEADFUNC
#endif

#define CLOCKS_PER_SEC 1000
#define NULL ((void *)0)

typedef unsigned int clock_t;

#ifndef __SIZE_T_DEFINED
#define __SIZE_T_DEFINED
#if defined(__64BIT__)

#if defined(__LONG64__)
typedef unsigned long size_t;
#else
typedef unsigned long long size_t;
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

typedef unsigned long time_t;

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

__PDPCLIB_HEADFUNC time_t time(time_t *timer);
__PDPCLIB_HEADFUNC clock_t clock(void);
__PDPCLIB_HEADFUNC double difftime(time_t time1, time_t time0);
__PDPCLIB_HEADFUNC time_t mktime(struct tm *timeptr);
__PDPCLIB_HEADFUNC char *asctime(const struct tm *timeptr);
__PDPCLIB_HEADFUNC char *ctime(const time_t *timer);
__PDPCLIB_HEADFUNC struct tm *gmtime(const time_t *timer);
__PDPCLIB_HEADFUNC struct tm *localtime(const time_t *timer);
__PDPCLIB_HEADFUNC size_t strftime(char *s, size_t maxsize,
                const char *format, const struct tm *timeptr);


#if defined(__PDOSGEN__)
#include <__os.h>

#define ctime __os->Xctime
#define localtime __os->Xlocaltime
#define time __os->Xtime
#define mktime __os->Xmktime
#define clock __os->Xclock
#define strftime __os->Xstrftime

#endif


#endif
