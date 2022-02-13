#ifndef Clock_h__
#define Clock_h__

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif
#include "Core/IntegerType.h"

class HighResClock
{
public:
	HighResClock();

	~HighResClock()
	{
	}

	/// Resets the initial reference time.
	void reset();

	/// Returns the time in ms since the last call to reset or since 
	/// the clock was created.
	uint64 getTimeMilliseconds() { return getTimeMicroseconds() / 1000; }
	uint64 getTimeMicroseconds();

private:

#if SYS_PLATFORM_WIN
	LARGE_INTEGER mClockFrequency;
	DWORD         mStartTick;
	LONGLONG      mPrevElapsedTime;
	LARGE_INTEGER mStartTime;
#endif

};




#endif // Clock_h__