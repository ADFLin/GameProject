#ifndef DateTime_h__
#define DateTime_h__

#include "Serialize/SerializeFwd.h"
#include "Core/IntegerType.h"

enum class EDateMonth
{
	Junuary,
	February,
	March,
	April,
	May,
	June,
	July,
	August,
	September,
	October,
	November,
	December,
};

enum class EDayOfWeek
{
	Sunday = 0,
	Monday,
	Tuesday,
	Wednesday,
	Thursday,
	Friday,
	Saturday,
};

struct TimeSpan
{
	int64 usec;
};

struct DateTime
{
public:
	DateTime() = default;


	explicit DateTime(uint64 ticks) : mTicks(ticks){}
	DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 sec, int32 millisecond);

	static int GetTotalDays(int32 year, int32 month);

	struct Date
	{
		int year;
		int month;
		int day;
	};
	static Date GetDate(int32 totalDays);

	static bool IsLeapYear(int year)
	{
		if ((year % 4) == 0)
		{
			return (((year % 100) != 0) || ((year % 400) == 0));
		}
		return false;
	}
	double getJulianDay() const
	{
		return 1721425.5 + double(mTicks / TicksPerDay) + double(mTicks % TicksPerDay) / TicksPerDay;
	}

	int32 getTotalDays() const
	{
		return mTicks / TicksPerDay;
	}

	Date getDate() const
	{
		return GetDate(getTotalDays());
	}

	uint64 getTicks() const
	{
		return mTicks;
	}

	int32 getYear() const
	{
		return getDate().year;
	}
	int32 getMonth() const
	{
		return getDate().month;
	}
	int32 getDay() const
	{
		return getDate().day;
	}
	int32 getHour() const { return (mTicks / TicksPerHour) % 24; }
	int32 getMinute() const { return (mTicks / TicksPerMinute) % 60; }
	int32 getSecond() const { return (mTicks / TicksPerSecond) % 60; }
	int32 getMillisecond() const { return (mTicks / TicksPerMillisecond) % (TicksPerSecond / TicksPerMillisecond); }

	EDayOfWeek getDayOfWeek() const
	{
		return EDayOfWeek(getTotalDays() % 7 + 1);
	}

	bool operator == (DateTime const& rhs) const
	{
		return mTicks == rhs.mTicks;
	}
	bool operator < (DateTime const& rhs) const
	{
		return mTicks < rhs.mTicks;
	}
	bool operator > (DateTime const& rhs) const
	{
		return mTicks > rhs.mTicks;
	}
	bool operator != (DateTime const& rhs) const { return !(*this == rhs); }
	bool operator >= (DateTime const& rhs) const { return !(*this < rhs); }
	bool operator <= (DateTime const& rhs) const { return !(*this > rhs); }

private:
	static constexpr uint64 TicksPerMillisecond = 1;
	static constexpr uint64 TicksPerSecond = TicksPerMillisecond * 10000;
	static constexpr uint64 TicksPerMinute = TicksPerSecond * 60;
	static constexpr uint64 TicksPerHour = TicksPerMinute * 60;
	static constexpr uint64 TicksPerDay = TicksPerHour * 24;

	uint64 mTicks;
};

TYPE_SERIALIZE_AS_RAW_DATA(DateTime);

#endif // DateTime_h__
