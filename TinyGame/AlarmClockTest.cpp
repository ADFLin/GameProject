

#include "SystemPlatform.h"

class DateTime;

enum class EClockTriggerRule
{
	Weekly ,
	Daily ,
	Monthly ,
	Yearly ,
	SpecDate ,
};


class ClockTriggerRule
{
public:
	virtual bool testFire(DateTime const& time) = 0;
};



struct ClockData
{


};

class AlarmClock
{
public:
	virtual void fire() = 0;



	bool testFire(DateTime const& time)
	{
		switch( mRule )
		{
		case EClockTriggerRule::SpecDate:
			if( mTargetTime.getYear() != time.getYear() )
				return false;
		case EClockTriggerRule::Yearly:
			if( mTargetTime.getMonth() != time.getMonth() )
				return false;
		case EClockTriggerRule::Monthly:
			if( mTargetTime.getDay() != time.getDay() )
				return false;
			break;
		case EClockTriggerRule::Weekly: 
			if( mTargetTime.getDayOfWeek() != time.getDayOfWeek() )
				return false;
			break;
		}

		if( mTargetTime.getHour() != time.getHour() )
			return false;
		if( mTargetTime.getMinute() != time.getMinute() )
			return false;

		return true;
	}
	EClockTriggerRule mRule;
	DateTime mTargetTime;
	
};



class AlarmClockManager
{
public:

	typedef std::unique_ptr< AlarmClock > ClockPtr;
	void tick()
	{
		DateTime nowTime = SystemPlatform::GetLocalTime();

		for( auto& clock : mClocks )
		{
			if( clock->testFire(nowTime) )
			{
				clock->fire();
			}
		}
	}

	std::vector< ClockPtr > mClocks;
};