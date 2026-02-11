#include "AsyncWork.h"

#include "SystemPlatform.h"
#include "InlineString.h"
#include <intrin.h>
#include "ProfileSystem.h"

#include <cassert>

static void  ExecuteWork(IQueuedWork* work)
{
	PROFILE_ENTRY("Worker.ExecuteWork");
	work->executeWork();
	work->release();
}

class PoolRunableThread : public RunnableThreadT< PoolRunableThread >
{
public:
	PoolRunableThread();

	unsigned run();
	void     doWork(IQueuedWork* work);
	void     waitToKill();

	QueueThreadPool*  mOwningPool;
	std::atomic< IQueuedWork* > mWork;
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
			runThread->waitToKill();
			delete runThread;
		}
		mAllThreads.clear();
		mQueuedThreads.clear();
	}

}

void QueueThreadPool::addWork(IQueuedWork* work)
{
	assert(work);
	PoolRunableThread* runThread = nullptr;
	{
		SpinLock::Locker locker(mQueueLock);
		if( mQueuedThreads.empty() )
		{
			mQueuedWorks.push_back(work);
			return;
		}

		runThread = mQueuedThreads.back();
		mQueuedThreads.pop_back();
	}

	runThread->doWork(work);
}

bool QueueThreadPool::retractWork(IQueuedWork* work)
{
	Mutex::Locker locker(mQueueMutex);
	return false;
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
	Mutex::Locker locker(mQueueMutex);
	mWaitCompleteCV.wait(locker, [this]()
	{
		SpinLock::Locker spinLock(mQueueLock);
		return mAllThreads.size() == mQueuedThreads.size() && mQueuedWorks.empty();
	});
}

void QueueThreadPool::waitAllWorkCompleteInWorker()
{
	// 1. 先處理完佇列中的工作
	while (true)
	{
		IQueuedWork* work = nullptr;
		{
			SpinLock::Locker locker(mQueueLock);
			if (mQueuedWorks.empty())
				break;
			work = mQueuedWorks.front();
			mQueuedWorks.pop_front();
		}

		if (work)
		{
			ExecuteWork(work);
		}
	}

	Mutex::Locker locker(mQueueMutex);
	mWaitCompleteCV.wait(locker, [this]()
	{
		SpinLock::Locker spinLock(mQueueLock);
		int idleCount = (int)mQueuedThreads.size();
		bool bFinished = mQueuedWorks.empty() && (idleCount == (int)mAllThreads.size() || idleCount == (int)mAllThreads.size() - 1);
		return bFinished;
	});
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

void QueueThreadPool::addWorks(IQueuedWork* works[], int count)
{
	if (count == 0) return;

	PoolRunableThread* dispatched[64];
	int numDispatched = 0;

	{
		SpinLock::Locker locker(mQueueLock);
		int workIdx = 0;
		while( !mQueuedThreads.empty() && workIdx < count )
		{
			PoolRunableThread* runThread = mQueuedThreads.back();
			mQueuedThreads.pop_back();
			runThread->mWork.store(works[workIdx++], std::memory_order_release);
			dispatched[numDispatched++] = runThread;
		}

		if( workIdx < count )
		{
			mQueuedWorks.addRange(works + workIdx, works + count);
		}
	}

	// Wake threads outside the lock (CV notify may block on mWaitWorkMutex)
	for (int i = 0; i < numDispatched; ++i)
	{
		Mutex::Locker locker(dispatched[i]->mWaitWorkMutex);
		dispatched[i]->mWaitWorkCV.notifyOne();
	}
}

IQueuedWork* QueueThreadPool::doWorkCompleted(PoolRunableThread* runThread)
{
	IQueuedWork* outWork = nullptr;
	bool bShouldNotify = false;
	{
		SpinLock::Locker locker(mQueueLock);
		if( mQueuedWorks.empty() )
		{
			mQueuedThreads.push_back(runThread);
			if (mQueuedWorks.empty())
			{
				bShouldNotify = true;
			}
		}
		else
		{
			outWork = mQueuedWorks.front();
			mQueuedWorks.pop_front();
		}
	}

	if (bShouldNotify)
	{
		Mutex::Locker lock(mQueueMutex);
		mWaitCompleteCV.notifyAll();
	}

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
			PROFILE_ENTRY("Worker.SpinWait");
			for (int i = 0; i < 1000; ++i)
			{
				if (mWork.load(std::memory_order_relaxed) != nullptr || mWantDie)
					break;
				_mm_pause(); 
			}
		}

		{
			PROFILE_ENTRY("Worker.Idle");
			if (mWork.load(std::memory_order_relaxed) == nullptr && !mWantDie)
			{
				Mutex::Locker locker(mWaitWorkMutex);
				mWaitWorkCV.wait(locker, [this]()->bool { return mWork.load(std::memory_order_relaxed) != nullptr || mWantDie; });
			}
		}


		IQueuedWork* currentWork = mWork.exchange(nullptr, std::memory_order_acq_rel);
		if (currentWork)
		{
			PROFILE_ENTRY("Worker.WorkLoop");
			while (currentWork)
			{
				ExecuteWork(currentWork);

				{
					PROFILE_ENTRY("Worker.FetchNext");
					currentWork = mOwningPool->doWorkCompleted(this);
				}
			}
		}
	}

	return 0;
}

void PoolRunableThread::doWork(IQueuedWork* work)
{
	mWork.store(work, std::memory_order_release);
	{
		Mutex::Locker locker(mWaitWorkMutex);
		mWaitWorkCV.notifyOne();
	}
}

void PoolRunableThread::waitToKill()
{
	{
		Mutex::Locker locker(mWaitWorkMutex);
		SystemPlatform::AtomExchange(&mWantDie, 1);
		mWaitWorkCV.notifyOne();
	}
	
	this->join();
}
