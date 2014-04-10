//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
// 

#include "rundlg.h"
#include "dialog.h"

typedef struct _RUN_CONTEXT {
	WCHAR ImagePath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];
	WCHAR DpcPath[MAX_PATH];
} RUN_CONTEXT, *PRUN_CONTEXT;

INT_PTR
RunDialog(
	IN HWND hWndParent,
	IN PWCHAR ImagePath,
	IN PWCHAR Argument,
	IN PWCHAR WorkPath,
	IN PWCHAR DpcPath
	)
{
	DIALOG_OBJECT Object = {0};
	RUN_CONTEXT Context = {0};
	INT_PTR Return;

	Object.Context = &Context;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_RUN;
	Object.Procedure = RunProcedure;
	
	Return = DialogCreate(&Object);

	StringCchCopy(ImagePath, MAX_PATH, Context.ImagePath);
	StringCchCopy(Argument, MAX_PATH, Context.Argument);
	StringCchCopy(WorkPath, MAX_PATH, Context.WorkPath);
	StringCchCopy(DpcPath, MAX_PATH, Context.DpcPath);

	return Return;
}

LRESULT
RunOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PRUN_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PRUN_CONTEXT)Object->Context;

	SdkSetMainIcon(hWnd);
	SdkCenterWindow(hWnd);
	return TRUE;
}

INT_PTR CALLBACK
RunProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	LRESULT Status = 0;

	switch (uMsg) {

		case WM_INITDIALOG:
			RunOnInitDialog(hWnd, uMsg, wp, lp);			
			return 1;

		case WM_COMMAND:
			RunOnCommand(hWnd, uMsg, wp, lp);
			break;
	}
	
	return Status;
}

LRESULT
RunOnCommand(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	switch (LOWORD(wp)) {
	
		case IDOK:
			RunOnOk(hWnd, uMsg, wp, lp);
			break;

		case IDCANCEL:
			RunOnCancel(hWnd, uMsg, wp, lp);
			break;

		case IDC_BUTTON_IMAGEPATH:
			RunOnImagePath(hWnd, uMsg, wp, lp);
			break;

		case IDC_BUTTON_DPCPATH:
			RunOnDpcPath(hWnd, uMsg, wp, lp);
			break;
	}

	return 0L;
}

LRESULT
RunOnOk(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PRUN_CONTEXT Context;
	WCHAR ImagePath[MAX_PATH];
	WCHAR DpcPath[MAX_PATH];
	WCHAR Argument[MAX_PATH];
	WCHAR WorkPath[MAX_PATH];
	HANDLE FileHandle;

	GetWindowText(GetDlgItem(hWnd, IDC_EDIT_IMAGEPATH), ImagePath, MAX_PATH);
	FileHandle = CreateFile(ImagePath,
	                        GENERIC_READ,
	                        FILE_SHARE_READ,
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		MessageBox(hWnd, L"Invalid image file path!", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	} else {
		CloseHandle(FileHandle);
	}

	GetWindowText(GetDlgItem(hWnd, IDC_EDIT_DPCPATH), DpcPath, MAX_PATH);
	FileHandle = CreateFile(DpcPath,
	                        GENERIC_READ,
	                        FILE_SHARE_READ,
	                        NULL,
	                        OPEN_EXISTING,
	                        FILE_ATTRIBUTE_NORMAL,
	                        NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		MessageBox(hWnd, L"Invalid dpc file path!", L"D Probe", MB_OK|MB_ICONWARNING);
		return 0;
	} else {
		CloseHandle(FileHandle);
	}

	GetWindowText(GetDlgItem(hWnd, IDC_EDIT_IMAGEARG), Argument, MAX_PATH);
	GetWindowText(GetDlgItem(hWnd, IDC_EDIT_WORKPATH), WorkPath, MAX_PATH);

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PRUN_CONTEXT)Object->Context;

	StringCchCopy(Context->ImagePath, MAX_PATH, ImagePath);
	StringCchCopy(Context->Argument, MAX_PATH, Argument);
	StringCchCopy(Context->DpcPath, MAX_PATH, DpcPath);
	StringCchCopy(Context->WorkPath, MAX_PATH, WorkPath);

	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
RunOnCancel(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	PDIALOG_OBJECT Object;
	PRUN_CONTEXT Context;

	Object = (PDIALOG_OBJECT)SdkGetObject(hWnd);
	Context = (PRUN_CONTEXT)Object->Context;

	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
RunOnImagePath(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	BOOL Status;
	WCHAR Path[MAX_PATH];	

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"Executable File (*.exe)\0*.exe\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (Status == FALSE) {
		return 0;
	}

	SetWindowText(GetDlgItem(hWnd, IDC_EDIT_IMAGEPATH), Ofn.lpstrFile);
	return 0;
}

LRESULT
RunOnDpcPath(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	OPENFILENAME Ofn;
	BOOL Status;
	WCHAR Path[MAX_PATH];	

	ZeroMemory(&Ofn, sizeof Ofn);
	ZeroMemory(Path, sizeof(Path));

	Ofn.lStructSize = sizeof(Ofn);
	Ofn.hwndOwner = hWnd;
	Ofn.hInstance = SdkInstance;
	Ofn.lpstrFilter = L"D Probe Configuration (*.dpc)\0*.dpc\0\0";
	Ofn.lpstrFile = Path;
	Ofn.nMaxFile = sizeof(Path); 
	Ofn.lpstrTitle = L"D Probe";
	Ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;

	Status = GetOpenFileName(&Ofn);
	if (Status == FALSE) {
		return 0;
	}

	SetWindowText(GetDlgItem(hWnd, IDC_EDIT_DPCPATH), Ofn.lpstrFile);
	return 0;
}