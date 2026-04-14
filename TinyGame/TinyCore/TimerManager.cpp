#include "TimerManager.h"
#include "Math/Base.h"

TimerHandle TimerManager::MakeHandle(int slotIndex, uint32 serialId)
{
	return (TimerHandle(uint32(slotIndex)) << 32) | serialId;
}

int TimerManager::ExtractSlotIndex(TimerHandle handle)
{
	return int(handle >> 32);
}

uint32 TimerManager::ExtractSerialId(TimerHandle handle)
{
	return uint32(handle & 0xffffffffu);
}

bool TimerManager::isHandleValid(TimerHandle handle) const
{
	int slotIndex = ExtractSlotIndex(handle);
	if (!mTimers.isValidIndex(slotIndex))
		return false;

	Timer const& timer = mTimers[slotIndex];
	return timer.bActive && timer.serialId == ExtractSerialId(handle);
}


void TimerManager::update(float dt)
{
	if (dt < 0)
		return;

	float const targetTime = mTotalTime + dt;
	while (mExecuteQueue.size())
	{
		Timer const& nextTimer = mTimers[mExecuteQueue[0]];
		if (nextTimer.expireTime > targetTime)
			break;

		int timerIndex = popNextTimerIndex();
		mTotalTime = mTimers[timerIndex].expireTime;

		if (!mTimers[timerIndex].bActive || !mTimers[timerIndex].callback)
		{
			deactivateTimer(timerIndex);
			continue;
		}

		mExecutingTimerHandle = MakeHandle(timerIndex, mTimers[timerIndex].serialId);
		mExecutingTimerStopped = false;
		bool bContinue = mTimers[timerIndex].callback();
		bool bStopped = mExecutingTimerStopped;
		mExecutingTimerHandle = 0;
		mExecutingTimerStopped = false;

		if (bStopped)
		{
			deactivateTimer(timerIndex);
			continue;
		}

		if (!mTimers[timerIndex].bLooped || !bContinue)
		{
			deactivateTimer(timerIndex);
			continue;
		}

		if (mTimers[timerIndex].duration <= 0)
		{
			mTimers[timerIndex].expireTime = mTotalTime + FLOAT_EPSILON;
			mTimers[timerIndex].orderKey = mNextOrderKey++;
			pushTimer(timerIndex);
			continue;
		}

		mTimers[timerIndex].expireTime += mTimers[timerIndex].duration;
		mTimers[timerIndex].orderKey = mNextOrderKey++;
		pushTimer(timerIndex);
	}

	mTotalTime = targetTime;
}

TimerHandle TimerManager::addTimerInternal(float duration, std::function<bool ()> callback, bool bLooped)
{
	duration = Math::Max(duration, 0.0f);

	int timerIndex = allocateTimerSlot();
	Timer& timer = mTimers[timerIndex];
	timer.expireTime = mTotalTime + duration;
	timer.duration = duration;
	timer.callback = std::move(callback);
	timer.serialId = mNextSerialId++;
	if (mNextSerialId == 0)
		mNextSerialId = 1;
	timer.queueIndex = INDEX_NONE;
	timer.orderKey = mNextOrderKey++;
	timer.bActive = true;
	timer.bLooped = bLooped;
	pushTimer(timerIndex);
	return MakeHandle(timerIndex, timer.serialId);
}

int TimerManager::allocateTimerSlot()
{
	if (mNextFreeSlotIndex != INDEX_NONE)
	{
		int result = mNextFreeSlotIndex;
		mNextFreeSlotIndex = mTimers[result].nextFreeSlotIndex;
		return result;
	}

	int result = mTimers.size();
	mTimers.push_back(Timer());
	return result;
}

void TimerManager::deactivateTimer(int index)
{
	Timer& timer = mTimers[index];
	if (!timer.bActive)
		return;

	timer.callback = nullptr;
	timer.bActive = false;
	timer.nextFreeSlotIndex = mNextFreeSlotIndex;
	mNextFreeSlotIndex = index;
}

void TimerManager::pushTimer(int timerIndex)
{
	Timer& timer = mTimers[timerIndex];
	int queueIndex = mExecuteQueue.size();
	mExecuteQueue.push_back(timerIndex);
	timer.queueIndex = queueIndex;
	heapifyUp(queueIndex);
}

int TimerManager::popNextTimerIndex()
{
	CHECK(!mExecuteQueue.empty());
	int result = mExecuteQueue[0];
	Timer& timer = mTimers[result];

	if (mExecuteQueue.size() == 1)
	{
		mExecuteQueue.pop_back();
		timer.queueIndex = INDEX_NONE;
		return result;
	}

	int movedTimerIndex = mExecuteQueue.back();
	mExecuteQueue[0] = movedTimerIndex;
	mExecuteQueue.pop_back();
	mTimers[movedTimerIndex].queueIndex = 0;
	timer.queueIndex = INDEX_NONE;
	heapifyDown(0);
	return result;
}

bool TimerManager::removeTimer(TimerHandle id)
{
	if (!isHandleValid(id))
		return false;

	int timerIndex = ExtractSlotIndex(id);
	Timer& timer = mTimers[timerIndex];
	int queueIndex = timer.queueIndex;
	CHECK(queueIndex != INDEX_NONE);

	int lastQueueIndex = mExecuteQueue.size() - 1;
	if (queueIndex == lastQueueIndex)
	{
		mExecuteQueue.pop_back();
		timer.queueIndex = INDEX_NONE;
		deactivateTimer(timerIndex);
		return true;
	}

	int movedTimerIndex = mExecuteQueue[lastQueueIndex];
	mExecuteQueue[queueIndex] = movedTimerIndex;
	mExecuteQueue.pop_back();
	mTimers[movedTimerIndex].queueIndex = queueIndex;
	timer.queueIndex = INDEX_NONE;
	deactivateTimer(timerIndex);

	int parentIndex = (queueIndex - 1) / 2;
	if (queueIndex > 0 && isTimerBefore(mExecuteQueue[queueIndex], mExecuteQueue[parentIndex]))
		heapifyUp(queueIndex);
	else
		heapifyDown(queueIndex);
	return true;
}

void TimerManager::heapifyUp(int index)
{
	while (index > 0)
	{
		int parentIndex = (index - 1) / 2;
		if (!isTimerBefore(mExecuteQueue[index], mExecuteQueue[parentIndex]))
			break;

		swapHeapNode(index, parentIndex);
		index = parentIndex;
	}
}

void TimerManager::heapifyDown(int index)
{
	for (;;)
	{
		int leftChild = 2 * index + 1;
		if (leftChild >= mExecuteQueue.size())
			break;

		int bestIndex = leftChild;
		int rightChild = leftChild + 1;
		if (rightChild < mExecuteQueue.size() && isTimerBefore(mExecuteQueue[rightChild], mExecuteQueue[leftChild]))
			bestIndex = rightChild;

		if (!isTimerBefore(mExecuteQueue[bestIndex], mExecuteQueue[index]))
			break;

		swapHeapNode(index, bestIndex);
		index = bestIndex;
	}
}

void TimerManager::swapHeapNode(int lhs, int rhs)
{
	if (lhs == rhs)
		return;

	using std::swap;
	swap(mExecuteQueue[lhs], mExecuteQueue[rhs]);
	mTimers[mExecuteQueue[lhs]].queueIndex = lhs;
	mTimers[mExecuteQueue[rhs]].queueIndex = rhs;
}

bool TimerManager::isTimerBefore(int lhsTimerIndex, int rhsTimerIndex) const
{
	Timer const& lhs = mTimers[lhsTimerIndex];
	Timer const& rhs = mTimers[rhsTimerIndex];
	if (lhs.expireTime != rhs.expireTime)
		return lhs.expireTime < rhs.expireTime;
	return lhs.orderKey < rhs.orderKey;
}

bool TimerManager::stopTimer(TimerHandle id)
{
	if (id == mExecutingTimerHandle)
	{
		mExecutingTimerStopped = true;
		return true;
	}

	return removeTimer(id);
}
