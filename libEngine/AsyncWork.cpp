#include "AsyncWork.h"

#include "SystemPlatform.h"
#include "FixString.h"

#include <cassert>


class PoolRunableThread : public RunnableThreadT< PoolRunableThread >
{
public:
	PoolRunableThread();

	unsigned run();
	void     doWork(IQueuedWork* work);
	void     waitTokill();

	QueueThreadPool* mOwningPool;
	IQueuedWork*     mWork;
	Condition        mWaitWorkCondition;
	Mutex            mWaitWorkMutex;
	volatile int32   mWantDie;
};

QueueThreadPool::QueueThreadPool()
{

}

QueueThreadPool::~QueueThreadPool()
{
	cleanup();
}

void QueueThreadPool::init(int numThread, uint32 stackSize)
{
	Mutex::Locker locker(mQueueMutex);

	for( int i = 0; i < numThread; ++i )
	{
		PoolRunableThread* thread = new PoolRunableThread;
		thread->mOwningPool = this;

		FixString< 128 > name;
		thread->start(stackSize);
		thread->setDisplayName(name.format("QueuedWorker%d", i + 1));
		mAllThreads.push_back(thread);
		mQueuedThreads.push_back(thread);
	}
}

void QueueThreadPool::cleanup()
{
	{
		Mutex::Locker locker(mQueueMutex);

		for( IQueuedWork* work : mQueuedWorks )
		{
			work->abandon();
			work->release();
		}
		mQueuedWorks.clear();
	}

	while( 1 )
	{
		{
			Mutex::Locker locker(mQueueMutex);
			if ( mAllThreads.size() == mQueuedThreads.size() )
				break;
		}
		SystemPlatform::Sleep(0);
	}

	{
		Mutex::Locker locker(mQueueMutex);
		for( PoolRunableThread* thread : mAllThreads )
		{
			thread->waitTokill();
			delete thread;
		}
		mAllThreads.clear();
		mQueuedThreads.clear();
	}

}

void QueueThreadPool::addWork(IQueuedWork* work)
{
	Mutex::Locker locker(mQueueMutex);

	if( mQueuedThreads.empty() )
	{
		mQueuedWorks.push_back(work);
		return;
	}

	PoolRunableThread* thread = mQueuedThreads.back();
	mQueuedThreads.pop_back();

	thread->doWork(work);
}

bool QueueThreadPool::retractWork(IQueuedWork* work)
{
	Mutex::Locker locker(mQueueMutex);
	auto iter = std::find(mQueuedWorks.begin(), mQueuedWorks.end(), work);
	if( iter == mQueuedWorks.end() )
		return false;
	mQueuedWorks.erase(iter);
	return true;
}

IQueuedWork* QueueThreadPool::doWorkCompleted(PoolRunableThread* thread)
{
	Mutex::Locker locker(mQueueMutex);

	if( mQueuedWorks.empty() )
	{
		mQueuedThreads.push_back(thread);
		return nullptr;
	}

	IQueuedWork* outWork = mQueuedWorks.front();
	mQueuedWorks.erase(mQueuedWorks.begin());

	return outWork;
}

PoolRunableThread::PoolRunableThread()
{
	mWantDie = 0;
	mWork = nullptr;
	mOwningPool = nullptr;
}

unsigned PoolRunableThread::run()
{
	while ( !mWantDie )
	{
		{
			Mutex::Locker locker(mWaitWorkMutex);
			mWaitWorkCondition.wait(locker, [=]()->bool { return mWork != nullptr || mWantDie; });
		}

		IQueuedWork* currentWork = mWork;
		mWork = nullptr;
		SystemPlatform::MemoryBarrier();
		while( currentWork )
		{
			currentWork->executeWork();
			currentWork->release();
			currentWork = mOwningPool->doWorkCompleted(this);
		}
	}

	return 0;
}

void PoolRunableThread::doWork(IQueuedWork* work)
{
	Mutex::Locker locker(mWaitWorkMutex);
	mWork = work;
	SystemPlatform::MemoryBarrier();

	mWaitWorkCondition.notify();
}

void PoolRunableThread::waitTokill()
{
	{
		Mutex::Locker locker(mWaitWorkMutex);
		SystemPlatform::InterlockedExchange(&mWantDie, 1);
		mWaitWorkCondition.notify();
	}
	this->join();
}
