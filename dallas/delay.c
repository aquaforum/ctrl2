#include "../platform.h"

#if defined(_WINDOWS_) || defined(_WINDOWS_CE_)
#include <windows.h>
#endif

#if defined(_LINUX_) || defined(_LINUX_EMBEDDED_)
#include <unistd.h>
#endif

#include "delay.h"

#if defined(_WINDOWS_) || defined(_WINDOWS_CE_)
static __int64 freq;

void delay_us_(u32 delayval)
{
	__int64 cnt, stop_time;
	if (!freq)
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&stop_time);
	stop_time += freq * delayval / 1000000;
	for(cnt = 0; cnt < stop_time; QueryPerformanceCounter((LARGE_INTEGER*)&cnt));
}

void delay_ms(u32 delayval)
{
	Sleep(delayval);
}
#endif

#if defined(_LINUX_) || defined(_LINUX_EMBEDDED_)
void delay_us_(u32 delayval)
{
    usleep(delayval);
}

void delay_ms(u32 delayval)
{
    usleep(delayval*1000);
}
#endif
