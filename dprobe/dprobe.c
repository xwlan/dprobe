//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "dprobe.h"
#include "frame.h"
#include "main.h"
#include "mdb.h"
#include "driver.h"
#include "perf.h"
#include "mspdtl.h"
#include "mspprocess.h"

BOOLEAN
ParseCommandLine(
	OUT PWCHAR Buffer
	)
{	
	PWSTR Argument;
	PWSTR *ArgList;
	int Count;

	Argument = GetCommandLineW();
	ArgList = CommandLineToArgvW(Argument, &Count);
	return TRUE;
}

int WINAPI 
wWinMain(
	IN HINSTANCE Instance,
	IN HINSTANCE Previous,
	IN PWSTR CommandLine,
	IN int nCmdShow
	)
{
	ULONG Status;
	USHORT Length;
	WCHAR Buffer[MAX_PATH];
	WCHAR FilterPath[MAX_PATH];
	WCHAR CachePath[MAX_PATH];

	if (ParseCommandLine(Buffer) != TRUE) {
		return FALSE;
	}

	Status = SdkInitialize(Instance);
	if (Status != S_OK) {
		return 0;
	}

	Status = BspInitialize();
	if (Status != S_OK) {
		return 0;
	}
	
	//
	// First try to load mdb, if failed, rebuild
	// the mdb using default value
	//

	Status = MdbInitialize(TRUE);
	if (Status != S_OK) {

		Status = MdbInitialize(FALSE);
		if (Status != S_OK) {
			return Status;
		}

		BspWriteLogEntry(LOG_LEVEL_WARNING, LOG_SOURCE_MDB, 
			             L"Mdb may be corrupted, succeed to rebuild default values.");
	}

	//
	// Ensure the user accept EULA
	//

	Status = MdbEnsureEula();
	if (Status != S_OK) {
		return Status;
	}

	//
	// N.B. Kernel driver should be checked against MDB
	//

#ifdef _KBTR
	#pragma message("kernel driver is enabled.")
	Status = BspConnectDriver(BTR_DRIVER_NAME, BTR_DOSDEVICE_NAME, &BspDriverHandle);
#endif

	BspGetFullPathName(L"dprobe.btr.dll", Buffer, MAX_PATH, &Length);
	BspGetProcessPath(FilterPath, MAX_PATH, &Length);

	MdbGetCachePath(CachePath, MAX_PATH);
	Status = MspInitialize(MSP_MODE_DLL, MSP_FLAG_LOGGING, 
		                   L"dprobe.btr.dll", Buffer,
						   FeatureRemote, FilterPath, 
						   CachePath);

	if (Status != S_OK) {
		return 0;
	}

	Status = PerfInitialize(NULL);
	if (Status != S_OK) {
		return 0;
	}

	Status = MainInitialize();
	if (!Status) {
		return 0;
	}

	MainMessagePump();
	return 0;
}

