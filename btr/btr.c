//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2012
//

#include "btr.h"
#include "thread.h"
#include "probe.h"
#include "hal.h"
#include "ipc.h"
#include "util.h"
#include "filter.h"
#include "heap.h"
#include "sched.h"
#include "stktrace.h"
#include "cache.h"
#include "trap.h"
#include "call.h"
#include "queue.h"

DECLSPEC_CACHEALIGN
ULONG BtrUnloading;

ULONG BtrIsInitialized;
ULONG BtrIsStarted;
ULONG BtrFeatures;
BOOLEAN BtrIsNativeHost;

#define BTR_MAJOR_VERSION 1
#define BTR_MINOR_VERSION 0

VOID
BtrVersion(
	OUT PUSHORT MajorVersion,
	OUT PUSHORT MinorVersion
	)
{
	*MajorVersion = BTR_MAJOR_VERSION;	
	*MinorVersion = BTR_MINOR_VERSION;	
}

ULONG
BtrInitialize(
	IN HMODULE DllHandle
	)
{
	ULONG Status;

	BtrDllBase = (ULONG_PTR)DllHandle;
	BtrFeatures = FeatureLibrary;

	//
	// Initialize Hal subsystem
	//

	Status = BtrInitializeHal();
	if (Status != S_OK) {
		return Status;
	}
	
	//
	// Current release don't support probe native process like csrss.exe,
	// it will crash intermittently
	//

	if (BtrIsNativeHost) {
		return S_FALSE;
	}

	//
	// Initialize Mm subsystem
	//

	Status = BtrInitializeMm(BTR_INIT_STAGE_0);
	if (Status != S_OK) {
		BtrUninitializeHal();
		return Status;
	}

	BtrInitializeQueue();

	//
	// Initialize Trap subsystem
	//

	Status = BtrInitializeTrap();
	if (Status != S_OK) {
		return Status;
	}

	//
	// Intialize Probe subsystem
	//

	Status = BtrInitializeProbe();
	if (Status != S_OK) {
		return Status;
	}

	//
	// Initialize Filter subsystem
	//

	Status = BtrInitializeFlt();
	return Status;
}

ULONG WINAPI
BtrFltStartRuntime(
	IN PULONG Features 
	)
{
	ULONG Status;

	if (BtrIsStarted) {
		return S_OK;
	}

	if (!FlagOn(*Features, FeatureLocal | FeatureRemote)) {
		return S_FALSE;
	}

	BtrFeatures = *Features;
	Status = BtrInitializeMm(BTR_INIT_STAGE_1);
	if (Status != S_OK) {
		return Status;
	}
	
	Status = BtrInitializeStackTrace(TRUE);
	if (Status != S_OK) {
		return Status;
	}

	Status = BtrInitializeThread();
	if (Status != S_OK) {
		return Status;
	}

	BtrIsStarted = TRUE;
	return S_OK;
}

ULONG WINAPI
BtrFltStopRuntime(
	IN PVOID Reserved
	)
{
	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		BtrUninitializeHal();
		BtrUninitializeMm();
		return S_OK;
	}

	if (FlagOn(BtrFeatures, FeatureLocal | FeatureRemote)) {
		return BtrStopRuntime(ThreadUser);
	}

	return S_OK;
}

ULONG WINAPI
BtrFltCurrentFeature(
	VOID
	)
{
	return BtrFeatures;
}

ULONG
BtrStopRuntime(
	IN ULONG Reason	
	)
{
	PBTR_THREAD Thread;
	PBTR_CONTROL Control;
	PBTR_NOTIFICATION_PACKET Notification;

	Thread = BtrGetCurrentThread();
	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	Control = (PBTR_CONTROL)BtrMalloc(sizeof(BTR_CONTROL));
	Control->ControlCode = SCHED_STOP_RUNTIME;
	Control->AckRequired = FALSE;

	Notification = (PBTR_NOTIFICATION_PACKET)BtrMalloc(sizeof(BTR_NOTIFICATION_PACKET));
	Notification->CompleteEvent = NULL;
	Notification->Packet = (PVOID)Control;
	Notification->Requestor = ThreadUser;

	//
	// N.B. Post notification to write procedure, note that this is a asynchronous
	// request, we don't expect return from it. write procedure is responsible to
	// free the allocated packets.
	//

	BtrQueueNotification(&BtrSystemQueue[ThreadSchedule], Notification); 
	return S_OK;
}

LONG CALLBACK
BtrExceptionFilter(
	IN EXCEPTION_POINTERS *Pointers
	)
{
	CONTEXT *Context;
	EXCEPTION_RECORD *Exception;
	
	Context = Pointers->ContextRecord;
	Exception = Pointers->ExceptionRecord;

	return EXCEPTION_EXECUTE_HANDLER;
}

BOOL WINAPI 
DllMain(
	IN HMODULE DllHandle,
    IN ULONG Reason,
	IN PVOID Reserved
	)
{
	BOOL Status;

	switch (Reason) {

	case DLL_PROCESS_ATTACH:
		Status = BtrOnProcessAttach(DllHandle);
		break;

	case DLL_THREAD_ATTACH:
		Status = BtrOnThreadAttach();
		break;

	case DLL_THREAD_DETACH:
		Status = BtrOnThreadDetach();
		break;

	case DLL_PROCESS_DETACH:
		Status = BtrOnProcessDetach();
		break;

	}

	return Status;
}

BOOL
BtrOnThreadAttach(
	VOID
	)
{
	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return TRUE;
	}

	if (BtrIsRuntimeThread(HalCurrentThreadId())){
		return TRUE;
	}

	//
	// Generate thread creation event
	//

	//BtrWriteEvent(EventThreadCreation);
	return TRUE;
}

BOOL
BtrOnThreadDetach(
	VOID
	)
{
	PBTR_THREAD Thread;
	ULONG_PTR Value;

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return TRUE;
	}

	if (BtrIsRuntimeThread(HalCurrentThreadId())){
		return TRUE;
	}

	Thread = (PBTR_THREAD)BtrGetTlsValue();
	if (!Thread) {
		BtrSetExemptionCookie(TRAP_EXEMPTION_COOKIE, &Value);
		return TRUE;
	}

	if (Thread->Type == NullThread) {
		BtrSetExemptionCookie(TRAP_EXEMPTION_COOKIE, &Value);
		return TRUE;
	}

	SetFlag(Thread->ThreadFlag, BTR_FLAG_EXEMPTION);

	__try {

		BtrAcquireLock(&BtrThreadLock);
		RemoveEntryList(&Thread->ListEntry);
		BtrActiveThreadCount -= 1;
		BtrReleaseLock(&BtrThreadLock);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {

		BtrReleaseLock(&BtrThreadLock);
	}

	//
	// N.B. Check FrameDepth ensure we don't prematurely clean thread state 
	//

	if (Thread->FrameDepth == 0) {

		BtrCloseMappingPerThread(Thread);

		if (Thread->RundownHandle != NULL) {
			CloseHandle(Thread->RundownHandle);
			Thread->RundownHandle = NULL;
		}

		if (Thread->Buffer) {
			BtrFree(Thread->Buffer);
			Thread->Buffer = NULL;
		}

		BtrFreeThreadLookaside(Thread);
	} 

	else {
	
		//
		// If there's pending calls for current thread,
		// queue thread object into rundown queue
		//

		SetFlag(Thread->ThreadFlag, BTR_FLAG_RUNDOWN);
		BtrQueueRundownThread(Thread);

	}

	//
	// N.B. Must set exemption cookie first, because BtrFree will trap
	// into runtime, and however, the thread object is being freed!
	//

	BtrSetExemptionCookie(TRAP_EXEMPTION_COOKIE, &Value);

	//
	// For normal thread, we always make its thread object live,
	// even it's terminating, because thread shutdown process still
	// can trap into runtime, we just set its thread object as
	// null thread object to exempt all probes.
	//

	return TRUE;
}

BOOL
BtrOnProcessAttach(
	IN HMODULE DllHandle	
	)
{
	ULONG Status;

	if (BtrIsInitialized != TRUE) {

		Status = BtrInitialize(DllHandle);
		if (Status != S_OK) {
			return FALSE;
		}

		BtrIsInitialized = TRUE;
	}

	return TRUE;

}

BOOL
BtrOnProcessDetach(
	VOID
	)
{
	InterlockedExchange(&BtrUnloading, TRUE);

	if (FlagOn(BtrFeatures, FeatureLibrary)) {
		return TRUE;
	}

	if (FlagOn(BtrFeatures, FeatureLocal|FeatureRemote)) {
		if (BtrTlsIndex != TLS_OUT_OF_INDEXES) {
			TlsFree(BtrTlsIndex);
		}
	}
	return TRUE;
}