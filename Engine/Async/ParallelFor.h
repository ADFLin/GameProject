#ifndef ParallelFor_h__
#define ParallelFor_h__

#include "AsyncWork.h"
#include "Memory/FrameAllocator.h"
#include "Meta/Concept.h"
#include "Meta/FunctionCall.h"
#include <utility>

template< typename TFunc >
auto ParallelForCallFunc(TFunc& func, int index, int batchIndex, int) -> decltype(func(index, batchIndex), void())
{
	static_assert(TCheckConcept< CFunctionCallable, TFunc, int, int >::Value, "ParallelFor func concept check failed");
	func(index, batchIndex);
}

template< typename TFunc >
void ParallelForCallFunc(TFunc& func, int index, int batchIndex, ...)
{
	static_assert(TCheckConcept< CFunctionCallable, TFunc, int >::Value, "ParallelFor func concept check failed");
	func(index);
}


template< typename TFunc >
class TParallelForWork : public IQueuedWork
{
public:
	template <typename UFunc>
	TParallelForWork(UFunc&& inFunc)
		:start(0)
		,end(0)
		,batchIndex(0)
		,func(std::forward<UFunc>(inFunc))
		,name(nullptr)
		,bUseRange(false)
	{
	}

	template <typename UFunc>
	TParallelForWork(int inStart, int inEnd, int inBatchIndex, UFunc&& inFunc, char const* inName)
		:start(inStart)
		,end(inEnd)
		,batchIndex(inBatchIndex)
		,func(std::forward<UFunc>(inFunc))
		,name(inName)
		,bUseRange(true)
	{
	}

	void executeWork() override 
	{ 
		if (bUseRange)
		{
			executeRange<TFunc>(0);
		}
		else
		{
			executeSingle<TFunc>(0);
		}
	}

	void release() override { this->~TParallelForWork(); }

	template< typename UFunc >
	auto executeSingle(int) -> decltype(std::declval<UFunc&>()(), void())
	{
		func();
	}

	template< typename UFunc >
	void executeSingle(...)
	{
	}

	template< typename UFunc >
	auto executeRange(int) -> decltype(std::declval<UFunc&>()(0, 0), void())
	{
		PROFILE_ENTRY(name);
		for (int k = start; k < end; ++k)
		{
			ParallelForCallFunc(func, k, batchIndex, 0);
		}
	}

	template< typename UFunc >
	auto executeRange(long) -> decltype(std::declval<UFunc&>()(0), void())
	{
		PROFILE_ENTRY(name);
		for (int k = start; k < end; ++k)
		{
			ParallelForCallFunc(func, k, batchIndex, 0);
		}
	}

	template< typename UFunc >
	void executeRange(...)
	{
	}

	int start;
	int end;
	int batchIndex;
	TFunc func;
	char const* name;
	bool bUseRange;
};

template< typename TFunc >
void ParallelForImpl(QueueThreadPool& threadPool, FrameAllocator& allocator, char const* taskName, int count, TFunc&& func, int batchSize = 64)
{
	if (count <= 0) 
		return;
	
	StackMaker marker(allocator);
	using StoredFunc = std::decay_t<TFunc>;

	int numTasks = (count + batchSize - 1) / batchSize;
	using WorkType = TParallelForWork<StoredFunc>;
	size_t workSize = sizeof(WorkType);
	size_t checkAlign = alignof(WorkType) > 16 ? alignof(WorkType) : 16;

	uint8* chunkStart = (uint8*)allocator.alloc(workSize * numTasks, checkAlign);
	IQueuedWork** works;
	works = (IQueuedWork**)allocator.alloc(sizeof(IQueuedWork*) * numTasks);

	for (int i = 0; i < numTasks; ++i)
	{
		int start = i * batchSize;
		int end = Math::Min(start + batchSize, count);
		WorkType* work = new (chunkStart + i * workSize) WorkType(start, end, i, func, taskName);
		works[i] = work;
	}

	threadPool.addWorks(works, numTasks);
}


template< typename TFunc >
void ParallelFor(QueueThreadPool& threadPool, FrameAllocator& allocator, char const* taskName, int count, TFunc&& func, int batchSize = 64)
{
	ParallelForImpl(threadPool, allocator, taskName, count, std::forward<TFunc>(func), batchSize);
	threadPool.waitAllWorkComplete();
}

template< typename TFunc >
void ParallelForInWorker(QueueThreadPool& threadPool, FrameAllocator& allocator, char const* taskName, int count, TFunc&& func, int batchSize = 64)
{
	ParallelForImpl(threadPool, allocator, taskName, count, std::forward<TFunc>(func), batchSize);
	threadPool.waitAllWorkCompleteInWorker();
}

#endif // ParallelFor_h__
