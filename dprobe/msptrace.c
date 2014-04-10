//
// Apsara Labs
// lan.john@gmail.com
// Copyright(C) 2009-2011
//

#include "msptrace.h"
#include "mspdefs.h"
#include "msputility.h"

#if defined (_M_IX86)
	#define TRACE_BUFFER_SIZE  (1024 * 1024 * 16)
#elif defined (_M_X64)
	#define TRACE_BUFFER_SIZE  (1024 * 1024 * 16 * 4)
#endif

#define BTR_RUNTIME_NAME  L"dprobe.btr.dll"
#define PAGESIZE  4096
