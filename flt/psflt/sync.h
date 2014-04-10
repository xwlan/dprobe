//
// Reserved
//

HANDLE WINAPI 
CreateEventExEnter(
	__in  LPSECURITY_ATTRIBUTES lpEventAttributes,
	__in  LPCTSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI 
OpenEventEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  LPCTSTR lpName
	);

BOOL WINAPI 
ResetEventEnter(
	__in  HANDLE hEvent
	);

BOOL WINAPI 
SetEventEnter(
	__in  HANDLE hEvent
	);

HANDLE WINAPI 
CreateMutexExEnter(
	__in  LPSECURITY_ATTRIBUTES lpMutexAttributes,
	__in  LPCTSTR lpName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI 
OpenMutexEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  LPCTSTR lpName
	);

BOOL WINAPI 
ReleaseMutexEnter(
	__in  HANDLE hMutex
	);

HANDLE WINAPI 
CreateSemaphoreExEnter(
	__in  LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
	__in  LONG lInitialCount,
	__in  LONG lMaximumCount,
	__in  LPCTSTR lpName,
	DWORD dwFlags,
	__in DWORD dwDesiredAccess
	);

HANDLE WINAPI 
OpenSemaphoreEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  LPCTSTR lpName
	);

BOOL WINAPI 
ReleaseSemaphoreEnter(
	__in  HANDLE hSemaphore,
	__in  LONG lReleaseCount,
	__out LPLONG lpPreviousCount
	);


HANDLE WINAPI 
CreateWaitableTimerExEnter(
	__in  LPSECURITY_ATTRIBUTES lpTimerAttributes,
	__in  LPCTSTR lpTimerName,
	__in  DWORD dwFlags,
	__in  DWORD dwDesiredAccess
	);

HANDLE WINAPI 
OpenWaitableTimerEnter(
	__in  DWORD dwDesiredAccess,
	__in  BOOL bInheritHandle,
	__in  LPCTSTR lpTimerName
	);

BOOL WINAPI 
SetWaitableTimerEnter(
	__in  HANDLE hTimer,
	__in  const LARGE_INTEGER* pDueTime,
	__in  LONG lPeriod,
	__in  PTIMERAPCROUTINE pfnCompletionRoutine,
	__in  LPVOID lpArgToCompletionRoutine,
	__in  BOOL fResume
	);

BOOL WINAPI 
CancelWaitableTimerEnter(
  __in  HANDLE hTimer
);

VOID
WINAPI
InitializeCriticalSectionEnter(
    __out LPCRITICAL_SECTION lpCriticalSection
    );

VOID
WINAPI
EnterCriticalSectionEnter(
    __inout LPCRITICAL_SECTION lpCriticalSection
    );

VOID
WINAPI
LeaveCriticalSectionEnter(
    __inout LPCRITICAL_SECTION lpCriticalSection
    );

BOOL
WINAPI
InitializeCriticalSectionAndSpinCountEnter(
    __out LPCRITICAL_SECTION lpCriticalSection,
    __in  DWORD dwSpinCount
    );

DWORD
WINAPI
SetCriticalSectionSpinCountEnter(
    __inout LPCRITICAL_SECTION lpCriticalSection,
    __in    DWORD dwSpinCount
    );

DWORD WINAPI 
WaitForSingleObjectExEnter(
  __in  HANDLE hHandle,
  __in  DWORD dwMilliseconds,
  __in  BOOL bAlertable
);

DWORD WINAPI 
WaitForMultipleObjectsExEnter(
  __in  DWORD nCount,
  __in  const HANDLE* lpHandles,
  __in  BOOL bWaitAll,
  __in  DWORD dwMilliseconds,
  __in  BOOL bAlertable
);

DWORD WINAPI 
SignalObjectAndWaitEnter(
  __in  HANDLE hObjectToSignal,
  __in  HANDLE hObjectToWaitOn,
  __in  DWORD dwMilliseconds,
  __in  BOOL bAlertable
);

DWORD WINAPI 
MsgWaitForMultipleObjectsExEnter(
  __in  DWORD nCount,
  __in  const HANDLE* pHandles,
  __in  DWORD dwMilliseconds,
  __in  DWORD dwWakeMask,
  __in  DWORD dwFlags
);
