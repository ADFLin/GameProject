#ifndef TimerManager_h__
#define TimerManager_h__

#include <functional>
#include "DataStructure/Array.h"
#include "GameConfig.h"

using TimerHandle = int;

class TimerManager
{
public:
	TimerManager() : mNextId(1), mTotalTime(0) {}

	struct Timer
	{
		float    expireTime;
		std::function<void()> callback;
		int      id;
	};

	void update(float dt);
	TimerHandle  addTimer(float duration, std::function<void()> callback);
	bool stopTimer(TimerHandle id);

private:

	TArray<Timer> mTimers;
	int mNextId;
	float mTotalTime;
};

#endif // TimerManager_h__
