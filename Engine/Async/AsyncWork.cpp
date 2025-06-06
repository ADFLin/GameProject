#include "AsyncWork.h"

#include "SystemPlatform.h"
#include "InlineString.h"

#include <cassert>


class PoolRunableThread : public RunnableThreadT< PoolRunableThread >
{
public:
	PoolRunableThread();

	unsigned run();
	void     doWork(IQueuedWork* work);
	void     waitTokill();

	QueueThreadPool*  mOwningPool;
	IQueuedWork*      mWork;
	ConditionVariable mWaitWorkCV;
	Mutex             mWaitWorkMutex;
	volatile int32    mWantDie;
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
		PoolRunableThread* runThread = new PoolRunableThread;
		runThread->mOwningPool = this;

		InlineString< 128 > name;
		runThread->start(stackSize);
		name.format("QueuedWorker%d", i + 1);
#if SYS_PLATFORM_WIN
		runThread->setDisplayName(name);
#endif
		mAllThreads.push_back(runThread);
		mQueuedThreads.push_back(runThread);
	}
}

void QueueThreadPool::cleanup()
{
	cencelAllWorks();
	waitAllThreadIdle();

	{
		Mutex::Locker locker(mQueueMutex);
		for( PoolRunableThread* runThread : mAllThreads )
		{
			runThread->waitTokill();
			delete runThread;
		}
		mAllThreads.clear();
		mQueuedThreads.clear();
	}

}

void QueueThreadPool::addWork(IQueuedWork* work)
{
	assert(work);
	Mutex::Locker locker(mQueueMutex);

	if( mQueuedThreads.empty() )
	{
		mQueuedWorks.push_back(work);
		return;
	}

	PoolRunableThread* runThread = mQueuedThreads.back();
	mQueuedThreads.pop_back();

	runThread->doWork(work);
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

void QueueThreadPool::waitAllThreadIdle()
{
	while( 1 )
	{
		{
			Mutex::Locker locker(mQueueMutex);
			if( mAllThreads.size() == mQueuedThreads.size() )
				break;
		}
		SystemPlatform::Sleep(0);
	}
}

void QueueThreadPool::waitAllWorkComplete()
{
	while( 1 )
	{
#if 1
		{
			Mutex::Locker locker(mQueueMutex);
			if( mAllThreads.size() == mQueuedThreads.size() && mQueuedWorks.empty() )
				break;
		}
		SystemPlatform::Sleep(0);
#else
		if (mQueueMutex.tryLock())
		{
			if (mAllThreads.size() == mQueuedThreads.size() && mQueuedWorks.empty())
			{
				mQueueMutex.unlock();
				break;
			}

			mQueueMutex.unlock();
		}
#endif
	}
}

void QueueThreadPool::cencelAllWorks()
{
	Mutex::Locker locker(mQueueMutex);

	for (IQueuedWork* work : mQueuedWorks)
	{
		work->abandon();
		work->release();
	}
	mQueuedWorks.clear();
}

IQueuedWork* QueueThreadPool::doWorkCompleted(PoolRunableThread* runThread)
{
	Mutex::Locker locker(mQueueMutex);

	if( mQueuedWorks.empty() )
	{
		mQueuedThreads.push_back(runThread);
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
			mWaitWorkCV.wait(locker, [this]()->bool { return mWork != nullptr || mWantDie; });
		}

		IQueuedWork* currentWork = mWork;
		mWork = nullptr;
		SystemPlatform::MemoryStoreFence();
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
	SystemPlatform::MemoryStoreFence();

	mWaitWorkCV.notifyOne();
}

void PoolRunableThread::waitTokill()
{
	{
		Mutex::Locker locker(mWaitWorkMutex);
		SystemPlatform::AtomExchange(&mWantDie, 1);
		mWaitWorkCV.notifyOne();
	}
	
	this->join();
}
