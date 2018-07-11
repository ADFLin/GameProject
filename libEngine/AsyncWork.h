#pragma once
#ifndef AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011
#define AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011

#include "Thread.h"
#include "Core/IntegerType.h"

#include <vector>

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
	bool  retractWork(IQueuedWork* work);

	template< class Fun >
	void  addFunctionWork(Fun fun)
	{
		class FunWork : public IQueuedWork
		{
		public:
			FunWork(Fun fun):mFun(fun){}
			virtual void executeWork() override { mFun(); }
			virtual void release() override { delete this; }
			Fun mFun;
		};

		FunWork* work = new FunWork(fun);
		addWork(work);
	}

	void  waitAllThreadIdle();
	void  waitAllWorkComplete();

	int   getQueuedWorkNum() { return (int)mQueuedWorks.size(); }
	int   getAllThreadNum() { return (int)mAllThreads.size(); }
	int   getIdleThreadNum() { return (int)mQueuedThreads.size();  }
	int   getWorkThreadNum() { return getAllThreadNum() - getIdleThreadNum(); }

private:
	void  cleanup();
	friend class PoolRunableThread;
	IQueuedWork* doWorkCompleted(PoolRunableThread* runThread);

	Mutex        mQueueMutex;
	std::vector< IQueuedWork* >       mQueuedWorks;
	std::vector< PoolRunableThread* > mQueuedThreads;

	std::vector< PoolRunableThread* > mAllThreads;
};



#endif // AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011
