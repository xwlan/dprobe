//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
// 

#include "sdk.h"
#include "resource.h"
#include "main.h"
#include "frame.h"
#include "euladlg.h"

HWND hWndMain;

BOOLEAN
MainInitialize(
	VOID
	)
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	return TRUE;
}

int
MainMessagePump(
	VOID
	)
{
	HACCEL hAccel;
	MSG msg;
	WCHAR Buffer[MAX_PATH];

	FRAME_OBJECT Frame = {
		NULL, NULL, NULL, NULL, NULL, NULL
	};

	//
	// Create main window
	//

	hWndMain = FrameCreate(&Frame);
	ASSERT(hWndMain != NULL);

	LoadString(SdkInstance, IDS_APP_TITLE, Buffer, MAX_PATH);
	if (BspIs64Bits) {
		wcscat_s(Buffer, MAX_PATH, L" x64");
	}

	SetWindowText(hWndMain, Buffer);

	hAccel = LoadAccelerators(SdkInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	ASSERT(hAccel != NULL);

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWndMain, hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}