/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  stdarg.h - stdarg header file.                                   */
/*                                                                   */
/*********************************************************************/

#ifndef __STDARG_INCLUDED
#define __STDARG_INCLUDED

/* don't use builtins on MVS until they have been implemented */
/* don't use on EMX either */
/* and they haven't been implemented in PDOS/386 either */
#if 0
if defined(__GNUC__) && !defined(__MVS__) && !defined(__CMS__) \
    && !defined(__MF32__) \
    && !defined(__VSE__) && !defined(__EMX__) && !defined(__PDOS386__) \
    && !defined(__gnu_linux__) && !defined(__NOBIVA__) \
    && !defined(__ARM__)
#endif

/* Try only using builtins for GCC 4.x and above, which
   appears to cover clang too */
/* We probably need the builtin for 64-bit where parameters are
   not all passed on the stack too */
/* It looks like above version 3 you need to use va_start,
   but version 3 uses stdarg_start */
#if defined (__GNUC__) && ((__GNUC__ > 3) || defined(__USEBIVA__))

#ifndef __GNUC_VA_LIST
#define __GNUC_VA_LIST
typedef __builtin_va_list __gnuc_va_list;
#endif

typedef __gnuc_va_list va_list;

#if (__GNUC__ > 3)
#define va_start(v,l)   __builtin_va_start(v,l)
#else
#define va_start(v,l)   __builtin_stdarg_start(v,l)
#endif

#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L
#define va_copy(d,s)    __builtin_va_copy(d,s)
#endif
#define __va_copy(d,s)  __builtin_va_copy(d,s)

#else /* __GNUC__ */

typedef char * va_list;

#ifdef __XWATCOMC__
#define va_start(ap, parmN) ap = (char far *)&parmN + sizeof(parmN)
#define va_arg(ap, type) *(type far *)(ap += sizeof(type), ap - sizeof(type))
#else
#if (defined(__MSC__) && defined(__ARM__) && defined(__64BIT__))
/* Visual C has a builtin that is required */
void __va_start(va_list *x, ...);
#define va_start(ap, parmN) \
    __va_start(&ap, &parmN, 8 /* size */, 8 /* alignment */, &parmN);
#else
#define va_start(ap, parmN) ap = (char *)&parmN + sizeof(parmN)
#endif

#ifdef __64BIT__
#define va_arg(ap, type) *(type *)(ap += (sizeof(type) < 8) ? 8 : sizeof(type),\
    ap - ((sizeof(type) < 8) ? 8 : sizeof(type)))
#else
#define va_arg(ap, type) *(type *)(ap += sizeof(type), ap - sizeof(type))
#endif
#endif
#define va_end(ap) ap = (char *)0

#endif

#endif
