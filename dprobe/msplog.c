//
// lan.john@gmail.com
// Apsara Labs
// Copyright(C) 2009-2012
//

#include "mspdefs.h"
#include "msputility.h"
#include "mspprocess.h"
#include <stdlib.h>

DECLSPEC_CACHEALIGN
MSP_LOG MspLogObject;

CHAR MspLogHead[] = "# time  pid  level  message";
ULONG MspLogHeadSize = sizeof(MspLogHead);

CHAR *LogLevelString[] = {
	"info",
	"error",
};

VOID
MspWriteLogEntry(
	IN ULONG ProcessId,
	IN MSP_LOG_LEVEL Level,
	IN PSTR Format,
	...
	)
{
	ULONG IsLogging;
	va_list Argument;
	PMSP_LOG_ENTRY Entry;
	CHAR Buffer[MSP_LOG_TEXT_LIMIT];

	IsLogging = InterlockedCompareExchange(&MspLogObject.Flag, 0, 0);
	if (!IsLogging) {
		return;
	}

	va_start(Argument, Format);
	StringCchVPrintfA(Buffer, MAX_PATH, Format, Argument);
	va_end(Argument);

	Entry = (PMSP_LOG_ENTRY)MspMalloc(sizeof(MSP_LOG_ENTRY));
	Entry->ProcessId = ProcessId;

	GetLocalTime(&Entry->TimeStamp);
	StringCchCopyA(Entry->Text, MSP_LOG_TEXT_LIMIT, Buffer);

	EnterCriticalSection(&MspLogObject.Lock);
	InsertTailList(&MspLogObject.ListHead, &Entry->ListEntry);
	MspLogObject.ListDepth += 1;
	LeaveCriticalSection(&MspLogObject.Lock);

	return;
}

ULONG
MspCurrentLogListDepth(
	VOID
	)
{
	ULONG Depth;

	Depth = InterlockedCompareExchange(&MspLogObject.ListDepth, 0, 0);
	return Depth;
}

ULONG CALLBACK
MspLogProcedure(
	IN PVOID Context
	)
{
	PLIST_ENTRY ListEntry;
	PMSP_LOG_ENTRY LogEntry;
	ULONG IsLogging;
	ULONG Length;
	PCHAR Buffer;
	ULONG Complete;

	IsLogging = InterlockedCompareExchange(&MspLogObject.Flag, 0, 0);
	if (!IsLogging) {
		return 0;
	}

	EnterCriticalSection(&MspLogObject.Lock);

	if (IsListEmpty(&MspLogObject.ListHead)) {
		LeaveCriticalSection(&MspLogObject.Lock);
		return 0;
	}

	//
	// Hand over queued log entries to QueueListHead
	//

	Buffer = _alloca(4096);

	while (IsListEmpty(&MspLogObject.ListHead) != TRUE) {

		ListEntry = RemoveHeadList(&MspLogObject.ListHead);
		LogEntry = CONTAINING_RECORD(ListEntry, MSP_LOG_ENTRY, ListEntry);

		StringCchPrintfA(Buffer, 4096, "\r\n%02d:%02d:%02d:%03d %04x %s %s", 
			    LogEntry->TimeStamp.wHour, LogEntry->TimeStamp.wMinute, 
				LogEntry->TimeStamp.wSecond, LogEntry->TimeStamp.wMilliseconds, 
				LogEntry->ProcessId, LogLevelString[LogEntry->Level], LogEntry->Text); 

		Length = (ULONG) strlen(Buffer);
		WriteFile(MspLogObject.FileObject, Buffer, Length, &Complete, NULL);
		MspFree(LogEntry);
	}

	MspLogObject.ListDepth = 0;
	LeaveCriticalSection(&MspLogObject.Lock);
	return 0;
}

ULONG
MspInitializeLog(
	IN ULONG Flag	
	)
{
	SYSTEMTIME Time;
	HANDLE Handle;
	ULONG Complete;
	LARGE_INTEGER Size;

	RtlZeroMemory(&MspLogObject, sizeof(MspLogObject));

	//
	// If logging is not enabled, do nothing
	//

	if (!FlagOn(Flag, MSP_FLAG_LOGGING)) {
		return MSP_STATUS_OK;
	}

	InitializeCriticalSection(&MspLogObject.Lock);
	InitializeListHead(&MspLogObject.ListHead);
	MspLogObject.ListDepth = 0;

	//
	// Create log file based on process information and time
	//

	GetLocalTime(&Time);
	StringCchCopyA(MspLogObject.Path, MAX_PATH, "dprobe.log");

	Handle = CreateFileA(MspLogObject.Path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ,
		                 NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (Handle == INVALID_HANDLE_VALUE) {
		DeleteCriticalSection(&MspLogObject.Lock);
		return MSP_STATUS_CREATEFILE;
	}

	GetFileSizeEx(Handle, &Size);

	//
	// If it's a new file, add a format description 
	//

	if (Size.QuadPart == 0) {
		WriteFile(Handle, MspLogHead, MspLogHeadSize, &Complete, NULL);
	}
	else {

		//
		// Adjust file pointer to end of file
		//

		Size.QuadPart = 0;
		SetFilePointerEx(Handle, Size, NULL, FILE_END);
	}

	MspLogObject.FileObject = Handle;
	MspLogObject.Flag = 1;
	return MSP_STATUS_OK;
}

ULONG
MspUninitializeLog(
	VOID
	)
{
	if (!MspLogObject.Flag) {
		return MSP_STATUS_OK;
	}

	InterlockedExchange(&MspLogObject.Flag, 0);

	CloseHandle(MspLogObject.FileObject);
	MspLogObject.FileObject = NULL;

	DeleteCriticalSection(&MspLogObject.Lock);
	return MSP_STATUS_OK;
}

VOID
MspGetLogFilePath(
	OUT PCHAR Buffer,
	IN ULONG Length
	)
{
	StringCchCopyA(Buffer, Length, MspLogObject.Path);
}

BOOLEAN
MspIsLoggingEnabled(
	VOID
	)
{
	return (BOOLEAN)FlagOn(MspLogObject.Flag, MSP_FLAG_LOGGING);
}