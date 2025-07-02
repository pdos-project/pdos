/*********************************************************************/
/*                                                                   */
/*  This Program Written by Alica Okano.                             */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  dllcrt.c - entry point for WIN32 DLLs                            */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <locale.h>
#include <setjmp.h>
#include <math.h>
#include <assert.h>

#include <windows.h>


__PDPCLIB_API__ int __start(char *p);

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved);


#ifdef __WATCOMC__
int __watcall main(int argc, int argv)
{
    return (0);
}
#endif

BOOL CALLBACK _DllMainCRTStartup(HINSTANCE hinstDll,
                              DWORD fdwReason,
                              LPVOID lpvReserved)
{
    BOOL bRet;

    /* DllMain() is optional, so it would be good to handle that case too. */
    bRet = DllMain(hinstDll, fdwReason, lpvReserved);

    return (bRet);
}

int __needdum = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
#ifdef NEED_START

    __start(0); /* this needs to be called */
    if (__needdum)
    {
        /* define dummy calls to get the whole of PDPCLIB linked in */
        jmp_buf jmpenv;

        time(NULL);
        localeconv();
        setjmp(jmpenv);
        assert(0);
        sin(0.0);
    }
#endif

    return (TRUE);
}

#ifdef NEED_MAIN
int main(void) { return 0; }
#endif
