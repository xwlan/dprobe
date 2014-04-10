//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012 
// 

#include "bsp.h"
#include <stdio.h>
#include <psapi.h>
#include <strsafe.h>
#include "ntapi.h"

PVOID
BspMapImageFile(
	IN PWSTR ModuleName,
	OUT PHANDLE FileHandle,
	OUT PHANDLE ViewHandle
	)
{
	PIMAGE_DOS_HEADER MappedBase;
	PIMAGE_NT_HEADERS NtHeadersVa;

	if (ModuleName == NULL || FileHandle == NULL || ViewHandle == NULL) {
		return NULL;
	}

	*FileHandle = CreateFile(ModuleName, GENERIC_READ, 
							 FILE_SHARE_READ, NULL, 
							 OPEN_EXISTING, 0, 0);

	if (*FileHandle == INVALID_HANDLE_VALUE) {
		*ViewHandle = NULL;
		return NULL;
	}

	*ViewHandle = CreateFileMapping(*FileHandle, NULL, PAGE_READONLY,
									0, 0, NULL);

	if (*ViewHandle == NULL) {
		CloseHandle(*FileHandle);
		*FileHandle = INVALID_HANDLE_VALUE;
		return NULL;
	}

	MappedBase = (PIMAGE_DOS_HEADER) MapViewOfFile(*ViewHandle, FILE_MAP_READ, 0, 0, 0);
	if (MappedBase == NULL) {
		CloseHandle(*ViewHandle);
		CloseHandle(*FileHandle);
		return NULL;
	}

	//
	// Simple validation of PE signature
	//

	if (MappedBase->e_magic != IMAGE_DOS_SIGNATURE) {
		UnmapViewOfFile(MappedBase);
		CloseHandle(*ViewHandle);
		CloseHandle(*FileHandle);
		return NULL;
	}

	NtHeadersVa = (PIMAGE_NT_HEADERS)((ULONG_PTR)MappedBase + (ULONG_PTR)MappedBase->e_lfanew);
	if (NtHeadersVa->Signature != IMAGE_NT_SIGNATURE) {
		UnmapViewOfFile(MappedBase);
		CloseHandle(*ViewHandle);
		CloseHandle(*FileHandle);
		return NULL;
	}

	return MappedBase;
}

VOID
BspUnmapImageFile(
	IN PVOID  MappedBase,
	IN HANDLE FileHandle,
	IN HANDLE ViewHandle
	)
{
	UnmapViewOfFile(MappedBase);
	CloseHandle(ViewHandle);
	CloseHandle(FileHandle);
}

ULONG
BspRvaToOffset(
	IN ULONG Rva,
	IN PIMAGE_SECTION_HEADER Headers,
	IN ULONG NumberOfSections
	)
{
	ULONG i;

	for(i = 0; i < NumberOfSections; i++) {
		if(Rva >= Headers->VirtualAddress) {
			if(Rva < Headers->VirtualAddress + Headers->Misc.VirtualSize) {
				return (ULONG)(Rva - Headers->VirtualAddress + Headers->PointerToRawData);
			}
		}
		Headers += 1;
	}

	return (ULONG)-1;
}
