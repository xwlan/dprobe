#include "bsp.h"
#include <strsafe.h>

VOID
DebugTrace(
	IN PSTR Format,
	...
	)
{
#ifdef _DEBUG

	va_list arg;
	char format[512];
	char buffer[512];
	
	va_start(arg, Format);

	StringCchVPrintfA(format, 512, Format, arg);
	StringCchPrintfA(buffer, 512, "[dprobe]: %s\n", format);
	OutputDebugStringA(buffer);
	
	va_end(arg);

#endif
}