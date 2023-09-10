#include "MiscTestRegister.h"
#include "PlatformThread.h"

#include "SystemPlatform.h"

#include "Core/LockFreeList.h"
#include "ProfileSystem.h"

#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#include <atomic>

class TestListRunnable : public RunnableThreadT< TestListRunnable >
{
public:
	unsigned run() 
	{
		while(1)
		{
			PROFILE_ENTRY("Test");
			//SystemPlatform::Sleep(200);
			int const count = 100;
			for (int i = 0; i < count; ++i)
			{
				list->push(idx * count + i);
				//SystemPlatform::Sleep(rand() % 10);
			}
			SystemPlatform::AtomIncrement(pNumFinish);
			LogMsg("%d Thead is finished", idx);
		};
		return 0; 	
	}

	int idx;
	volatile int32* pNumFinish;
	TLockFreeFIFOList<int>* list;
};

void printValues(int values[], int num)
{
	std::string str;
	for( int i = 0; i < num; ++i )
	{
		str += std::to_string(values[i]);
		str += " ";
	}

	//LogMsg(str.c_str());
}
void LockFreeListTest()
{

	volatile int32 numFinish = 0;
	TestListRunnable runnables[10];
	TLockFreeFIFOList<int> list;
	numFinish = 0;
	for( int i = 0; i < ARRAY_SIZE(runnables); ++i )
	{
		auto& rannable = runnables[i];
		rannable.idx = i;
		rannable.list = &list;
		rannable.pNumFinish = &numFinish;
		rannable.start();
		rannable.setDisplayName(InlineString<>::Make("TestThread %d", i));
	}

	int savedValues[10];
	int numSaved = 0;
	while( numFinish < ARRAY_SIZE(runnables) )
	{
		while ( !list.empty() )
		{
			int value = list.pop();
			savedValues[numSaved] = value;
			++numSaved;
			if( numSaved == ARRAY_SIZE(savedValues) )
			{
				printValues(savedValues, numSaved);
				numSaved = 0;
			}
		}
	}
	
	LogMsg( "%d operator Missed" , list.numMissed );
	if ( numSaved )
	{
		printValues(savedValues, numSaved);
		numSaved = 0;
	}

	for( int i = 0; i < ARRAY_SIZE(runnables); ++i )
	{
		runnables[i].join();
	}


}

REGISTER_MISC_TEST_ENTRY("LockFreeList Test", LockFreeListTest);