//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012 
// 

#include "find.h"
#include "mspdtl.h"

BOOLEAN
FindIsComplete(
	IN struct _FIND_CONTEXT *Context,
	IN struct _MSP_DTL_OBJECT *DtlObject,
	IN ULONG Current
	)
{
	PMSP_FILTER_FILE_OBJECT FileObject;
	ULONG Count;

	if (Context->FindForward) {
		
		FileObject = MspGetFilterFileObject(DtlObject);

		if (!FileObject) {
			Count = MspGetRecordCount(DtlObject);
		} else {
			Count = MspGetFilteredRecordCount(FileObject);
		}

		if (Count > Current) {
			return FALSE;
		}

		return TRUE;
	}

	if (Current != -1) {
		return FALSE;
	}
	
	return TRUE;
}