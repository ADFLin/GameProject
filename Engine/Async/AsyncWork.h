#pragma once
#ifndef AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011
#define AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011

#include "PlatformThread.h"
#include "Core/IntegerType.h"
#include "DataStructure/Array.h"
#include "DataStructure/CycleQueue.h"
#include <atomic>

class IQueuedWork
{
public:
	virtual void executeWork() = 0;
	virtual void abandon() {};
	virtual void release() = 0;
};

class PoolRunableThread;

class QueueThreadPool
{
public:
	QueueThreadPool();
	~QueueThreadPool();

	void  init(int numThread , uint32 stackSize = 0);
	void  addWork(IQueuedWork* work);
	void  addWorks(IQueuedWork* works[], int count);
	bool  retractWork(IQueuedWork* work);

	template< class TFunc >
	void  addFunctionWork(TFunc&& func)
	{
		class TFuncWork : public IQueuedWork
		{
		public:
			TFuncWork(TFunc&& func):mFunc(std::forward<TFunc>(func)){}
			virtual void executeWork() override { mFunc(); }
			virtual void release() override { delete this; }
			TFunc mFunc;
		};

		TFuncWork* work = new TFuncWork(std::forward<TFunc>(func));
		addWork(work);
	}

	void  waitAllThreadIdle();
	void  waitAllWorkComplete(bool bHelpUpdate = false);
	void  waitAllWorkCompleteInWorker();
	void  cencelAllWorks();

	int   getQueuedWorkNum() { return (int)mQueuedWorks.size(); }
	int   getAllThreadNum() { return (int)mAllThreads.size(); }
	int   getIdleThreadNum() { return (int)mQueuedThreads.size();  }
	int   getWorkThreadNum() { return getAllThreadNum() - getIdleThreadNum(); }

private:
	void  cleanup();
	friend class PoolRunableThread;
	IQueuedWork* doWorkCompleted(PoolRunableThread* runThread);

	Mutex        mQueueMutex;
	SpinLock     mQueueLock;
	TCycleQueue< IQueuedWork* >  mQueuedWorks;
	TArray< PoolRunableThread* > mQueuedThreads;

	TArray< PoolRunableThread* > mAllThreads;
	ConditionVariable            mWaitCompleteCV;
};



#endif // AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011
