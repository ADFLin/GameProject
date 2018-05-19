#include "TestStageHeader.h"

#include "AsyncWork.h"
#include "SystemPlatform.h"


static QueueThreadPool* gPool;
static Mutex gMutex;

int64 gNum = 1000000000LL;
class MyWork : public IQueuedWork
{
public:
	virtual void executeWork() override
	{
		int64 num = gNum + ::rand();
		int64 sum = 0;
		for( int64 i = 0; i < num; ++i )
			sum += i;
		SystemPlatform::Sleep(waitTime);
		{
			//Mutex::Locker locker(gMutex);
			LogMsg("%d work is complete! sum = %lld", idx , sum);
		}
		
	}
	virtual void release() override
	{
		delete this;
	}
	int idx;
	int waitTime;
};
static void AsyncWorkTest()
{

	if( gPool == nullptr )
	{
		gPool = new QueueThreadPool;
		int numCPU = SystemPlatform::GetProcessorNumber();
		gPool->init( std::max( numCPU - 2, 1 ) , 0);
	}
	
	for( int i = 0; i < 100; ++i )
	{
		MyWork* work = new MyWork;
		work->idx = i;
		work->waitTime = 0;
		gPool->addWork(work);
	}

	LogMsg("%d work in queue", gPool->getQueuedWorkNum());

#if 0
	SystemPlatform::Sleep(1000);

	delete gPool;
	gPool = nullptr;
#endif
}

REGISTER_MISC_TEST("Async Work Test", AsyncWorkTest);