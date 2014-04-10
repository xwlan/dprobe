//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _FSFLT_H_
#define _FSFLT_H_

//
// New API requires higher WINNT version
//

#define _WIN32_WINNT 0x0600

#define _BTR_SDK_ 
#include "btrsdk.h"
#include <strsafe.h>

#ifndef ASSERT
 #include <assert.h>
 #define ASSERT assert
#endif

#ifdef __cplusplus 
extern "C" {
#endif

//
// GUID to identify file system filter 
//

extern GUID FsGuid;

//
// N.B. These ordinal numbers must match probe registration array
//

typedef enum _FS_PROBE_ORDINAL {
	_CreateFileW,
	_ReadFile,
	_WriteFile,
	_DeviceIoControl,
	_DeleteFile,
	_CopyFile,
	_MoveFile,
	_LockFile,
	_UnlockFile,
	_DefineDosDevice,
	_QueryDosDevice,
	_FindFirstFile,
	_FindNextFile,
	_FindClose,
	_GetFileSizeEx,
	_SetFilePointer,
	_SetEndOfFile,
	_SetFileValidData,
	_SetFileAttributes,
	_GetFileAttributes,
	_SetFileInformationByHandle,
	_SearchPath,
	_ReplaceFile,
	_GetTempFileName,
	_GetFileInformationByHandle,
	_CreateHardLink,
	_GetTempPath,
	_CreateDirectory,
	_CreateDirectoryEx,
	_RemoveDirectory,
	_FindFirstChangeNotification,
	_FindNextChangeNotification,
	_GetCurrentDirectory,
	_SetCurrentDirectory,
	_ReadDirectoryChangesW,
	_GetDiskFreeSpace,
	_FindFirstVolume,
	_FindNextVolume,
	_FindVolumeClose,
	_GetVolumeInformation,
	_GetDriveType,
	_GetLogicalDrives,
	_GetLogicalDriveStrings,
	FS_PROBE_NUMBER,
} FS_PROBE_ORDINAL, *PFS_PROBE_ORDINAL;

VOID 
DebugTrace(
	IN PSTR Format,
	...
	);

typedef struct _FS_CREATEFILE {
	WCHAR Name[MAX_PATH];
	ULONG DesiredAccess;
	ULONG ShareMode;
	SECURITY_ATTRIBUTES *Security;
	ULONG CreationDisposition;
	ULONG FlagsAndAttributes;
	HANDLE TemplateFile;
	HANDLE FileHandle;
	ULONG LastErrorStatus;
} FS_CREATEFILE, *PFS_CREATEFILE;

typedef struct _FS_READFILE {
	HANDLE FileHandle;
	PVOID Buffer;
	ULONG ReadBytes;
	PVOID CompleteBytesPtr;
	OVERLAPPED Overlapped;
	BOOL Status;
	ULONG CompleteBytes;
	ULONG LastErrorStatus;
} FS_READFILE, *PFS_READFILE;

typedef struct _FS_WRITEFILE{
	HANDLE FileHandle;
	PVOID Buffer;
	ULONG WriteBytes;
	PVOID CompleteBytesPtr;
	ULONG CompleteBytes;
	OVERLAPPED Overlapped;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_WRITEFILE, *PFS_WRITEFILE;

typedef struct _FS_SETFILEPOINTER {
	HANDLE FileHandle;
	LONG DistanceToMove;
	PLONG DistanceToMoveHighPtr;
	LONG DistanceToMoveHigh;
	ULONG MoveMethod;
	LONG FilePointerLow;
	ULONG LastErrorStatus;
} FS_SETFILEPOINTER, *PFS_SETFILEPOINTER;

typedef struct _FS_SETFILEVALIDDATA {
	HANDLE FileHandle;
	LONGLONG ValidDataLength;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_SETFILEVALIDDATA, *PFS_SETFILEVALIDDATA;

typedef struct _FS_SETENDOFFILE {
	HANDLE FileHandle;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_SETENDOFFILE, *PFS_SETENDOFFILE;

typedef struct _FS_SETFILEATTRIBUTES {
	WCHAR FileName[MAX_PATH];
	ULONG FileAttributes;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_SETFILEATTRIBUTES, *PFS_SETFILEATTRIBUTES;

typedef struct _FS_MOVEFILE {
	WCHAR ExistingFileName[MAX_PATH];
	WCHAR NewFileName[MAX_PATH];
	BOOL Status;
	ULONG LastErrorStatus;
} FS_MOVEFILE, *PFS_MOVEFILE;

typedef struct _FS_LOCKFILE {
	HANDLE FileHandle;
	ULONG FileOffsetLow;
	ULONG FileOffsetHigh;
	ULONG NumberOfBytesToLockLow;
	ULONG NumberOfBytesToLockHigh;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_LOCKFILE, *PFS_LOCKFILE;

typedef struct _FS_UNLOCKFILE {
	HANDLE FileHandle;
	ULONG FileOffsetLow;
	ULONG FileOffsetHigh;
	ULONG NumberOfBytesToUnlockLow;
	ULONG NumberOfBytesToUnlockHigh;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_UNLOCKFILE, *PFS_UNLOCKFILE;

typedef struct _FS_GETFILESIZEEX {
	HANDLE FileHandle;
	PLARGE_INTEGER SizePtr;
	LARGE_INTEGER Size;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_GETFILESIZEEX, *PFS_GETFILESIZEEX;

typedef struct _FS_GETFILEINFORMATIONBYHANDLE {
	HANDLE FileHandle;
	PVOID InformationPointer;
	BY_HANDLE_FILE_INFORMATION Information;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_GETFILEINFORMATIONBYHANDLE, *PFS_GETFILEINFORMATIONBYHANDLE;

typedef struct _FS_GETFILEATTRIBUTES {
	WCHAR FileName[MAX_PATH];
	ULONG Attributes;
	ULONG LastErrorStatus;
} FS_GETFILEATTRIBUTES, *PFS_GETFILEATTRIBUTES;

typedef struct _FS_DELETEFILE {
	WCHAR FileName[MAX_PATH];
	BOOL Status;
	ULONG LastErrorStatus;
} FS_DELETEFILE, *PFS_DELETEFILE;

typedef struct _FS_CREATESYMBOLICLINK {
	WCHAR SymlinkFileName[MAX_PATH];
	WCHAR TargetFileName[MAX_PATH];
	ULONG Flag;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_CREATESYMBOLICLINK, *PFS_CREATESYMBOLICLINK;

typedef struct _FS_COPYFILE {
	WCHAR ExistingFileName[MAX_PATH];
	WCHAR NewFileName[MAX_PATH];
	BOOL FailIfExists;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_COPYFILE, *PFS_COPYFILE;

typedef struct _FS_FINDFIRSTFILE {
	WCHAR FileName[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	WIN32_FIND_DATA *FindFileDataPointer;
	HANDLE FileHandle;
	ULONG LastErrorStatus;
} FS_FINDFIRSTFILE, *PFS_FINDFIRSTFILE;

typedef struct _FS_FINDNEXTFILE {
	HANDLE FileHandle;
	WIN32_FIND_DATA FindFileData;
	WIN32_FIND_DATA *FindFileDataPointer;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_FINDNEXTFILE, *PFS_FINDNEXTFILE;

typedef struct _FS_FINDCLOSE {
	HANDLE FileHandle;
	BOOL Status;
	ULONG LastErrorStatus;
} FS_FINDCLOSE, *PFS_FINDCLOSE;

typedef struct _FS_DEFINEDOSDEVICE {
	ULONG Flag;
	WCHAR DeviceName[MAX_PATH];
	WCHAR TargetPath[MAX_PATH];
	BOOL Status;
	ULONG LastErrorStatus;
} FS_DEFINEDOSDEVICE, *PFS_DEFINEDOSDEVICE;

typedef struct _FS_QUERYDOSDEVICE {
	WCHAR DeviceName[MAX_PATH];
	WCHAR TargetPath[MAX_PATH];
	PWCHAR TargetPathPointer;
	ULONG MaxCharCount;
	ULONG StoredCharCount;
	ULONG LastErrorStatus;
} FS_QUERYDOSDEVICE, *PFS_QUERYDOSDEVICE;

typedef struct _FS_SETFILEINFORMATIONBYHANDLE {
	HANDLE FileHandle;
	FILE_INFO_BY_HANDLE_CLASS FileInformationClass;
	LPVOID lpFileInformation;
    DWORD dwBufferSize;
	WCHAR Buffer[MAX_PATH];
	BOOL Status;
	ULONG LastErrorStatus;
} FS_SETFILEINFORMATIONBYHANDLE, *PFS_SETFILEINFORMATIONBYHANDLE;

typedef struct _FS_GETTEMPPATH {
	ULONG BufferLength;
	PVOID BufferPointer;
	WCHAR Buffer[MAX_PATH];
	ULONG Result;
	ULONG LastErrorStatus;
} FS_GETTEMPPATH, *PFS_GETTEMPPATH;

typedef struct _FS_CREATEHARDLINK {
	WCHAR FileName[MAX_PATH];
	WCHAR ExistingFileName[MAX_PATH];
	PVOID SecurityAttributes;
	BOOL  Result;
	ULONG LastErrorStatus;
} FS_CREATEHARDLINK, *PFS_CREATEHARDLINK;

typedef struct _FS_GETTEMPFILENAME {
	WCHAR PathName[MAX_PATH];
	WCHAR PrefixString[64]; 
	ULONG UniqueId;
	WCHAR TempFileName[MAX_PATH];
	ULONG Result;
	ULONG LastErrorStatus;
} FS_GETTEMPFILENAME, *PFS_GETTEMPFILENAME;

typedef struct _FS_REPLACEFILE {
	WCHAR ReplacedFileName[MAX_PATH];
	WCHAR ReplacementFileName[MAX_PATH];
	WCHAR BackupFileName[MAX_PATH];
	ULONG dwReplaceFlags;
	PVOID Exclude;
	PVOID Reserved;
	BOOL Result;
	ULONG LastErrorStatus;
} FS_REPLACEFILE, *PFS_REPLACEFILE;

typedef struct _FS_SEARCHPATH {
	WCHAR Path[MAX_PATH];
	WCHAR FileName[MAX_PATH];
	WCHAR Extension[8];
	ULONG BufferLength;
	PVOID BufferPointer;
	PVOID FilePartPointer;
	WCHAR ResultFile[MAX_PATH];
	ULONG Result;
	ULONG LastErrorStatus;
} FS_SEARCHPATH, *PFS_SEARCHPATH;

//
// CreateFile
//

HANDLE WINAPI
CreateFileEnter(
	IN PWSTR lpFileName,
	IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
	);

typedef HANDLE 
(WINAPI *CREATEFILE_ROUTINE)(
	IN PWSTR lpFileName,
	IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
	);

ULONG 
CreateFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// ReadFile
//

BOOL WINAPI
ReadFileEnter(
	IN HANDLE FileHandle,
	IN PVOID Buffer,
	IN ULONG ReadBytes,
	OUT PULONG CompleteBytes,
	IN OVERLAPPED *Overlapped
	);

typedef BOOL 
(WINAPI *READFILE_ROUTINE)(
	IN HANDLE FileHandle,
	IN PVOID Buffer,
	IN ULONG ReadBytes,
	OUT PULONG CompleteBytes,
	IN OVERLAPPED *Overlapped
	);

ULONG 
ReadFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// WriteFile
//

BOOL WINAPI 
WriteFileEnter(
	IN  HANDLE hFile,
	IN  LPVOID lpBuffer,
	IN  DWORD nNumberOfBytesToWrite,
	OUT LPDWORD lpNumberOfBytesWritten,
	IN  LPOVERLAPPED lpOverlapped
	);

typedef BOOL 
(WINAPI *WRITEFILE_ROUTINE)(
	IN  HANDLE hFile,
	IN  LPVOID lpBuffer,
	IN  DWORD nNumberOfBytesToWrite,
	OUT LPDWORD lpNumberOfBytesWritten,
	IN  LPOVERLAPPED lpOverlapped
	);

ULONG 
WriteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// DeviceIoControl
//

typedef struct _FS_DEVICEIOCONTROL {
	ULONG TotalLength;
	HANDLE hDevice;
	DWORD dwIoControlCode;
	LPVOID lpInBuffer;
	DWORD nInBufferSize;
	LPVOID lpOutBuffer;
	DWORD nOutBufferSize;
	LPDWORD lpBytesReturned;
	LPOVERLAPPED lpOverlapped;
	USHORT InSize  : 8;
	USHORT OutSize : 8;
	UCHAR InBuffer[16];
	UCHAR OutBuffer[16];
} FS_DEVICEIOCONTROL, *PFS_DEVICEIOCONTROL;

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
	);

typedef BOOL 
(WINAPI *DEVICEIOCONTROL_ROUTINE)(
	__in  HANDLE hDevice,
	__in  DWORD dwIoControlCode,
	__in  LPVOID lpInBuffer,
	__in  DWORD nInBufferSize,
	__out LPVOID lpOutBuffer,
	__in  DWORD nOutBufferSize,
	__out LPDWORD lpBytesReturned,
	__in  LPOVERLAPPED lpOverlapped
	);

ULONG
DeviceIoControlDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);
	
//
// SetFilePointer
//

DWORD WINAPI 
SetFilePointerEnter(
	IN  HANDLE hFile,
	IN  LONG lDistanceToMove,
	OUT PLONG lpDistanceToMoveHigh,
	IN  DWORD dwMoveMethod
	);

typedef DWORD 
(WINAPI *SETFILEPOINTER_ROUTINE)(
	IN  HANDLE hFile,
	IN  LONG lDistanceToMove,
	OUT PLONG lpDistanceToMoveHigh,
	IN  DWORD dwMoveMethod
	);

ULONG 
SetFilePointerDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// SetFileValidData
//

BOOL WINAPI 
SetFileValidDataEnter(
	IN HANDLE hFile,
	IN LONGLONG ValidDataLength
	);

typedef BOOL
(WINAPI *SETFILEVALIDDATA_ROUTINE)(
	IN HANDLE hFile,
	IN LONGLONG ValidDataLength
	);

ULONG 
SetFileValidDataDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// SetEndOfFile
//

BOOL WINAPI 
SetEndOfFileEnter(
	IN HANDLE hFile
	);

typedef BOOL
(WINAPI *SETENDOFFILE_ROUTINE)(
	IN HANDLE hFile
	);

ULONG 
SetEndOfFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// SetFileAttributes
//

BOOL WINAPI 
SetFileAttributesEnter(
	IN LPCWSTR lpFileName,
	IN DWORD dwFileAttributes
	);

typedef BOOL
(WINAPI *SETFILEATTRIBUTES_ROUTINE)(
	IN LPCWSTR lpFileName,
	IN DWORD dwFileAttributes
	);

ULONG 
SetFileAttributesDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// MoveFile
//

BOOL WINAPI 
MoveFileEnter(
	IN LPCWSTR lpExistingFileName,
	IN LPCWSTR lpNewFileName
	);

typedef BOOL
(WINAPI *MOVEFILE_ROUTINE)(
	IN LPCWSTR lpExistingFileName,
	IN LPCWSTR lpNewFileName
	);

ULONG 
MoveFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// LockFile
//

BOOL WINAPI 
LockFileEnter(
	IN HANDLE hFile,
	IN DWORD dwFileOffsetLow,
	IN DWORD dwFileOffsetHigh,
	IN DWORD nNumberOfBytesToLockLow,
	IN DWORD nNumberOfBytesToLockHigh
	);

typedef BOOL
(WINAPI *LOCKFILE_ROUTINE)(
	IN HANDLE hFile,
	IN DWORD dwFileOffsetLow,
	IN DWORD dwFileOffsetHigh,
	IN DWORD nNumberOfBytesToLockLow,
	IN DWORD nNumberOfBytesToLockHigh
	);

ULONG 
LockFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetFileSizeEx 
//

BOOL WINAPI 
GetFileSizeExEnter(
	IN  HANDLE hFile,
	OUT PLARGE_INTEGER lpFileSize
	);

typedef BOOL
(WINAPI *GETFILESIZEEX_ROUTINE)(
	IN  HANDLE hFile,
	OUT PLARGE_INTEGER lpFileSize
	);

ULONG 
GetFileSizeExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetFileInformationByHandle
//

BOOL WINAPI 
GetFileInformationByHandleEnter(
	IN  HANDLE hFile,
	OUT LPBY_HANDLE_FILE_INFORMATION lpFileInformation
	);

typedef BOOL
(WINAPI *GETFILEINFORMATIONBYHANDLE_ROUTINE)(
	IN  HANDLE hFile,
	OUT LPBY_HANDLE_FILE_INFORMATION lpFileInformation
	);

ULONG 
GetFileInformationByHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetFileAttributes
//

DWORD WINAPI 
GetFileAttributesEnter(
	IN LPCWSTR lpFileName
	);

typedef DWORD
(WINAPI *GETFILEATTRIBUTES_ROUTINE)(
	IN LPCWSTR lpFileName
	);

ULONG 
GetFileAttributesDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// DeleteFile
//

BOOL WINAPI 
DeleteFileEnter(
	IN LPCWSTR lpFileName
	);

typedef BOOL 
(WINAPI *DELETEFILE_ROUTINE)(
	IN LPCWSTR lpFileName
	);

ULONG 
DeleteFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// CreateSymbolicLink
//

BOOL WINAPI 
CreateSymbolicLinkEnter(
	IN LPCWSTR lpSymlinkFileName,
	IN LPCWSTR lpTargetFileName,
	IN DWORD dwFlags
	);

typedef BOOL
(WINAPI *CREATESYMBOLICLINK_ROUTINE)(
	IN LPCWSTR lpSymlinkFileName,
	IN LPCWSTR lpTargetFileName,
	IN DWORD dwFlags
	);

ULONG
CreateSymbolicLinkDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// CopyFile
//

BOOL WINAPI 
CopyFileEnter(
	IN LPCWSTR lpExistingFileName,
	IN LPCWSTR lpNewFileName,
	IN BOOL bFailIfExists
	);

typedef BOOL
(WINAPI *COPYFILE_ROUTINE)(
	IN LPCWSTR lpExistingFileName,
	IN LPCWSTR lpNewFileName,
	IN BOOL bFailIfExists
	);

ULONG
CopyFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindFirstFile
//


HANDLE WINAPI 
FindFirstFileEnter(
	IN  LPCWSTR lpFileName,
	OUT LPWIN32_FIND_DATA lpFindFileData
	);

typedef HANDLE
(WINAPI *FINDFIRSTFILE_ROUTINE)(
	IN  LPCWSTR lpFileName,
	OUT LPWIN32_FIND_DATA lpFindFileData
	);

ULONG
FindFirstFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindNextFile
//

BOOL WINAPI 
FindNextFileEnter(
	IN  HANDLE hFindFile,
	OUT LPWIN32_FIND_DATA lpFindFileData
	);

typedef BOOL
(WINAPI *FINDNEXTFILE_ROUTINE)(
	IN  HANDLE hFindFile,
	OUT LPWIN32_FIND_DATA lpFindFileData
	);

ULONG
FindNextFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindClose
//

BOOL WINAPI
FindCloseEnter(
	IN HANDLE hFile
	);

typedef BOOL 
(WINAPI *FINDCLOSE_ROUTINE)(
	IN HANDLE hFile
	);

ULONG
FindCloseDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// DefineDosDevice
//

BOOL WINAPI 
DefineDosDeviceEnter(
	IN DWORD dwFlags,
	IN LPCWSTR lpDeviceName,
	IN LPCWSTR lpTargetPath
	);

typedef BOOL 
(WINAPI *DEFINEDOSDEVICE_ROUTINE)(
	IN DWORD dwFlags,
	IN LPCWSTR lpDeviceName,
	IN LPCWSTR lpTargetPath
	);

ULONG
DefineDosDeviceDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// QueryDosDevice
//

DWORD WINAPI 
QueryDosDeviceEnter(
	IN  LPCWSTR lpDeviceName,
	OUT LPWSTR lpTargetPath,
	IN  DWORD ucchMax
	);

typedef DWORD 
(WINAPI *QUERYDOSDEVICE_ROUTINE)(
	IN  LPCWSTR lpDeviceName,
	OUT LPWSTR lpTargetPath,
	IN  DWORD ucchMax
	);

ULONG
QueryDosDeviceDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// UnlockFile
//

BOOL WINAPI 
UnlockFileEnter(
	IN HANDLE hFile,
	IN DWORD dwFileOffsetLow,
	IN DWORD dwFileOffsetHigh,
	IN DWORD nNumberOfBytesToUnlockLow,
	IN DWORD nNumberOfBytesToUnlockHigh
	);

typedef BOOL 
(WINAPI *UNLOCKFILE_ROUTINE)(
	IN HANDLE hFile,
	IN DWORD dwFileOffsetLow,
	IN DWORD dwFileOffsetHigh,
	IN DWORD nNumberOfBytesToUnlockLow,
	IN DWORD nNumberOfBytesToUnlockHigh
	);

ULONG
UnlockFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// SetFileInformationByHandle
//

BOOL WINAPI 
SetFileInformationByHandleEnter(
	IN HANDLE hFile,
	IN FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
	IN LPVOID lpFileInformation,
	IN DWORD dwBufferSize
	);

typedef BOOL 
(WINAPI *SETFILEINFORMATIONBYHANDLE_ROUTINE)(
	IN HANDLE hFile,
	IN FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
	IN LPVOID lpFileInformation,
	IN DWORD dwBufferSize
	);

ULONG
SetFileInformationByHandleDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// SearchPath
//

DWORD WINAPI 
SearchPathEnter(
	IN  LPCTSTR lpPath,
	IN  LPCTSTR lpFileName,
	IN  LPCTSTR lpExtension,
	IN  DWORD nBufferLength,
	OUT LPTSTR lpBuffer,
	OUT LPTSTR* lpFilePart
	);

typedef DWORD 
(WINAPI *SEARCHPATH_ROUTINE)(
	IN  LPCTSTR lpPath,
	IN  LPCTSTR lpFileName,
	IN  LPCTSTR lpExtension,
	IN  DWORD nBufferLength,
	OUT LPTSTR lpBuffer,
	OUT LPTSTR* lpFilePart
	);

ULONG
SearchPathDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// ReplaceFile
//

BOOL WINAPI 
ReplaceFileEnter(
	IN LPCTSTR lpReplacedFileName,
	IN LPCTSTR lpReplacementFileName,
	IN LPCTSTR lpBackupFileName,
	IN DWORD dwReplaceFlags,
	IN LPVOID lpExclude,  
	IN LPVOID lpReserved  
	);

typedef BOOL
(WINAPI *REPLACEFILE_ROUTINE)(
	IN LPCTSTR lpReplacedFileName,
	IN LPCTSTR lpReplacementFileName,
	IN LPCTSTR lpBackupFileName,
	IN DWORD dwReplaceFlags,
	IN LPVOID lpExclude, 
	IN LPVOID lpReserved 
	);

ULONG
ReplaceFileDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetTempFileName
//

UINT WINAPI 
GetTempFileNameEnter(
	IN LPCTSTR lpPathName,
	IN LPCTSTR lpPrefixString,
	IN UINT uUnique,
	OUT LPTSTR lpTempFileName
	);

typedef UINT
(WINAPI *GETTEMPFILENAME_ROUTINE)(
	IN LPCTSTR lpPathName,
	IN LPCTSTR lpPrefixString,
	IN UINT uUnique,
	OUT LPTSTR lpTempFileName
	);

ULONG
GetTempFileNameDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// CreateHardLink
//

BOOL WINAPI 
CreateHardLinkEnter(
	IN LPCTSTR lpFileName,
	IN LPCTSTR lpExistingFileName,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

typedef BOOL 
(WINAPI *CREATEHARDLINK_ROUTINE)(
	IN LPCTSTR lpFileName,
	IN LPCTSTR lpExistingFileName,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

ULONG
CreateHardLinkDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetTempPath
//

DWORD WINAPI 
GetTempPathEnter(
	IN  DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	);

typedef DWORD 
(WINAPI *GETTEMPPATH_ROUTINE)(
	IN  DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	);

ULONG
GetTempPathDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// CreateDirectory
//

typedef struct _FS_CREATEDIRECTORY {
	WCHAR Path[MAX_PATH];
	PVOID SecurityAttributes;
	BOOL Result;
	ULONG LastErrorStatus;
} FS_CREATEDIRECTORY, *PFS_CREATEDIRECTORY;

BOOL WINAPI 
CreateDirectoryEnter(
	IN LPCTSTR lpPathName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

typedef BOOL 
(WINAPI *CREATEDIRECTORY_ROUTINE)(
	IN LPCTSTR lpPathName,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

ULONG
CreateDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// CreateDirectoryEx
//

typedef struct _FS_CREATEDIRECTORYEX {
	WCHAR TemplatePath[MAX_PATH];
	WCHAR NewPath[MAX_PATH];
	PVOID SecurityAttributes;
	BOOL Result;
	ULONG LastErrorStatus;
} FS_CREATEDIRECTORYEX, *PFS_CREATEDIRECTORYEX;

BOOL WINAPI 
CreateDirectoryExEnter(
	IN LPCTSTR lpTemplateDirectory, 
	IN LPCTSTR lpNewDirectory,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

typedef BOOL 
(WINAPI *CREATEDIRECTORYEX_ROUTINE)(
	IN LPCTSTR lpTemplateDirectory, 
	IN LPCTSTR lpNewDirectory,
	IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
	);

ULONG
CreateDirectoryExDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// RemoveDirectory
//

typedef struct _FS_REMOVEDIRECTORY {
	WCHAR Directory[MAX_PATH];
	BOOL Status;
	ULONG LastErrorStatus;
} FS_REMOVEDIRECTORY, *PFS_REMOVEDIRECTORY;

BOOL WINAPI 
RemoveDirectoryEnter(
	IN LPCTSTR lpPathName
	);

typedef BOOL 
(WINAPI *REMOVEDIRECTORY_ROUTINE)(
	IN LPCTSTR lpPathName
	);

ULONG
RemoveDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindFirstChangeNotification
//

typedef struct _FS_FINDFIRSTCHANGENOTIFICATION {
	WCHAR Path[MAX_PATH];
	BOOL WatchSubTree;
	ULONG Filter;
	HANDLE Result;
	ULONG LastErrorStatus;
} FS_FINDFIRSTCHANGENOTIFICATION, *PFS_FINDFIRSTCHANGENOTIFICATION;

HANDLE WINAPI 
FindFirstChangeNotificationEnter(
	IN LPCTSTR lpPathName,
	IN BOOL bWatchSubtree,
	IN DWORD dwNotifyFilter
	);

typedef HANDLE 
(WINAPI *FINDFIRSTCHANGENOTIFICATION_ROUTINE)(
	IN LPCTSTR lpPathName,
	IN BOOL bWatchSubtree,
	IN DWORD dwNotifyFilter
	);

ULONG
FindFirstChangeNotificationDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindNextChangeNotification
//

typedef struct _FS_FINDNEXTCHANGENOTIFICATION {
	HANDLE ChangeHandle;
	BOOL Result;
	ULONG LastErrorStatus;
} FS_FINDNEXTCHANGENOTIFICATION, *PFS_FINDNEXTCHANGENOTIFICATION;

BOOL WINAPI 
FindNextChangeNotificationEnter(
	IN HANDLE hChangeHandle
	);

typedef BOOL 
(WINAPI *FINDNEXTCHANGENOTIFICATION_ROUTINE)(
	IN HANDLE hChangeHandle
	);

ULONG
FindNextChangeNotificationDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetCurrentDirectory
//

typedef struct _FS_GETCURRENTDIRECTORY {
	PVOID Buffer;
	ULONG BufferLength;
	WCHAR Path[MAX_PATH];
	ULONG Result;
	ULONG LastErrorStatus;
} FS_GETCURRENTDIRECTORY, *PFS_GETCURRENTDIRECTORY;

DWORD WINAPI 
GetCurrentDirectoryEnter(
	IN DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	);

typedef DWORD 
(WINAPI *GETCURRENTDIRECTORY_ROUTINE)(
	IN DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	);

ULONG
GetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// SetCurrentDirectory
//

typedef struct _FS_SETCURRENTDIRECTORY {
	WCHAR Path[MAX_PATH];
	BOOL Result;
	ULONG LastErrorStatus;
} FS_SETCURRENTDIRECTORY, *PFS_SETCURRENTDIRECTORY;

BOOL WINAPI 
SetCurrentDirectoryEnter(
	IN LPCTSTR lpPathName
	);

typedef BOOL 
(WINAPI *SETCURRENTDIRECTORY_ROUTINE)(
	IN LPCTSTR lpPathName
	);

ULONG
SetCurrentDirectoryDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// ReadDirectoryChangesW
//

typedef struct _FS_READDIRECTORYCHANGESW {
	HANDLE hDirectory;
	LPVOID lpBuffer;
	DWORD nBufferLength;
	BOOL bWatchSubtree;
	DWORD dwNotifyFilter;
	LPDWORD lpBytesReturned;
	LPOVERLAPPED lpOverlapped;
	LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine;
	BOOL Result;
	ULONG LastErrorStatus;
} FS_READDIRECTORYCHANGESW, *PFS_READDIRECTORYCHANGESW;

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
	);

typedef BOOL 
(WINAPI *READDIRECTORYCHANGESW_ROUTINE)(
	IN HANDLE hDirectory,
	OUT LPVOID lpBuffer,
	IN DWORD nBufferLength,
	IN BOOL bWatchSubtree,
	IN DWORD dwNotifyFilter,
	OUT LPDWORD lpBytesReturned,
	OUT LPOVERLAPPED lpOverlapped,
	OUT LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);

ULONG
ReadDirectoryChangesWDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetDiskFreeSpace
//

typedef struct _FS_GETDISKFREESPACE{
	WCHAR RootPathName[MAX_PATH];
	PULONG lpSectorsPerCluster;
	PULONG lpBytesPerSector;
	PULONG lpNumberOfFreeClusters;
	PULONG lpTotalNumberOfClusters;
	BOOL Result;
	ULONG LastErrorStatus;
} FS_GETDISKFREESPACE, *PFS_GETDISKFREESPACE;

BOOL WINAPI 
GetDiskFreeSpaceEnter(
	IN PWSTR lpRootPathName,
	OUT LPDWORD lpSectorsPerCluster,
	OUT LPDWORD lpBytesPerSector,
	OUT LPDWORD lpNumberOfFreeClusters,
	OUT LPDWORD lpTotalNumberOfClusters
	);

typedef BOOL 
(WINAPI *GETDISKFREESPACE_ROUTINE)(
	IN LPCTSTR lpRootPathName,
	OUT LPDWORD lpSectorsPerCluster,
	OUT LPDWORD lpBytesPerSector,
	OUT LPDWORD lpNumberOfFreeClusters,
	OUT LPDWORD lpTotalNumberOfClusters
	);

ULONG
GetDiskFreeSpaceDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindFirstVolume
//

typedef struct _FS_FINDFIRSTVOLUME{ 
	PVOID lpszVolumeName;
	ULONG BufferLength; 
	HANDLE VolumeHandle;
	WCHAR VolumeName[MAX_PATH];
	ULONG LastErrorStatus;
} FS_FINDFIRSTVOLUME, *PFS_FINDFIRSTVOLUME;

HANDLE WINAPI 
FindFirstVolumeEnter(
	OUT LPTSTR lpszVolumeName,
	IN  DWORD cchBufferLength
	);

typedef HANDLE 
(WINAPI *FINDFIRSTVOLUME_ROUTINE)(
	OUT LPTSTR lpszVolumeName,
	IN  DWORD cchBufferLength
	);

ULONG
FindFirstVolumeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindNextVolume
//

typedef struct _FS_FINDNEXTVOLUME{ 
	HANDLE hFindVolume;
	PVOID lpszVolumeName;
	WCHAR VolumeName[MAX_PATH];
	ULONG BufferLength; 
	BOOL Result;
	ULONG LastErrorStatus;
} FS_FINDNEXTVOLUME, *PFS_FINDNEXTVOLUME;

BOOL WINAPI 
FindNextVolumeEnter(
	IN HANDLE hFindVolume,
	OUT LPTSTR lpszVolumeName,
	IN DWORD cchBufferLength
	);

typedef BOOL 
(WINAPI *FINDNEXTVOLUME_ROUTINE)(
	IN HANDLE hFindVolume,
	OUT LPTSTR lpszVolumeName,
	IN DWORD cchBufferLength
	);

ULONG
FindNextVolumeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// FindVolumeClose
//

typedef struct _FS_FINDVOLUMECLOSE {
	HANDLE hFindVolume;
	BOOL Return;
	ULONG LastErrorStatus;
} FS_FINDVOLUMECLOSE, *PFS_FINDVOLUMECLOSE;

BOOL WINAPI 
FindVolumeCloseEnter(
	IN HANDLE hFindVolume
	);

typedef BOOL
(WINAPI *FINDVOLUMECLOSE_ROUTINE)(
	IN HANDLE hFindVolume
	);

ULONG
FindVolumeCloseDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetVolumeInformation
//

typedef struct _FS_GETVOLUMEINFORMATION { 
	LPCTSTR lpRootPathName;
	WCHAR RootPathName[64];
	LPTSTR lpVolumeNameBuffer;
	WCHAR VolumeName[64];
	ULONG nVolumeNameSize;
	PULONG lpVolumeSerialNumber;
	ULONG VolumeSerialNumber;
	PULONG lpMaximumComponentLength; 
	ULONG MaximumComponentLength; 
	PULONG lpFileSystemFlags;
	ULONG FileSystemFlags;
	LPTSTR lpFileSystemNameBuffer;
	WCHAR FileSystemName[MAX_PATH];
	ULONG nFileSystemNameSize;
	BOOL Return;
	ULONG LastErrorStatus;
} FS_GETVOLUMEINFORMATION, *PFS_GETVOLUMEINFORMATION;

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
	);

typedef BOOL 
(WINAPI *GETVOLUMEINFORMATION_ROUTINE)(
	IN  LPCTSTR lpRootPathName,
	OUT LPTSTR lpVolumeNameBuffer,
	IN  DWORD nVolumeNameSize,
	OUT LPDWORD lpVolumeSerialNumber,
	OUT LPDWORD lpMaximumComponentLength,
	OUT LPDWORD lpFileSystemFlags,
	OUT LPTSTR lpFileSystemNameBuffer,
	IN  DWORD nFileSystemNameSize
	);

ULONG
GetVolumeInformationDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetDriveType
//

typedef struct _FS_GETDRIVETYPE {
	LPCTSTR lpRootPathName;
	WCHAR RootPathName[MAX_PATH];
	UINT Return;
	ULONG LastErrorStatus;
} FS_GETDRIVETYPE, *PFS_GETDRIVETYPE;

UINT WINAPI 
GetDriveTypeEnter(
	IN LPCTSTR lpRootPathName
	);

typedef UINT
(WINAPI *GETDRIVETYPE_ROUTINE)(
	IN LPCTSTR lpRootPathName
	);
	
ULONG
GetDriveTypeDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetLogicalDrives
//

typedef struct _FS_GETLOGICALDRIVES {
	DWORD Return;
	ULONG LastErrorStatus;
} FS_GETLOGICALDRIVES, *PFS_GETLOGICALDRIVES;

DWORD WINAPI 
GetLogicalDrivesEnter(
	VOID
	);

typedef DWORD
(WINAPI *GETLOGICALDRIVES_ROUTINE)(
	VOID
	);

ULONG
GetLogicalDrivesDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

//
// GetLogicalDriveStrings
//

typedef struct _FS_GETLOGICALDRIVESTRINGS {
	DWORD nBufferLength;
	LPTSTR lpBuffer;
	WCHAR Buffer[MAX_PATH];
	DWORD Return;
	ULONG LastErrorStatus;
} FS_GETLOGICALDRIVESTRINGS, *PFS_GETLOGICALDRIVESTRINGS;

DWORD WINAPI 
GetLogicalDriveStringsEnter(
	IN DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	);

typedef DWORD 
(WINAPI *GETLOGICALDRIVESTRINGS_ROUTINE)(
	IN DWORD nBufferLength,
	OUT LPTSTR lpBuffer
	);

ULONG
GetLogicalDriveStringsDecode(
	IN PBTR_FILTER_RECORD Record,
	IN ULONG MaxCount,
	OUT PWCHAR Buffer
	);

#ifdef __cplusplus 
}
#endif

#endif
