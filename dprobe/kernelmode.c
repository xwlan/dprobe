//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "kernelmode.h"
#include "dprobe.h"
#include "driver.h"

BOOLEAN
KernelDialogCreate(
	IN HWND hWndParent
	)
{
	DIALOG_OBJECT Object = {0};

	Object.Context = NULL;
	Object.hWndParent = hWndParent;
	Object.ResourceId = IDD_DIALOG_KERNELMODE;
	Object.Procedure = KernelProcedure;
	
	DialogCreate(&Object);
	return TRUE;
}

INT_PTR CALLBACK
KernelProcedure(
	IN HWND hWnd, 
	IN UINT uMsg, 
	IN WPARAM wp, 
	IN LPARAM lp
	)
{
	INT_PTR Status;
	Status = FALSE;

	switch (uMsg) {
		
		case WM_COMMAND:
			KernelOnCommand(hWnd, uMsg, wp, lp);
			Status = TRUE;
			break;
		
		case WM_INITDIALOG:
			KernelOnInitDialog(hWnd, uMsg, wp, lp);
			SdkCenterWindow(hWnd);
			Status = TRUE;
			break;
	}

	return Status;
}

LRESULT
KernelOnCommand(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	//
	// Source       HIWORD(wParam)  LOWORD(wParam)  lParam 
	// Menu         0               MenuId          0   
	// Accelerator  1               AcceleratorId	0
	// Control      NotifyCode      ControlId       hWndCtrl
	//

	switch (LOWORD(wp)) {

	case IDOK:
		KernelOnOk(hWnd, uMsg, wp, lp);
	    break;

	case IDCANCEL:
		KernelOnCancel(hWnd, uMsg, wp, lp);
	    break;

    case IDC_BUTTON_IRQL_WAKEUP:
        KernelOnIrqlWakeUp(hWnd, uMsg, wp, lp);
        break;

	}

	return 0;
}

LRESULT
KernelOnInitDialog(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	SetWindowText(hWnd, L"Kernel Mode");
	SdkSetMainIcon(hWnd);
	return 0;
}

LRESULT
KernelOnOk(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	EndDialog(hWnd, IDOK);
	return 0;
}

LRESULT
KernelOnCancel(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	EndDialog(hWnd, IDCANCEL);
	return 0;
}

LRESULT
KernelOnIrqlWakeUp(
	IN HWND hWnd,
	IN UINT uMsg,
	IN WPARAM wp,
	IN LPARAM lp
	)
{
	BOOL Status;
	ULONG BytesReturned;
	WCHAR Buffer[MAX_PATH];

	ASSERT(BspDriverHandle != NULL);

	Status = DeviceIoControl(BspDriverHandle, 
		                     IOCTL_KBTR_METHOD_WAKEUP_THREAD, 
		                     0, 0, 0, 0, &BytesReturned, NULL);
	if (!Status) {
		StringCchPrintf(Buffer, MAX_PATH, L"Error: code=%d", GetLastError());
		MessageBox(hWnd, Buffer, L"", MB_OK|MB_ICONERROR);
	}

	return 0;
}