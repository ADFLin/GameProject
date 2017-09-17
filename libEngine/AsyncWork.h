#pragma once
#ifndef AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011
#define AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011

#include "Thread.h"
#include "IntegerType.h"

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

	void  init(int numThread , uint32 stackSize );
	void  addWork(IQueuedWork* work);
	bool  retractWork(IQueuedWork* work);

	int   getQueuedWorkNum() { return (int)mQueuedWorks.size(); }
	int   getAllThreadNum() { return (int)mAllThreads.size(); }
	int   getIdleThreadNum() { return (int)mQueuedThreads.size();  }
	int   getWorkThreadNum() { return getAllThreadNum() - getIdleThreadNum(); }

private:
	void  cleanup();
	friend class PoolRunableThread;
	IQueuedWork* doWorkCompleted(PoolRunableThread* thread);

	Mutex        mQueueMutex;
	std::vector< IQueuedWork* >       mQueuedWorks;
	std::vector< PoolRunableThread* > mQueuedThreads;

	std::vector< PoolRunableThread* > mAllThreads;
};



#endif // AsyncWork_H_A8DF845F_D24D_4A7E_AA91_4093CD084011
