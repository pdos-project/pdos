/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  kernel32 - implementation of Windows based on PDOS/386           */
/*                                                                   */
/*********************************************************************/

#include <windows.h>

#include <string.h>

#include <pos.h>

#ifndef NOLIBALLOC
#include <liballoc.h>
#endif

#include "dllsyscall.h"

static DWORD lasterr = 0;

/* We don't actually use kernel32.dll so we just have dummy
   routines to get a clean link for ARM */
#if defined(__ARM__) || defined(__M68K__)
void int86(void) {}
void int86x(void) {}
#endif

HANDLE WINAPI GetStdHandle(DWORD nStdHandle)
{
    return PosGetStdHandle(nStdHandle);
}

BOOL WINAPI WriteFile(HANDLE hFile,
                      LPCVOID lpBuffer,
                      DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten,
                      LPOVERLAPPED lpOverlapped)
{
    unsigned int written;
    int ret;

    ret = PosWriteFile((int)hFile, lpBuffer,
                       (size_t)nNumberOfBytesToWrite, &written);
    *lpNumberOfBytesWritten = written;
    /* Positive return code means success. */
    return (!ret);
}


void WINAPI ExitProcess(UINT uExitCode)
{
    PosTerminate(uExitCode);
}

BOOL WINAPI CloseHandle(HANDLE hObject)
{
    int ret;

    ret = PosCloseFile((int)hObject);
    return (ret == 0);
}

HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile)
{
    int handle;

    if (dwCreationDisposition == CREATE_ALWAYS)
    {
        if (PosCreatFile(lpFileName, 0, &handle))
        {
            return (INVALID_HANDLE_VALUE);
        }
        return ((HANDLE)handle);
    }
    if (dwCreationDisposition == OPEN_EXISTING)
    {
        if (PosOpenFile(lpFileName, 0, &handle))
        {
            return (INVALID_HANDLE_VALUE);
        }
        return ((HANDLE)handle);
    }

    return (INVALID_HANDLE_VALUE);
}

BOOL WINAPI CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    return (PosMakeDir(lpPathName));
}

BOOL WINAPI RemoveDirectoryA(
    LPCSTR lpPathName)
{
    return (PosRemoveDir(lpPathName));
}

DWORD WINAPI GetFileAttributesA(
    LPCSTR lpFileName)
{
    int attr;
    PosGetFileAttributes(lpFileName, &attr);
    return attr;
}

BOOL WINAPI CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    POSEXEC_PARMBLOCK parmblock = { 0, NULL, NULL, NULL };

    if (lpCommandLine == NULL)
    {
        return (1);
    }
    parmblock.cmdtail = (unsigned char *)lpCommandLine;
    PosExec((char *)lpApplicationName, &parmblock);
    return (1);
}

BOOL WINAPI DeleteFileA(LPCSTR lpFileName)
{
    int ret;

    ret = PosDeleteFile((char *)lpFileName);
    /* 0 from PosDeleteFile means success */
    /* 0 from DeleteFileA means failure */
    return (ret == 0);
}

LPTSTR WINAPI GetCommandLineA(void)
{
    return ((LPTSTR)PosGetCommandLine());
}

LPTCH WINAPI GetEnvironmentStringsA(void)
{
    char *env = PosGetEnvBlock();

    return ((LPTCH)env);
}

BOOL WINAPI FreeEnvironmentStringsA(LPCH x)
{
    return 1;
}

BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode)
{
    *lpExitCode = (DWORD)PosGetReturnCode();
    return (1);
}

DWORD WINAPI GetLastError(void)
{
    return (lasterr);
}

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes)
{
#ifndef NOLIBALLOC
    return (__malloc(dwBytes));
#else
    return (PosAllocMem(dwBytes, POS_LOC32));
#endif
}

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem)
{
#ifndef NOLIBALLOC
    __free(hMem);
#else
    PosFreeMem(hMem);
#endif
    return (NULL);
}

BOOL WINAPI MoveFileA(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName)
{
    return (PosRenameFile(lpExistingFileName, lpNewFileName) == 0);
}

BOOL WINAPI ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped)
{
    return (!PosReadFile((int)hFile, lpBuffer,
                         nNumberOfBytesToRead, (unsigned int *)lpNumberOfBytesRead));
}

DWORD WINAPI SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod)
{
    int ret;
    long newpos;

    ret = PosMoveFilePointer((int)hFile,
                             (long)lDistanceToMove,
                             dwMoveMethod,
                             &newpos);
    return (newpos);
}

DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    return (0);
}

void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
    unsigned int year, month, day, dow, olddow;
    unsigned int hour, min, sec, hundredths;

    PosGetSystemDate(&year, &month, &day, &dow);
    PosGetSystemTime(&hour, &min, &sec, &hundredths);
    olddow = dow;
    PosGetSystemDate(&year, &month, &day, &dow);
    if (olddow != dow)
    {
        PosGetSystemTime(&hour, &min, &sec, &hundredths);
    }
    lpSystemTime->wYear = year;
    lpSystemTime->wMonth = month;
    lpSystemTime->wDay = day;
    lpSystemTime->wDayOfWeek = dow;
    lpSystemTime->wHour = hour;
    lpSystemTime->wMinute = min;
    lpSystemTime->wSecond = sec;
    lpSystemTime->wMilliseconds = hundredths * 10;
    return;
}

DWORD WINAPI GetVersion(void)
{
    return (0);
}

BOOL WINAPI SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine, BOOL Add)
{
    return (0);
}

void WINAPI SetLastError(DWORD dwErrCode)
{
    lasterr = dwErrCode;
    return;
}

BOOL WINAPI GetConsoleScreenBufferInfo(
    HANDLE hFile,
    CONSOLE_SCREEN_BUFFER_INFO *pcsbi)
{
    pcsbi->dwSize.X = 80;
    pcsbi->dwSize.Y = 25;
    pcsbi->dwMaximumWindowSize = pcsbi->dwSize;
    return (TRUE);
}

BOOL WINAPI WriteConsoleOutputA(
    HANDLE hFile,
    const CHAR_INFO *cinfo,
    COORD bufferSize,
    COORD bufferCoord,
    SMALL_RECT *rect)
{
    char *vidptr = (char *)0xb8000;
    int tot;
    int x;

    tot = bufferSize.X * bufferSize.Y;
    for (x = 0; x < tot; x++)
    {
        *(vidptr + (10 * 80 + x) * 2) = cinfo[x].Char.AsciiChar;
        *(vidptr + (10 * 80 + x) * 2 + 1) = (char)cinfo[x].Attributes;
    }
    return (TRUE);
}

BOOL WINAPI GetConsoleMode(HANDLE hFile, DWORD *dw)
{
    unsigned int devinfo;

    *dw = 0;
    PosGetDeviceInformation((int)hFile, &devinfo);
    if ((devinfo & (1 << 5)) == 0)
    {
        *dw = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
    }
    return (1);
}

BOOL WINAPI SetConsoleMode(HANDLE hFile, DWORD dw)
{
    unsigned int devinfo = 0;

    if ((dw & ENABLE_PROCESSED_INPUT) == 0)
    {
        devinfo = 1 << 5;
    }
    PosSetDeviceInformation((int)hFile, devinfo);
    return (1);
}

BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD lpd)
{
    unsigned int len = 44;
    PosWriteFile(1, "GetNumberOfConsoleMouseButtons unimplemented\r\n",
                 len, &len);
    for (;;) ;
    return (0);
}

HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, WIN32_FIND_DATA *FindFileData)
{
    DTA *dta;
    int ret;

    dta = PosGetDTA();
    ret = PosFindFirst((char *)lpFileName, 0x10);
    if (ret != 0)
    {
        if (ret == 2)
        {
            lasterr = ERROR_FILE_NOT_FOUND;
        }
        else
        {
            lasterr = -1;
        }
        return (INVALID_HANDLE_VALUE);
    }
    if (dta->lfn[0] != '\0')
    {
        strcpy(FindFileData->cFileName, (char *)dta->lfn);
    }
    else
    {
        strcpy(FindFileData->cFileName, dta->file_name);
    }
    FindFileData->nFileSizeLow = dta->file_size;
    return ((HANDLE)dta);
}

BOOL WINAPI FindNextFileA(HANDLE h, WIN32_FIND_DATA *FindFileData)
{
    DTA *dta = (DTA *)h;
    int ret;

    ret = PosFindNext();
    if (ret != 0)
    {
        lasterr = ERROR_NO_MORE_FILES;
        return (0);
    }
    if (dta->lfn[0] != '\0')
    {
        strcpy(FindFileData->cFileName, (char *)dta->lfn);
    }
    else
    {
        strcpy(FindFileData->cFileName, dta->file_name);
    }
    FindFileData->nFileSizeLow = dta->file_size;
    return (1);
}

BOOL WINAPI FindClose(HANDLE h)
{
    return (1);
}

/* assume they have provided at least 66 bytes */

DWORD WINAPI GetCurrentDirectoryA(DWORD d, LPTSTR lpbuffer)
{
    int drive;

    drive = PosGetDefaultDrive();
    lpbuffer[0] = 'A' + drive;
    lpbuffer[1] = ':';
    lpbuffer[2] = '\\';
    PosGetCurDir(0, &lpbuffer[3]);
    return (strlen(lpbuffer));
}

/* assume they have provided at least 66 bytes */

DWORD WINAPI GetTempPathA(DWORD d, LPTSTR lpbuffer)
{
    int drive;

    drive = PosGetDefaultDrive();
    lpbuffer[0] = 'A' + drive;
    lpbuffer[1] = ':';
    lpbuffer[2] = '\\';
    PosGetCurDir(0, &lpbuffer[3]);
    return (strlen(lpbuffer));
}

BOOL WINAPI SetCurrentDirectoryA(LPCTSTR dir)
{
    int ret;

    ret = PosChangeDir(dir);
    return (ret == 0);
}

BOOL WINAPI GetFileTime (HANDLE hFile,
                         LPFILETIME lpCreationTime,
                         LPFILETIME lpLastAccessTime,
                         LPFILETIME lpLastWriteTime)
{
    return 0; /* failure */
}                        

BOOL WINAPI SetFileTime (HANDLE hFile,
                         const FILETIME *lpCreationTime,
                         const FILETIME *lpLastAccessTime,
                         const FILETIME *lpLastWriteTime)
{
    return 0; /* failure */
}                        

void WINAPI GetSystemTimeAsFileTime(FILETIME *a)
{
    return;
}

#ifndef __PDOS386__
HMODULE WINAPI GetModuleHandleA (LPCSTR lpModuleName)
{
    return (HMODULE)system_call_1 (0, lpModuleName);
}
#endif

/* auto-genned dummy functions made compatible with current windows.h */
BOOL WINAPI ConnectNamedPipe (HANDLE hNamedPipe,
                              LPOVERLAPPED lpOverlapped)
{
    unsigned int len = 32;
    PosWriteFile(1, "ConnectNamedPipe unimplemented\r\n", len, &len);
    for (;;) ;
}

HANDLE WINAPI CreateNamedPipeA (LPCSTR lpName,
                                DWORD dwOpenMode,
                                DWORD dwPipeMode,
                                DWORD nMaxInstances,
                                DWORD nOutBufferSize,
                                DWORD nInBufferSize,
                                DWORD nDefaultTimeOut,
                                LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    unsigned int len = 32;
    PosWriteFile(1, "CreateNamedPipeA unimplemented\r\n", len, &len);
    for (;;) ;
}

BOOL WINAPI ReadFileEx (HANDLE hFile,
                        LPVOID lpBuffer,
                        DWORD nNumberOfBytesToRead,
                        LPOVERLAPPED lpOverlapped,
                        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    unsigned int len = 26;
    PosWriteFile(1, "ReadFileEx unimplemented\r\n", len, &len);
    for (;;) ;
}

BOOL WINAPI TerminateProcess (HANDLE hProcess,
                              UINT uExitCode)
{
    unsigned int len = 32;
    PosWriteFile(1, "TerminateProcess unimplemented\r\n", len, &len);
    for (;;) ;
}

BOOL WINAPI WriteFileEx (HANDLE hFile,
                         LPCVOID lpBuffer,
                         DWORD nNumberOfBytesToWrite,
                         LPOVERLAPPED lpOverlapped,
                         LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
    unsigned int len = 27;
    PosWriteFile(1, "WriteFileEx unimplemented\r\n", len, &len);
    for (;;) ;
}



BOOL WINAPI BuildCommDCBA(char *str, void *x)
{
    return (TRUE);
}

BOOL WINAPI GetCommState(HANDLE a, void *b)
{
    return (TRUE);
}

BOOL WINAPI SetCommState(HANDLE a, void *b)
{
    return (TRUE);
}

BOOL WINAPI ClearCommError(HANDLE a, void *b, void *c)
{
    return (TRUE);
}

void WINAPI Sleep(DWORD dwMilliseconds)
{
    return;
}

LPVOID WINAPI VirtualAlloc (LPVOID lpAddress,
                            SIZE_T dwSize,
                            DWORD flAllocationType,
                            DWORD flProtect)
{
#ifndef NOLIBALLOC
    return (__malloc(dwSize));
#else
    return (PosAllocMem(dwSize, POS_LOC32));
#endif
}

BOOL WINAPI VirtualFree (LPVOID lpAddress,
                         SIZE_T dwSize,
                         DWORD dwFreeType)
{
#ifndef NOLIBALLOC
    __free(lpAddress);
#else
    PosFreeMem(lpAddress);
#endif
    return (TRUE);
}




/* auto-genned dummy functions */

BOOL WINAPI AllocConsole(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "AllocConsole unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
void WINAPI AttachConsole(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "AttachConsole unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CallNamedPipeA(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "CallNamedPipeA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CancelIo(void)
{
    unsigned int len = 24;
    PosWriteFile(1, "CancelIo unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ClearCommBreak(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "ClearCommBreak unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CompareFileTime(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "CompareFileTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CompareStringW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "CompareStringW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CreateFileW(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "CreateFileW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CreatePipe(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "CreatePipe unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CreateProcessW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "CreateProcessW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CreateRemoteThread(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "CreateRemoteThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CreateTapePartition(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "CreateTapePartition unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CreateThread(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "CreateThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI DebugBreak(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "DebugBreak unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI DeleteCriticalSection(void)
{
    unsigned int len = 37;
    PosWriteFile(1, "DeleteCriticalSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI DeviceIoControl(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "DeviceIoControl unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI DisconnectNamedPipe(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "DisconnectNamedPipe unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI DuplicateHandle(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "DuplicateHandle unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI EnterCriticalSection(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "EnterCriticalSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI EraseTape(void)
{
    unsigned int len = 25;
    PosWriteFile(1, "EraseTape unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI EscapeCommFunction(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "EscapeCommFunction unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ExitThread(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "ExitThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ExpandEnvironmentStringsW(void)
{
    unsigned int len = 41;
    PosWriteFile(1, "ExpandEnvironmentStringsW unimplemented\r\n", len, &len);
    for (;;) ;
}
BOOL WINAPI FillConsoleOutputAttribute(HANDLE h,
                                       WORD w,
                                       DWORD d,
                                       COORD c,
                                       LPDWORD lpd)
{
    unsigned int len = 42;
    PosWriteFile(1, "FillConsoleOutputAttribute unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
BOOL WINAPI FillConsoleOutputCharacterA(HANDLE h,
                                        TCHAR t,
                                        DWORD d,
                                        COORD c,
                                        LPDWORD lpd)
{
    unsigned int len = 43;
    PosWriteFile(1, "FillConsoleOutputCharacterA unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
void WINAPI FindFirstVolumeW(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "FindFirstVolumeW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FindNextVolumeW(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "FindNextVolumeW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FindVolumeClose(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "FindVolumeClose unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FlushConsoleInputBuffer(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "FlushConsoleInputBuffer unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FlushFileBuffers(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "FlushFileBuffers unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FlushViewOfFile(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "FlushViewOfFile unimplemented\r\n", len, &len);
    for (;;) ;
}
BOOL WINAPI FreeConsole(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "FreeConsole unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
void WINAPI FreeEnvironmentStringsW(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "FreeEnvironmentStringsW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FreeLibrary(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "FreeLibrary unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetBinaryTypeW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "GetBinaryTypeW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCPInfo(void)
{
    unsigned int len = 25;
    PosWriteFile(1, "GetCPInfo unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCommModemStatus(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "GetCommModemStatus unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCommandLineW(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "GetCommandLineW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetComputerNameA(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetComputerNameA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetComputerNameW(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetComputerNameW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetConsoleCP(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "GetConsoleCP unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetConsoleCursorInfo(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "GetConsoleCursorInfo unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetConsoleWindow(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetConsoleWindow unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCurrentProcess(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "GetCurrentProcess unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCurrentProcessId(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "GetCurrentProcessId unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCurrentThread(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetCurrentThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetCurrentThreadId(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "GetCurrentThreadId unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetEnvironmentStringsW(void)
{
    unsigned int len = 38;
    PosWriteFile(1, "GetEnvironmentStringsW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetEnvironmentVariableA(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "GetEnvironmentVariableA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetEnvironmentVariableW(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "GetEnvironmentVariableW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetExitCodeThread(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "GetExitCodeThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetFileSize(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "GetFileSize unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetFileSizeEx(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "GetFileSizeEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetFileType(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "GetFileType unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetHandleInformation(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "GetHandleInformation unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetLocaleInfoA(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "GetLocaleInfoA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetLocaleInfoW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "GetLocaleInfoW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetLogicalDriveStringsA(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "GetLogicalDriveStringsA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetLogicalDrives(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetLogicalDrives unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetMailslotInfo(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "GetMailslotInfo unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetModuleFileNameA(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "GetModuleFileNameA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetModuleFileNameW(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "GetModuleFileNameW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetModuleHandleExW(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "GetModuleHandleExW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetModuleHandleW(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetModuleHandleW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetNumberOfConsoleInputEvents(void)
{
    unsigned int len = 45;
    PosWriteFile(1, "GetNumberOfConsoleInputEvents unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetOverlappedResult(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "GetOverlappedResult unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetPriorityClass(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetPriorityClass unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetProcAddress(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "GetProcAddress unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetProcessWorkingSetSize(void)
{
    unsigned int len = 40;
    PosWriteFile(1, "GetProcessWorkingSetSize unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetStartupInfoA(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "GetStartupInfoA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetStartupInfoW(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "GetStartupInfoW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetSystemDirectoryW(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "GetSystemDirectoryW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetSystemInfo(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "GetSystemInfo unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetSystemWindowsDirectoryW(void)
{
    unsigned int len = 42;
    PosWriteFile(1, "GetSystemWindowsDirectoryW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetSystemWow64DirectoryW(void)
{
    unsigned int len = 40;
    PosWriteFile(1, "GetSystemWow64DirectoryW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetTapeParameters(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "GetTapeParameters unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetTapePosition(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "GetTapePosition unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetTapeStatus(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "GetTapeStatus unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetThreadContext(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetThreadContext unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetThreadPriority(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "GetThreadPriority unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetTickCount(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "GetTickCount unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetTimeZoneInformation(void)
{
    unsigned int len = 38;
    PosWriteFile(1, "GetTimeZoneInformation unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetVersionExA(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "GetVersionExA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetVolumePathNamesForVolumeNameW(void)
{
    unsigned int len = 48;
    PosWriteFile(1, "GetVolumePathNamesForVolumeNameW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GlobalLock(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "GlobalLock unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GlobalMemoryStatusEx(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "GlobalMemoryStatusEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GlobalSize(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "GlobalSize unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GlobalUnlock(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "GlobalUnlock unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI InitializeCriticalSection(void)
{
    unsigned int len = 41;
    PosWriteFile(1, "InitializeCriticalSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI IsBadStringPtrA(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "IsBadStringPtrA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI IsDebuggerPresent(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "IsDebuggerPresent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI IsProcessInJob(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "IsProcessInJob unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI IsProcessorFeaturePresent(void)
{
    unsigned int len = 41;
    PosWriteFile(1, "IsProcessorFeaturePresent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LCMapStringW(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "LCMapStringW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LeaveCriticalSection(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "LeaveCriticalSection unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LoadLibraryA(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "LoadLibraryA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LoadLibraryExA(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "LoadLibraryExA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LoadLibraryExW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "LoadLibraryExW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LoadLibraryW(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "LoadLibraryW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LocalFree(void)
{
    unsigned int len = 25;
    PosWriteFile(1, "LocalFree unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI MapViewOfFile(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "MapViewOfFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI MapViewOfFileEx(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "MapViewOfFileEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI MultiByteToWideChar(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "MultiByteToWideChar unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI OpenProcess(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "OpenProcess unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI OpenThread(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "OpenThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI OutputDebugStringA(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "OutputDebugStringA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI PeekConsoleInputW(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "PeekConsoleInputW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI PeekNamedPipe(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "PeekNamedPipe unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI PrepareTape(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "PrepareTape unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI PurgeComm(void)
{
    unsigned int len = 25;
    PosWriteFile(1, "PurgeComm unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI QueryDosDeviceW(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "QueryDosDeviceW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI QueryInformationJobObject(void)
{
    unsigned int len = 41;
    PosWriteFile(1, "QueryInformationJobObject unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI QueryPerformanceCounter(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "QueryPerformanceCounter unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI QueryPerformanceFrequency(void)
{
    unsigned int len = 41;
    PosWriteFile(1, "QueryPerformanceFrequency unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI QueueUserAPC(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "QueueUserAPC unimplemented\r\n", len, &len);
    for (;;) ;
}
BOOL WINAPI ReadConsoleInputA(HANDLE h, PINPUT_RECORD pi, DWORD d, LPDWORD lpd)
{
    unsigned int len = 33;
    PosWriteFile(1, "ReadConsoleInputA unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
BOOL WINAPI PeekConsoleInputA(HANDLE h, PINPUT_RECORD pi, DWORD d, LPDWORD lpd)
{
    unsigned int len = 33;
    PosWriteFile(1, "PeekConsoleInputA unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
void WINAPI ReadConsoleInputW(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "ReadConsoleInputW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ReadConsoleOutputW(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "ReadConsoleOutputW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ReadProcessMemory(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "ReadProcessMemory unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ReleaseMutex(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "ReleaseMutex unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ReleaseSemaphore(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "ReleaseSemaphore unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ResetEvent(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "ResetEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ResumeThread(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "ResumeThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlCaptureContext(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "RtlCaptureContext unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI RtlUnwind(void)
{
    unsigned int len = 25;
    PosWriteFile(1, "RtlUnwind unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI ScrollConsoleScreenBufferA(void)
{
    unsigned int len = 42;
    PosWriteFile(1, "ScrollConsoleScreenBufferA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetCommBreak(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "SetCommBreak unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetCommMask(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "SetCommMask unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetCommTimeouts(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "SetCommTimeouts unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetConsoleCursorInfo(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "SetConsoleCursorInfo unimplemented\r\n", len, &len);
    for (;;) ;
}
BOOL WINAPI SetConsoleCursorPosition(HANDLE h, COORD c)
{
    unsigned int len = 40;
    PosWriteFile(1, "SetConsoleCursorPosition unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
void WINAPI SetConsoleTextAttribute(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "SetConsoleTextAttribute unimplemented\r\n", len, &len);
    for (;;) ;
}
BOOL WINAPI SetConsoleTitleW(LPCTSTR x)
{
    unsigned int len = 32;
    PosWriteFile(1, "SetConsoleTitleW unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
BOOL WINAPI SetConsoleTitleA(LPCTSTR x)
{
    unsigned int len = 32;
    PosWriteFile(1, "SetConsoleTitleA unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
BOOL WINAPI GetConsoleTitleA(LPCTSTR x, DWORD y)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetConsoleTitleA unimplemented\r\n", len, &len);
    for (;;) ;
    return (0);
}
void WINAPI SetEnvironmentVariableW(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "SetEnvironmentVariableW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetErrorMode(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "SetErrorMode unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetEvent(void)
{
    unsigned int len = 24;
    PosWriteFile(1, "SetEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetFilePointerEx(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "SetFilePointerEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetHandleInformation(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "SetHandleInformation unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetNamedPipeHandleState(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "SetNamedPipeHandleState unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetPriorityClass(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "SetPriorityClass unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetProcessWorkingSetSize(void)
{
    unsigned int len = 40;
    PosWriteFile(1, "SetProcessWorkingSetSize unimplemented\r\n", len, &len);
    for (;;) ;
}


BOOL WINAPI SetStdHandle(DWORD nStdHandle, HANDLE hHandle)
{
    return PosSetStdHandle(nStdHandle, hHandle);
}

void WINAPI SetSystemTime(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "SetSystemTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetTapeParameters(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "SetTapeParameters unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetTapePosition(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "SetTapePosition unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetThreadAffinityMask(void)
{
    unsigned int len = 37;
    PosWriteFile(1, "SetThreadAffinityMask unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetThreadContext(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "SetThreadContext unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetThreadPriority(void)
{
    unsigned int len = 33;
    PosWriteFile(1, "SetThreadPriority unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SuspendThread(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "SuspendThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SwitchToThread(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "SwitchToThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI TerminateThread(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "TerminateThread unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI TlsAlloc(void)
{
    unsigned int len = 24;
    PosWriteFile(1, "TlsAlloc unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI TlsFree(void)
{
    unsigned int len = 23;
    PosWriteFile(1, "TlsFree unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI TlsGetValue(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "TlsGetValue unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI TlsSetValue(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "TlsSetValue unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI TransmitCommChar(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "TransmitCommChar unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI UnmapViewOfFile(void)
{
    unsigned int len = 31;
    PosWriteFile(1, "UnmapViewOfFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI VirtualProtect(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "VirtualProtect unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI VirtualProtectEx(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "VirtualProtectEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI VirtualQuery(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "VirtualQuery unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI VirtualQueryEx(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "VirtualQueryEx unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WaitCommEvent(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "WaitCommEvent unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WaitForMultipleObjects(void)
{
    unsigned int len = 38;
    PosWriteFile(1, "WaitForMultipleObjects unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WaitNamedPipeW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "WaitNamedPipeW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WideCharToMultiByte(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "WideCharToMultiByte unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WriteConsoleOutputW(void)
{
    unsigned int len = 35;
    PosWriteFile(1, "WriteConsoleOutputW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WriteConsoleW(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "WriteConsoleW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WriteProcessMemory(void)
{
    unsigned int len = 34;
    PosWriteFile(1, "WriteProcessMemory unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI WriteTapemark(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "WriteTapemark unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI CompareStringA(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "CompareStringA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FileTimeToLocalFileTime(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "FileTimeToLocalFileTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FileTimeToSystemTime(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "FileTimeToSystemTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI FormatMessageA(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "FormatMessageA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetACP(void)
{
    unsigned int len = 22;
    PosWriteFile(1, "GetACP unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetDriveTypeA(void)
{
    unsigned int len = 29;
    PosWriteFile(1, "GetDriveTypeA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetFullPathNameA(void)
{
    unsigned int len = 32;
    PosWriteFile(1, "GetFullPathNameA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetLocalTime(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "GetLocalTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetOEMCP(void)
{
    unsigned int len = 24;
    PosWriteFile(1, "GetOEMCP unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetStringTypeA(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "GetStringTypeA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI GetStringTypeW(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "GetStringTypeW unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI HeapAlloc(void)
{
    unsigned int len = 25;
    PosWriteFile(1, "HeapAlloc unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI HeapCreate(void)
{
    unsigned int len = 26;
    PosWriteFile(1, "HeapCreate unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI HeapDestroy(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "HeapDestroy unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI HeapFree(void)
{
    unsigned int len = 24;
    PosWriteFile(1, "HeapFree unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI HeapReAlloc(void)
{
    unsigned int len = 27;
    PosWriteFile(1, "HeapReAlloc unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LCMapStringA(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "LCMapStringA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI LocalFileTimeToFileTime(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "LocalFileTimeToFileTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetEndOfFile(void)
{
    unsigned int len = 28;
    PosWriteFile(1, "SetEndOfFile unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetEnvironmentVariableA(void)
{
    unsigned int len = 39;
    PosWriteFile(1, "SetEnvironmentVariableA unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SetHandleCount(void)
{
    unsigned int len = 30;
    PosWriteFile(1, "SetHandleCount unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI SystemTimeToFileTime(void)
{
    unsigned int len = 36;
    PosWriteFile(1, "SystemTimeToFileTime unimplemented\r\n", len, &len);
    for (;;) ;
}
void WINAPI UnhandledExceptionFilter(void)
{
    unsigned int len = 40;
    PosWriteFile(1, "UnhandledExceptionFilter unimplemented\r\n", len, &len);
    for (;;) ;
}

/* end of auto-genned functions */
