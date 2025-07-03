/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  windows prototypes                                               */
/*                                                                   */
/*********************************************************************/

#include <stddef.h>

#if defined(__ARM__) || defined(__M68K__)
#define __stdcall
#endif

/* Subheader: basetsd.h */
#ifndef __EXPORT__

#if defined(__SUBC__) || defined(__NODECLSPEC__)
#define WINAPI
#else
#define WINAPI __declspec(dllimport) __stdcall
#endif

#else
#if defined(__NODECLSPEC__)
#define WINAPI
#else
#define WINAPI __declspec(dllexport) __stdcall
#endif
#endif

#define PASCAL __stdcall

#ifdef __64BIT__
#ifndef _WIN64
#define _WIN64
#endif
#endif

#define __int64 long long

#define APIENTRY WINAPI
#define CALLBACK __stdcall

#define CONST const
#define VOID void

#define FALSE 0
#define TRUE  1

typedef int BOOL;
typedef unsigned char BYTE;
typedef char CCHAR;
typedef char CHAR;
typedef unsigned int DWORD;
typedef unsigned __int64 DWORDLONG;
typedef int INT;
typedef int LONG;
typedef void *PVOID;
typedef short SHORT;
typedef unsigned int UINT;
#ifndef __SUBC__
typedef unsigned long long ULONGLONG;
#endif
typedef unsigned short USHORT;
typedef unsigned short WCHAR;
typedef unsigned short WORD;

#ifdef _WIN64
typedef __int64 LONG_PTR;
typedef __int64 INT_PTR;
typedef unsigned __int64 UINT_PTR;
typedef unsigned __int64 ULONG_PTR;
#else
typedef int LONG_PTR;
typedef int INT_PTR;
typedef unsigned int UINT_PTR;
typedef unsigned int ULONG_PTR;
#endif

typedef WORD ATOM;
typedef BYTE BOOLEAN;
typedef DWORD COLORREF;
typedef PVOID HANDLE;
typedef unsigned int SIZE_T;

typedef HANDLE HICON;

typedef HANDLE HBITMAP;
typedef HANDLE HBRUSH;
typedef HANDLE HCOLORSPACE;
typedef HANDLE HCONV;
typedef HANDLE HCONVLIST;
typedef HICON HCURSOR;
typedef HANDLE HDC;
typedef HANDLE HGDIOBJ;
typedef HANDLE HGLOBAL;
typedef HANDLE HHOOK;
typedef HANDLE HINSTANCE;
typedef HANDLE HKEY;
typedef HANDLE HKL;
typedef HANDLE HLOCAL;
typedef HANDLE HMENU;
typedef HANDLE HMODULE;
typedef HANDLE HMONITOR;
typedef HANDLE HPALETTE;
typedef HANDLE HPEN;
typedef HANDLE HRESULT;
typedef HANDLE HRGN;
typedef HANDLE HRSRC;
typedef HANDLE HSZ;
typedef HANDLE HWINSTA;
typedef HANDLE HWND;
typedef LONG_PTR LPARAM;
typedef BOOL *LPBOOL;
typedef BYTE *LPBYTE;
typedef DWORD *LPCOLORREF;
typedef CONST CHAR *LPCSTR;
typedef CONST void *LPCVOID;
typedef DWORD *LPDWORD;
typedef HANDLE *LPHANDLE;
typedef INT *LPINT;
typedef LONG *LPLONG;
typedef CHAR *LPSTR;
typedef void *LPVOID;
typedef WORD *LPWORD;
typedef LONG_PTR LRESULT;
typedef BOOL *PBOOL;
typedef BOOLEAN *PBOOLEAN;
typedef BYTE *PBYTE;
typedef CHAR *PCHAR;
typedef CONST CHAR *PCSTR;
typedef DWORD *PDWORD;
typedef HANDLE *PHANDLE;
typedef INT *PINT;
typedef LONG *PLONG;
typedef UINT_PTR WPARAM;

/* ifndef UNICODE... */
typedef LPCSTR LPCTSTR;
typedef LPSTR LPTSTR;
typedef PCSTR PCTSTR;
typedef char TCHAR;
typedef TCHAR *LPTCH;
typedef CHAR *LPCH;

#define INFINITE 0xFFFFFFFF

/* Unknown subheader. */
#define LOWORD(dwValue) ((WORD)((dwValue) & 0xffff))
#define HIWORD(dwValue) ((WORD)((dwValue) >> 16))

/* Subheader: windef.h */
#define MAX_PATH 260

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *NPPOINT, *LPPOINT;

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *NPRECT, *LPRECT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *PSIZE, *LPSIZE;

/* Subheader: minwinbase.h */
#define STATUS_SUCCESS 0x00000000
#define STATUS_PENDING 0x00000103

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        } DUMMYSTRUCTNAME;
        PVOID Pointer;
    } DUMMYUNIONNAME;
    HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

#define WIN32_FIND_DATA WIN32_FIND_DATAA
typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLASTWRITETIME;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[260];
    CHAR cAlternateFileName[14];
    DWORD obsolete_do_not_use1;
    DWORD obsolete_do_not_use2;
    WORD obsolete_do_not_use3;
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

/* Subheader: wtypesbase.h */
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

/* Subheader: winbase.h */
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

#define CREATE_BREAKAWAY_FROM_JOB         0x01000000
#define CREATE_DEFAULT_ERROR_MODE         0x04000000
#define CREATE_NEW_CONSOLE                0x00000010
#define CREATE_NEW_PROCESS_GROUP          0x00000200
#define CREATE_NO_WINDOW                  0x08000000
#define CREATE_PROTECTED_PROCESS          0x00040000
#define CREATE_PRESERVE_CODE_AUTHZ_LEVEL  0x02000000
#define CREATE_SECURE_PROCESS             0x00400000
#define CREATE_SEPARATE_WOW_VDM           0x00000800
#define CREATE_SHARED_WOW_VDM             0x00001000
#define CREATE_SUSPENDED                  0x00000004
#define CREATE_UNICODE_ENVIRONMENT        0x00000400
#define DEBUG_ONLY_THIS_PROCESS           0x00000002
#define DEBUG_PROCESS                     0x00000001
#define DETACHED_PROCESS                  0x00000008
#define EXTENDED_STARTUPINFO_PRESENT      0x00080000
#define INHERIT_PARENT_AFFINITY           0x00010000

#define PIPE_ACCESS_DUPLEX             0x00000003
#define PIPE_ACCESS_INBOUND            0x00000001
#define PIPE_ACCESS_OUTBOUND           0x00000002

#define FILE_FLAG_FIRST_PIPE_INSTANCE  0x00080000
#define FILE_FLAG_WRITE_THROUGH        0x80000000
#define FILE_FLAG_OVERLAPPED           0x40000000

#define WRITE_DAC                      0x00040000L
#define WRITE_OWNER                    0x00080000L
#define ACCESS_SYSTEM_SECURITY         0x01000000L

#define PIPE_TYPE_BYTE              0x00000000
#define PIPE_TYPE_MESSAGE           0x00000004

#define PIPE_READMODE_BYTE          0x00000000
#define PIPE_READMODE_MESSAGE       0x00000002

#define PIPE_WAIT                   0x00000000
#define PIPE_NOWAIT                 0x00000001

#define PIPE_ACCEPT_REMOTE_CLIENTS  0x00000000
#define PIPE_REJECT_REMOTE_CLIENTS  0x00000008

#define CreateNamedPipe CreateNamedPipeA
HANDLE WINAPI CreateNamedPipeA (LPCSTR lpName,
                                DWORD dwOpenMode,
                                DWORD dwPipeMode,
                                DWORD nMaxInstances,
                                DWORD nOutBufferSize,
                                DWORD nInBufferSize,
                                DWORD nDefaultTimeOut,
                                LPSECURITY_ATTRIBUTES lpSecurityAttributes);

#define GetCurrentDirectory GetCurrentDirectoryA
DWORD WINAPI GetCurrentDirectoryA (DWORD nBufferLength, LPTSTR lpBuffer);

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes);

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem);

#define MoveFile MoveFileA
BOOL WINAPI MoveFileA(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName);

#define SetCurrentDirectory SetCurrentDirectoryA
BOOL WINAPI SetCurrentDirectoryA (LPCTSTR lpPathName);

/* Subheader: namedpipeapi.h */
BOOL WINAPI ConnectNamedPipe (HANDLE hNamedPipe,
                              LPOVERLAPPED lpOverlapped);

/* Subheader: fileapi.h */
/* Access mask. */
#define GENERIC_ALL     0x10000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_WRITE   0x40000000
#define GENERIC_READ    0x80000000

#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5

#define FILE_WRITE_ATTRIBUTES 0x100

#define FILE_SHARE_DELETE 0x00000004
#define FILE_SHARE_READ   0x00000001
#define FILE_SHARE_WRITE  0x00000002

#define FILE_ATTRIBUTE_ARCHIVE               0x00000020
#define FILE_ATTRIBUTE_COMPRESSED            0x00000800
#define FILE_ATTRIBUTE_DEVICE                0x00000040
#define FILE_ATTRIBUTE_DIRECTORY             0x00000010
#define FILE_ATTRIBUTE_ENCRYPTED             0x00004000
#define FILE_ATTRIBUTE_HIDDEN                0x00000002
#define FILE_ATTRIBUTE_INTEGRITY_STREAM      0x00008000
#define FILE_ATTRIBUTE_NORMAL                0x00000080
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED   0x00002000
#define FILE_ATTRIBUTE_NO_SCRUB_DATA         0x00020000
#define FILE_ATTRIBUTE_OFFLINE               0x00001000
#define FILE_ATTRIBUTE_READONLY              0x00000001
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS 0x00400000
#define FILE_ATTRIBUTE_RECALL_ON_OPEN        0x00040000
#define FILE_ATTRIBUTE_REPARSE_POINT         0x00000400
#define FILE_ATTRIBUTE_SPARSE_FILE           0x00000200
#define FILE_ATTRIBUTE_SYSTEM                0x00000004
#define FILE_ATTRIBUTE_TEMPORARY             0x00000100
#define FILE_ATTRIBUTE_VIRTUAL               0x00010000

#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2

typedef void (CALLBACK *LPOVERLAPPED_COMPLETION_ROUTINE)(DWORD dwErrorCode,
                                                         DWORD dwNumberOfBytesTransfered,
                                                         LPOVERLAPPED lpOverlapped);

typedef struct {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD dwVolumeSerialNumber;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD nNumberOfLinks;
    DWORD nFileIndexHigh;
    DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

#define CreateDirectory CreateDirectoryA
BOOL WINAPI CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes);

#define CreateFile CreateFileA
HANDLE WINAPI CreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile);

#define DeleteFile DeleteFileA
BOOL WINAPI DeleteFileA(LPCSTR lpFileName);

#define FindFirstFile FindFirstFileA
HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, WIN32_FIND_DATA *FindFileData);

#define FindNextFile FindNextFileA
BOOL WINAPI FindNextFileA(HANDLE h, WIN32_FIND_DATA *FindFileData);

BOOL WINAPI FindClose(HANDLE h);

#define GetFileAttributes GetFileAttributesA
DWORD WINAPI GetFileAttributesA(
    LPCSTR lpFileName);

BOOL WINAPI GetFileTime (HANDLE hFile,
                         LPFILETIME lpCreationTime,
                         LPFILETIME lpLastAccessTime,
                         LPFILETIME lpLastWriteTime);

#define GetTempPath GetTempPathA
DWORD WINAPI GetTempPathA(DWORD d, LPTSTR lpbuffer);

BOOL WINAPI ReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped);

BOOL WINAPI ReadFileEx (HANDLE hFile,
                        LPVOID lpBuffer,
                        DWORD nNumberOfBytesToRead,
                        LPOVERLAPPED lpOverlapped,
                        LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

#define RemoveDirectory RemoveDirectoryA
BOOL WINAPI RemoveDirectoryA(
    LPCSTR lpPathName);

DWORD WINAPI SetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod);

BOOL WINAPI SetFileTime (HANDLE hFile,
                         const FILETIME *lpCreationTime,
                         const FILETIME *lpLastAccessTime,
                         const FILETIME *lpLastWriteTime);

BOOL WINAPI WriteFile(HANDLE hFile,
                      LPCVOID lpBuffer,
                      DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten,
                      LPOVERLAPPED lpOverlapped);

BOOL WINAPI WriteFileEx (HANDLE hFile,
                         LPCVOID lpBuffer,
                         DWORD nNumberOfBytesToWrite,
                         LPOVERLAPPED lpOverlapped,
                         LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

/* Subheader: processthreadapi.h */
#define STILL_ACTIVE STATUS_PENDING

#define STARTF_USESTDHANDLES 0x00000100

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

#define STARTUPINFO STARTUPINFOA
typedef struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

#define CreateProcess CreateProcessA
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
    LPPROCESS_INFORMATION lpProcessInformation);

/* we can use this non-standard keyword, but then it
   causes a warning in compk32, for no obvious benefit */
/* __declspec(noreturn) */ void WINAPI ExitProcess(UINT uExitCode);

BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);

BOOL WINAPI TerminateProcess (HANDLE hProcess,
                              UINT uExitCode);

/* Subheader: wingdi.h */
#define BI_RGB        0
#define BI_BIRFIELDS  3

typedef enum {
    DIB_RGB_COLORS = 0x00,
    DIB_PAL_COLORS = 0x01,
    DIB_PAL_INDICES = 0x02
} DIBColors;

#define BLACKNESS          66
#define CAPTUREBLT         1073741824
#define DSTINVERT          5570569
#define MERGECOPY          12583114
#define MERGEPAINT         12255782
#define NOMIRRORBITMAP     -2147483648
#define NOTSRCCOPY         3342344
#define NOTSRCERASE        1114278
#define PATCOPY            15728673
#define PATINVERT          5898313
#define PATPAINT           16452105
#define SRCAND             8913094
#define SRCCOPY            13369376
#define SRCERASE           4457256
#define SRCINVERT          6684742
#define SRCPAINT           15597702
#define WHITENESS          16711778

typedef struct RGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors;
} BITMAPINFO, *LPBITMAPINFO, *PBITMAPINFO;

BOOL WINAPI BitBlt (HDC hdc,
                    int x,
                    int y,
                    int cx,
                    int cy,
                    HDC hdcSrc,
                    int x1,
                    int y1,
                    DWORD rop);

HDC WINAPI CreateCompatibleDC (HDC hdc);

HBITMAP WINAPI CreateDIBSection (HDC hdc,
                                 const BITMAPINFO *pbmi,
                                 UINT usage,
                                 VOID **ppvBits,
                                 HANDLE hSection,
                                 DWORD offset);

HBRUSH WINAPI CreateSolidBrush (COLORREF color);

BOOL WINAPI DeleteDC (HDC hdc);

BOOL WINAPI DeleteObject (HGDIOBJ ho);

BOOL WINAPI Rectangle (HDC hdc,
                       int left,
                       int top,
                       int right,
                       int bottom);

HGDIOBJ WINAPI SelectObject (HDC hdc,
                             HGDIOBJ h);

#define TextOut TextOutA
BOOL WINAPI TextOutA (HDC hdc,
                      int x,
                      int y,
                      LPCSTR lpString,
                      int c);

/* Subheader: sysinfoapi.h */
void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime);
void WINAPI GetSystemTimeAsFileTime (LPFILETIME lpSystemTimeAsFileTime);

/* Subheader: synchapi.h */
DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

/* Subheader: wincon.h */
#define ENABLE_WINDOW_INPUT                     0x0008
#define ENABLE_PROCESSED_INPUT                  0x0001
#define ENABLE_PROCESSED_OUTPUT                 0x0001
#define ENABLE_LINE_INPUT                       0x0002
#define ENABLE_ECHO_INPUT                       0x0004
#define ENABLE_MOUSE_INPUT                      0x0010

#define FOREGROUND_BLUE                         0x0001
#define FOREGROUND_GREEN                        0x0002
#define FOREGROUND_RED                          0x0004

#define KEY_EVENT                               0x0001
#define MOUSE_EVENT                             0x0002
#define WINDOW_BUFFER_SIZE_EVENT                0x0004

#define SHIFT_PRESSED                           0x0010
#define RIGHT_CTRL_PRESSED                      0x0004
#define LEFT_CTRL_PRESSED                       0x0008
#define RIGHT_ALT_PRESSED                       0x0001
#define LEFT_ALT_PRESSED                        0x0002

#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)

#ifndef __SUBC__
typedef struct _CHAR_INFO {
    union {
        WCHAR UnicodeChar;
        CHAR AsciiChar;
    } Char;
    WORD Attributes;
} CHAR_INFO;

typedef CHAR_INFO *PCHAR_INFO;
#endif

typedef struct _COORD {
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;

typedef struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT;

typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO;

#ifndef __SUBC__
typedef struct _KEY_EVENT_RECORD {
    BOOL  bKeyDown;
    WORD  wRepeatCount;
    WORD  wVirtualKeyCode;
    WORD  wVirtualScanCode;
    union {
      WCHAR UnicodeChar;
      CHAR  AsciiChar;
    } uChar;
    DWORD dwControlKeyState;
} KEY_EVENT_RECORD;
#endif

typedef struct _MOUSE_EVENT_RECORD {
    COORD dwMousePosition;
    DWORD dwButtonState;
    DWORD dwControlKeyState;
    DWORD dwEventFlags;
} MOUSE_EVENT_RECORD;

typedef struct _WINDOW_BUFFER_SIZE_RECORD {
  COORD dwSize;
} WINDOW_BUFFER_SIZE_RECORD;

#ifndef __SUBC__
typedef struct _INPUT_RECORD {
    WORD EventType;
    union {
    KEY_EVENT_RECORD KeyEvent;
    MOUSE_EVENT_RECORD MouseEvent;
    WINDOW_BUFFER_SIZE_RECORD WindowBufferSizeEvent;
    } Event;
} INPUT_RECORD, *PINPUT_RECORD;
#endif

BOOL WINAPI AllocConsole(void);

BOOL WINAPI FillConsoleOutputAttribute(HANDLE h,
                                       WORD w,
                                       DWORD d,
                                       COORD c,
                                       LPDWORD lpd);

#define FillConsoleOutputCharacter FillConsoleOutputCharacterA
BOOL WINAPI FillConsoleOutputCharacterA(HANDLE h,
                                        TCHAR t,
                                        DWORD d,
                                        COORD c,
                                        LPDWORD lpd);

BOOL WINAPI FreeConsole(void);

BOOL WINAPI GetConsoleScreenBufferInfo(
    HANDLE hFile,
    CONSOLE_SCREEN_BUFFER_INFO *pcsbi);

BOOL WINAPI GetConsoleMode(HANDLE hFile, DWORD *dw);

#define GetConsoleTitle GetConsoleTitleA
BOOL WINAPI GetConsoleTitleA(LPCTSTR x, DWORD y);

BOOL WINAPI GetNumberOfConsoleMouseButtons(LPDWORD lpd);

HANDLE WINAPI GetStdHandle(DWORD nStdHandle);

#ifndef __SUBC__
typedef BOOL (CALLBACK *PHANDLER_ROUTINE)(DWORD CtrlType);
#endif

#define ReadConsoleInput ReadConsoleInputA
#ifndef __SUBC__
BOOL WINAPI ReadConsoleInputA(HANDLE h, PINPUT_RECORD pi, DWORD d, LPDWORD lpd);
#endif

#define PeekConsoleInput PeekConsoleInputA
#ifndef __SUBC__
BOOL WINAPI PeekConsoleInputA(HANDLE h, PINPUT_RECORD pi, DWORD d, LPDWORD lpd);
#endif

BOOL WINAPI SetConsoleCursorPosition(HANDLE h, COORD c);

BOOL WINAPI SetConsoleMode(HANDLE hFile, DWORD dw);

#define SetConsoleTitle SetConsoleTitleA
BOOL WINAPI SetConsoleTitleA(LPCTSTR x);

BOOL WINAPI SetStdHandle(DWORD nStdHandle, HANDLE hHandle);

#ifndef __SUBC__
#define WriteConsoleOutput WriteConsoleOutputA
BOOL WINAPI WriteConsoleOutputA(
    HANDLE hFile,
    const CHAR_INFO *cinfo,
    COORD bufferSize,
    COORD bufferCoord,
    SMALL_RECT *rect);
#endif

/* Subheader: process.h */
#define DLL_PROCESS_ATTACH 1
#define DLL_PROCESS_DETACH 0
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3

/* Subheader: processenv.h */
#define GetCommandLine GetCommandLineA
LPTSTR WINAPI GetCommandLineA(void);

#define GetEnvironmentStrings GetEnvironmentStringsA
LPCH WINAPI GetEnvironmentStrings(void);

#define FreeEnvironmentStrings FreeEnvironmentStringsA
BOOL WINAPI FreeEnvironmentStrings(LPCH x);

/* Subheader: handleapi.h */
BOOL WINAPI CloseHandle(HANDLE hObject);

/* Subheader: winerror.h */
#define ERROR_SUCCESS         0
#define ERROR_FILE_NOT_FOUND  2
#define ERROR_NO_MORE_FILES   18

/* Subheader: errhandlingapi.h */
DWORD WINAPI GetLastError(void);

/* Subheader: winuser.h */
#define COLOR_WINDOW  5

#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_CLASSDC          0x0040
#define CS_DBLCLKS          0x0008
#define CS_DROPSHADOW       0x00020000
#define CS_GLOBALCLASS      0x4000
#define CS_HREDRAW          0x0002
#define CS_NOCLOSE          0x0200
#define CS_OWNDC            0x0020
#define CS_PARENTDC         0x0080
#define CS_SAVEBITS         0x0800
#define CS_VREDRAW          0x0001

#define CW_USEDEFAULT  0x80000000

#define IDABORT     3
#define IDCANCEL    2
#define IDCONTINUE  11
#define IDIGNORE    5
#define IDNO        7
#define IDOK        1
#define IDRETRY     4
#define IDTRYAGAIN  10
#define IDYES       6

#define IDC_ARROW        MAKEINTRESOURCE(32512)
#define IDC_IBEAM        MAKEINTRESOURCE(32513)
#define IDC_WAIT         MAKEINTRESOURCE(32514)
#define IDC_CROSS        MAKEINTRESOURCE(32515)
#define IDC_UPARROW      MAKEINTRESOURCE(32516)
#define IDC_SIZENWSE     MAKEINTRESOURCE(32642)
#define IDC_SIZENESW     MAKEINTRESOURCE(32643)
#define IDC_SIZEWE       MAKEINTRESOURCE(32644)
#define IDC_SIZENS       MAKEINTRESOURCE(32645)
#define IDC_SIZEALL      MAKEINTRESOURCE(32646)
#define IDC_NO           MAKEINTRESOURCE(32648)
#define IDC_HAND         MAKEINTRESOURCE(32649)
#define IDC_APPSTARTING  MAKEINTRESOURCE(32650)
#define IDC_HELP         MAKEINTRESOURCE(32651)
#define IDC_PIN          MAKEINTRESOURCE(32671)
#define IDC_PERSON       MAKEINTRESOURCE(32672)

#define ISC_SHOWUICOMPOSITIONWINDOW  0x80000000
#define ISC_SHOWUICANDIDATEWINDOW    0x00000001

#define MWMO_ALERTABLE       0x0002
#define MWMO_INPUTAVAILABLE  0x0004
#define MWMO_WAITALL         0x0001

#define PM_NOREMOVE        0x0000
#define PM_REMOVE          0x0001
#define PM_NOYIELD         0x0002
#define PM_QS_INPUT        (QS_INPUT << 16)
#define PM_QS_PAINT        (QS_PAINT << 16)
#define PM_QS_POSTMESSAGE  \
 ((QS_POSTMESSAGE | QS_HOTKEY | QS_TIMER) << 16)
#define PM_QS_SENDMESSAGE  (QS_SENDMESSAGE << 16)

#define QS_KEY             0x0001
#define QS_MOUSEMOVE       0x0002
#define QS_MOUSEBUTTON     0x0004
#define QS_POSTMESSAGE     0x0008
#define QS_TIMER           0x0010
#define QS_PAINT           0x0020
#define QS_SENDMESSAGE     0x0040
#define QS_HOTKEY          0x0080
#define QS_ALLPOSTMESSAGE  0x0100
#define QS_RAWINPUT        0x0400
#define QS_TOUCH           0x0800
#define QS_POINTER         0x1000
#define QS_MOUSE           (QS_MOUSEMOVE | QS_MOUSEBUTTON)
#define QS_INPUT           \
 (QS_MOUSE | QS_KEY | QS_RAWINPUT | QS_TOUCH | QS_POINTER)
#define QS_ALLEVENTS       \
 (QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_HOTKEY)
#define QS_ALLINPUT        \
 (QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_HOTKEY | QS_SENDMESSAGE)

#define SM_ARRANGE                      56
#define SM_CLEANBOOT                    67
#define SM_CMONITORS                    80
#define SM_CMOUSEBUTTONS                43
#define SM_CONVERTIBLESLATEMODE         0x2003
#define SM_CXBORDER                     5
#define SM_CXCURSOR                     13
#define SM_CXDLGFRAME                   7
#define SM_CXDOUBLECLICK                36
#define SM_CXDRAG                       68
#define SM_CXEDGE                       45
#define SM_CXFIXEDFRAME                 7
#define SM_CXFOCUSBORDER                83
#define SM_CXFRAME                      32
#define SM_CXFULLSCREEN                 16
#define SM_CXHSCROLL                    21
#define SM_CXHTHUMB                     10
#define SM_CXICON                       11
#define SM_CXICONSPACING                38
#define SM_CXMAXIMIZED                  61
#define SM_CXMAXTRACK                   59
#define SM_CXMENUCHECK                  71
#define SM_CXMENUSIZE                   54
#define SM_CXMIN                        28
#define SM_CXMINIMIZED                  57
#define SM_CXMINSPACING                 47
#define SM_CXMINTRACK                   34
#define SM_CXPADDEDBORDER               92
#define SM_CXSCREEN                     0
#define SM_CXSIZE                       30
#define SM_CXSIZEFRAME                  32
#define SM_CXSMICON                     49
#define SM_CXSMSIZE                     52
#define SM_CXVIRTUALSCREEN              78
#define SM_CXVSCROLL                    2
#define SM_CYBORDER                     6
#define SM_CYCAPTION                    4
#define SM_CYCURSOR                     14
#define SM_CYDLGFRAME                   8
#define SM_CYDOUBLECLICK                37
#define SM_CYDRAG                       69
#define SM_CYEDGE                       46
#define SM_CYFIXEDFRAME                 8
#define SM_CYFOCUSBORDER                84
#define SM_CYFRAME                      33
#define SM_CYFULLSCREEN                 17
#define SM_CYHSCROLL                    3
#define SM_CYICON                       12
#define SM_CYICONSPACING                39
#define SM_CYKANJIWINDOW                18
#define SM_CYMAXIMIZED                  62
#define SM_CYMAXTRACK                   60
#define SM_CYMENU                       15
#define SM_CYMENUCHECK                  72
#define SM_CYMENUSIZE                   55
#define SM_CYMIN                        29
#define SM_CYMINIMIZED                  58
#define SM_CYMINSPACING                 48
#define SM_CYMINTRACK                   35
#define SM_CYSCREEN                     1
#define SM_CYSIZE                       31
#define SM_CYSIZEFRAME                  33
#define SM_CYSMCAPTION                  51
#define SM_CYSMICON                     50
#define SM_CYSMSIZE                     53
#define SM_CYVIRTUALSCREEN              79
#define SM_CYVSCROLL                    20
#define SM_CYVTHUMB                     9
#define SM_DBCSENABLED                  42
#define SM_DEBUG                        22
#define SM_DIGITIZER                    94
#define SM_IMMENABLED                   82
#define SM_MAXIMUMTOUCHES               95
#define SM_MEDIACENTER                  87
#define SM_MENUDROPALIGNMENT            40
#define SM_MIDEASTENABLED               74
#define SM_MOUSEPRESENT                 19
#define SM_MOUSEHORIZONTALWHEELPRESENT  91
#define SM_MOUSEWHEELPRESENT            75
#define SM_NETWORK                      63
#define SM_PENWINDOWS                   41
#define SM_REMOTECONTROL                0x2001
#define SM_REMOTESESSION                0x1000
#define SM_SAMEDISPLAYFORMAT            81
#define SM_SECURE                       44
#define SM_SERVERR2                     89
#define SM_SHOWSOUNDS                   70
#define SM_SHUTTINGDOWN                 0x2000
#define SM_SLOWMACHINE                  73
#define SM_STARTER                      88
#define SM_SWAPBUTTON                   23
#define SM_SYSTEMDOCKED                 0x2004
#define SM_TABLETPC                     86
#define SM_XVIRTUALSCREEN               76
#define SM_YVIRTUALSCREEN               77

#define SS_BLACKFRAME      0x7
#define SS_BLACKRECT       0x4
#define SS_CENTER          0x1
#define SS_ETCHEDFRAME     0x12
#define SS_ETCHEDHORZ      0x10
#define SS_ETCHEDVERT      0x11
#define SS_GRAYFRAME       0x8
#define SS_GRAYRECT        0x5
#define SS_LEFT            0x0
#define SS_LEFTNOWORDWRAP  0xC
#define SS_NOPREFIX        0x80
#define SS_NOTIFY          0x100
#define SS_RIGHT           0x2
#define SS_SUNKEN          0x1000
#define SS_WHITEFRAME      0x9
#define SS_WHITERECT       0x6     

#define VK_LBUTTON              0x01
#define VK_RBUTTON              0x02
#define VK_CANCEL               0x03
#define VK_MBUTTON              0x04
#define VK_XBUTTON1             0x05
#define VK_XBUTTON2             0x06
#define VK_BACK                 0x08
#define VK_TAB                  0x09
#define VK_CLEAR                0x0C
#define VK_RETURN               0x0D
#define VK_SHIFT                0x10
#define VK_CONTROL              0x11
#define VK_MENU                 0x12
#define VK_PAUSE                0x13
#define VK_CAPITAL              0x14
#define VK_KANA                 0x15
#define VK_HANGUL               0x15
#define VK_IME_ON               0x16
#define VK_JUNJA                0x17
#define VK_FINAL                0x18
#define VK_HANJA                0x19
#define VK_KANJI                0x19
#define VK_IME_OFF              0x1A
#define VK_ESCAPE               0x1B
#define VK_CONVERT              0x1C
#define VK_NONCONVERT           0x1D
#define VK_ACCEPT               0x1E
#define VK_MODECHANGE           0x1F
#define VK_SPACE                0x20
#define VK_PRIOR                0x21
#define VK_NEXT                 0x22
#define VK_END                  0x23
#define VK_HOME                 0x24
#define VK_LEFT                 0x25
#define VK_UP                   0x26
#define VK_RIGHT                0x27
#define VK_DOWN                 0x28
#define VK_SELECT               0x29
#define VK_PRINT                0x2A
#define VK_EXECUTE              0x2B
#define VK_SNAPSHOT             0x2C
#define VK_INSERT               0x2D
#define VK_DELETE               0x2E
#define VK_HELP                 0x2F
#define VK_LWIN                 0x5B
#define VK_RWIN                 0x5C
#define VK_APPS                 0x5D
#define VK_SLEEP                0x5F
#define VK_NUMPAD0              0x60
#define VK_NUMPAD1              0x61
#define VK_NUMPAD2              0x62
#define VK_NUMPAD3              0x63
#define VK_NUMPAD4              0x64
#define VK_NUMPAD5              0x65
#define VK_NUMPAD6              0x66
#define VK_NUMPAD7              0x67
#define VK_NUMPAD8              0x68
#define VK_NUMPAD9              0x69
#define VK_MULTIPLY             0x6A
#define VK_ADD                  0x6B
#define VK_SEPARATOR            0x6C
#define VK_SUBTRACT             0x6D
#define VK_DECIMAL              0x6E
#define VK_DIVIDE               0x6F
#define VK_F1                   0x70
#define VK_F2                   0x71
#define VK_F3                   0x72
#define VK_F4                   0x73
#define VK_F5                   0x74
#define VK_F6                   0x75
#define VK_F7                   0x76
#define VK_F8                   0x77
#define VK_F9                   0x78
#define VK_F10                  0x79
#define VK_F11                  0x7A
#define VK_F12                  0x7B
#define VK_F13                  0x7C
#define VK_F14                  0x7D
#define VK_F15                  0x7E
#define VK_F16                  0x7F
#define VK_F17                  0x80
#define VK_F18                  0x81
#define VK_F19                  0x82
#define VK_F20                  0x83
#define VK_F21                  0x84
#define VK_F22                  0x85
#define VK_F23                  0x86
#define VK_F24                  0x87
#define VK_NUMLOCK              0x90
#define VK_SCROLL               0x91
#define VK_LSHIFT               0xA0
#define VK_RSHIFT               0xA1
#define VK_LCONTROL             0xA2
#define VK_RCONTROL             0xA3
#define VK_LMENU                0xA4
#define VK_RMENU                0xA5
#define VK_BROWSER_BACK         0xA6
#define VK_BROWSER_FORWARD      0xA7
#define VK_BROWSER_REFRESH      0xA8
#define VK_BROWSER_STOP         0xA9
#define VK_BROWSER_SEARCH       0xAA
#define VK_BROWSER_FAVORITES    0xAB
#define VK_BROWSER_HOME         0xAC
#define VK_VOLUME_MUTE          0xAD
#define VK_VOLUME_DOWN          0xAE
#define VK_VOLUME_UP            0xAF
#define VK_MEDIA_NEXT_TRACK     0xB0
#define VK_MEDIA_PREV_TRACK     0xB1
#define VK_MEDIA_STOP           0xB2
#define VK_MEDIA_PLAY_PAUSE     0xB3
#define VK_LAUNCH_MAIL          0xB4
#define VK_LAUNCH_MEDIA_SELECT  0xB5
#define VK_LAUNCH_APP1          0xB6
#define VK_LAUNCH_APP2          0xB7
#define VK_OEM_1                0xBA
#define VK_OEM_PLUS             0xBB
#define VK_OEM_COMMA            0xBC
#define VK_OEM_MINUS            0xBD
#define VK_OEM_PERIOD           0xBE
#define VK_OEM_2                0xBF
#define VK_OEM_3                0xC0
#define VK_OEM_4                0xDB
#define VK_OEM_5                0xDC
#define VK_OEM_6                0xDD
#define VK_OEM_7                0xDE
#define VK_OEM_8                0xDF
#define VK_OEM_102              0xE2
#define VK_PROCESSKEY           0xE5
#define VK_PACKET               0xE7
#define VK_ATTN                 0xF6
#define VK_CRSEL                0xF7
#define VK_EXSEL                0xF8
#define VK_EREOF                0xF9
#define VK_PLAY                 0xFA
#define VK_ZOOM                 0xFB
#define VK_NONAME               0xFC
#define VK_PA1                  0xFD
#define VK_OEM_CLEAR            0xFE

#define WAIT_OBJECT_0       0
#define WAIT_ABANDONED_0    0x00000080L
#define WAIT_IO_COMPLETION  0x000000C0L
#define WAIT_TIMEOUT        258L
#define WAIT_FAILED         ((DWORD)0xFFFFFFFF)

#define WM_IME_CHAR                     0x0286
#define WM_IME_COMPOSITION              0x010F
#define WM_IME_COMPOSITIONFULL          0x0284
#define WM_IME_CONTROL                  0x0283
#define WM_IME_ENDCOMPOSITION           0x010E
#define WM_IME_KEYDOWN                  0x0290
#define WM_IME_KEYUP                    0x0291
#define WM_IME_NOTIFY                   0x0282
#define WM_IME_REQUEST                  0x0288
#define WM_IME_SELECT                   0x0285
#define WM_IME_SETCONTEXT               0x0281
#define WM_IME_STARTCOMPOSITION         0x010D

#define MN_GETHMENU                     0x01E1
#define WM_ERASEBKGND                   0x0014
#define WM_GETFONT                      0x0031
#define WM_GETTEXT                      0x000D
#define WM_GETTEXTLENGTH                0x000E
#define WM_SETFONT                      0x0030
#define WM_SETICON                      0x0080
#define WM_SETTEXT                      0x000C

#define WM_CTLCOLORDLG                  0x0136
#define WM_ENTERIDLE                    0x0121
#define WM_GETDLGCODE                   0x0087
#define WM_INITDIALOG                   0x0110
#define WM_NEXTDLGCTL                   0x0028

#define WM_COMMAND                      0x0111
#define WM_CONTEXTMENU                  0x007B
#define WM_ENTERMENULOOP                0x0211
#define WM_EXITMENULOOP                 0x0212
#define WM_GETTITLEBARINFOEX            0x033F
#define WM_MENUCOMMAND                  0x0126
#define WM_MENUDRAG                     0x0123
#define WM_MENUGETOBJECT                0x0124
#define WM_MENURBUTTONUP                0x0122
#define WM_NEXTMENU                     0x0213
#define WM_UNINITMENUPOPUP              0x0125

#define WM_DISPLAYCHANGE                0x007E
#define WM_NCPAINT                      0x0085
#define WM_PAINT                        0x000F
#define WM_PRINT                        0x0317
#define WM_PRINTCLIENT                  0x0318
#define WM_SETREDRAW                    0x000B
#define WM_SYNCPAINT                    0x0088

#define WM_ACTIVATEAPP                  0x001C
#define WM_CANCELMODE                   0x001F
#define WM_CHILDACTIVATE                0x0022
#define WM_CLOSE                        0x0010
#define WM_CREATE                       0x0001
#define WM_DESTROY                      0x0002
#define WM_ENABLE                       0x000A
#define WM_ENTERSIZEMOVE                0x0231
#define WM_EXITSIZEMOVE                 0x0232
#define WM_GETICON                      0x007F
#define WM_GETMINMAXINFO                0x0024
#define WM_INPUTLANGCHANGE              0x0051
#define WM_INPUTLANGCHANGEREQUEST       0x0050
#define WM_MOVE                         0x0003
#define WM_MOVING                       0x0216
#define WM_NCACTIVATE                   0x0086
#define WM_NCCALCSIZE                   0x0083
#define WM_NCCREATE                     0x0081
#define WM_NCDESTROY                    0x0082
#define WM_NULL                         0x0000
#define WM_QUERYDRAGICON                0x0037
#define WM_QUERYOPEN                    0x0013
#define WM_QUIT                         0x0012
#define WM_SHOWWINDOW                   0x0018
#define WM_SIZE                         0x0005
#define WM_SIZING                       0x0214
#define WM_STYLECHANGED                 0x007D
#define WM_STYLECHANGING                0x007C
#define WM_THEMECHANGED                 0x031A
#define WM_WINDOWPOSCHANGED             0x0047
#define WM_WINDOWPOSCHANGING            0x0046

#define WM_ACTIVATE                     0x0006
#define WM_APPCOMMAND                   0x0319
#define WM_CHAR                         0x0102
#define WM_DEADCHAR                     0x0103
#define WM_HOTKEY                       0x0312
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
#define WM_KILLFOCUS                    0x0008
#define WM_SETFOCUS                     0x0007
#define WM_SYSDEADCHAR                  0x0107
#define WM_SYSKEYDOWN                   0x0104
#define WM_SYSKEYUP                     0x0105
#define WM_UNICHAR                      0x0109

#define WM_CAPTURECHANGED               0x0215
#define WM_LBUTTONDBLCLK                0x0203
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_MBUTTONDBLCLK                0x0209
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MOUSEACTIVATE                0x0021
#define WM_MOUSEHOVER                   0x02A1
#define WM_MOUSEHWHEEL                  0x020E
#define WM_MOUSELEAVE                   0x02A3
#define WM_MOUSEMOVE                    0x0200
#define WM_MOUSEWHEEL                   0x020A
#define WM_NCHITTEST                    0x0084
#define WM_NCLBUTTONDBLCLK              0x00A3
#define WM_NCLBUTTONDOWN                0x00A1
#define WM_NCLBUTTONUP                  0x00A2
#define WM_NCMBUTTONDBLCLK              0x00A9
#define WM_NCMBUTTONDOWN                0x00A7
#define WM_NCMBUTTONUP                  0x00A8
#define WM_NCMOUSEHOVER                 0x02A0
#define WM_NCMOUSELEAVE                 0x02A2
#define WM_NCMOUSEMOVE                  0x00A0
#define WM_NCRBUTTONDBLCLK              0x00A6
#define WM_NCRBUTTONDOWN                0x00A4
#define WM_NCRBUTTONUP                  0x00A5
#define WM_NCXBUTTONDBLCLK              0x00AD
#define WM_NCXBUTTONDOWN                0x00AB
#define WM_NCXBUTTONUP                  0x00AC
#define WM_RBUTTONDBLCLK                0x0206
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_XBUTTONDBLCLK                0x020D
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C

#define WS_BORDER            0x00800000L
#define WS_CAPTION           0x00C00000L
#define WS_CHILD             0x40000000L
#define WS_CHILDWINDOW       WS_CHILD
#define WS_CLIPCHILDREN      0x02000000L
#define WS_CLIPSIBLINGS      0x04000000L
#define WS_DISABLED          0x08000000L
#define WS_DLGFRAME          0x00400000L
#define WS_GROUP             0x00020000L
#define WS_HSCROLL           0x00100000L
#define WS_ICONIC            0x20000000L
#define WS_MAXIMIZE          0x01000000L
#define WS_MAXIMIZEBOX       0x00010000L
#define WS_MINIMIZE          WS_ICONIC
#define WS_OVERLAPPED        0x00000000L
#define WS_OVERLAPPEDWINDOW  \
 (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU \
  | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUP             0x80000000L
#define WS_POPUPWINDOWS      (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_SIZEBOX           0x00040000L
#define WS_SYSMENU           0x00080000L
#define WS_TABSTOP           0x00010000L
#define WS_THICKFRAME        WS_SIZEBOX
#define WS_TILED             WS_OVERLAPPED
#define WS_TILEDWINDOW       WS_OVERLAPPEDWINDOW
#define WS_VISIBLE           0x10000000L
#define WS_VSCROLL           0x00200000L

typedef struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
} DLGTEMPLATE, *LPDLGTEMPLATE;

typedef CONST DLGTEMPLATE *LPCDLGTEMPLATEA;
typedef LPCDLGTEMPLATEA LPCDLGTEMPLATE;

typedef INT_PTR (CALLBACK *DLGPROC) (HWND unnamedParam1,
                                     UINT unnamedParam2,
                                     WPARAM unnamedParam3,
                                     LPARAM unnamedParam4);

typedef struct tagPAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *PPAINTSTRUCT, *NPPAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
    DWORD lPrivate;
} MSG, *PMSG, *NPMSG, *LPMSG;

typedef LRESULT (CALLBACK *WNDPROC) (HWND unnamedParam1,
                                     UINT unnamedParam2,
                                     WPARAM unnamedParam3,
                                     LPARAM unnamedParam4);

#define WNDCLASS WNDCLASSA
typedef struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    PCSTR lpszClassName;
} WNDCLASSA, *PWNDCLASSA, *NPWNDCLASSA, *LPWNDCLASSA;

HDC WINAPI BeginPaint (HWND hWnd,
                       LPPAINTSTRUCT lpPaint);

#define CreateWindowEx CreateWindowExA
HWND WINAPI CreateWindowExA (DWORD dwExStyle,
                             LPCSTR lpClassName,
                             LPCSTR lpWindowName,
                             DWORD dwStyle,
                             int X,
                             int Y,
                             int nWidth,
                             int nHeight,
                             HWND hWndParent,
                             HMENU hMenu,
                             HINSTANCE hInstance,
                             LPVOID lpParam);

#define CreateWindowA(lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) \
 CreateWindowExA (0L, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam)
#define CreateWindow CreateWindowA

#define DefWindowProc DefWindowProcA
LRESULT WINAPI DefWindowProcA (HWND hWnd,
                               UINT Msg,
                               WPARAM wParam,
                               LPARAM lParam);

BOOL WINAPI DestroyWindow (HWND hWnd);

#define DialogBoxIndirect DialogBoxIndirectA
#define DialogBoxIndirectA(hInstance, lpTemplate, hWndParent, lpDialogFunc) \
 DialogBoxIndirectParamA (hInstance, lpTemplate, hWndParent, lpDialogFunc, 0)

#define DialogBoxIndirectParam DialogBoxIndirectParamA
INT_PTR WINAPI DialogBoxIndirectParamA (HINSTANCE hInstance,
                                        LPCDLGTEMPLATEA hDialogTemplate,
                                        HWND hWndParent,
                                        DLGPROC lpDialogFunc,
                                        LPARAM dwInitParam);

#define DispatchMessage DispatchMessageA
LRESULT WINAPI DispatchMessageA (const MSG *lpMsg);

BOOL WINAPI EndDialog (HWND hDlg,
                       INT_PTR nResult);

BOOL WINAPI EndPaint (HWND hWnd,
                      const PAINTSTRUCT *lpPaint);

BOOL WINAPI GetClientRect (HWND hWnd,
                           LPRECT lpRect);

HDC WINAPI GetDC (HWND hWnd);

#define GetDlgItemText GetDlgItemTextA
UINT WINAPI GetDlgItemTextA (HWND hDlg,
                             int nIDDlgItem,
                             LPSTR lpString,
                             int cchMax);

#define GetMessage GetMessageA
BOOL WINAPI GetMessageA (LPMSG lpMsg,
                         HWND hWnd,
                         UINT wMsgFilterMin,
                         UINT wMsgFilterMax);

int WINAPI GetSystemMetrics (int nIndex);

BOOL WINAPI InvalidateRect (HWND hWnd,
                            const RECT *lpRect,
                            BOOL bErase);

#define LoadCursor LoadCursorA
HCURSOR WINAPI LoadCursorA (HINSTANCE hInstance,
                            LPCSTR lpCursorName);

#define MAKEINTRESOURCE MAKEINTRESOURCEA
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))

DWORD WINAPI MsgWaitForMultipleObjects (DWORD nCount,
                                        const HANDLE *pHandles,
                                        BOOL fWaitAll,
                                        DWORD dwMilliseconds,
                                        DWORD dwWakeMask);

DWORD WINAPI MsgWaitForMultipleObjectsEx (DWORD nCount,
                                          const HANDLE *pHandles,
                                          DWORD dwMilliseconds,
                                          DWORD dwWakeMask,
                                          DWORD dwFlags);

#define PeekMessage PeekMessageA
BOOL WINAPI PeekMessageA (LPMSG lpMsg,
                          HWND hWnd,
                          UINT wMsgFilterMin,
                          UINT wMsgFilterMax,
                          UINT wRemoveMsg);

void WINAPI PostQuitMessage (int nExitCode);

#define RegisterClass RegisterClassA
ATOM WINAPI RegisterClassA (const WNDCLASSA *lpWndClass);

int WINAPI ReleaseDC (HWND hWnd,
                      HDC hDC);

BOOL WINAPI ShowWindow (HWND hWnd,
                        int nCmdShow);

BOOL WINAPI TranslateMessage (const MSG *lpMsg);

BOOL WINAPI UpdateWindow (HWND hWnd);

/* Subheader: libloaderapi.h */
#define GetModuleHandle GetModuleHandleA
HMODULE WINAPI GetModuleHandleA (LPCSTR lpModuleName);

/* Subheader: memoryapi.h */

#define MEM_RELEASE 0x8000
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_READWRITE 0x4

LPVOID WINAPI VirtualAlloc (LPVOID lpAddress,
                            SIZE_T dwSize,
                            DWORD flAllocationType,
                            DWORD flProtect);

BOOL WINAPI VirtualFree (LPVOID lpAddress,
                         SIZE_T dwSize,
                         DWORD dwFreeType);

/* Subheader: winbase.h */
#define CBR_110 110
#define CBR_300 300
#define CBR_600 600
#define CBR_1200 1200
#define CBR_2400 2400
#define CBR_4800 4800
#define CBR_9600 9600
#define CBR_14400 14400
#define CBR_19200 19200
#define CBR_38400 38400U
#define CBR_56000 56000U
#define CBR_57600 57600U
#define CBR_115200 115200UL
#define CBR_128000 128000UL
#define CBR_256000 256000UL
#define NOPARITY 0
#define ODDPARITY 1
#define EVENPARITY 2
#define DTR_CONTROL_DISABLE 0
#define DTR_CONTROL_ENABLE 1

typedef struct {
    DWORD dummy1;
    DWORD BaudRate;
    DWORD dummy2:4;
    int fDtrControl:2;
    DWORD dummy3:26;
    char dummy4[6];
    BYTE ByteSize;
    BYTE Parity;
    BYTE StopBits;
    char dummy5[7];
} DCB;

typedef struct {
    DWORD dummy1;
    DWORD cbInQue;
    DWORD cbOutQue;
} COMSTAT;

#define BuildCommDCB BuildCommDCBA
BOOL WINAPI BuildCommDCBA(char *str, void *x);

BOOL WINAPI GetCommState(HANDLE a, void *b);

BOOL WINAPI SetCommState(HANDLE a, void *b);

BOOL WINAPI ClearCommError(HANDLE a, void *b, void *c);


/* Subheader: synchapi.h */
void WINAPI Sleep(DWORD dwMilliseconds);
