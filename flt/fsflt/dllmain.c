//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "fsflt.h"
#include "resource.h"

BOOLEAN FsInitializedOk = FALSE;
HMODULE FsModuleHandle = NULL;

//
// File system filter guid
//

GUID FsGuid = { 0xe3725047, 0xd62b, 0x41c7, { 0x8d, 0xa9, 0x63, 0xa8, 0x9, 0xc8, 0x33, 0x80 } };

//
// File system probe defs
//

BTR_FILTER_PROBE FsProbes[FS_PROBE_NUMBER] = {
	{ _CreateFileW, 0, ReturnPtr,  NULL, CreateFileEnter, CreateFileDecode, L"kernel32.dll", L"CreateFileW" },
	{ _ReadFile, 0, ReturnInt32,   NULL, ReadFileEnter, ReadFileDecode, L"kernel32.dll", L"ReadFile" },
	{ _WriteFile,0, ReturnInt32,   NULL, WriteFileEnter, WriteFileDecode, L"kernel32.dll", L"WriteFile" },
	{ _DeviceIoControl, 0, ReturnInt32, NULL,DeviceIoControlEnter, DeviceIoControlDecode, L"kernel32.dll", L"DeviceIoControl"},
	{ _DeleteFile, 0, ReturnInt32, NULL, DeleteFileEnter, DeleteFileDecode, L"kernel32.dll", L"DeleteFileW" },
	{ _CopyFile, 0, ReturnInt32, NULL, CopyFileEnter, CopyFileDecode, L"kernel32.dll", L"CopyFileW" },
	{ _MoveFile, 0, ReturnInt32, NULL, MoveFileEnter, MoveFileDecode, L"kernel32.dll", L"MoveFileW" },
	{ _LockFile, 0, ReturnInt32, NULL, LockFileEnter, LockFileDecode, L"kernel32.dll", L"LockFile" },
	{ _UnlockFile, 0, ReturnInt32, NULL, UnlockFileEnter, UnlockFileDecode, L"kernel32.dll", L"UnlockFile" },
	{ _DefineDosDevice, 0, ReturnInt32, NULL, DefineDosDeviceEnter, DefineDosDeviceDecode, L"kernel32.dll", L"DefineDosDeviceW" },
	{ _QueryDosDevice, 0, ReturnUInt32, NULL, QueryDosDeviceEnter, QueryDosDeviceDecode, L"kernel32.dll", L"QueryDosDeviceW" },
	{ _FindFirstFile, 0, ReturnPtr, NULL, FindFirstFileEnter, FindFirstFileDecode, L"kernel32.dll", L"FindFirstFileW" },
	{ _FindNextFile, 0, ReturnInt32, NULL, FindNextFileEnter, FindNextFileDecode, L"kernel32.dll", L"FindNextFileW" },
	{ _FindClose, 0, ReturnInt32, NULL, FindCloseEnter, FindCloseDecode, L"kernel32.dll", L"FindClose" },
	{ _GetFileSizeEx, 0, ReturnInt32, NULL, GetFileSizeExEnter, GetFileSizeExDecode, L"kernel32.dll", L"GetFileSizeEx" },
	{ _SetFilePointer, 0, ReturnUInt32, NULL, SetFilePointerEnter, SetFilePointerDecode, L"kernel32.dll", L"SetFilePointer" },
	{ _SetEndOfFile, 0, ReturnInt32, NULL, SetEndOfFileEnter, SetEndOfFileDecode, L"kernel32.dll", L"SetEndOfFile" },
	{ _SetFileValidData, 0, ReturnInt32, NULL, SetFileValidDataEnter, SetFileValidDataDecode, L"kernel32.dll", L"SetFileValidData" },
	{ _SetFileAttributes, 0, ReturnInt32, NULL, SetFileAttributesEnter, SetFileAttributesDecode, L"kernel32.dll", L"SetFileAttributesW" },
	{ _GetFileAttributes, 0, ReturnUInt32, NULL, GetFileAttributesEnter, GetFileAttributesDecode, L"kernel32.dll", L"GetFileAttributesW" },
	{ _SetFileInformationByHandle, 0, ReturnInt32, NULL, SetFileInformationByHandleEnter, SetFileInformationByHandleDecode, L"kernel32.dll", L"SetFileInformationByHandle" },
	{ _SearchPath, 0, ReturnUInt32, NULL, SearchPathEnter, SearchPathDecode, L"kernel32.dll", L"SearchPathW" },
	{ _ReplaceFile,0, ReturnInt32, NULL, ReplaceFileEnter, ReplaceFileDecode, L"kernel32.dll", L"ReplaceFileW" },
	{ _GetTempFileName,0, ReturnUInt32, NULL, GetTempFileNameEnter, GetTempFileNameDecode, L"kernel32.dll", L"GetTempFileNameW" },
	{ _GetFileInformationByHandle, 0, ReturnInt32, NULL, GetFileInformationByHandleEnter, GetFileInformationByHandleDecode, L"kernel32.dll", L"GetFileInformationByHandle" },
	{ _CreateHardLink, 0, ReturnInt32, NULL, CreateHardLinkEnter, CreateHardLinkDecode, L"kernel32.dll", L"CreateHardLinkW" },
	{ _GetTempPath, 0, ReturnUInt32, NULL, GetTempPathEnter, GetTempPathDecode, L"kernel32.dll", L"GetTempPathW" },
	{ _CreateDirectory, 0, ReturnInt32, NULL, CreateDirectoryEnter, CreateDirectoryDecode, L"kernel32.dll", L"CreateDirectoryW" },
	{ _CreateDirectoryEx, 0, ReturnInt32, NULL, CreateDirectoryExEnter, CreateDirectoryExDecode, L"kernel32.dll", L"CreateDirectoryExW" },
	{ _RemoveDirectory, 0, ReturnInt32, NULL, RemoveDirectoryEnter, RemoveDirectoryDecode, L"kernel32.dll", L"RemoveDirectoryW" },
	{ _FindFirstChangeNotification, 0, ReturnPtr, NULL, FindFirstChangeNotificationEnter, FindFirstChangeNotificationDecode, L"kernel32.dll", L"FindFirstChangeNotificationW" },
	{ _FindNextChangeNotification, 0, ReturnInt32, NULL, FindNextChangeNotificationEnter, FindNextChangeNotificationDecode, L"kernel32.dll", L"FindNextChangeNotification" },
	{ _GetCurrentDirectory, 0, ReturnUInt32, NULL, GetCurrentDirectoryEnter, GetCurrentDirectoryDecode, L"kernel32.dll", L"GetCurrentDirectoryW" },
	{ _SetCurrentDirectory, 0, ReturnInt32, NULL, SetCurrentDirectoryEnter, SetCurrentDirectoryDecode, L"kernel32.dll", L"SetCurrentDirectoryW" },
	{ _ReadDirectoryChangesW, 0, ReturnInt32, NULL, ReadDirectoryChangesWEnter, ReadDirectoryChangesWDecode, L"kernel32.dll", L"ReadDirectoryChangesW" },
	{ _GetDiskFreeSpace, 0, ReturnInt32, NULL, GetDiskFreeSpaceEnter, GetDiskFreeSpaceDecode, L"kernel32.dll", L"GetDiskFreeSpaceW" },
	{ _FindFirstVolume, 0, ReturnPtr, NULL, FindFirstVolumeEnter, FindFirstVolumeDecode, L"kernel32.dll", L"FindFirstVolumeW" },
	{ _FindNextVolume, 0, ReturnInt32, NULL, FindNextVolumeEnter, FindNextVolumeDecode, L"kernel32.dll", L"FindNextVolumeW" },
	{ _FindVolumeClose, 0, ReturnInt32, NULL, FindVolumeCloseEnter, FindVolumeCloseDecode, L"kernel32.dll", L"FindVolumeClose" },
	{ _GetVolumeInformation, 0, ReturnInt32, NULL, GetVolumeInformationEnter, GetVolumeInformationDecode, L"kernel32.dll", L"GetVolumeInformationW" },
	{ _GetDriveType, 0, ReturnUInt32, NULL, GetDriveTypeEnter, GetDriveTypeDecode, L"kernel32.dll", L"GetDriveTypeW" },
	{ _GetLogicalDrives, 0, ReturnUInt32, NULL, GetLogicalDrivesEnter, GetLogicalDrivesDecode, L"kernel32.dll", L"GetLogicalDrives" },
	{ _GetLogicalDriveStrings, 0, ReturnUInt32, NULL, GetLogicalDriveStringsEnter, GetLogicalDriveStringsDecode, L"kernel32.dll", L"GetLogicalDriveStringsW" },
};

BTR_FILTER FsRegistration;

BOOLEAN
FsInitialize(
	IN BTR_FILTER_MODE Mode 
	);

PBTR_FILTER WINAPI
BtrFltGetObject(
	IN BTR_FILTER_MODE Mode	
	)
{
	if (FsInitializedOk) {
		return &FsRegistration;
	}

	FsInitializedOk = FsInitialize(Mode);

	if (FsInitializedOk) {
		return &FsRegistration;
	}

	return NULL;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
FsUnregisterCallback(
	VOID
	)
{
	//
	// N.B. When filter is unregistered, this routine is called
	// by runtime to give filter a chance to release allocated
	// resources. Typically filter should provide this routine.
	//

	DebugTrace("FltUnregisterCallback");
	return 0;
}

//
// This routine is called in context of runtime worker thread
// however it's not guaranteed to be in same thread
//

ULONG WINAPI
FsControlCallback(
	IN PBTR_FILTER_CONTROL Control,
	OUT PBTR_FILTER_CONTROL *Ack
	)
{
	PBTR_FILTER_CONTROL Packet;

	//
	// Control callback is called when user send a message
	// to current filter via MspSendFilterControl(), filter
	// should send a ack to this control back to user.
	//

	//
	// Allocate and build a dummy ack packet back to user, the packet
	// don't belong to filter anymore once callback return, never touch
	// it, runtime will free the buffer later.
	//

	Packet = (PBTR_FILTER_CONTROL)BtrFltMalloc(sizeof(BTR_FILTER_CONTROL));
	Packet->Length = sizeof(BTR_FILTER_CONTROL);
	Packet->Control[0] = 0;

	*Ack = Packet;

	DebugTrace("FsControlCallback");
	return 0;
}

BOOLEAN
FsInitialize(
	IN BTR_FILTER_MODE Mode 
	)
{
	PVOID Address;
	HMODULE DllHandle;
	PCHAR Buffer;
	int i;

	StringCchCopy(FsRegistration.FilterName,  MAX_PATH - 1, L"File System Filter");
	StringCchCopy(FsRegistration.Author, MAX_PATH - 1, L"lan.john@gmail.com");
	StringCchCopy(FsRegistration.Description, MAX_PATH - 1, L"Win32 File System API Filter");

	FsRegistration.FilterTag = 0;
	FsRegistration.MajorVersion = 1;
	FsRegistration.MinorVersion = 0;

	GetModuleFileName(FsModuleHandle, FsRegistration.FilterFullPath, MAX_PATH);

	FsRegistration.Probes = FsProbes;
	FsRegistration.ProbesCount = FS_PROBE_NUMBER;
	
	FsRegistration.ControlCallback = FsControlCallback;
	FsRegistration.UnregisterCallback = FsUnregisterCallback;
	FsRegistration.FilterGuid = FsGuid;

	if (Mode == FILTER_MODE_DECODE) {
		return TRUE;
	}

	// 
	// FILTER_MODE_CAPTURE requires to fill API addresses.
	//

	for(i = 0; i < FS_PROBE_NUMBER; i++) {

		DllHandle = LoadLibrary(FsProbes[i].DllName);
		if (DllHandle) {

			BtrFltConvertUnicodeToAnsi(FsProbes[i].ApiName, &Buffer);
			Address = (PVOID)GetProcAddress(DllHandle, Buffer);
			BtrFltFree(Buffer);

			if (Address) {
				FsProbes[i].Address = Address;
			} else {
				continue;
			} 

		} else {
			return FALSE;
		}
	}

	return TRUE;
}

BOOL WINAPI 
DllMain(
	IN HMODULE ModuleHandle,
    IN ULONG Reason,
	IN PVOID Reserved
	)
{
	switch (Reason) {

	case DLL_PROCESS_ATTACH:
		if (!FsModuleHandle) {
			FsModuleHandle = ModuleHandle;
		}
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	
	return TRUE;
}
