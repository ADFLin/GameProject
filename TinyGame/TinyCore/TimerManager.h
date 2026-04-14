#ifndef TimerManager_h__
#define TimerManager_h__

#include <functional>
#include <type_traits>
#include <utility>
#include "DataStructure/Array.h"
#include "GameConfig.h"

using TimerHandle = uint64;

class TimerManager
{
public:
	TimerManager() : mNextSerialId(1), mNextFreeSlotIndex(INDEX_NONE), mExecutingTimerHandle(0), mExecutingTimerStopped(false), mNextOrderKey(0), mTotalTime(0) {}

	struct Timer
	{
		float    expireTime;
		float    duration;
		std::function<bool()> callback;
		uint32   serialId;
		union
		{
			int queueIndex;
			int nextFreeSlotIndex;
		};
		uint64   orderKey;
		bool     bActive;
		bool     bLooped;
	};

	void update(float dt);
	bool stopTimer(TimerHandle id);

	template <typename Func>
	TimerHandle addTimer(float duration, Func&& callback, bool bLooped = false)
	{
		return addTimerInternal(duration, WrapCallback(std::forward<Func>(callback)), bLooped);
	}

private:
	static TimerHandle MakeHandle(int slotIndex, uint32 serialId);
	static int ExtractSlotIndex(TimerHandle handle);
	static uint32 ExtractSerialId(TimerHandle handle);
	bool isHandleValid(TimerHandle handle) const;
	TimerHandle addTimerInternal(float duration, std::function<bool()> callback, bool bLooped);
	int allocateTimerSlot();
	void deactivateTimer(int index);
	void pushTimer(int timerIndex);
	int popNextTimerIndex();
	bool removeTimer(TimerHandle id);
	void heapifyUp(int index);
	void heapifyDown(int index);
	void swapHeapNode(int lhs, int rhs);
	bool isTimerBefore(int lhsTimerIndex, int rhsTimerIndex) const;

	template <typename Func>
	static auto WrapCallback(Func&& callback) ->
		typename std::enable_if<std::is_same<typename std::result_of<Func&()>::type, bool>::value, std::function<bool()>>::type
	{
		return std::forward<Func>(callback);
	}

	template <typename Func>
	static auto WrapCallback(Func&& callback) ->
		typename std::enable_if<std::is_same<typename std::result_of<Func&()>::type, void>::value, std::function<bool()>>::type
	{
		return [callback = std::forward<Func>(callback)]() mutable
		{
			callback();
			return true;
		};
	}

	TArray<Timer> mTimers;
	TArray<int> mExecuteQueue;
	uint32 mNextSerialId;
	int mNextFreeSlotIndex;
	TimerHandle mExecutingTimerHandle;
	bool mExecutingTimerStopped;
	uint64 mNextOrderKey;
	float mTotalTime;
};

#endif // TimerManager_h__
