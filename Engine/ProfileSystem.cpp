#include "ProfileSystem.h"

//#include "TMessageShow.h"

#include "Clock.h"
#include "PlatformThread.h"

#include <atomic>


CORE_API HighResClock  GProfileClock;
inline void Profile_GetTicks(uint64 * ticks)
{
	*ticks = GProfileClock.getTimeMicroseconds();
}

inline float Profile_GetTickRate()
{
	//	return 1000000.f;
	return 1000.f;
}



class ThreadProfileData
{
public:
	ThreadProfileData(char const* rootName);

	std::atomic_bool          mResetRequest;
	uint32                    mThreadId;
	ProfileSampleNode*	      mCurSample;
	unsigned                  mCurFlag;
	unsigned                  mCurStackNum;
	TPtrHolder< ProfileSampleNode >  mRootSample;


	void resetSample();
	void tryStartTiming(const char * name, unsigned flag);
	void stopTiming();

	void cleanup();


	ProfileSampleNode* getRootSample()
	{
		return mRootSample.get();
	}

	static void cleanupNode(ProfileSampleNode* node)
	{
		if (node->mChild)
			cleanupNode(node->mChild);

		if (node->mSibling)
			cleanupNode(node->mSibling);

		node->unlink();
		DestoryNode(node);
	}

	static ProfileSampleNode* CreateNode(char const* name, ProfileSampleNode* parent)
	{
		return new ProfileSampleNode(name, parent);
	}
	static void               DestoryNode(ProfileSampleNode* node)
	{
		delete node;
	}
};

class ProfileSystemImpl : public ProfileSystem
{
public:

	ProfileSystemImpl(char const* rootName = "Root");
	~ProfileSystemImpl();

	void   cleanup() override;
	void   resetSample() override;

	void   incrementFrameCount() override;
	int	   getFrameCountSinceReset() override { return mFrameCount; }
	double getDurationSinceReset() override;

	void updateDuration(bool bForce = false)
	{
		if (!mbDurationDirty && !bForce)
			return;
		
		mbDurationDirty = false;
		uint64 time;
		Profile_GetTicks(&time);
		mDurationSinceReset = double(time - mResetTime) / Profile_GetTickRate();
		mLastFrameDuration = double(time - mFrameStartTime) / Profile_GetTickRate();
	}

	double getLastFrameDuration() override
	{
		updateDuration();
		return mLastFrameDuration;
	}

	ProfileSampleNode* getRootSample(uint32 threadId = 0) override
	{
		if (threadId)
		{
			//#TODO:impl
			return nullptr;
		}
		else
		{
			return GetCurrentThreadData()->getRootSample();
		}
	}

	static ThreadProfileData* GetCurrentThreadData();
	static ThreadProfileData* GetThreadData(uint32 threadId);


	static ProfileSystemImpl& Get() { return static_cast< ProfileSystemImpl& >( ProfileSystem::Get()); }



private:


	friend class ThreadProfileData;

	int	       mFrameCount;
	uint64     mResetTime;
	uint64     mFrameStartTime;
	uint64     mLastFrameDuration;
	uint64     mDurationSinceReset;
	bool       mbDurationDirty;
};

struct TimeScopeResult
{
	~TimeScopeResult() = default;

	std::string name;
	std::vector< std::unique_ptr< TimeScopeResult > > children;
	uint64 duration;

	void logMsg(int level = 0)
	{
		if (level)
		{
			LogMsg("%*c%s%s = %.3f", 2 * level, ' ', "|-" , name.c_str(), duration / 1000.0f);
		}
		else
		{
			LogMsg("%s = %.3f", name.c_str(), duration / 1000.0f);
		}
		for (int i = 0; i < children.size(); ++i)
		{
			TimeScopeResult* child = children[i].get();
			child->logMsg(level + 1);
		}
	}
};

#if CORE_SHARE_CODE

thread_local std::vector< TimeScopeResult* > sTimeScopeStacks;
CORE_API std::vector< TimeScopeResult* >& TimeScope::GetResultStack()
{
	return sTimeScopeStacks;
}


#if 0
Mutex gInstanceLock;
std::atomic< ProfileSystem* > gInstance;
ProfileSystem& ProfileSystem::Get()
{
	ProfileSystem* instance = gInstance.load(std::memory_order_acquire);

	if( instance == nullptr )
	{
		Mutex::Locker locker(gInstanceLock);
		instance = gInstance.load(std::memory_order_relaxed);
		if( instance == nullptr )
		{
			instance = new ProfileSystem("Root");
			gInstance.store( instance, std::memory_order_release);
		}
	}
	return *instance;
}
#else
ProfileSystem& ProfileSystem::Get()
{
	static ProfileSystemImpl sInstance;
	return sInstance;
}
#endif

ProfileSampleScope::ProfileSampleScope(const char * name, unsigned flag)
{
	ProfileSystemImpl::GetCurrentThreadData()->tryStartTiming(name, flag);
}

ProfileSampleScope::~ProfileSampleScope(void)
{
	ProfileSystemImpl::GetCurrentThreadData()->stopTiming();
}

#endif //CORE_SHARE_CODE


ProfileSystemImpl::ProfileSystemImpl(char const* rootName)
	:mFrameCount(0)
	,mResetTime(0)
{


}

ProfileSystemImpl::~ProfileSystemImpl()
{
	cleanup();
}

void ProfileSystemImpl::cleanup()
{

}


std::unordered_map< uint32, ThreadProfileData* > ThreadDataMap;

void ProfileSystemImpl::resetSample()
{
	GProfileClock.reset();
	mFrameCount = 0;
	Profile_GetTicks(&mResetTime);

	mbDurationDirty = 0;
	//ThreadData reset
	for (auto& pair : ThreadDataMap)
	{
		pair.second->resetSample();
	}
}

void ProfileSystemImpl::incrementFrameCount()
{
	updateDuration();
	++mFrameCount;
	mbDurationDirty = true;
	Profile_GetTicks(&mFrameStartTime);
}

double ProfileSystemImpl::getDurationSinceReset()
{
	updateDuration();
	return mDurationSinceReset;
}

thread_local ThreadProfileData* gThreadDataLocal = nullptr;

ThreadProfileData* ProfileSystemImpl::GetCurrentThreadData()
{
	if( gThreadDataLocal == nullptr )
	{
		gThreadDataLocal = new ThreadProfileData("Root");
		gThreadDataLocal->mThreadId = Thread::GetCurrentThreadId();
		//mThreadDataMap.insert({ gThreadDataLocal->mThreadId , gThreadDataLocal });
	}
	return gThreadDataLocal;
}

ThreadProfileData* ProfileSystemImpl::GetThreadData(uint32 threadId)
{
	auto iter = ThreadDataMap.find(threadId);
	if (iter == ThreadDataMap.end())
		return nullptr;

	return iter->second;
}

ProfileSampleNode::ProfileSampleNode( const char * name, ProfileSampleNode * parent )
	:mName( name )
	,mFrameCalls( 0 )
	,mTotalCalls( 0 )
	,mLastCallFrame(-1)
	,mTotalTime( 0 )
	,mStartTime( 0 )
	,mRecursionCounter( 0 )
	,mParent( parent )
	,mChild(nullptr)
	,mSibling(nullptr)
	,mIsShowChild( true )
{
	reset();
}

ProfileSampleNode::~ProfileSampleNode()
{

}

void ProfileSampleNode::unlink()
{
	mChild = nullptr;
	mSibling = nullptr;
}


ProfileSampleNode* ProfileSampleNode::getSubNode( const char * name )
{
	// Try to find this sub node
	ProfileSampleNode* child = mChild;
	while ( child ) 
	{
		if ( child->mName == name ) 
		{
			return child;
		}
		child = child->mSibling;
	}
	return nullptr;

}


void ProfileSampleNode::reset()
{
	mTotalCalls = 0;
	mFrameCalls = 0;
	mTotalTime = 0.0f;
	mRecursionCounter = 0;

	if ( mChild ) 
	{
		mChild->reset();
	}
	if ( mSibling ) 
	{
		mSibling->reset();
	}
}


void ProfileSampleNode::notifyCall()
{
	if (mRecursionCounter == 0) 
	{
		Profile_GetTicks(&mStartTime);
		mFrameCalls = 1;
	}
	++mRecursionCounter;
	++mTotalCalls;
}

bool ProfileSampleNode::notifyReturn()
{
	--mRecursionCounter;
	if (mRecursionCounter != 0)
		return false;

	if ( mTotalCalls != 0 ) 
	{ 
		uint64 time;
		Profile_GetTicks(&time);
		time -= mStartTime;
		mLastFrameTime = double(time) / Profile_GetTickRate();
		mTotalTime += mLastFrameTime;
	}
	return true;
}

void ProfileSampleNode::showAllChild( bool beShow )
{
	ProfileSampleNode* node = getChild();

	showChild( beShow  );
	while ( node )
	{
		node->showAllChild( beShow );
		node = node->getSibling();
	}
}

ThreadProfileData::ThreadProfileData(char const* rootName)
{
	mCurFlag = 0;

	mRootSample.reset(CreateNode(rootName, nullptr));
	mRootSample->mPrevFlag = 0;
	mRootSample->mStackNum = 0;

	mCurSample = mRootSample.get();

	resetSample();
}


void ThreadProfileData::resetSample()
{
	mRootSample->reset();
	mRootSample->notifyCall();
}

void ThreadProfileData::tryStartTiming(const char* name, unsigned flag)
{
	if( flag & PROF_FORCCE_ENABLE )
	{
		flag |= (mCurFlag & ~(PROF_DISABLE_THIS | PROF_DISABLE_CHILDREN));
		flag &= ~PROF_FORCCE_ENABLE;
	}
	else
	{
		flag |= mCurFlag;
	}

	if( flag & PROF_DISABLE_THIS )
	{
		mCurFlag = flag;
		return;
	}

	bool needCall = true;
	if( name != mCurSample->getName() )
	{
		if( flag & PROF_DISABLE_CHILDREN )
			flag |= PROF_DISABLE_THIS;

		ProfileSampleNode* node;
		if( flag & PROF_RECURSIVE_ENTRY )
		{
			node = mCurSample->getParent();
			while( node )
			{
				if( name = node->getName() )
					break;
				node = node->getParent();
			}

			if( node )
			{
				while( node != mCurSample )
				{
					stopTiming();
				}
			}
		}
		else
		{
			node = mCurSample->getSubNode(name);
		}

		if( node == nullptr )
		{
			node = CreateNode(name, mCurSample);
			mCurSample->addSubNode(node);
		}
		mCurSample = node;
	}

	mCurSample->notifyCall();
	mCurSample->mPrevFlag = mCurFlag;
	mCurSample->mLastCallFrame = ProfileSystemImpl::Get().mFrameCount;
	mCurFlag = flag;
}

void ThreadProfileData::stopTiming()
{
	if( mCurFlag & PROF_DISABLE_THIS )
		return;

	if( mCurSample->notifyReturn() )
	{
		mCurFlag = mCurSample->mPrevFlag;
		mCurSample = mCurSample->getParent();
	}
}

void ThreadProfileData::cleanup()
{
	if( mRootSample->mChild )
		cleanupNode(mRootSample->mChild);
	if( mRootSample->mSibling )
		cleanupNode(mRootSample->mSibling);

	mRootSample->mChild = nullptr;
	mRootSample->mSibling = nullptr;
	resetSample();

}

TimeScope::TimeScope(char const* name, bool bUseStack)
{
	if (bUseStack)
	{
		mResult = new TimeScopeResult;
		mResult->name = name;

		auto& stack = GetResultStack();
		stack.push_back(mResult);
	}
	else
	{
		mResult = nullptr;
		mName = name;
	}

	Profile_GetTicks(&startTime);
}

TimeScope::~TimeScope()
{
	uint64 endTime;
	Profile_GetTicks(&endTime);

	if (mResult)
	{
		mResult->duration = endTime - startTime;
		auto& stack = GetResultStack();
		stack.pop_back();

		if (stack.empty())
		{
			mResult->logMsg();
			delete mResult;
		}
		else
		{
			TimeScopeResult* parent = stack.back();
			parent->children.push_back(std::unique_ptr<TimeScopeResult>(mResult) );
		}
	}
	else
	{
		LogMsg("%s = %.3f", mName, (endTime - startTime) / 1000.0f);
	}
}
