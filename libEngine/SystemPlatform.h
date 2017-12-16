#pragma once
#ifndef SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
#define SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB

#include "PlatformConfig.h"
#include "Core/IntegerType.h"

#undef InterlockedExchange

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
	Sunday ,
	Monday ,
	Tuesday ,
	Wednesday ,
	Thursday ,
	Friday ,
	Saturday ,
};

struct TimeSpan
{
	uint64 usec;
};
struct DateTime
{
public:
	DateTime()
	{

	}
	DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 sec, int32 millisecond)
	{
		mYear = year;
		mMonth = month;
		mDay = day;
		mHour = hour;
		mMinute = minute;
		mSecond = sec;
		mMillisecond = mMillisecond;
	}
	int32 getYear() const { return mYear; }
	int32 getMonth() const { return mMonth; }
	int32 getDay() const { return mDay; }
	int32 getDayOfWeek() const{ return mDayOfWeek; }
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

	bool operator <= (DateTime const& rhs) const
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
		return mMillisecond <= rhs.mMillisecond;
	}


	bool operator != (DateTime const& rhs) const { return !(*this == rhs); }
	bool operator >= (DateTime const& rhs) const { return !(*this < rhs); }
	bool operator > (DateTime const& rhs) const { return !(*this <= rhs); }

private:

	int32 mYear;
	int32 mMonth;
	int32 mDayOfWeek;
	int32 mDay;
	int32 mHour;
	int32 mMinute;
	int32 mSecond;
	int32 mMillisecond;
};
class SystemPlatform
{
public:
	static int    GetProcessorNumber();
	static void   MemoryBarrier();
	static void   Sleep( uint32 millionSecond );

	static double  GetHighResolutionTime();

	static int32    InterlockedExchange(volatile int32* value, int32 exchange);

	static DateTime GetUTCTime();
	static DateTime GetLocalTime();
};
#endif // SystemPlatform_H_8E107E72_296E_43B2_AAF2_09A1A3C6D4AB
