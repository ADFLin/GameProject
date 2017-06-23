#ifndef Clock_h__
#define Clock_h__

#include "WindowsHeader.h"

class TClock
{
public:
	TClock();

	~TClock()
	{
	}

	/// Resets the initial reference time.
	void reset();

	/// Returns the time in ms since the last call to reset or since 
	/// the btClock was created.
	unsigned long int getTimeMilliseconds();
	unsigned long int getTimeMicroseconds();

private:
	LARGE_INTEGER mClockFrequency;
	DWORD         mStartTick;
	LONGLONG      mPrevElapsedTime;
	LARGE_INTEGER mStartTime;

};



#endif // Clock_h__