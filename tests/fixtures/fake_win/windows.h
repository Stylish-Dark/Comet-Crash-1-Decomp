#pragma once
#include <stdint.h>
#include <stddef.h>
#define CALLBACK
#define WINAPI
#define TRUE 1
#define FALSE 0
#define MAX_PATH 260
#define WS_EX_TOOLWINDOW 0x80
#define WS_EX_TOPMOST 0x8
#define WS_EX_NOACTIVATE 0x08000000
#define WS_POPUP 0x80000000u
#define WS_VISIBLE 0x10000000
#define WS_OVERLAPPEDWINDOW 0x00CF0000
#define GWL_EXSTYLE -20
#define GWL_STYLE -16
#define GW_OWNER 4
#define MONITOR_DEFAULTTONEAREST 2
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define SWP_FRAMECHANGED 0x0020
#define WM_ERASEBKGND 0x14
#define WM_TIMER 0x113
#define WM_PAINT 0x0F
#define VK_F1 0x70
#define VK_UP 0x26
#define VK_DOWN 0x28
#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B
#define VK_LBUTTON 1
#define VK_RBUTTON 2
#define VK_MBUTTON 4
#define IDC_ARROW ((const char*)32512)
#define DEFAULT_CHARSET 1
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define CLEARTYPE_QUALITY 5
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0
#define FW_NORMAL 400
#define TRANSPARENT 1
#define HWND_TOPMOST ((HWND)(intptr_t)-1)
#define HWND_TOP ((HWND)0)
#define HWND_NOTOPMOST ((HWND)(intptr_t)-2)
#define CW_USEDEFAULT ((int)0x80000000)
#define RGB(r,g,b) ((uint32_t)(((uint8_t)(r))|((uint16_t)((uint8_t)(g))<<8)|(((uint32_t)(uint8_t)(b))<<16)))
typedef void* HANDLE; typedef void* HWND; typedef void* HINSTANCE; typedef void* HCURSOR; typedef void* HDC; typedef void* HBRUSH; typedef void* HFONT; typedef void* HGDIOBJ; typedef void* HMONITOR;
typedef int BOOL; typedef long long LONG_PTR; typedef uint32_t DWORD; typedef short SHORT; typedef unsigned int UINT; typedef uintptr_t UINT_PTR; typedef intptr_t LPARAM; typedef uintptr_t WPARAM; typedef intptr_t LRESULT; typedef uint32_t COLORREF;
typedef struct { long left,top,right,bottom; } RECT; typedef struct { long x,y; } POINT; typedef struct { uint32_t cbSize; RECT rcMonitor; RECT rcWork; DWORD dwFlags; } MONITORINFO;
typedef struct { void* hdc; int fErase; RECT rcPaint; int fRestore; int fIncUpdate; unsigned char rgbReserved[32]; } PAINTSTRUCT;
typedef LRESULT (CALLBACK *WNDPROC)(HWND,UINT,WPARAM,LPARAM);
typedef struct { UINT style; WNDPROC lpfnWndProc; int cbClsExtra,cbWndExtra; HINSTANCE hInstance; void* hIcon; HCURSOR hCursor; HBRUSH hbrBackground; const char* lpszMenuName; const char* lpszClassName; } WNDCLASSA;
#ifdef __cplusplus
extern "C" {
#endif
DWORD GetModuleFileNameA(HINSTANCE,char*,DWORD); SHORT GetAsyncKeyState(int); BOOL IsWindowVisible(HWND); DWORD GetWindowThreadProcessId(HWND,DWORD*); LONG_PTR GetWindowLongPtrA(HWND,int); BOOL GetClientRect(HWND,RECT*); BOOL EnumWindows(BOOL(CALLBACK*)(HWND,LPARAM),LPARAM); BOOL IsWindow(HWND); HWND GetWindow(HWND,UINT); BOOL ClientToScreen(HWND,POINT*); BOOL SetWindowPos(HWND,HWND,int,int,int,int,UINT); BOOL GetWindowRect(HWND,RECT*); HMONITOR MonitorFromWindow(HWND,DWORD); BOOL GetMonitorInfoA(HMONITOR,MONITORINFO*); LONG_PTR SetWindowLongPtrA(HWND,int,LONG_PTR); BOOL AdjustWindowRect(RECT*,DWORD,BOOL); HINSTANCE GetModuleHandleA(const char*); unsigned short RegisterClassA(const WNDCLASSA*); HCURSOR LoadCursor(HINSTANCE,const char*); HWND CreateWindowExA(DWORD,const char*,const char*,DWORD,int,int,int,int,HWND,void*,HINSTANCE,void*); UINT_PTR SetTimer(HWND,UINT_PTR,UINT,void*); BOOL InvalidateRect(HWND,const RECT*,BOOL); HDC BeginPaint(HWND,PAINTSTRUCT*); BOOL EndPaint(HWND,const PAINTSTRUCT*); HBRUSH CreateSolidBrush(COLORREF); int FillRect(HDC,const RECT*,HBRUSH); BOOL DeleteObject(void*); int SetBkMode(HDC,int); COLORREF SetTextColor(HDC,COLORREF); HFONT CreateFontA(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,const char*); HGDIOBJ SelectObject(HDC,HGDIOBJ); BOOL TextOutA(HDC,int,int,const char*,int); LRESULT DefWindowProcA(HWND,UINT,WPARAM,LPARAM); BOOL DestroyWindow(HWND); HWND GetForegroundWindow(void); BOOL GetCursorPos(POINT*); DWORD GetCurrentProcessId(void); int _putenv_s(const char*,const char*);
#ifdef __cplusplus
}
#endif

#ifndef COMET_FAKE_WIN_EXTRA
#define COMET_FAKE_WIN_EXTRA
typedef long LONG; typedef unsigned long ULONG; typedef uintptr_t ULONG_PTR; typedef void* LPVOID; typedef unsigned long long ULONGLONG;
typedef struct _EXCEPTION_RECORD_FAKE { DWORD ExceptionCode; ULONG_PTR ExceptionInformation[2]; } EXCEPTION_RECORD;
typedef struct _EXCEPTION_POINTERS { EXCEPTION_RECORD* ExceptionRecord; void* ContextRecord; } EXCEPTION_POINTERS;
#define EXCEPTION_ACCESS_VIOLATION 0xC0000005u
#define EXCEPTION_CONTINUE_EXECUTION -1
#define EXCEPTION_CONTINUE_SEARCH 0
#define MEM_COMMIT 0x1000
#define MEM_RESERVE 0x2000
#define PAGE_READWRITE 0x04
#ifdef __cplusplus
extern "C" {
#endif
LONG InterlockedIncrement(volatile LONG*);
ULONGLONG GetTickCount64(void);
void Sleep(DWORD);
void* VirtualAlloc(void*,size_t,DWORD,DWORD);
void* AddVectoredExceptionHandler(ULONG,LONG (WINAPI*)(EXCEPTION_POINTERS*));
HANDLE CreateThread(void*,size_t,DWORD (WINAPI*)(LPVOID),LPVOID,DWORD,DWORD*);
#ifdef __cplusplus
}
#endif
#endif
