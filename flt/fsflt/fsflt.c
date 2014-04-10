//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "fsflt.h"
#include "ioctlcode.h"
#include <ntddscsi.h>
#include <ntddndis.h>

VOID 
DebugTrace(
	IN PSTR Format,
	...
	)
{

#ifdef _DEBUG
	
	va_list arg;
	size_t length;
	char buffer[512];

	va_start(arg, Format);

	StringCchCopyA(buffer, 512, "[fsflt] : ");
	StringCchVPrintfA(buffer + 10, 400, Format, arg);
	StringCchLengthA(buffer, 512, &length);
	
	if (length < 512) {
		StringCchCatA(buffer, 512, "\n");
	}

	va_end(arg);
	OutputDebugStringA(buffer);	

#endif

}

HANDLE WINAPI
CreateFileEnter(
	IN PWSTR lpFileName,
	IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_CREATEFILE Filter;
	CREATEFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	HANDLE FileHandle;

	BtrFltGetContext(&Context);
	Routine = (CREATEFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	FileHandle = (*Routine)(lpFileName, dwDesiredAccess,
							dwShareMode, lpSecurityAttributes,
							dwCreationDisposition, 
							dwFlagsAndAttributes,
							hTemplateFile);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_CREATEFILE), 
		                          FsGuid, _CreateFileW);

	Filter = FILTER_RECORD_POINTER(Record, FS_CREATEFILE);

	if (lpFileName) {
		StringCchCopy(Filter->Name, MAX_PATH, lpFileName);
	}

	Filter->DesiredAccess = dwDesiredAccess;
	Filter->ShareMode = dwShareMode;
	Filter->Security = lpSecurityAttributes;
	Filter->CreationDisposition = dwCreationDisposition;
	Filter->FlagsAndAttributes = dwFlagsAndAttributes;
	Filter->TemplateFile = hTemplateFile;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return FileHandle;
}

ULONG 
CreateFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_CREATEFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _CreateFileW);

	Data = (PFS_CREATEFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
					L"Name=%ws,Access=0x%08x,ShareMode=0x%08x,Handle=0x%p,Status=0x%08x", 
					Data->Name, Data->DesiredAccess, Data->ShareMode, Data->FileHandle, 
					Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI
ReadFileEnter(
	IN HANDLE FileHandle,
	IN PVOID Buffer,
	IN ULONG ReadBytes,
	OUT PULONG CompleteBytes,
	IN OVERLAPPED *Overlapped
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_READFILE Filter;
	READFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (READFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);
	
	Status = (*Routine)(FileHandle, Buffer, ReadBytes,
		                CompleteBytes, Overlapped);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_READFILE), 
		                          FsGuid, _ReadFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_READFILE);
	Filter->FileHandle = FileHandle;
	Filter->Buffer = Buffer;
	Filter->ReadBytes = ReadBytes;
	Filter->CompleteBytesPtr = CompleteBytes;
	Filter->LastErrorStatus = GetLastError();
	Filter->Status = Status;
	
	if (Overlapped != NULL) {
		memcpy(&Filter->Overlapped, Overlapped, sizeof(OVERLAPPED));	
	} 
	
	//
	// N.B. Only care about whether the CompleteBytes is valid
	//

	if (Filter->CompleteBytesPtr) {
		Filter->CompleteBytes = *CompleteBytes;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
ReadFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_READFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _ReadFile);

	Data = (PFS_READFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
			L"FileHandle=0x%p,Buffer=0x%p,ReadBytes=0x%x,CompleteBytes=0x%x,Overlapped=0x%p,Return=%d,Status=0x%08x", 
			Data->FileHandle, Data->Buffer, Data->ReadBytes, Data->CompleteBytes, 
			Data->Overlapped, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
WriteFileEnter(
	IN  HANDLE hFile,
	IN  LPVOID lpBuffer,
	IN  DWORD nNumberOfBytesToWrite,
	OUT LPDWORD lpNumberOfBytesWritten,
	IN  LPOVERLAPPED lpOverlapped
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_WRITEFILE Filter;
	WRITEFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (WRITEFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, lpBuffer, nNumberOfBytesToWrite,
		                lpNumberOfBytesWritten, lpOverlapped);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_WRITEFILE), 
		                          FsGuid, _WriteFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_WRITEFILE);
	Filter->Status = Status;
	Filter->FileHandle = hFile;
	Filter->Buffer = lpBuffer;
	Filter->WriteBytes = nNumberOfBytesToWrite;
	Filter->CompleteBytesPtr = lpNumberOfBytesWritten;
	
	if (lpOverlapped != NULL) {
		memcpy(&Filter->Overlapped, lpOverlapped, sizeof(OVERLAPPED));	
	} 
	
	Filter->LastErrorStatus = GetLastError();
	if (lpNumberOfBytesWritten) {
		Filter->CompleteBytes = *lpNumberOfBytesWritten;
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
WriteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_WRITEFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _WriteFile);

	Data = (PFS_WRITEFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"FileHandle=0x%p,Buffer=0x%p,WriteBytes=0x%x,CompleteBytes=0x%x,Overlapped=0x%p,Return=%d,Status=0x%08x", 
		Data->FileHandle, Data->Buffer, Data->WriteBytes, Data->CompleteBytes, 
		Data->Overlapped, Data->Status, Data->LastErrorStatus);

	return S_OK;
}


BOOLEAN
FsDecodeIoControl(
	IN ULONG IoControlCode,
	IN PWCHAR Code,
	IN ULONG Size
	)
{
	switch (IoControlCode) {

		//
		// SCSI
		//

	case IOCTL_SCSI_PASS_THROUGH:
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_PASS_THROUGH");
		break;
	case IOCTL_SCSI_MINIPORT:
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT");
		break;
	case IOCTL_SCSI_GET_INQUIRY_DATA:
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_GET_INQUIRY_DATA");
		break;
	case IOCTL_SCSI_GET_CAPABILITIES :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_GET_CAPABILITIES");
		break;
	case IOCTL_SCSI_PASS_THROUGH_DIRECT :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_PASS_THROUGH_DIRECT");
		break;
	case IOCTL_SCSI_GET_ADDRESS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_GET_ADDRESS");
		break;
	case IOCTL_SCSI_RESCAN_BUS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_RESCAN_BUS");
		break;
	case IOCTL_SCSI_GET_DUMP_POINTERS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_GET_DUMP_POINTERS");
		break;
	case IOCTL_SCSI_FREE_DUMP_POINTERS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_FREE_DUMP_POINTERS");
		break;
	case IOCTL_IDE_PASS_THROUGH :
		StringCchCopyW(Code, Size, L"IOCTL_IDE_PASS_THROUGH");
		break;
	case IOCTL_ATA_PASS_THROUGH :
		StringCchCopyW(Code, Size, L"IOCTL_ATA_PASS_THROUGH");
		break;
	case IOCTL_ATA_PASS_THROUGH_DIRECT :
		StringCchCopyW(Code, Size, L"IOCTL_ATA_PASS_THROUGH_DIRECT");
		break;
	case IOCTL_ATA_MINIPORT :
		StringCchCopyW(Code, Size, L"IOCTL_ATA_MINIPORT");
		break;

		//
		// NDIS
		//

	case IOCTL_NDIS_QUERY_GLOBAL_STATS :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_QUERY_GLOBAL_STATS");
		break;
	case IOCTL_NDIS_QUERY_ALL_STATS :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_QUERY_ALL_STATS");
		break;
	case IOCTL_NDIS_DO_PNP_OPERATION :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_DO_PNP_OPERATION");
		break;
	case IOCTL_NDIS_QUERY_SELECTED_STATS :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_QUERY_SELECTED_STATS");
		break;
	case IOCTL_NDIS_ENUMERATE_INTERFACES :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_ENUMERATE_INTERFACES");
		break;
	case IOCTL_NDIS_ADD_TDI_DEVICE :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_ADD_TDI_DEVICE");
		break;
	case IOCTL_NDIS_GET_LOG_DATA :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_GET_LOG_DATA");
		break;
	case IOCTL_NDIS_GET_VERSION :
		StringCchCopyW(Code, Size, L"IOCTL_NDIS_GET_VERSION");
		break;

		//
		// STORAGE
		//

	case IOCTL_STORAGE_CHECK_VERIFY :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_CHECK_VERIFY");
		break;
	case IOCTL_STORAGE_CHECK_VERIFY2 :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_CHECK_VERIFY2");
		break;
	case IOCTL_STORAGE_MEDIA_REMOVAL :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_MEDIA_REMOVAL");
		break;
	case IOCTL_STORAGE_EJECT_MEDIA :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_EJECT_MEDIA");
		break;
	case IOCTL_STORAGE_LOAD_MEDIA :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_LOAD_MEDIA");
		break;
	case IOCTL_STORAGE_LOAD_MEDIA2 :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_LOAD_MEDIA2");
		break;
	case IOCTL_STORAGE_RESERVE :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_RESERVE");
		break;
	case IOCTL_STORAGE_RELEASE :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_RELEASE");
		break;
	case IOCTL_STORAGE_FIND_NEW_DEVICES :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_FIND_NEW_DEVICES");
		break;
	case IOCTL_STORAGE_EJECTION_CONTROL :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_EJECTION_CONTROL");
		break;
	case IOCTL_STORAGE_MCN_CONTROL :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_MCN_CONTROL");
		break;
	case IOCTL_STORAGE_GET_MEDIA_TYPES :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_GET_MEDIA_TYPES");
		break;
	case IOCTL_STORAGE_GET_MEDIA_TYPES_EX :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_GET_MEDIA_TYPES_EX");
		break;
	case IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER");
		break;
	case IOCTL_STORAGE_GET_HOTPLUG_INFO :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_GET_HOTPLUG_INFO");
		break;
	case IOCTL_STORAGE_SET_HOTPLUG_INFO :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_SET_HOTPLUG_INFO");
		break;
	case IOCTL_STORAGE_RESET_DEVICE :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_RESET_DEVICE");
		break;
	case IOCTL_STORAGE_BREAK_RESERVATION :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_BREAK_RESERVATION");
		break;
	case IOCTL_STORAGE_PERSISTENT_RESERVE_IN :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_PERSISTENT_RESERVE_IN");
		break;
	case IOCTL_STORAGE_PERSISTENT_RESERVE_OUT :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_PERSISTENT_RESERVE_OUT");
		break;
	case IOCTL_STORAGE_GET_DEVICE_NUMBER :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_GET_DEVICE_NUMBER");
		break;
	case IOCTL_STORAGE_PREDICT_FAILURE :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_PREDICT_FAILURE");
		break;
	case IOCTL_STORAGE_READ_CAPACITY :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_READ_CAPACITY");
		break;
	case IOCTL_STORAGE_QUERY_PROPERTY :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_QUERY_PROPERTY");
		break;
	case IOCTL_STORAGE_GET_BC_PROPERTIES :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_GET_BC_PROPERTIES");
		break;
	case IOCTL_STORAGE_ALLOCATE_BC_STREAM :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_ALLOCATE_BC_STREAM");
		break;
	case IOCTL_STORAGE_FREE_BC_STREAM :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_FREE_BC_STREAM");
		break;
	case IOCTL_STORAGE_CHECK_PRIORITY_HINT_SUPPORT :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_CHECK_PRIORITY_HINT_SUPPORT");
		break;
	case IOCTL_STORAGE_RESET_BUS :
		StringCchCopyW(Code, Size, L"IOCTL_STORAGE_RESET_BUS");
		break;

		//
		// DISK
		//

	case IOCTL_DISK_GET_DRIVE_GEOMETRY :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_DRIVE_GEOMETRY");
		break;
	case IOCTL_DISK_GET_PARTITION_INFO :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_PARTITION_INFO");
		break;
	case IOCTL_DISK_SET_PARTITION_INFO :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_SET_PARTITION_INFO");
		break;
	case IOCTL_DISK_GET_DRIVE_LAYOUT :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_DRIVE_LAYOUT");
		break;
	case IOCTL_DISK_SET_DRIVE_LAYOUT :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_SET_DRIVE_LAYOUT");
		break;
	case IOCTL_DISK_VERIFY :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_VERIFY");
		break;
	case IOCTL_DISK_FORMAT_TRACKS :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_FORMAT_TRACKS");
		break;
	case IOCTL_DISK_REASSIGN_BLOCKS :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_REASSIGN_BLOCKS");
		break;
	case IOCTL_DISK_PERFORMANCE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_PERFORMANCE");
		break;
	case IOCTL_DISK_IS_WRITABLE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_IS_WRITABLE");
		break;
	case IOCTL_DISK_LOGGING :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_LOGGING");
		break;
	case IOCTL_DISK_FORMAT_TRACKS_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_FORMAT_TRACKS_EX");
		break;
	case IOCTL_DISK_HISTOGRAM_STRUCTURE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_HISTOGRAM_STRUCTURE");
		break;
	case IOCTL_DISK_HISTOGRAM_DATA :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_HISTOGRAM_DATA");
		break;
	case IOCTL_DISK_HISTOGRAM_RESET :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_HISTOGRAM_RESET");
		break;
	case IOCTL_DISK_REQUEST_STRUCTURE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_REQUEST_STRUCTURE");
		break;
	case IOCTL_DISK_REQUEST_DATA :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_REQUEST_DATA");
		break;
	case IOCTL_DISK_PERFORMANCE_OFF :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_PERFORMANCE_OFF");
		break;
	case IOCTL_DISK_CONTROLLER_NUMBER :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_CONTROLLER_NUMBER");
		break;
	case IOCTL_DISK_GET_PARTITION_INFO_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_PARTITION_INFO_EX");
		break;
	case IOCTL_DISK_SET_PARTITION_INFO_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_SET_PARTITION_INFO_EX");
		break;
	case IOCTL_DISK_GET_DRIVE_LAYOUT_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_DRIVE_LAYOUT_EX");
		break;
	case IOCTL_DISK_SET_DRIVE_LAYOUT_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX");
		break;
	case IOCTL_DISK_CREATE_DISK :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_CREATE_DISK");
		break;
	case IOCTL_DISK_GET_LENGTH_INFO :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_LENGTH_INFO");
		break;
	case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_DRIVE_GEOMETRY_EX");
		break;
	case IOCTL_DISK_REASSIGN_BLOCKS_EX :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_REASSIGN_BLOCKS_EX");
		break;
	case IOCTL_DISK_UPDATE_DRIVE_SIZE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_UPDATE_DRIVE_SIZE");
		break;
	case IOCTL_DISK_GROW_PARTITION :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GROW_PARTITION");
		break;
	case IOCTL_DISK_GET_CACHE_INFORMATION :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_CACHE_INFORMATION");
		break;
	case IOCTL_DISK_SET_CACHE_INFORMATION :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_SET_CACHE_INFORMATION");
		break;
	case OBSOLETE_DISK_GET_WRITE_CACHE_STATE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_WRITE_CACHE_STATE");
		break;
	case IOCTL_DISK_DELETE_DRIVE_LAYOUT :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_DELETE_DRIVE_LAYOUT");
		break;
	case IOCTL_DISK_UPDATE_PROPERTIES :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_UPDATE_PROPERTIES");
		break;
	case IOCTL_DISK_FORMAT_DRIVE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_FORMAT_DRIVE");
		break;
	case IOCTL_DISK_SENSE_DEVICE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_SENSE_DEVICE");
		break;
	case IOCTL_DISK_CHECK_VERIFY :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_CHECK_VERIFY");
		break;
	case IOCTL_DISK_MEDIA_REMOVAL :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_MEDIA_REMOVAL");
		break;
	case IOCTL_DISK_EJECT_MEDIA :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_EJECT_MEDIA");
		break;
	case IOCTL_DISK_LOAD_MEDIA :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_LOAD_MEDIA");
		break;
	case IOCTL_DISK_RESERVE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_RESERVE");
		break;
	case IOCTL_DISK_RELEASE :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_RELEASE");
		break;
	case IOCTL_DISK_FIND_NEW_DEVICES :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_FIND_NEW_DEVICES");
		break;
	case IOCTL_DISK_GET_MEDIA_TYPES :
		StringCchCopyW(Code, Size, L"IOCTL_DISK_GET_MEDIA_TYPES");
		break;

		//
		// CHANGER
		//

	case IOCTL_CHANGER_GET_PARAMETERS :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_GET_PARAMETERS");
		break;
	case IOCTL_CHANGER_GET_STATUS :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_GET_STATUS");
		break;
	case IOCTL_CHANGER_GET_PRODUCT_DATA :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_GET_PRODUCT_DATA");
		break;
	case IOCTL_CHANGER_SET_ACCESS :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_SET_ACCESS");
		break;
	case IOCTL_CHANGER_GET_ELEMENT_STATUS :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_GET_ELEMENT_STATUS");
		break;
	case IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS");
		break;
	case IOCTL_CHANGER_SET_POSITION :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_SET_POSITION");
		break;
	case IOCTL_CHANGER_EXCHANGE_MEDIUM :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_EXCHANGE_MEDIUM");
		break;
	case IOCTL_CHANGER_MOVE_MEDIUM :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_MOVE_MEDIUM");
		break;
	case IOCTL_CHANGER_REINITIALIZE_TRANSPORT :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_REINITIALIZE_TRANSPORT");
		break;
	case IOCTL_CHANGER_QUERY_VOLUME_TAGS :
		StringCchCopyW(Code, Size, L"IOCTL_CHANGER_QUERY_VOLUME_TAGS");
		break;
	case IOCTL_SERIAL_LSRMST_INSERT :
		StringCchCopyW(Code, Size, L"IOCTL_SERIAL_LSRMST_INSERT");
		break;
	case IOCTL_SERENUM_EXPOSE_HARDWARE :
		StringCchCopyW(Code, Size, L"IOCTL_SERENUM_EXPOSE_HARDWARE");
		break;
	case IOCTL_SERENUM_REMOVE_HARDWARE :
		StringCchCopyW(Code, Size, L"IOCTL_SERENUM_REMOVE_HARDWARE");
		break;
	case IOCTL_SERENUM_PORT_DESC :
		StringCchCopyW(Code, Size, L"IOCTL_SERENUM_PORT_DESC");
		break;
	case IOCTL_SERENUM_GET_PORT_NAME :
		StringCchCopyW(Code, Size, L"IOCTL_SERENUM_GET_PORT_NAME");
		break;

		//
		// FS
		//

	case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
		StringCchCopyW(Code, Size, L"FSCTL_REQUEST_OPLOCK_LEVEL_1");
		break;
	case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
		StringCchCopyW(Code, Size, L"FSCTL_REQUEST_OPLOCK_LEVEL_2");
		break;
	case FSCTL_REQUEST_BATCH_OPLOCK :
		StringCchCopyW(Code, Size, L"FSCTL_REQUEST_BATCH_OPLOCK");
		break;
	case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :
		StringCchCopyW(Code, Size, L"FSCTL_OPLOCK_BREAK_ACKNOWLEDGE");
		break;
	case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
		StringCchCopyW(Code, Size, L"FSCTL_OPBATCH_ACK_CLOSE_PENDING");
		break;
	case FSCTL_OPLOCK_BREAK_NOTIFY :
		StringCchCopyW(Code, Size, L"FSCTL_OPLOCK_BREAK_NOTIFY");
		break;
	case FSCTL_LOCK_VOLUME :
		StringCchCopyW(Code, Size, L"FSCTL_LOCK_VOLUME");
		break;
	case FSCTL_UNLOCK_VOLUME :
		StringCchCopyW(Code, Size, L"FSCTL_UNLOCK_VOLUME");
		break;
	case FSCTL_DISMOUNT_VOLUME :
		StringCchCopyW(Code, Size, L"FSCTL_DISMOUNT_VOLUME");
		break;
	case FSCTL_IS_VOLUME_MOUNTED :
		StringCchCopyW(Code, Size, L"FSCTL_IS_VOLUME_MOUNTED");
		break;
	case FSCTL_IS_PATHNAME_VALID :
		StringCchCopyW(Code, Size, L"FSCTL_IS_PATHNAME_VALID");
		break;
	case FSCTL_MARK_VOLUME_DIRTY :
		StringCchCopyW(Code, Size, L"FSCTL_MARK_VOLUME_DIRTY");
		break;
	case FSCTL_QUERY_RETRIEVAL_POINTERS :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_RETRIEVAL_POINTERS");
		break;
	case FSCTL_GET_COMPRESSION :
		StringCchCopyW(Code, Size, L"FSCTL_GET_COMPRESSION");
		break;
	case FSCTL_SET_COMPRESSION :
		StringCchCopyW(Code, Size, L"FSCTL_SET_COMPRESSION");
		break;
	case FSCTL_MARK_AS_SYSTEM_HIVE :
		StringCchCopyW(Code, Size, L"FSCTL_MARK_AS_SYSTEM_HIVE");
		break;
	case FSCTL_OPLOCK_BREAK_ACK_NO_2 :
		StringCchCopyW(Code, Size, L"FSCTL_OPLOCK_BREAK_ACK_NO_2");
		break;
	case FSCTL_INVALIDATE_VOLUMES :
		StringCchCopyW(Code, Size, L"FSCTL_INVALIDATE_VOLUMES");
		break;
	case FSCTL_QUERY_FAT_BPB :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_FAT_BPB");
		break;
	case FSCTL_REQUEST_FILTER_OPLOCK :
		StringCchCopyW(Code, Size, L"FSCTL_REQUEST_FILTER_OPLOCK");
		break;
	case FSCTL_FILESYSTEM_GET_STATISTICS :
		StringCchCopyW(Code, Size, L"FSCTL_FILESYSTEM_GET_STATISTICS");
		break;
	case FSCTL_GET_NTFS_VOLUME_DATA :
		StringCchCopyW(Code, Size, L"FSCTL_GET_NTFS_VOLUME_DATA");
		break;
	case FSCTL_GET_NTFS_FILE_RECORD :
		StringCchCopyW(Code, Size, L"FSCTL_GET_NTFS_FILE_RECORD");
		break;
	case FSCTL_GET_VOLUME_BITMAP :
		StringCchCopyW(Code, Size, L"FSCTL_GET_VOLUME_BITMAP");
		break;
	case FSCTL_GET_RETRIEVAL_POINTERS :
		StringCchCopyW(Code, Size, L"FSCTL_GET_RETRIEVAL_POINTERS");
		break;
	case FSCTL_MOVE_FILE :
		StringCchCopyW(Code, Size, L"FSCTL_MOVE_FILE");
		break;
	case FSCTL_IS_VOLUME_DIRTY :
		StringCchCopyW(Code, Size, L"FSCTL_IS_VOLUME_DIRTY");
		break;
	case FSCTL_ALLOW_EXTENDED_DASD_IO :
		StringCchCopyW(Code, Size, L"FSCTL_ALLOW_EXTENDED_DASD_IO");
		break;
	case FSCTL_FIND_FILES_BY_SID :
		StringCchCopyW(Code, Size, L"FSCTL_FIND_FILES_BY_SID");
		break;
	case FSCTL_SET_OBJECT_ID :
		StringCchCopyW(Code, Size, L"FSCTL_SET_OBJECT_ID");
		break;
	case FSCTL_GET_OBJECT_ID :
		StringCchCopyW(Code, Size, L"FSCTL_GET_OBJECT_ID");
		break;
	case FSCTL_DELETE_OBJECT_ID :
		StringCchCopyW(Code, Size, L"FSCTL_DELETE_OBJECT_ID");
		break;
	case FSCTL_SET_REPARSE_POINT :
		StringCchCopyW(Code, Size, L"FSCTL_SET_REPARSE_POINT");
		break;
	case FSCTL_GET_REPARSE_POINT :
		StringCchCopyW(Code, Size, L"FSCTL_GET_REPARSE_POINT");
		break;
	case FSCTL_DELETE_REPARSE_POINT :
		StringCchCopyW(Code, Size, L"FSCTL_DELETE_REPARSE_POINT");
		break;
	case FSCTL_ENUM_USN_DATA :
		StringCchCopyW(Code, Size, L"FSCTL_ENUM_USN_DATA");
		break;
	case FSCTL_SECURITY_ID_CHECK :
		StringCchCopyW(Code, Size, L"FSCTL_SECURITY_ID_CHECK");
		break;
	case FSCTL_READ_USN_JOURNAL :
		StringCchCopyW(Code, Size, L"FSCTL_READ_USN_JOURNAL");
		break;
	case FSCTL_SET_OBJECT_ID_EXTENDED :
		StringCchCopyW(Code, Size, L"FSCTL_SET_OBJECT_ID_EXTENDED");
		break;
	case FSCTL_CREATE_OR_GET_OBJECT_ID :
		StringCchCopyW(Code, Size, L"FSCTL_CREATE_OR_GET_OBJECT_ID");
		break;
	case FSCTL_SET_SPARSE :
		StringCchCopyW(Code, Size, L"FSCTL_SET_SPARSE");
		break;
	case FSCTL_SET_ZERO_DATA :
		StringCchCopyW(Code, Size, L"FSCTL_SET_ZERO_DATA");
		break;
	case FSCTL_QUERY_ALLOCATED_RANGES :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_ALLOCATED_RANGES");
		break;
	case FSCTL_ENABLE_UPGRADE :
		StringCchCopyW(Code, Size, L"FSCTL_ENABLE_UPGRADE");
		break;
	case FSCTL_SET_ENCRYPTION :
		StringCchCopyW(Code, Size, L"FSCTL_SET_ENCRYPTION");
		break;
	case FSCTL_ENCRYPTION_FSCTL_IO :
		StringCchCopyW(Code, Size, L"FSCTL_ENCRYPTION_FSCTL_IO");
		break;
	case FSCTL_WRITE_RAW_ENCRYPTED :
		StringCchCopyW(Code, Size, L"FSCTL_WRITE_RAW_ENCRYPTED");
		break;
	case FSCTL_READ_RAW_ENCRYPTED :
		StringCchCopyW(Code, Size, L"FSCTL_READ_RAW_ENCRYPTED");
		break;
	case FSCTL_CREATE_USN_JOURNAL :
		StringCchCopyW(Code, Size, L"FSCTL_CREATE_USN_JOURNAL");
		break;
	case FSCTL_READ_FILE_USN_DATA :
		StringCchCopyW(Code, Size, L"FSCTL_READ_FILE_USN_DATA");
		break;
	case FSCTL_WRITE_USN_CLOSE_RECORD :
		StringCchCopyW(Code, Size, L"FSCTL_WRITE_USN_CLOSE_RECORD");
		break;
	case FSCTL_EXTEND_VOLUME :
		StringCchCopyW(Code, Size, L"FSCTL_EXTEND_VOLUME");
		break;
	case FSCTL_QUERY_USN_JOURNAL :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_USN_JOURNAL");
		break;
	case FSCTL_DELETE_USN_JOURNAL :
		StringCchCopyW(Code, Size, L"FSCTL_DELETE_USN_JOURNAL");
		break;
	case FSCTL_MARK_HANDLE :
		StringCchCopyW(Code, Size, L"FSCTL_MARK_HANDLE");
		break;
	case FSCTL_SIS_COPYFILE :
		StringCchCopyW(Code, Size, L"FSCTL_SIS_COPYFILE");
		break;
	case FSCTL_SIS_LINK_FILES :
		StringCchCopyW(Code, Size, L"FSCTL_SIS_LINK_FILES");
		break;
	case FSCTL_HSM_MSG :
		StringCchCopyW(Code, Size, L"FSCTL_HSM_MSG");
		break;
	case FSCTL_HSM_DATA :
		StringCchCopyW(Code, Size, L"FSCTL_HSM_DATA");
		break;
	case FSCTL_RECALL_FILE :
		StringCchCopyW(Code, Size, L"FSCTL_RECALL_FILE");
		break;
	case FSCTL_READ_FROM_PLEX :
		StringCchCopyW(Code, Size, L"FSCTL_READ_FROM_PLEX");
		break;
	case FSCTL_FILE_PREFETCH :
		StringCchCopyW(Code, Size, L"FSCTL_FILE_PREFETCH");
		break;
	case FSCTL_MAKE_MEDIA_COMPATIBLE :
		StringCchCopyW(Code, Size, L"FSCTL_MAKE_MEDIA_COMPATIBLE");
		break;
	case FSCTL_SET_DEFECT_MANAGEMENT :
		StringCchCopyW(Code, Size, L"FSCTL_SET_DEFECT_MANAGEMENT");
		break;
	case FSCTL_QUERY_SPARING_INFO :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_SPARING_INFO");
		break;
	case FSCTL_QUERY_ON_DISK_VOLUME_INFO :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_ON_DISK_VOLUME_INFO");
		break;
	case FSCTL_SET_VOLUME_COMPRESSION_STATE :
		StringCchCopyW(Code, Size, L"FSCTL_SET_VOLUME_COMPRESSION_STATE");
		break;
	case FSCTL_TXFS_MODIFY_RM :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_MODIFY_RM");
		break;
	case FSCTL_TXFS_QUERY_RM_INFORMATION :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_QUERY_RM_INFORMATION");
		break;
	case FSCTL_TXFS_ROLLFORWARD_REDO :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_ROLLFORWARD_REDO");
		break;
	case FSCTL_TXFS_ROLLFORWARD_UNDO :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_ROLLFORWARD_UNDO");
		break;
	case FSCTL_TXFS_START_RM :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_START_RM");
		break;
	case FSCTL_TXFS_SHUTDOWN_RM :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_SHUTDOWN_RM");
		break;
	case FSCTL_TXFS_READ_BACKUP_INFORMATION :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_READ_BACKUP_INFORMATION");
		break;
	case FSCTL_TXFS_WRITE_BACKUP_INFORMATION :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_WRITE_BACKUP_INFORMATION");
		break;
	case FSCTL_TXFS_CREATE_SECONDARY_RM :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_CREATE_SECONDARY_RM");
		break;
	case FSCTL_TXFS_GET_METADATA_INFO :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_GET_METADATA_INFO");
		break;
	case FSCTL_TXFS_GET_TRANSACTED_VERSION :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_GET_TRANSACTED_VERSION");
		break;
	case FSCTL_TXFS_CREATE_MINIVERSION :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_CREATE_MINIVERSION");
		break;
	case FSCTL_TXFS_TRANSACTION_ACTIVE :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_TRANSACTION_ACTIVE");
		break;
	case FSCTL_SET_ZERO_ON_DEALLOCATION :
		StringCchCopyW(Code, Size, L"FSCTL_SET_ZERO_ON_DEALLOCATION");
		break;
	case FSCTL_SET_REPAIR :
		StringCchCopyW(Code, Size, L"FSCTL_SET_REPAIR");
		break;
	case FSCTL_GET_REPAIR :
		StringCchCopyW(Code, Size, L"FSCTL_GET_REPAIR");
		break;
	case FSCTL_WAIT_FOR_REPAIR :
		StringCchCopyW(Code, Size, L"FSCTL_WAIT_FOR_REPAIR");
		break;
	case FSCTL_INITIATE_REPAIR :
		StringCchCopyW(Code, Size, L"FSCTL_INITIATE_REPAIR");
		break;
	case FSCTL_CSC_INTERNAL :
		StringCchCopyW(Code, Size, L"FSCTL_CSC_INTERNAL");
		break;
	case FSCTL_SHRINK_VOLUME :
		StringCchCopyW(Code, Size, L"FSCTL_SHRINK_VOLUME");
		break;
	case FSCTL_SET_SHORT_NAME_BEHAVIOR :
		StringCchCopyW(Code, Size, L"FSCTL_SET_SHORT_NAME_BEHAVIOR");
		break;
	case FSCTL_DFSR_SET_GHOST_HANDLE_STATE :
		StringCchCopyW(Code, Size, L"FSCTL_DFSR_SET_GHOST_HANDLE_STATE");
		break;
	case FSCTL_TXFS_LIST_TRANSACTIONS :
		StringCchCopyW(Code, Size, L"FSCTL_TXFS_LIST_TRANSACTIONS");
		break;
	case FSCTL_QUERY_PAGEFILE_ENCRYPTION :
		StringCchCopyW(Code, Size, L"FSCTL_QUERY_PAGEFILE_ENCRYPTION");
		break;

		//
		// VOLUME
		//

	case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS :
		StringCchCopyW(Code, Size, L"IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS");
		break;
	case IOCTL_VOLUME_IS_CLUSTERED :
		StringCchCopyW(Code, Size, L"IOCTL_VOLUME_IS_CLUSTERED");
		break;

		//
		// WDK headers
		//

	case IOCTL_SCSI_EXECUTE_IN :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_EXECUTE_IN");
		break;
	case IOCTL_SCSI_EXECUTE_OUT :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_EXECUTE_OUT");
		break;
	case IOCTL_SCSI_EXECUTE_NONE :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_EXECUTE_NONE");
		break;
	case IOCTL_SCSI_MINIPORT_SMART_VERSION :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_SMART_VERSION");
		break;
	case IOCTL_SCSI_MINIPORT_IDENTIFY :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_IDENTIFY");
		break;
	case IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS");
		break;
	case IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS");
		break;
	case IOCTL_SCSI_MINIPORT_ENABLE_SMART :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_ENABLE_SMART");
		break;
	case IOCTL_SCSI_MINIPORT_DISABLE_SMART :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_DISABLE_SMART");
		break;
	case IOCTL_SCSI_MINIPORT_RETURN_STATUS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_RETURN_STATUS");
		break;
	case IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE");
		break;
	case IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES");
		break;
	case IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS");
		break;
	case IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE");
		break;
	case IOCTL_SCSI_MINIPORT_READ_SMART_LOG :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_READ_SMART_LOG");
		break;
	case IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG :
		StringCchCopyW(Code, Size, L"IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG");
		break;
	case IOCTL_MOUNTMGR_CREATE_POINT :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_CREATE_POINT");
		break;
	case IOCTL_MOUNTMGR_DELETE_POINTS :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_DELETE_POINTS");
		break;
	case IOCTL_MOUNTMGR_QUERY_POINTS :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_QUERY_POINTS");
		break;
	case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY");
		break;
	case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER");
		break;
	case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS");
		break;
	case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED");
		break;
	case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED");
		break;
	case IOCTL_MOUNTMGR_CHANGE_NOTIFY :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_CHANGE_NOTIFY");
		break;
	case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE");
		break;
	case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES");
		break;
	case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION");
		break;
	case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTDEV_QUERY_DEVICE_NAME");
		break;
	case IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH");
		break;
	case IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS");
		break;
	case IOCTL_MOUNTMGR_SCRUB_REGISTRY :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_SCRUB_REGISTRY");
		break;
	case IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT");
		break;
	case IOCTL_MOUNTMGR_SET_AUTO_MOUNT :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_SET_AUTO_MOUNT");
		break;
	case IOCTL_MOUNTMGR_BOOT_DL_ASSIGNMENT :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_BOOT_DL_ASSIGNMENT");
		break;
	case IOCTL_MOUNTMGR_TRACELOG_CACHE :
		StringCchCopyW(Code, Size, L"IOCTL_MOUNTMGR_TRACELOG_CACHE");
		break;

		//
		// UNKNOWN
		//

	default:
		StringCchCopyW(Code, Size, L"UNKNOWN");
		break;
	}

	return TRUE;
}

BOOL WINAPI 
DeviceIoControlEnter(
	__in  HANDLE hDevice,
	__in  DWORD dwIoControlCode,
	__in  LPVOID lpInBuffer,
	__in  DWORD nInBufferSize,
	__out LPVOID lpOutBuffer,
	__in  DWORD nOutBufferSize,
	__out LPDWORD lpBytesReturned,
	__in  LPOVERLAPPED lpOverlapped
	)
{
	BOOL Status;
	PBTR_FILTER_RECORD Record;
	PFS_DEVICEIOCONTROL Filter;
	DEVICEIOCONTROL_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	DWORD Size;

	BtrFltGetContext(&Context);
	Routine = (DEVICEIOCONTROL_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hDevice, dwIoControlCode,
		                lpInBuffer, nInBufferSize,
						lpOutBuffer, nOutBufferSize,
						lpBytesReturned, lpOverlapped);
	BtrFltSetContext(&Context);

	//
	// Limit copy data size to be maximum 16 bytes,
	// better method is to decode IOCTL code to decide
	// how to capture the data, here we blindly capture
	// the data given the arguments.
	//

	if (lpBytesReturned && !lpOverlapped) {
		Size = *lpBytesReturned;
		Size = min(Size, 16);
	} else {
		Size = 0;
	}

	Record = BtrFltAllocateRecord(sizeof(FS_DEVICEIOCONTROL), 
		                         FsGuid, _DeviceIoControl);

	Filter = FILTER_RECORD_POINTER(Record, FS_DEVICEIOCONTROL);
	Filter->hDevice = hDevice;
	Filter->dwIoControlCode = dwIoControlCode;
	Filter->lpInBuffer = lpInBuffer;
	Filter->nInBufferSize = nInBufferSize;
	Filter->lpOutBuffer = lpOutBuffer;
	Filter->nOutBufferSize = nOutBufferSize;
	Filter->lpOverlapped = lpOverlapped;
	Filter->InSize = 0;
	Filter->OutSize = 0;

	if (lpBytesReturned) {
		Filter->lpBytesReturned = (LPDWORD)Size;
	}

	__try {
		if (Size != 0 && lpInBuffer != NULL) {
			memcpy(Filter->InBuffer, lpInBuffer, Size); 
			Filter->InSize = (USHORT)Size;
		}
		if (Size != 0 && lpOutBuffer != NULL) {
			memcpy(Filter->OutBuffer, lpOutBuffer, Size); 
			Filter->OutSize = (USHORT)Size;
		}
	}__except(1) {

	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
DeviceIoControlDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_DEVICEIOCONTROL Data;
	WCHAR InBuffer[MAX_PATH];
	WCHAR OutBuffer[MAX_PATH];
	WCHAR Code[MAX_PATH];
	PWCHAR Pointer;
	int i;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _DeviceIoControl);

	Data = (PFS_DEVICEIOCONTROL)Record->Data;
	InBuffer[0] = 0;
	OutBuffer[0] = 0;

	FsDecodeIoControl(Data->dwIoControlCode, Code, MAX_PATH);

	if (Data->InSize) {
		Pointer = (PWCHAR)InBuffer;
		for (i = 0; i < Data->InSize; i++) {
			StringCchPrintf(Pointer, 4, L"%02x ", Data->InBuffer[i]);	
			Pointer += 3;
		}
	}

	if (Data->OutSize) {
		Pointer = (PWCHAR)OutBuffer;
		for (i = 0; i < Data->OutSize; i++) {
			StringCchPrintf(Pointer, 4, L"%02x ", Data->OutBuffer[i]);	
			Pointer += 3;
		}
	}

	StringCchPrintf(
			Buffer, MaxCount - 1, 
			L"Device: 0x%x, Code: %s(0x%x), InBuffer: 0x%x, InSize: 0x%x,"
			L"OutBuffer: 0x%x, OutSize: 0x%x, Overlap: 0x%x, Complete: 0x%x,"
			L"in(%d):{ %s }, out(%d):{ %s }",
			Data->hDevice, Code, Data->dwIoControlCode, Data->lpInBuffer, Data->nInBufferSize, 
			Data->lpOutBuffer, Data->nOutBufferSize, Data->lpOverlapped, (DWORD)Data->lpBytesReturned,
			Data->InSize, InBuffer, Data->OutSize, OutBuffer );

	return S_OK;
}

DWORD WINAPI 
SetFilePointerEnter(
	IN  HANDLE hFile,
	IN  LONG lDistanceToMove,
	OUT PLONG lpDistanceToMoveHigh,
	IN  DWORD dwMoveMethod
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SETFILEPOINTER Filter;
	SETFILEPOINTER_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	DWORD FilePointerLow;

	BtrFltGetContext(&Context);
	Routine = (SETFILEPOINTER_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	FilePointerLow = (*Routine)(hFile, lDistanceToMove,
		                        lpDistanceToMoveHigh,
								dwMoveMethod);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SETFILEPOINTER), 
		                          FsGuid, _SetFilePointer);

	Filter = FILTER_RECORD_POINTER(Record, FS_SETFILEPOINTER);
	Filter->FilePointerLow = FilePointerLow;
	Filter->FileHandle = hFile;
	Filter->DistanceToMove = lDistanceToMove;
	Filter->DistanceToMoveHighPtr = lpDistanceToMoveHigh;
	Filter->MoveMethod = dwMoveMethod;	
	Filter->LastErrorStatus = GetLastError();

	if (lpDistanceToMoveHigh) {
		Filter->DistanceToMoveHigh = *lpDistanceToMoveHigh;
	} else {
		Filter->DistanceToMoveHigh = 0;
	}

	BtrFltQueueRecord(Record);
	return FilePointerLow;
}

ULONG
SetFilePointerDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SETFILEPOINTER Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SetFilePointer);

	Data = (PFS_SETFILEPOINTER)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p,DistanceToMove=0x%, DistanceToMoveHigh =0x%x,MoveMethod=0x%x,"
		L"FilePointerLow=0x%x, FilePointerHigh=0x%x, Status=0x%08x", 
		Data->FileHandle, Data->DistanceToMove, Data->DistanceToMoveHighPtr, Data->MoveMethod, 
		Data->FilePointerLow, Data->DistanceToMoveHigh, Data->LastErrorStatus);
	
	return S_OK;
}

BOOL WINAPI 
SetFileValidDataEnter(
	IN HANDLE hFile,
	IN LONGLONG ValidDataLength
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SETFILEVALIDDATA Filter;
	SETFILEVALIDDATA_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (SETFILEVALIDDATA_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, ValidDataLength);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SETFILEVALIDDATA),
		                          FsGuid, _SetFileValidData);

	Filter = FILTER_RECORD_POINTER(Record, FS_SETFILEVALIDDATA);
	Filter->FileHandle = hFile;
	Filter->ValidDataLength = ValidDataLength;
	Filter->Status = Status;

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
SetFileValidDataDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SETFILEVALIDDATA Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SetFileValidData);

	Data = (PFS_SETFILEVALIDDATA)Record->Data;

	//
	// ValidDataLength is formatted as 64 bits hexdecimal
	//

	StringCchPrintf(Buffer, MaxCount - 1, 
					L"FileHandle=0x%p,ValidDataLength=0x%.16I64x,Status=0x%08x,LastErrorStatus=0x%08x", 
					Data->FileHandle, Data->ValidDataLength, Data->Status, Data->LastErrorStatus);
	return S_OK;
}

BOOL WINAPI 
SetEndOfFileEnter(
	IN HANDLE hFile
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SETENDOFFILE Filter;
	SETENDOFFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (SETENDOFFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SETENDOFFILE),
		                          FsGuid, _SetEndOfFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_SETENDOFFILE);
	Filter->FileHandle = hFile;
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
SetEndOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SETENDOFFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SetEndOfFile);

	Data = (PFS_SETENDOFFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
					L"FileHandle=0x%p,Status=0x%08x,LastErrorStatus=0x%08x", 
					Data->FileHandle, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
SetFileAttributesEnter(
	IN LPCWSTR lpFileName,
	IN DWORD dwFileAttributes
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SETFILEATTRIBUTES Filter;
	SETFILEATTRIBUTES_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (SETFILEATTRIBUTES_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpFileName, dwFileAttributes);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SETFILEATTRIBUTES),
		                          FsGuid, _SetFileAttributes);

	Filter = FILTER_RECORD_POINTER(Record, FS_SETFILEATTRIBUTES);

	if (lpFileName) {
		StringCchCopy(Filter->FileName, MAX_PATH, lpFileName);
	}

	Filter->Status = Status;
	Filter->FileAttributes = dwFileAttributes;	
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
SetFileAttributesDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SETFILEATTRIBUTES Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SetFileAttributes);

	Data = (PFS_SETFILEATTRIBUTES)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileName=%ws,Attributes=0x%08x, Status=0x%08x,LastErrorStatus=0x%08x", 
		Data->FileName, Data->FileAttributes, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
MoveFileEnter(
	IN LPCWSTR lpExistingFileName,
	IN LPCWSTR lpNewFileName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_MOVEFILE Filter;
	MOVEFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (MOVEFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpExistingFileName, lpNewFileName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_MOVEFILE),
		                          FsGuid, _MoveFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_MOVEFILE);

	if (lpExistingFileName) {
		StringCchCopy(Filter->ExistingFileName, MAX_PATH, lpExistingFileName);
	}

	if (lpNewFileName) {
		StringCchCopy(Filter->NewFileName, MAX_PATH, lpNewFileName);
	}

	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
MoveFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_MOVEFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _MoveFile);

	Data = (PFS_MOVEFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"ExistingFileName=%ws, NewFileName=%ws, Status=0x%08x,LastErrorStatus=0x%08x", 
		Data->ExistingFileName, Data->NewFileName, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
LockFileEnter(
	IN HANDLE hFile,
	IN DWORD dwFileOffsetLow,
	IN DWORD dwFileOffsetHigh,
	IN DWORD nNumberOfBytesToLockLow,
	IN DWORD nNumberOfBytesToLockHigh
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_LOCKFILE Filter;
	LOCKFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (LOCKFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, dwFileOffsetLow,
		                dwFileOffsetHigh, nNumberOfBytesToLockLow,
						nNumberOfBytesToLockHigh);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_LOCKFILE),
		                          FsGuid, _LockFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_LOCKFILE);
	Filter->Status = Status;
	Filter->FileHandle = hFile;
	Filter->FileOffsetLow = dwFileOffsetLow;
	Filter->FileOffsetHigh = dwFileOffsetHigh;
	Filter->NumberOfBytesToLockLow = nNumberOfBytesToLockLow;
	Filter->NumberOfBytesToLockHigh = nNumberOfBytesToLockHigh;
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
LockFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_LOCKFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _LockFile);

	Data = (PFS_LOCKFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p, OffsetLow=0x%08x, OffsetHigh=0x%08x, LockBytesLow=0x%08x, LockBytesHigh=0x%08x"
		L"Status=0x%08x,LastErrorStatus=0x%08x", 
		Data->FileHandle, Data->FileOffsetLow, Data->FileOffsetHigh,
		Data->NumberOfBytesToLockLow, Data->NumberOfBytesToLockHigh,
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
GetFileSizeExEnter(
	IN  HANDLE hFile,
	OUT PLARGE_INTEGER lpFileSize
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETFILESIZEEX Filter;
	GETFILESIZEEX_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (GETFILESIZEEX_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, lpFileSize);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETFILESIZEEX),
		                          FsGuid, _GetFileSizeEx);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETFILESIZEEX);
	Filter->FileHandle = hFile;
	Filter->Status = Status;

	if (lpFileSize) {
		Filter->Size = *lpFileSize;
	}

	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
GetFileSizeExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETFILESIZEEX Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetFileSizeEx);

	Data = (PFS_GETFILESIZEEX)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p, SizeLow=0x%08x, SizeHigh=0x%08x, Status=0x%08x,LastErrorStatus=0x%08x", 
		Data->FileHandle, Data->Size.LowPart, Data->Size.HighPart,
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
GetFileInformationByHandleEnter(
	IN  HANDLE hFile,
	OUT LPBY_HANDLE_FILE_INFORMATION lpFileInformation
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETFILEINFORMATIONBYHANDLE Filter;
	GETFILEINFORMATIONBYHANDLE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (GETFILEINFORMATIONBYHANDLE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, lpFileInformation);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETFILEINFORMATIONBYHANDLE),
		                          FsGuid, _GetFileInformationByHandle);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETFILEINFORMATIONBYHANDLE);
	Filter->Status = Status;
	Filter->FileHandle = hFile;
	Filter->InformationPointer = lpFileInformation;
	Filter->LastErrorStatus = GetLastError();

	if (lpFileInformation != NULL) {
		memcpy(&Filter->Information, lpFileInformation, sizeof(BY_HANDLE_FILE_INFORMATION));
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
GetFileInformationByHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETFILEINFORMATIONBYHANDLE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetFileInformationByHandle);
	
	Data = (PFS_GETFILEINFORMATIONBYHANDLE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p, InformationPointer=0x%p,Information.FileAttributes=0x%08x,Status=0x%08x,LastErrorStatus=0x%08x", 
		Data->FileHandle, Data->InformationPointer, Data->Information.dwFileAttributes,
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

DWORD WINAPI 
GetFileAttributesEnter(
	IN LPCWSTR lpFileName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETFILEATTRIBUTES Filter;
	GETFILEATTRIBUTES_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	ULONG Attributes;

	BtrFltGetContext(&Context);
	Routine = (GETFILEATTRIBUTES_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Attributes = (*Routine)(lpFileName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETFILEATTRIBUTES),
		                          FsGuid, _GetFileAttributes);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETFILEATTRIBUTES);

	if (lpFileName) {
		StringCchCopy(Filter->FileName, MAX_PATH, lpFileName);
	}

	Filter->LastErrorStatus = GetLastError();
	Filter->Attributes = Attributes;

	BtrFltQueueRecord(Record);
	return Attributes;
}

ULONG 
GetFileAttributesDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETFILEATTRIBUTES Data;

	Data = (PFS_GETFILEATTRIBUTES)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileName=%ws, Attributes=0x%08x, LastErrorStatus=0x%08x", 
		Data->FileName, Data->Attributes, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
DeleteFileEnter(
	IN LPCWSTR lpFileName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_DELETEFILE Filter;
	DELETEFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (DELETEFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpFileName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_DELETEFILE),
		                          FsGuid, _DeleteFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_DELETEFILE);
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();

	if (lpFileName) {
		StringCchCopy(Filter->FileName, MAX_PATH, lpFileName);
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
DeleteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_DELETEFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _DeleteFile);

	Data = (PFS_DELETEFILE)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"FileName=%ws,Result=%d, Satus=0x%08x", 
		Data->FileName, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
CopyFileEnter(
	IN LPCWSTR lpExistingFileName,
	IN LPCWSTR lpNewFileName,
	IN BOOL bFailIfExists
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_COPYFILE Filter;
	COPYFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (COPYFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpExistingFileName, lpNewFileName, bFailIfExists);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_COPYFILE),
		                          FsGuid, _CopyFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_COPYFILE);
	Filter->FailIfExists = bFailIfExists;
	Filter->Status = Status;

	if (lpExistingFileName)
		StringCchCopy(Filter->ExistingFileName, MAX_PATH, lpExistingFileName);

	if (lpNewFileName)
		StringCchCopy(Filter->NewFileName, MAX_PATH, lpNewFileName);

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
CopyFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_COPYFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _CopyFile);

	Data = (PFS_COPYFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"NewFileName=%ws, FailIfExists=%d, Status=%d, LastErrorStatus=0x%08x", 
		Data->NewFileName, Data->FailIfExists, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

HANDLE WINAPI 
FindFirstFileEnter(
	IN  LPCWSTR lpFileName,
	OUT LPWIN32_FIND_DATA lpFindFileData
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDFIRSTFILE Filter;
	FINDFIRSTFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	HANDLE FileHandle;

	BtrFltGetContext(&Context);
	Routine = (FINDFIRSTFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	FileHandle = (*Routine)(lpFileName, lpFindFileData);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDFIRSTFILE),
		                          FsGuid, _FindFirstFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDFIRSTFILE);
	Filter->FindFileDataPointer = lpFindFileData;

	if (lpFileName)
		StringCchCopy(Filter->FileName, MAX_PATH, lpFileName);

	Filter->FileHandle = FileHandle;
	Filter->LastErrorStatus = GetLastError();

	if (lpFindFileData) {
		memcpy(&Filter->FindFileData, lpFindFileData, sizeof(WIN32_FIND_DATA));
	}

	BtrFltQueueRecord(Record);
	return FileHandle;
}

ULONG 
FindFirstFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDFIRSTFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindFirstFile);

	Data = (PFS_FINDFIRSTFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileName=%ws, WIN32_FIND_DATA.FileName=%ws, FileHandle=0x%p, LastErrorStatus=0x%08x", 
		Data->FileName, Data->FindFileData.cFileName, Data->FileHandle, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
FindNextFileEnter(
	IN  HANDLE hFindFile,
	OUT LPWIN32_FIND_DATA lpFindFileData
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDNEXTFILE Filter;
	FINDNEXTFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (FINDNEXTFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFindFile, lpFindFileData);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDNEXTFILE),
		                          FsGuid, _FindNextFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDNEXTFILE);
	Filter->FileHandle = hFindFile;
	Filter->FindFileDataPointer = lpFindFileData;
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();

	if (lpFindFileData) {
		memcpy(&Filter->FindFileData, lpFindFileData, sizeof(WIN32_FIND_DATA));
	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
FindNextFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDNEXTFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindNextFile);

	Data = (PFS_FINDNEXTFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"WIN32_FIND_DATA.FileName=%ws,FileHandle=0x%p,Status=%d, LastErrorStatus=0x%08x", 
		Data->FindFileData.cFileName, Data->FileHandle, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI
FindCloseEnter(
	IN HANDLE hFile
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDCLOSE Filter;
	FINDCLOSE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (FINDCLOSE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDCLOSE),
		                          FsGuid, _FindClose);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDCLOSE);
	Filter->FileHandle = hFile;
	Filter->Status;	
	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
FindCloseDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDCLOSE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindClose);

	Data = (PFS_FINDCLOSE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p, Status=%d, LastErrorStatus=0x%08x", 
		Data->FileHandle, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
DefineDosDeviceEnter(
	IN DWORD dwFlags,
	IN LPCWSTR lpDeviceName,
	IN LPCWSTR lpTargetPath
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_DEFINEDOSDEVICE Filter;
	DEFINEDOSDEVICE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (DEFINEDOSDEVICE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(dwFlags, lpDeviceName, lpTargetPath);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_DEFINEDOSDEVICE),
		                          FsGuid, _DefineDosDevice);

	Filter = FILTER_RECORD_POINTER(Record, FS_DEFINEDOSDEVICE);
	Filter->Status = Status;
	Filter->Flag = dwFlags;

	if (lpDeviceName)
		StringCchCopy(Filter->DeviceName, MAX_PATH, lpDeviceName);

	if (lpTargetPath)
		StringCchCopy(Filter->TargetPath, MAX_PATH, lpTargetPath);

	Filter->LastErrorStatus = GetLastError();

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
DefineDosDeviceDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_DEFINEDOSDEVICE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _DefineDosDevice);

	Data = (PFS_DEFINEDOSDEVICE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"DeviceName=%ws,TargetPath=%ws, Status=%d, LastErrorStatus=0x%08x", 
		Data->DeviceName, Data->TargetPath, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

DWORD WINAPI 
QueryDosDeviceEnter(
	IN  LPCWSTR lpDeviceName,
	OUT LPWSTR lpTargetPath,
	IN  DWORD ucchMax
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_QUERYDOSDEVICE Filter;
	QUERYDOSDEVICE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	DWORD StoredCharCount;

	BtrFltGetContext(&Context);
	Routine = (QUERYDOSDEVICE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	StoredCharCount = (*Routine)(lpDeviceName, lpTargetPath, ucchMax);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_QUERYDOSDEVICE),
		                          FsGuid, _QueryDosDevice);

	Filter = FILTER_RECORD_POINTER(Record, FS_QUERYDOSDEVICE);
	Filter->MaxCharCount = ucchMax;
	Filter->TargetPathPointer = lpTargetPath;

	if (lpDeviceName)
		StringCchCopy(Filter->DeviceName, MAX_PATH, lpDeviceName);

	Filter->StoredCharCount = StoredCharCount;
	Filter->LastErrorStatus = GetLastError();
	
	//
	// Copy target device path
	//

	if (lpTargetPath) {
		StringCchCopy(Filter->TargetPath, MAX_PATH, lpTargetPath);
	}

	BtrFltQueueRecord(Record);
	return StoredCharCount;
}

ULONG 
QueryDosDeviceDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_QUERYDOSDEVICE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _QueryDosDevice);

	Data = (PFS_QUERYDOSDEVICE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"DeviceName=%ws,TargetPathPointer=0x%p, TargetPath=%ws, LastErrorStatus=0x%08x", 
		Data->DeviceName, Data->TargetPathPointer, Data->TargetPath, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
UnlockFileEnter(
	IN HANDLE hFile,
	IN DWORD dwFileOffsetLow,
	IN DWORD dwFileOffsetHigh,
	IN DWORD nNumberOfBytesToUnlockLow,
	IN DWORD nNumberOfBytesToUnlockHigh
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_UNLOCKFILE Filter;
	UNLOCKFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (UNLOCKFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, dwFileOffsetLow, dwFileOffsetHigh,
		                nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_UNLOCKFILE),
		                          FsGuid, _UnlockFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_UNLOCKFILE);
	Filter->FileHandle = hFile;
	Filter->FileOffsetLow = dwFileOffsetLow;
	Filter->FileOffsetHigh = dwFileOffsetHigh;
	Filter->NumberOfBytesToUnlockLow = nNumberOfBytesToUnlockLow;
	Filter->NumberOfBytesToUnlockHigh = nNumberOfBytesToUnlockHigh;
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
UnlockFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_UNLOCKFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _UnlockFile);

	Data = (PFS_UNLOCKFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p, OffsetLow=0x%08x, OffsetHigh=0x%08x, UnlockBytesLow=0x%08x, UnlockBytesHigh=0x%08x"
		L"Status=0x%08x,LastErrorStatus=0x%08x", 
		Data->FileHandle, Data->FileOffsetLow, Data->FileOffsetHigh,
		Data->NumberOfBytesToUnlockLow, Data->NumberOfBytesToUnlockHigh,
		Data->Status, Data->LastErrorStatus);

	return S_OK;
}

//
// N.B. This API requires at least Vista.
//

BOOL WINAPI 
SetFileInformationByHandleEnter(
	IN HANDLE hFile,
	IN FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
	IN LPVOID lpFileInformation,
	IN DWORD dwBufferSize
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SETFILEINFORMATIONBYHANDLE Filter;
	SETFILEINFORMATIONBYHANDLE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;

	BtrFltGetContext(&Context);
	Routine = (SETFILEINFORMATIONBYHANDLE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(hFile, FileInformationClass, lpFileInformation, dwBufferSize);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SETFILEINFORMATIONBYHANDLE),
		                          FsGuid, _SetFileInformationByHandle);

	Filter = FILTER_RECORD_POINTER(Record, FS_SETFILEINFORMATIONBYHANDLE);
	Filter->FileHandle = hFile;
	Filter->FileInformationClass = FileInformationClass;
	Filter->lpFileInformation = lpFileInformation;
	Filter->Status = Status;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG 
SetFileInformationByHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SETFILEINFORMATIONBYHANDLE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SetFileInformationByHandle);

	Data = (PFS_SETFILEINFORMATIONBYHANDLE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileHandle=0x%p, InformationClass=0x%p, LastErrorStatus=0x%08x", 
		Data->FileHandle, Data->FileInformationClass, Data->LastErrorStatus);

	return S_OK;
}

DWORD WINAPI 
SearchPathEnter(
	IN  LPCTSTR lpPath,
	IN  LPCTSTR lpFileName,
	IN  LPCTSTR lpExtension,
	IN  DWORD nBufferLength,
	OUT LPTSTR lpBuffer,
	OUT LPTSTR* lpFilePart
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SEARCHPATH Filter;
	SEARCHPATH_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	DWORD Result;

	BtrFltGetContext(&Context);
	Routine = (SEARCHPATH_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpPath, lpFileName, lpExtension, nBufferLength,
		                lpBuffer, lpFilePart);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SEARCHPATH),
		                          FsGuid, _SearchPath);

	Filter = FILTER_RECORD_POINTER(Record, FS_SEARCHPATH);
	if (lpPath) {
		StringCchCopy(Filter->Path, MAX_PATH, lpPath);
	}
	if (lpFileName) {
		StringCchCopy(Filter->FileName, MAX_PATH, lpFileName);
	}
	if (lpExtension) {
		StringCchCopy(Filter->Extension, 8, lpExtension);
	}
	Filter->BufferLength = nBufferLength;
	Filter->BufferPointer = lpBuffer;
	Filter->LastErrorStatus = GetLastError();	                        
	Filter->Result = Result;

	if (Filter->Result && lpFilePart != NULL) {
		StringCchCopy(Filter->ResultFile, MAX_PATH, *lpFilePart);
	}

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
SearchPathDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SEARCHPATH Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SearchPath);

	Data = (PFS_SEARCHPATH)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"SearchPath=%ws, FileName=%ws, Extension=%ws, BufferPointer=0x%p,"
		L"BufferLength=%d, FilePartPointer=0x%p, Result=%d, ResultFile=%ws, LastErrorStatus=0x%08x", 
		Data->Path, Data->FileName, Data->Extension, 
		Data->BufferPointer, Data->BufferLength, Data->FilePartPointer, 
		Data->Result, Data->ResultFile, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
ReplaceFileEnter(
	IN LPCTSTR lpReplacedFileName,
	IN LPCTSTR lpReplacementFileName,
	IN LPCTSTR lpBackupFileName,
	IN DWORD dwReplaceFlags,
	IN LPVOID lpExclude,  
	IN LPVOID lpReserved 
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_REPLACEFILE Filter;
	REPLACEFILE_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (REPLACEFILE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpReplacedFileName, lpReplacementFileName, lpBackupFileName, 
		                dwReplaceFlags, lpExclude, lpReserved);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_REPLACEFILE),
		                          FsGuid, _ReplaceFile);

	Filter = FILTER_RECORD_POINTER(Record, FS_REPLACEFILE);

	if (lpReplacedFileName)
		StringCchCopy(Filter->ReplacedFileName, MAX_PATH, lpReplacedFileName);

	if (lpReplacementFileName)
		StringCchCopy(Filter->ReplacementFileName, MAX_PATH, lpReplacementFileName);

	if (lpBackupFileName)
		StringCchCopy(Filter->BackupFileName, MAX_PATH, lpBackupFileName);

	Filter->dwReplaceFlags = dwReplaceFlags;
	Filter->Exclude = lpExclude;
	Filter->Reserved = lpReserved;
	Filter->LastErrorStatus = GetLastError();	                        
	Filter->Result = Result;

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
ReplaceFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_REPLACEFILE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _ReplaceFile);

	Data = (PFS_REPLACEFILE)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"ReplacedFile=%ws, ReplacementFile=%ws, BackupFile=%ws, Flag=0x%x, Status=%d, LastErrorStatus=0x%08x", 
		Data->ReplacedFileName, Data->ReplacementFileName, Data->BackupFileName, 
		Data->dwReplaceFlags, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

UINT WINAPI 
GetTempFileNameEnter(
	IN LPCTSTR lpPathName,
	IN LPCTSTR lpPrefixString,
	IN UINT uUnique,
	OUT LPTSTR lpTempFileName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETTEMPFILENAME Filter;
	GETTEMPFILENAME_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	UINT Result;

	BtrFltGetContext(&Context);
	Routine = (GETTEMPFILENAME_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpPathName, lpPrefixString, uUnique, lpTempFileName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETTEMPFILENAME),
		                          FsGuid, _GetTempFileName);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETTEMPFILENAME);

	if (lpPathName)
		StringCchCopy(Filter->PathName, MAX_PATH, lpPathName);

	if (lpPrefixString)
		StringCchCopy(Filter->PrefixString, MAX_PATH, lpPrefixString);

	Filter->UniqueId = uUnique;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	if (lpTempFileName)
		StringCchCopy(Filter->TempFileName, MAX_PATH, lpTempFileName);

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
GetTempFileNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETTEMPFILENAME Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetTempFileName);

	Data = (PFS_GETTEMPFILENAME)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"PathName=%ws, Prefix=%ws, UniqueId=%d, TempFileName=%ws, Result=%d, LastErrorStatus=0x%08x", 
		Data->PathName, Data->PathName, Data->TempFileName, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
CreateHardLinkEnter(
	IN LPCTSTR lpFileName,
	IN LPCTSTR lpExistingFileName,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_CREATEHARDLINK Filter;
	CREATEHARDLINK_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (CREATEHARDLINK_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpFileName, lpExistingFileName, lpSecurityAttributes);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_CREATEHARDLINK),
		                          FsGuid, _CreateHardLink);

	Filter = FILTER_RECORD_POINTER(Record, FS_CREATEHARDLINK);

	if (lpFileName)
		StringCchCopy(Filter->FileName, MAX_PATH, lpFileName);

	if (lpExistingFileName)
		StringCchCopy(Filter->ExistingFileName, MAX_PATH, lpExistingFileName);

	Filter->SecurityAttributes = lpSecurityAttributes;
	Filter->LastErrorStatus = GetLastError();	                        
	Filter->Result = Result;

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
CreateHardLinkDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_CREATEHARDLINK Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _CreateHardLink);

	Data = (PFS_CREATEHARDLINK)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"FileName=%ws, ExistingFileName=%ws, SecurityAttributePointer=0x%p,Status=%d, LastErrorStatus=0x%08x", 
		Data->FileName, Data->ExistingFileName, Data->SecurityAttributes, Data->Result, Data->LastErrorStatus);		

	return S_OK;
}

DWORD WINAPI 
GetTempPathEnter(
	IN  DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETTEMPPATH Filter;
	GETTEMPPATH_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	DWORD Result;

	BtrFltGetContext(&Context);
	Routine = (GETTEMPPATH_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(nBufferLength, lpBuffer);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETTEMPPATH),
		                          FsGuid, _GetTempPath);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETTEMPPATH);
	Filter->BufferPointer = lpBuffer;
	Filter->BufferLength = nBufferLength;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	//
	// Access user buffer must be protected by SEH
	//

	__try {
		if (lpBuffer != NULL && lpBuffer[0] != 0) {
			StringCchCopy(Filter->Buffer, MAX_PATH, lpBuffer);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
	}

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
GetTempPathDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETTEMPPATH Data;

	if (Record->ProbeOrdinal != _GetTempPath) {
		return S_FALSE;
	}

	Data = (PFS_GETTEMPPATH)Record->Data;
	StringCchPrintf(Buffer, MaxCount - 1, 
		L"BufferLength=0x%x, BufferPointer=0x%p, TempPath=%ws, LastErrorStatus=%d", 
		Data->BufferLength, Data->BufferPointer, Data->Buffer, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
CreateDirectoryEnter(
	IN LPCTSTR lpPathName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_CREATEDIRECTORY Filter;
	CREATEDIRECTORY_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (CREATEDIRECTORY_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpPathName, lpSecurityAttributes);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_CREATEDIRECTORY),
		                          FsGuid, _CreateDirectory);

	Filter = FILTER_RECORD_POINTER(Record, FS_CREATEDIRECTORY);

	if (lpPathName)
		StringCchCopy(Filter->Path, MAX_PATH, lpPathName);

	Filter->SecurityAttributes = lpSecurityAttributes;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
CreateDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_CREATEDIRECTORY Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _CreateDirectory);

	Data = (PFS_CREATEDIRECTORY)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"Path=%ws, SecurityAttributePointer=0x%p, Result=%d, LastErrorStatus=0x%08x", 
		Data->Path, Data->SecurityAttributes, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
CreateDirectoryExEnter(
	IN LPCTSTR lpTemplateDirectory, 
	IN LPCTSTR lpNewDirectory,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_CREATEDIRECTORYEX Filter;
	CREATEDIRECTORYEX_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (CREATEDIRECTORYEX_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_CREATEDIRECTORYEX),
		                          FsGuid, _CreateDirectoryEx);

	Filter = FILTER_RECORD_POINTER(Record, FS_CREATEDIRECTORYEX);

	if (lpTemplateDirectory)
		StringCchCopy(Filter->TemplatePath, MAX_PATH, lpTemplateDirectory);

	if (lpNewDirectory)
		StringCchCopy(Filter->NewPath, MAX_PATH, lpNewDirectory);

	Filter->SecurityAttributes = lpSecurityAttributes;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
CreateDirectoryExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_CREATEDIRECTORYEX Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _CreateDirectoryEx);

	Data = (PFS_CREATEDIRECTORYEX)Record->Data;
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"TemplatePath=%ws, NewPath=%ws, SecurityAttributePointer=0x%p, Result=%d, LastErrorStatus=0x%08x", 
		Data->TemplatePath, Data->NewPath, Data->SecurityAttributes, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
RemoveDirectoryEnter(
	IN LPCTSTR lpPathName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_REMOVEDIRECTORY Filter;
	REMOVEDIRECTORY_ROUTINE Routine;
	BTR_PROBE_CONTEXT Context;
	BOOL Status;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (REMOVEDIRECTORY_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Status = (*Routine)(lpPathName);
	BtrFltSetContext(&Context);
	LastError = GetLastError();	                        

	Record = BtrFltAllocateRecord(sizeof(FS_REMOVEDIRECTORY),
		                          FsGuid, _RemoveDirectory);

	Filter = FILTER_RECORD_POINTER(Record, FS_REMOVEDIRECTORY);
	Filter->Status = Status;
	Filter->LastErrorStatus = LastError;

	__try {
		if (lpPathName) {
			StringCchCopy(Filter->Directory, MAX_PATH, lpPathName);
		}
	}__except(1) {

	}

	BtrFltQueueRecord(Record);
	return Status;
}

ULONG
RemoveDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_REMOVEDIRECTORY Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _RemoveDirectory);

	Data = (PFS_REMOVEDIRECTORY)Record->Data;
	StringCchPrintf(
			Buffer, MaxCount - 1, 
			L"Directory=%ws, Status=%d, LastErrorStatus=0x%08x", 
			Data->Directory, Data->Status, Data->LastErrorStatus);

	return S_OK;
}

HANDLE WINAPI 
FindFirstChangeNotificationEnter(
	IN LPCTSTR lpPathName,
	IN BOOL bWatchSubtree,
	IN DWORD dwNotifyFilter
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDFIRSTCHANGENOTIFICATION Filter; 
	FINDFIRSTCHANGENOTIFICATION_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	HANDLE Result;

	BtrFltGetContext(&Context);
	Routine = (FINDFIRSTCHANGENOTIFICATION_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpPathName, bWatchSubtree, dwNotifyFilter);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDFIRSTCHANGENOTIFICATION),
		                          FsGuid, _FindFirstChangeNotification);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDFIRSTCHANGENOTIFICATION); 

	if (lpPathName)
		StringCchCopy(Filter->Path, MAX_PATH, lpPathName);

	Filter->WatchSubTree = bWatchSubtree;
	Filter->Filter = dwNotifyFilter;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
FindFirstChangeNotificationDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDFIRSTCHANGENOTIFICATION Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindFirstChangeNotification);

	Data = (PFS_FINDFIRSTCHANGENOTIFICATION)Record->Data; 
	StringCchPrintf(
			Buffer, MaxCount - 1, 
			L"Path=%ws, WatchSubTree=%d, Filter=0x%08x, Handle=0x%08x, LastErrorStatus=0x%08x", 
			Data->Path, Data->WatchSubTree, Data->Filter, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
FindNextChangeNotificationEnter(
	IN HANDLE hChangeHandle
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDNEXTCHANGENOTIFICATION Filter; 
	FINDNEXTCHANGENOTIFICATION_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (FINDNEXTCHANGENOTIFICATION_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(hChangeHandle);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDNEXTCHANGENOTIFICATION),
		                          FsGuid, _FindNextChangeNotification);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDNEXTCHANGENOTIFICATION); 
	Filter->ChangeHandle = hChangeHandle;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
FindNextChangeNotificationDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDNEXTCHANGENOTIFICATION Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindNextChangeNotification);

	Data = (PFS_FINDNEXTCHANGENOTIFICATION)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"hChangeHandle=0x%08x, Status=%d, LastErrorStatus=0x%08x", 
		Data->ChangeHandle, Data->Result, Data->LastErrorStatus);
	
	return S_OK;
}

DWORD WINAPI 
GetCurrentDirectoryEnter(
	IN DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETCURRENTDIRECTORY Filter; 
	GETCURRENTDIRECTORY_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	DWORD Result;

	BtrFltGetContext(&Context);
	Routine = (GETCURRENTDIRECTORY_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(nBufferLength, lpBuffer);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETCURRENTDIRECTORY),
		                          FsGuid, _GetCurrentDirectory);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETCURRENTDIRECTORY); 
	Filter->Buffer = lpBuffer;
	Filter->BufferLength = nBufferLength;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	if ((lpBuffer != NULL) && (nBufferLength != 0) && (Filter->Result > 0)) {
		StringCchCopy(Filter->Path, MAX_PATH, lpBuffer);
	} 

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
GetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETCURRENTDIRECTORY Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetCurrentDirectory);

	Data = (PFS_GETCURRENTDIRECTORY)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"BufferLength=0x%08x, Buffer=0x%p, Path=%ws, Status=%d, LastErrorStatus=0x%08x", 
		Data->BufferLength, Data->Buffer, Data->Path, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
SetCurrentDirectoryEnter(
	IN LPCTSTR lpPathName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_SETCURRENTDIRECTORY Filter; 
	SETCURRENTDIRECTORY_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (SETCURRENTDIRECTORY_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpPathName);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_SETCURRENTDIRECTORY),
		                          FsGuid, _SetCurrentDirectory);

	Filter = FILTER_RECORD_POINTER(Record, FS_SETCURRENTDIRECTORY); 

	if (lpPathName)
		StringCchCopy(Filter->Path, MAX_PATH, lpPathName);

	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
SetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_SETCURRENTDIRECTORY Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _SetCurrentDirectory);

	Data = (PFS_SETCURRENTDIRECTORY)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"Path=%ws, Status=%d, LastErrorStatus=0x%08x", 
		Data->Path, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
ReadDirectoryChangesWEnter(
	IN HANDLE hDirectory,
	OUT LPVOID lpBuffer,
	IN DWORD nBufferLength,
	IN BOOL bWatchSubtree,
	IN DWORD dwNotifyFilter,
	OUT LPDWORD lpBytesReturned,
	IN LPOVERLAPPED lpOverlapped,
	IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_READDIRECTORYCHANGESW Filter; 
	READDIRECTORYCHANGESW_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (READDIRECTORYCHANGESW_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(hDirectory, lpBuffer, nBufferLength, bWatchSubtree,
		                dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_READDIRECTORYCHANGESW),
		                          FsGuid, _ReadDirectoryChangesW);

	Filter = FILTER_RECORD_POINTER(Record, FS_READDIRECTORYCHANGESW); 
	Filter->hDirectory = hDirectory;
	Filter->lpBuffer = lpBuffer;
	Filter->nBufferLength = nBufferLength;
	Filter->bWatchSubtree = bWatchSubtree;
	Filter->dwNotifyFilter = dwNotifyFilter;
	Filter->lpBytesReturned = lpBytesReturned;
	Filter->lpOverlapped = lpOverlapped;
	Filter->lpCompletionRoutine = lpCompletionRoutine;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
ReadDirectoryChangesWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_READDIRECTORYCHANGESW Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _ReadDirectoryChangesW);

	Data = (PFS_READDIRECTORYCHANGESW)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"hDirectory=0x%p, Buffer=0x%p, BufferLength=0x%08x, WatchSubtree=%d, Filter=0x%x,"
		L"lpBytesReturned=0x%x, Overlapped=0x%p, CompletionRoutine=0x%p, Status=%d, LastErrorStatus=0x%08x", 
		Data->hDirectory, Data->lpBuffer, Data->nBufferLength, Data->bWatchSubtree,
		Data->dwNotifyFilter, Data->lpBytesReturned, Data->lpOverlapped, Data->lpCompletionRoutine,
		Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
GetDiskFreeSpaceEnter(
	IN PWSTR lpRootPathName,
	OUT LPDWORD lpSectorsPerCluster,
	OUT LPDWORD lpBytesPerSector,
	OUT LPDWORD lpNumberOfFreeClusters,
	OUT LPDWORD lpTotalNumberOfClusters
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETDISKFREESPACE Filter; 
	GETDISKFREESPACE_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (GETDISKFREESPACE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(lpRootPathName, lpSectorsPerCluster, lpBytesPerSector, 
		                lpNumberOfFreeClusters, lpTotalNumberOfClusters);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETDISKFREESPACE),
		                          FsGuid, _GetDiskFreeSpace);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETDISKFREESPACE); 

	if (lpRootPathName)
		StringCchCopy(Filter->RootPathName, MAX_PATH, lpRootPathName);

	Filter->lpSectorsPerCluster = lpSectorsPerCluster;
	Filter->lpBytesPerSector = lpBytesPerSector;
	Filter->lpNumberOfFreeClusters = lpNumberOfFreeClusters;
	Filter->lpTotalNumberOfClusters = lpTotalNumberOfClusters;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
GetDiskFreeSpaceDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETDISKFREESPACE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetDiskFreeSpace);

	Data = (PFS_GETDISKFREESPACE)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"RootPath=%ws, lpSectorsPerCluster=0x%x, lpBytesPerSector=0x%x, lpNumberOfFreeClusters=%d, lpTotalNumberOfClusters=%d"
		L"Status=%d, LastErrorStatus=0x%08x", 
		Data->RootPathName, Data->lpSectorsPerCluster, Data->lpBytesPerSector, 
		Data->lpNumberOfFreeClusters, Data->lpTotalNumberOfClusters, 
		Data->Result, Data->LastErrorStatus);

	return S_OK;
}

HANDLE WINAPI 
FindFirstVolumeEnter(
	OUT LPTSTR lpszVolumeName,
	IN  DWORD cchBufferLength
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDFIRSTVOLUME Filter; 
	FINDFIRSTVOLUME_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	HANDLE VolumeHandle;

	BtrFltGetContext(&Context);
	Routine = (FINDFIRSTVOLUME_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	VolumeHandle = (*Routine)(lpszVolumeName, cchBufferLength);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDFIRSTVOLUME),
		                          FsGuid, _FindFirstVolume);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDFIRSTVOLUME); 
	Filter->lpszVolumeName = lpszVolumeName;
	Filter->BufferLength = cchBufferLength;

	if (Filter->VolumeHandle != INVALID_HANDLE_VALUE && lpszVolumeName != NULL) {
		StringCchCopy(Filter->VolumeName, MAX_PATH, lpszVolumeName);
	}

	Filter->VolumeHandle = VolumeHandle;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return VolumeHandle;
}

ULONG
FindFirstVolumeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDFIRSTVOLUME Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindFirstVolume);
		
	Data = (PFS_FINDFIRSTVOLUME)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"lpszVolumeName=0x%p (%ws), cchBufferLength=0x%x, Return=0x%p, LastErrorStatus=0x%08x",
		Data->lpszVolumeName, Data->VolumeName, Data->BufferLength, Data->VolumeHandle, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
FindNextVolumeEnter(
	IN HANDLE hFindVolume,
	OUT LPTSTR lpszVolumeName,
	IN DWORD cchBufferLength
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDNEXTVOLUME Filter; 
	FINDNEXTVOLUME_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Result;

	BtrFltGetContext(&Context);
	Routine = (FINDNEXTVOLUME_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Result = (*Routine)(hFindVolume, lpszVolumeName, cchBufferLength);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDNEXTVOLUME),
		                          FsGuid, _FindNextVolume);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDNEXTVOLUME); 
	Filter->hFindVolume = hFindVolume;
	Filter->lpszVolumeName = lpszVolumeName;
	Filter->BufferLength = cchBufferLength;
	Filter->Result = Result;
	Filter->LastErrorStatus = GetLastError();	                        

	if (Filter->Result && lpszVolumeName) {
		StringCchCopy(Filter->VolumeName, MAX_PATH, lpszVolumeName);
	}

	BtrFltQueueRecord(Record);
	return Result;
}

ULONG
FindNextVolumeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDNEXTVOLUME Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindNextVolume);
		
	Data = (PFS_FINDNEXTVOLUME)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"hFindVolume=0x%p, lpszVolumeName=0x%p (%ws), cchBufferLength=0x%x, Return=%d, LastErrorStatus=0x%08x",
		Data->hFindVolume, Data->lpszVolumeName, Data->VolumeName, Data->BufferLength, Data->Result, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
FindVolumeCloseEnter(
	IN HANDLE hFindVolume
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_FINDVOLUMECLOSE Filter; 
	FINDVOLUMECLOSE_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Return;

	BtrFltGetContext(&Context);
	Routine = (FINDVOLUMECLOSE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)(hFindVolume);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_FINDVOLUMECLOSE),
		                          FsGuid, _FindVolumeClose);

	Filter = FILTER_RECORD_POINTER(Record, FS_FINDVOLUMECLOSE); 
	Filter->hFindVolume = hFindVolume;
	Filter->Return = Return;
	Filter->LastErrorStatus = GetLastError();	                        

	BtrFltQueueRecord(Record);
	return Return;
}

ULONG 
FindVolumeCloseDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_FINDVOLUMECLOSE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _FindVolumeClose);

	Data = (PFS_FINDVOLUMECLOSE)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"hFindVolume=0x%p, Return=%d, LastErrorStatus=0x%08x",
		Data->hFindVolume, Data->Return, Data->LastErrorStatus);

	return S_OK;
}

BOOL WINAPI 
GetVolumeInformationEnter(
	IN  LPCTSTR lpRootPathName,
	OUT LPTSTR lpVolumeNameBuffer,
	IN  DWORD nVolumeNameSize,
	OUT LPDWORD lpVolumeSerialNumber,
	OUT LPDWORD lpMaximumComponentLength,
	OUT LPDWORD lpFileSystemFlags,
	OUT LPTSTR lpFileSystemNameBuffer,
	IN  DWORD nFileSystemNameSize
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETVOLUMEINFORMATION Filter; 
	GETVOLUMEINFORMATION_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	BOOL Return;

	BtrFltGetContext(&Context);
	Routine = (GETVOLUMEINFORMATION_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)(lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize,
		                lpVolumeSerialNumber, lpMaximumComponentLength,
						lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETVOLUMEINFORMATION),
		                          FsGuid, _GetVolumeInformation);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETVOLUMEINFORMATION); 
	Filter->lpRootPathName = lpRootPathName;
	Filter->lpVolumeNameBuffer = lpVolumeNameBuffer;
	Filter->nVolumeNameSize = nVolumeNameSize;
	Filter->lpVolumeSerialNumber = lpVolumeSerialNumber;
	Filter->lpMaximumComponentLength = lpMaximumComponentLength;
	Filter->lpFileSystemFlags = lpFileSystemFlags;
	Filter->lpFileSystemNameBuffer = lpFileSystemNameBuffer;
	Filter->nFileSystemNameSize = nFileSystemNameSize;
	Filter->Return = Return;
	Filter->LastErrorStatus = GetLastError();	                        

	if (Filter->Return) {

		__try {
			if (lpVolumeNameBuffer)
				StringCchCopy(Filter->VolumeName, 64, lpVolumeNameBuffer);

			if (lpVolumeSerialNumber)
				Filter->VolumeSerialNumber = *lpVolumeSerialNumber;

			if (lpMaximumComponentLength)
				Filter->MaximumComponentLength = *lpMaximumComponentLength;

			if (lpFileSystemFlags)
				Filter->FileSystemFlags = *lpFileSystemFlags;

			if (lpFileSystemNameBuffer)
				StringCchCopy(Filter->FileSystemName, MAX_PATH, lpFileSystemNameBuffer);
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
		}
	}

	BtrFltQueueRecord(Record);
	return Return;
}

ULONG
GetVolumeInformationDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETVOLUMEINFORMATION Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetVolumeInformation);

	Data = (PFS_GETVOLUMEINFORMATION)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"lpRootPathName=0x%p (%ws), lpVolumeNameBuffer=0x%p (%ws), nVolumeNameSize=%d"
		L"lpVolumeSerialNumber=0x%p (0x%x), lpMaximumComponentLength=0x%p (%d)"
		L"lpFileSystemFlags=0x%p (0x%x), lpFileSystemNameBuffer=0x%p (%ws), nFileSystemNameSize=%d"
		L"Return=%d, LastErrorStatus=0x%08x",
		Data->lpRootPathName, Data->RootPathName, Data->lpVolumeNameBuffer, Data->VolumeName, 
		Data->nVolumeNameSize, Data->lpVolumeSerialNumber, Data->VolumeSerialNumber, 
		Data->lpMaximumComponentLength, Data->MaximumComponentLength, 
		Data->lpFileSystemFlags, Data->FileSystemFlags, Data->lpFileSystemNameBuffer,
		Data->FileSystemName, Data->nFileSystemNameSize, Data->Return, Data->LastErrorStatus);

	return S_OK;
}

UINT WINAPI 
GetDriveTypeEnter(
	IN LPCTSTR lpRootPathName
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETDRIVETYPE Filter; 
	GETDRIVETYPE_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	UINT Return;
	ULONG LastError;

	BtrFltGetContext(&Context);
	Routine = (GETDRIVETYPE_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)(lpRootPathName);
	BtrFltSetContext(&Context);
	LastError = GetLastError();	                        

	Record = BtrFltAllocateRecord(sizeof(FS_GETDRIVETYPE),
		                          FsGuid, _GetDriveType);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETDRIVETYPE); 
	Filter->Return = Return;
	Filter->LastErrorStatus = LastError;

	__try {
		if (lpRootPathName) {
			StringCchCopy(Filter->RootPathName, MAX_PATH, lpRootPathName);
		}
	}
	__except(1) {

	}

	BtrFltQueueRecord(Record);
	return Return;
}

ULONG
GetDriveTypeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETDRIVETYPE Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetDriveType);
		
	Data = (PFS_GETDRIVETYPE)Record->Data; 
	StringCchPrintf(
		Buffer, MaxCount - 1, 
		L"lpRootPathName=0x%p (%ws), Return=%d, LastErrorStatus=0x%08x",
		Data->lpRootPathName, Data->RootPathName, Data->Return, Data->LastErrorStatus);

	return S_OK;
}

DWORD WINAPI 
GetLogicalDrivesEnter(
	VOID
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETLOGICALDRIVES Filter; 
	GETLOGICALDRIVES_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	DWORD Return;

	BtrFltGetContext(&Context);
	Routine = (GETLOGICALDRIVES_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)();
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETLOGICALDRIVES),
		                          FsGuid, _GetLogicalDrives);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETLOGICALDRIVES); 
	Filter->LastErrorStatus = GetLastError();	                        
	Filter->Return = Return;

	BtrFltQueueRecord(Record);
	return Return;
}

ULONG
GetLogicalDrivesDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETLOGICALDRIVES Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetLogicalDrives);

	Data = (PFS_GETLOGICALDRIVES)Record->Data; 
	StringCchPrintf(Buffer, MaxCount - 1, 
					L"Return=0x%08x, LastErrorStatus=0x%08x", 
					Data->Return, Data->LastErrorStatus);
	return S_OK;
}

DWORD WINAPI 
GetLogicalDriveStringsEnter(
	IN DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	)
{
	PBTR_FILTER_RECORD Record;
	PFS_GETLOGICALDRIVESTRINGS Filter; 
	GETLOGICALDRIVESTRINGS_ROUTINE Routine; 
	BTR_PROBE_CONTEXT Context;
	DWORD Return;

	BtrFltGetContext(&Context);
	Routine = (GETLOGICALDRIVESTRINGS_ROUTINE)Context.Destine;
	ASSERT(Routine != NULL);

	Return = (*Routine)(nBufferLength, lpBuffer);
	BtrFltSetContext(&Context);

	Record = BtrFltAllocateRecord(sizeof(FS_GETLOGICALDRIVESTRINGS),
		                          FsGuid, _GetLogicalDriveStrings);

	Filter = FILTER_RECORD_POINTER(Record, FS_GETLOGICALDRIVESTRINGS); 
	Filter->Return = Return;
	Filter->LastErrorStatus = GetLastError();	                        

	if (Filter->Return && lpBuffer != NULL) {
		StringCchCopy(Filter->Buffer, MAX_PATH, lpBuffer);
	}

	BtrFltQueueRecord(Record);
	return Return;
}


ULONG
GetLogicalDriveStringsDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	)
{
	PFS_GETLOGICALDRIVESTRINGS Data;

	ASSERT(Record != NULL);
	ASSERT(Record->ProbeOrdinal == _GetLogicalDriveStrings);

	Data = (PFS_GETLOGICALDRIVESTRINGS)Record->Data; 
	StringCchPrintf(Buffer, MaxCount - 1, 
					L"BufferLength=%d, lpBuffer=0x%p (%ws), Return=%d, LastErrorStatus=0x%08x", 
					Data->nBufferLength, Data->lpBuffer, Data->Buffer, Data->Return, Data->LastErrorStatus);
	return S_OK;
}