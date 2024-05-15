#include "DateTime.h"

DateTime::DateTime(int32 year, int32 month, int32 day, int32 hour, int32 minute, int32 sec, int32 millisecond)
{
	mTicks = 0;
	mTicks += (GetTotalDays(year, month) + day - 1) * TicksPerDay;
	mTicks += hour * TicksPerHour;
	mTicks += minute * TicksPerMinute;
	mTicks += sec * TicksPerSecond;
	mTicks += millisecond * TicksPerMillisecond;
}

int DateTime::GetTotalDays(int32 year, int32 month)
{
	static constexpr int MonthToAccDayMap[] = { 0, 31 , 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
	int result = 0;
	if (month > 2 && IsLeapYear(year))
		result += 1;

	--year;
	--month;

	result += 365 * year;
	result += year / 4 - year / 100 + year / 400;
	result += MonthToAccDayMap[month];
	return result;
}

DateTime::Date DateTime::GetDate(int32 totalDays)
{
	// Based on FORTRAN code in:
	// Fliegel, H. F. and van Flandern, T. C.,
	// Communications of the ACM, Vol. 11, No. 10 (October 1968).
	int32 i, j, k, l, n;

	//l = FMath::FloorToInt32(GetDate().GetJulianDay() + 0.5) + 68569;
	l = 1721426 + totalDays + 68569;
	n = 4 * l / 146097;
	l = l - (146097 * n + 3) / 4;
	i = 4000 * (l + 1) / 1461001;
	l = l - 1461 * i / 4 + 31;
	j = 80 * l / 2447;
	k = l - 2447 * j / 80;
	l = j / 11;
	j = j + 2 - 12 * l;
	i = 100 * (n - 49) + i + l;
	Date result;
	result.year = i;
	result.month = j;
	result.day = k;
	return result;
}

