#pragma once
#ifndef SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
#define SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB

#include "PlatformConfig.h"
#include "Core/IntegerType.h"
#include "Serialize/SerializeFwd.h"
#include "Template/ArrayView.h"

enum class EDateMonth
{
	Junuary ,
	February ,
	March ,
	April ,
	May ,
	June ,
	July ,
	August ,
	September ,
	October,
	November ,
	December ,
};

enum class EDateWeekOfDay
{
	Sunday = 0,
	Monday ,
	Tuesday ,
	Wednesday ,
	Thursday ,
	Friday ,
	Saturday ,
};

struct TimeSpan
{
	int64 usec;
};
struct DateTime
{
public:
	DateTime()
	{
		mCachedDayOfWeek = -1;
	}


	DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 sec, int32 millisecond)
	{
		mYear = year;
		mMonth = month;
		mDay = day;
		mHour = hour;
		mMinute = minute;
		mSecond = sec;
		mMillisecond = millisecond;
		mCachedDayOfWeek = -1;
	}
	int32 getYear() const { return mYear; }
	int32 getMonth() const { return mMonth; }
	int32 getDay() const { return mDay; }
	EDateWeekOfDay getDayOfWeek() const
	{ 
		if( mCachedDayOfWeek == -1 )
		{
			//TODO
		}
		return EDateWeekOfDay(mCachedDayOfWeek); 
	}
	int32 getHour() const { return mHour; }
	int32 getMinute() const { return mMinute; }
	int32 getSecond() const { return mSecond; }
	int32 getMillisecond() const { return mMillisecond; }

	bool operator == (DateTime const& rhs) const
	{
		return mYear == rhs.mYear &&
		   mMonth == rhs.mMonth &&
		   mDay == rhs.mDay &&
		   mHour == rhs.mHour &&
		   mMinute == rhs.mMinute &&
		   mSecond == rhs.mSecond &&
		   mMillisecond == rhs.mMillisecond;
	}



	bool operator < (DateTime const& rhs) const
	{
#define CMP( VAR )\
	if ( VAR < rhs.VAR ) return true;\
	if ( rhs.VAR < VAR ) return false;

		CMP(mYear);
		CMP(mMonth);
		CMP(mDay);
		CMP(mHour);
		CMP(mMinute);
		CMP(mSecond);

#undef CMP
		return mMillisecond < rhs.mMillisecond;
	}

	bool operator > (DateTime const& rhs) const
	{
#define CMP( VAR )\
	if ( VAR > rhs.VAR ) return true;\
	if ( rhs.VAR > VAR ) return false;

		CMP(mYear);
		CMP(mMonth);
		CMP(mDay);
		CMP(mHour);
		CMP(mMinute);
		CMP(mSecond);

#undef CMP
		return mMillisecond > rhs.mMillisecond;
	}


	bool operator != (DateTime const& rhs) const { return !(*this == rhs); }
	bool operator >= (DateTime const& rhs) const { return !(*this < rhs); }
	bool operator <= (DateTime const& rhs) const { return !(*this > rhs); }

private:

	int32 mYear;
	uint8 mMonth;
	mutable int8 mCachedDayOfWeek;
	uint8 mDay;
	uint8 mHour;
	uint8 mMinute;
	uint8 mSecond;
	int32 mMillisecond;
};

TYPE_SERIALIZE_AS_RAW_DATA(DateTime);

struct OpenFileFilterInfo
{
	char const* desc;
	char const* pattern;
};

class SystemPlatform
{
public:
	static int    GetProcessorNumber();
	static void   MemoryStoreFence();
	static void   Sleep( uint32 millionSecond );

	static double  GetHighResolutionTime();


	static char const* GetEnvironmentVariable( char const* key );
	static int32    AtomExchange(volatile int32* ptr, int32 value);
	static int64    AtomExchange(volatile int64* ptr, int64 value);
	static int32    AtomExchangeAdd(volatile int32* ptr, int32 value);
	static int64    AtomExchangeAdd(volatile int64* ptr, int64 value);
	static int32    AtomCompareExchange(volatile int32* ptr, int32 exchange, int32 comperand);
	static int64    AtomCompareExchange(volatile int64* ptr, int64 exchange , int64 comperand);
	static int32    AtomAdd(volatile int32* ptr, int32 value);
	static int64    AtomAdd(volatile int64* ptr, int64 value);
	static int32    AtomIncrement(volatile int32* ptr);
	static int64    AtomIncrement(volatile int64* ptr);
	static int32    AtomDecrement(volatile int32* ptr);
	static int64    AtomDecrement(volatile int64* ptr);
	static int32    AtomicRead(volatile int32* ptr);
	static int64    AtomicRead(volatile int64* ptr);

	static DateTime GetUTCTime();
	static DateTime GetLocalTime();

	static bool OpenFileName(char inoutPath[] , int pathSize, TArrayView< OpenFileFilterInfo const > filters, char const* initDir = nullptr, char const* title = nullptr , void* windowOwner = nullptr);
	static bool OpenDirectoryName(char outPath[], int pathSize, char const* initDir = nullptr , char const* title = nullptr, void* windowOwner = nullptr);
	static int64 GetTickCount();
};


struct ScopeTickCount
	{
	ScopeTickCount(int64& time):mTime(time)
	{
		mTime = SystemPlatform::GetTickCount();
	}
	~ScopeTickCount()
	{
		mTime = SystemPlatform::GetTickCount() - mTime;
	}

	int64& mTime;
};
#endif // SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
