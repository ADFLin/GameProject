#include "Clock.h"

#include <algorithm>

TClock::TClock()
{
	QueryPerformanceFrequency(&mClockFrequency);
	reset();
}

unsigned long int TClock::getTimeMicroseconds()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	LONGLONG elapsedTime = currentTime.QuadPart - 
		mStartTime.QuadPart;

	// Compute the number of millisecond ticks elapsed.
	unsigned long msecTicks = (unsigned long)(1000 * elapsedTime / 
		mClockFrequency.QuadPart);

	// Check for unexpected leaps in the Win32 performance counter.  
	// (This is caused by unexpected data across the PCI to ISA 
	// bridge, aka south bridge.  See Microsoft KB274323.)
	unsigned long elapsedTicks = GetTickCount() - mStartTick;
	signed long msecOff = (signed long)(msecTicks - elapsedTicks);
	if (msecOff < -100 || msecOff > 100)
	{
		// Adjust the starting time forwards.
		LONGLONG msecAdjustment = std::min(msecOff * 
			mClockFrequency.QuadPart / 1000, elapsedTime - 
			mPrevElapsedTime);
		mStartTime.QuadPart += msecAdjustment;
		elapsedTime -= msecAdjustment;
	}

	// Store the current elapsed time for adjustments next time.
	mPrevElapsedTime = elapsedTime;

	// Convert to microseconds.
	unsigned long usecTicks = (unsigned long)(1000000 * elapsedTime / 
		mClockFrequency.QuadPart);

	return usecTicks;
}

void TClock::reset()
{
	QueryPerformanceCounter(&mStartTime);
	mStartTick = GetTickCount();
	mPrevElapsedTime = 0;
}
