/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  mvssupc.c - C version of support routines for MVS                */
/*                                                                   */
/*********************************************************************/

#include "__mvs.h"

static int __csvc(int svcnum, regs *regsin, regs *regsout);

int __cwto(int len, int flags, char *msg)
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
    __csvc(35, &regsin, &regsout);
    return (0);
}

/* need __copen, __cpoint, __cread, __cwrite, __cclose */




static int __csvc(int svcnum, regs *regsin, regs *regsout)
{
    int ret;

#if 0
    if (__pgparm == 0)
    {
#endif
        ret = __svcrl(svcnum, regsin, regsout);
        return (ret);
#if 0
    }
    else
    {
        return (__pgparm->Xservice(svcnum, regsin, regsout));
    }
#endif
}

