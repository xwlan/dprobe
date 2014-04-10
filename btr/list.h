//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#ifndef _LIST_H_
#define _LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

VOID
InitializeListHead(
    IN PLIST_ENTRY ListHead
	);

BOOLEAN
IsListEmpty(
	IN PLIST_ENTRY ListHead 
	);

BOOLEAN
RemoveEntryList(
    IN PLIST_ENTRY Entry
    );

PLIST_ENTRY
RemoveHeadList(
    IN PLIST_ENTRY ListHead
    );

PLIST_ENTRY
RemoveTailList(
    IN PLIST_ENTRY ListHead
    );

VOID
InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    );

VOID 
InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    );

PSINGLE_LIST_ENTRY 
PopEntryList(
    IN PSINGLE_LIST_ENTRY  ListHead
    );

VOID 
PushEntryList(
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY Entry
    );

#ifdef __cplusplus
}
#endif

#endif