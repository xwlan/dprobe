//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "console.h"
#include <richedit.h>

#define IDC_RICHEDIT 100

PWSTR Copyright = L"D Probe (R) Live Debugger Version 1.0\n"
                  L"Copyright(C) Apsara Labs 2009-2011\n"
				  L"by lan.john@gmail.com\n"
				  L"\nld>";

PWSTR Stack = L"ChildEBP RetAddr  \n0012f5c8 7682feef ntdll!KiFastSystemCallRet\n0012f5cc 7682ff22 user32!NtUserGetMessage+0xc\n0012f5e8 0043c535 user32!GetMessageW+0x33\nWARNING: Stack unwind information not available. Following frames may be wrong.\n0012f6fc 0043c054 ThunderService+0x3c535\n0012f7f4 0043be16 ThunderService+0x3c054\n0012fb78 0043bb96 ThunderService+0x3be16\n0012fc80 0043b79d ThunderService+0x3bb96\n0012fee0 004650bc ThunderService+0x3b79d\n0012ff88 7778d0e9 ThunderService+0x650bc\n0012ff94 779619bb kernel32!BaseThreadInitThunk+0xe\n0012ffd4 7796198e ntdll!__RtlUserThreadStart+0x23\n0012ffec 00000000 ntdll!_RtlUserThreadStart+0x1b\n\nld>";

#define SDK_CONSOLE_CLASS  L"SdkConsole"

HMODULE  MsftEditDll;
UINT_PTR MsftSubclassId = 1;
BOOLEAN StackOrCopyright;

HWND hWndConsole;

BOOLEAN
ConsoleRegisterClass(
	VOID
	)
{
	ATOM Atom;
	WNDCLASSEX wc = {0};

	wc.cbSize = sizeof(wc);
	wc.hbrBackground  = GetSysColorBrush(COLOR_BTNFACE);
    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = SdkMainIcon;
    wc.hIconSm        = SdkMainIcon;
    wc.hInstance      = SdkInstance;
    wc.lpfnWndProc    = ConsoleProcedure;
    wc.lpszClassName  = SDK_CONSOLE_CLASS;
	wc.lpszMenuName   = NULL; 

    Atom = RegisterClassEx(&wc);
	return Atom ? TRUE : FALSE;
}

BOOLEAN
ConsoleCreate(
	IN RECT *Rect,
	IN PWSTR Title 
	)
{
	HANDLE ThreadHandle;
	ULONG ThreadId;
	PCONSOLE_THREAD_CONTEXT Context;

	//
	// N.B. Only single instance of console window is allowed
	//

	if (hWndConsole) {
		SetForegroundWindow(hWndConsole);
		return TRUE;
	}

	Context = (PCONSOLE_THREAD_CONTEXT)BspMalloc(sizeof(CONSOLE_THREAD_CONTEXT));
	RtlCopyMemory(&Context->Rect, Rect, sizeof(RECT));
	wcscpy_s(Context->Caption, MAX_PATH, Title);

	ThreadHandle = CreateThread(NULL, 0, ConsoleWorkThread, Context, 0, &ThreadId);

	if (ThreadHandle != NULL) {
		CloseHandle(ThreadHandle);
		return TRUE;
	}

	return FALSE;
}

HWND
ConsoleCreateWindow(
	IN PCONSOLE_THREAD_CONTEXT Context
	)
{
	HWND hWnd;

	hWnd = CreateWindowEx(0, SDK_CONSOLE_CLASS, L"", 
						  WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
						  Context->Rect.left, Context->Rect.top,
						  Context->Rect.right - Context->Rect.left, 
						  Context->Rect.bottom - Context->Rect.top,
						  NULL, 0, SdkInstance, NULL);

	ASSERT(hWnd != NULL);

	if (hWnd != NULL) {
		
		SetWindowText(hWnd, Context->Caption);
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		return hWnd;

	} else {
		return NULL;
	}
}

LRESULT
ConsoleOnCreate(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	HWND hWndEdit;
	CHARFORMAT *Cf;
	PCONSOLE_OBJECT Object;

	Object = (PCONSOLE_OBJECT)SdkMalloc(sizeof(CONSOLE_OBJECT));
	Object->hWnd = hWnd;
	Object->BackColor = RGB(255, 255, 255);
	Object->TextColor = RGB(0, 0, 0);

	if (!MsftEditDll) {
		MsftEditDll = LoadLibrary(L"msftedit.dll");
		ASSERT(MsftEditDll != NULL);
	}
	
	hWndEdit = CreateWindowEx(0, MSFTEDIT_CLASS, L"", 
		                      WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_WANTRETURN |
							  ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_MULTILINE | ES_LEFT, 
							  0, 0, 0, 0, hWnd, (HMENU)IDC_RICHEDIT, SdkInstance, NULL);

	Object->hWndEdit = hWndEdit;
	SdkSetObject(hWnd, Object);

	Cf = &Object->CharFormat;
	Cf->cbSize = sizeof(CHARFORMAT);
	Cf->dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE | CFM_SIZE;
	Cf->crTextColor = RGB(0, 0, 0);
	Cf->bPitchAndFamily = FF_MODERN;
	Cf->yHeight = 200;
	wcscpy_s(Cf->szFaceName, 32, L"Courier");

	SendMessage(hWndEdit, EM_SETSEL, -1, -1);
	SendMessage(hWndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)Cf);
	SendMessage(hWndEdit, EM_SETEVENTMASK, 0, ENM_KEYEVENTS | ENM_PROTECTED);
	SendMessage(hWndEdit, EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(0xFF, 0xFF, 0xFF));

	ConsoleInsertText(Object, Copyright);
	ConsoleProtectText(Object);
	Object->CmdStartPos = ConsoleGetTextLength(Object);

	ConsoleCheckLength(Object);
	ConsoleSetCaret(Object);
	SdkCenterWindow(hWnd);
	return TRUE;
}

LRESULT
ConsoleOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {
		case IDC_RICHEDIT:
			break;
	}

	return TRUE;
}

VOID
ConsoleProtectText(
	IN PCONSOLE_OBJECT Object
	)
{
	HWND hWndEdit;
	CHARFORMAT Cf = {0};

	hWndEdit = Object->hWndEdit;

	Cf.cbSize = sizeof(CHARFORMAT);
	Cf.dwMask = CFM_PROTECTED;
	Cf.dwEffects = CFE_PROTECTED;

	SendMessage(hWndEdit, EM_SETSEL, 0, -1);
	SendMessage(hWndEdit, EM_HIDESELECTION, TRUE, 0);
	SendMessage(hWndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&Cf);
	SendMessage(hWndEdit, EM_HIDESELECTION, FALSE, 0);
}

LRESULT
ConsoleOnKeyReturn(
	IN PCONSOLE_OBJECT Object
	)
{
	HWND hWndEdit;
	ULONG Length;
	PWCHAR Buffer;
	CHARRANGE Range = {0};
	SETTEXTEX Text = {0};
	WCHAR Command[MAX_PATH];

	ASSERT(Object != NULL);
	hWndEdit = Object->hWndEdit;

	//
	// Copy command string
	//

	Length = ConsoleGetTextLength(Object);
	ConsoleGetText(Object, Command, MAX_PATH, Object->CmdStartPos, Length);

	//
	// Run input command, and return its output string
	//

	ConsoleRunCommand(Object, Command, &Buffer);

	ConsoleInsertText(Object, Stack);
	ConsoleProtectText(Object);

	Object->CmdStartPos = ConsoleGetTextLength(Object);
	ConsoleSetCaret(Object);
	return 0;
}

ULONG
ConsoleRunCommand(
	IN PCONSOLE_OBJECT Object,
	IN PWSTR Command,
	OUT PWCHAR *Buffer
	)
{
	return 0;
}

LRESULT
ConsoleOnFilter(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PCONSOLE_OBJECT Object;
	MSGFILTER *Filter;
	
	Filter = (MSGFILTER *)lp;
	Object = (PCONSOLE_OBJECT)SdkGetObject(hWnd);

	if (Filter->msg == WM_KEYUP && Filter->wParam == VK_RETURN) {
		return ConsoleOnKeyReturn(Object);
	}

	if (Filter->msg == WM_KEYDOWN && Filter->wParam == VK_CONTROL) {
		
		SHORT Key;
		Key = GetKeyState('V');
		if (Key != 0) {
			return TRUE;
		}
		
		Key = GetKeyState('v');
		if (Key != 0) {
			return TRUE;
		}
	}

	return 0;
}

LRESULT
ConsoleOnProtected(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PCONSOLE_OBJECT Object;
	ENPROTECTED *Protected;
	GETTEXTLENGTHEX Gtl = {0};
	ULONG Length;
	SHORT Key;
	HWND hWndEdit;
	MSG msg;
	BOOL MouseOp;

	Protected = (ENPROTECTED *)lp;
	Object = (PCONSOLE_OBJECT)SdkGetObject(hWnd);
	hWndEdit = Object->hWndEdit;

	//
	// Allow for mouse selection operation
	//

	MouseOp = PeekMessage(&msg, hWndEdit, WM_MOUSEFIRST, WM_MOUSELAST, FALSE);
	if (MouseOp) {
		return FALSE;
	}
	
	Key = GetKeyState(VK_CONTROL);
	if (Key < 0) {
		
		//
		// Allow for CTRL+C copy operation
		//

		Key = GetKeyState('C');
		if (Key < 0) {
			return FALSE;
		}

		Key = GetKeyState('c');
		if (Key < 0) {
			return FALSE;
		}
		
		//
		// Forbid for CTRL+V paste operation
		//

		Key = GetKeyState('V');
		if (Key < 0) {
			return TRUE;
		}

		Key = GetKeyState('v');
		if (Key < 0) {
			return TRUE;
		}
	}

	Gtl.flags = GTL_DEFAULT;
	Gtl.codepage = 1200;

	Length = (ULONG)SendMessage(hWndEdit, EM_GETTEXTLENGTHEX, (WPARAM)&Gtl, 0);
	if ((ULONG)Protected->chrg.cpMax < Object->CmdStartPos) {
		return TRUE;
	}	

	if ((ULONG)Protected->chrg.cpMax == Object->CmdStartPos) {
		Key = GetKeyState(VK_BACK);
		if (Key != 0) {
			return TRUE;
		}
	}

	return FALSE;
}

LRESULT
ConsoleOnNotify(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {
		case IDC_RICHEDIT:
			if (((LPNMHDR)lp)->code == EN_MSGFILTER) {
				return ConsoleOnFilter(hWnd, uMsg, wp, lp);
			}
			if (((LPNMHDR)lp)->code == EN_PROTECTED) {
				return ConsoleOnProtected(hWnd, uMsg, wp, lp);
			}
			break;
	}
	return 0;
}

LRESULT 
ConsoleOnClose(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PCONSOLE_OBJECT Object;

	Object = (PCONSOLE_OBJECT)SdkGetObject(hWnd);
	SdkFree(Object);

	DestroyWindow(hWnd);
	return 0;
}

LRESULT 
ConsoleOnSize(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PCONSOLE_OBJECT Object;
	RECT Rect;

	Object = (PCONSOLE_OBJECT)SdkGetObject(hWnd);
	ASSERT(Object->hWndEdit != NULL);

	GetClientRect(hWnd, &Rect);
	MoveWindow(Object->hWndEdit, Rect.left, Rect.top, 
		       Rect.right - Rect.left, 
		       Rect.bottom - Rect.top, TRUE); 
	return 0;
}

LRESULT CALLBACK 
ConsoleEditProcedure(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wp,
    IN LPARAM lp,
	IN UINT_PTR uIdSubclass,
    IN DWORD_PTR dwRefData
    )
{
    return DefSubclassProc(hWnd, uMsg, wp, lp);
}

LRESULT CALLBACK
ConsoleProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		case WM_CREATE:
			ConsoleOnCreate(hWnd, uMsg, wp, lp);
			break;

		case WM_COMMAND:
			ConsoleOnCommand(hWnd, uMsg, wp, lp);
			break;

		case WM_CLOSE:
			ConsoleOnClose(hWnd, uMsg, wp, lp); 
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_NOTIFY:
			return ConsoleOnNotify(hWnd, uMsg, wp, lp);

		case WM_SIZE:
			ConsoleOnSize(hWnd, uMsg, wp, lp);
			break;

		case WM_ERASEBKGND:
			return TRUE;

		default:
			break;
	}

	return DefWindowProc(hWnd, uMsg, wp, lp);
}

ULONG
ConsoleSetText(
	IN PCONSOLE_OBJECT Object,
	IN PWSTR Text,
	IN ULONG Start,
	IN ULONG End
	)
{
	ULONG Status;

	Status = (ULONG)SendMessage(Object->hWndEdit, EM_REPLACESEL, Start, (LPARAM)Text);
	return Status;
}

ULONG
ConsoleGetText(
	IN PCONSOLE_OBJECT Object,
	IN PWCHAR Buffer,
	IN ULONG Length,
	IN ULONG First,
	IN ULONG Last 
	)
{
	ULONG Status;
	TEXTRANGE Range = {0};

	ASSERT(Last >= First);

	if (Last - First + 1 > Length) {
		return 0;
	}

	Range.chrg.cpMin = First;
	Range.chrg.cpMax = Last;
	Range.lpstrText = Buffer;

	Status = (ULONG)SendMessage(Object->hWndEdit, EM_GETTEXTRANGE, 0, (LPARAM)&Range);
	return Status;
}

ULONG
ConsoleSetFont(
	IN PCONSOLE_OBJECT Object
	)
{
	CHARFORMAT2 Format = {0};
	ULONG Status;
	ULONG Mask;

	Mask = CFM_COLOR | CFM_SIZE | CFM_FACE;

	Format.cbSize = sizeof(CHARFORMAT2);
	Format.dwMask = Mask ;
	Format.dwEffects = 0;
	Format.yHeight = 200;
	Format.crTextColor = Object->TextColor;
	Format.crBackColor = Object->BackColor;
	wcscpy_s(Format.szFaceName, 32, L"Courier");
	
	Status = (ULONG)SendMessage(Object->hWndEdit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&Format);
	return Status;
}

ULONG
ConsoleGetTextLength(
	IN PCONSOLE_OBJECT Object 
	)
{
	ULONG Length; 
	GETTEXTLENGTHEX Gtl = {0};

	Gtl.flags = GTL_DEFAULT;
	Gtl.codepage = 1200;

	Length = (ULONG)SendMessage(Object->hWndEdit, EM_GETTEXTLENGTHEX, (WPARAM)&Gtl, 0);
	return Length;
}

ULONG
ConsoleSelectText(
	IN PCONSOLE_OBJECT Object,
	IN ULONG Start,
	IN ULONG End
	)
{
	ULONG Status;
	CHARRANGE Range;

	Range.cpMin = Start;
	Range.cpMax = End;

	Status = SendMessage(Object->hWndEdit, EM_EXSETSEL, 0, (LPARAM)&Range);
	return Status;
}

ULONG CALLBACK
ConsoleWorkThread(
	IN PVOID Context
	)
{
	BOOL Ok;
	MSG msg;
    PCONSOLE_THREAD_CONTEXT ThreadContext;
    ThreadContext = (PCONSOLE_THREAD_CONTEXT)Context;

    hWndConsole = ConsoleCreateWindow(ThreadContext);
	
	do {
		if (Ok = GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} while (Ok && msg.message != WM_QUIT);

	hWndConsole = NULL;
	FreeLibrary(MsftEditDll);
	MsftEditDll = NULL;
	BspFree(Context);
    return 0;
}

VOID
ConsoleInsertText(
	IN PCONSOLE_OBJECT Object,
	IN PWSTR Buffer
	)
{
	HWND hWndEdit;
	ULONG Length;
	CHARRANGE Cr = {0};
	GETTEXTLENGTHEX Gtl = {0};

	hWndEdit = Object->hWndEdit;

	Gtl.codepage = 1200;
	Gtl.flags = GTL_DEFAULT;

	Length = SendMessage(hWndEdit, EM_GETTEXTLENGTHEX, (WPARAM)&Gtl, 0);
	SendMessage(hWndEdit, EM_SETSEL, Length, -1);
	SendMessage(hWndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&Object->CharFormat);
	SendMessage(hWndEdit, EM_REPLACESEL, FALSE, (LPARAM)Buffer);
}

VOID
ConsoleSetCaret(
	IN PCONSOLE_OBJECT Object
	)
{
	SendMessage(Object->hWndEdit, EM_SETSEL, -1, -1);
	SetFocus(Object->hWndEdit);
}

VOID
ConsoleCheckLength(
	IN PCONSOLE_OBJECT Object
	)
{
	ULONG Length;
	ULONG Length2;
	GETTEXTLENGTHEX Gtl = {0};
	WCHAR Buffer[MAX_PATH];
	GETTEXTEX Gtx = {0};

	Gtl.codepage = 1200;
	Gtl.flags = GTL_DEFAULT;

	Length = SendMessage(Object->hWndEdit, EM_GETTEXTLENGTHEX, (WPARAM)&Gtl, 0);
	Length2 = GetWindowTextLength(Object->hWndEdit);

	Gtx.cb = sizeof(Buffer);
	Gtx.codepage = 1200;
	Gtx.flags = GT_USECRLF;

	SendMessage(Object->hWndEdit, EM_GETTEXTEX, (WPARAM)&Gtx, (LPARAM)Buffer);
	DebugTraceW(L"%ws", Buffer);
}

ULONG CALLBACK
ConsoleStreamCallback(
	IN DWORD_PTR Cookie,
    IN PVOID Buffer,
    IN LONG Length,
    OUT PLONG ActualLength
	)
{
	ULONG Status;
	PCONSOLE_OBJECT Object;
	LARGE_INTEGER Size = {0};

	Object = (PCONSOLE_OBJECT)Cookie;
	ASSERT(Object != NULL);

	if (!Object->FileHandle) {

		HANDLE FileHandle;
		FileHandle = CreateFile(Object->FilePath, GENERIC_READ | GENERIC_WRITE,
								0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,NULL);
		if (FileHandle == INVALID_HANDLE_VALUE) {
			Object->FileHandle = NULL;
			Status = GetLastError();
			return Status;
		}

		Object->FileHandle = FileHandle;
	}

	//
	// N.B. In case the file length exceed configured limits, we will purge all its content
	// and write from the begin of the log file.
	//

	GetFileSizeEx(Object->FileHandle, &Size);
	if ((ULONG)Size.QuadPart > Object->FileSizeQuota) {
		SetFilePointer(Object->FileHandle, 0, 0, FILE_BEGIN);
	}

	Status = WriteFile(Object->FileHandle, Buffer, (DWORD)Length, (DWORD *)ActualLength, NULL);
	if (Status != TRUE) {
		Status = GetLastError();
		return Status;
	}

	return 0;
}

ULONG
ConsoleWriteFile(
	IN PCONSOLE_OBJECT Object
	)
{
	ULONG Status;
	EDITSTREAM Es = {0};

	Es.dwCookie = (DWORD_PTR)Object;
	Es.pfnCallback = ConsoleStreamCallback;

	Status = (ULONG)SendMessage(Object->hWndEdit, EM_STREAMOUT, SF_UNICODE, (LPARAM)&Es);

	if (Object->FileHandle) {
		CloseHandle(Object->FileHandle);
		Object->FileHandle = NULL;
	}

	return Status;
}