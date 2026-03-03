#include "TimerManager.h"


void TimerManager::update(float dt)
{
	mTotalTime += dt;
	for (int i = 0; i < mTimers.size(); ++i)
	{
		if (mTimers[i].expireTime <= mTotalTime)
		{
			auto callback = std::move(mTimers[i].callback);
			mTimers.removeIndexSwap(i);
			--i;
			callback();
		}
	}
}

TimerHandle TimerManager::addTimer(float duration, std::function<void()> callback)
{
	Timer timer;
	timer.expireTime = mTotalTime + duration;
	timer.callback = std::move(callback);
	timer.id = mNextId++;
	mTimers.push_back(std::move(timer));
	return timer.id;
}

bool TimerManager::stopTimer(TimerHandle id)
{
	int index = mTimers.findIndexPred([id](auto const& timer) { return timer.id == id; });
	if (index != INDEX_NONE)
	{
		mTimers.removeIndexSwap(index);
		return true;
	}
	return false;
}
