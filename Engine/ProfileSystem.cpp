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


struct ProfileFrameData
{
	double durationSinceReset;
	double duration;
};

int    GWriteIndex = 0;
uint64 GFrameCount = 0;

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


	void resetSample(uint64 tick);
	void tryStartTiming(const char * name, unsigned flag);
	void stopTiming();

	void endFrame(ProfileFrameData const& frameData, uint64 tick)
	{
		ProfileSampleNode* node = mCurSample;
		while (node != nullptr)
		{
			node->notifyPause(tick);
			node = node->mParent;
		}
		auto& writeData = mRootSample->mRecoredData[GWriteIndex];
		writeData.execTime = frameData.duration;
		writeData.execTimeTotal = frameData.durationSinceReset;
	}

	void startFrame(uint64 tick)
	{
		ResumeNode(mCurSample);
		auto& writeData = mRootSample->mRecoredData[GWriteIndex];
		writeData.callCount = 1;
		writeData.callCountTotal = GFrameCount;
	}

	static void ResumeNode(ProfileSampleNode* node)
	{
#if 0
		if (node->mParent)
		{
			ResumeNode(node->mParent);
		}
		node->notifyResume();
#else
		while (node != nullptr)
		{
			node->notifyResume();
			node = node->mParent;
		}
#endif
	}
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



class ProfileSystemImpl final : public ProfileSystem
{
public:

	ProfileSystemImpl(char const* rootName = "Root");
	~ProfileSystemImpl();

	bool   isEnabled() { return bEnabled; }
	void   setEnabled(bool bNewEnabled)
	{
		if (bNewEnabled != bEnabled)
		{
			bEnabled = bNewEnabled;
		}
	}
	void   cleanup() override;
	void   resetSample() override;

	void   incrementFrameCount() override;
	int	   getFrameCountSinceReset() override { return GFrameCount; }
	double getDurationSinceReset() override
	{
		return mRecordFrames[ProfileSampleNode::GetReadIndex()].durationSinceReset;
	}


	double getLastFrameDuration() override
	{
		return mRecordFrames[ProfileSampleNode::GetReadIndex()].duration;
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

	bool   canSampling()
	{
		return true;
		return bEnabled &&  !bFinalized;
	}

	static ThreadProfileData* GetCurrentThreadData();
	static ThreadProfileData* GetThreadData(uint32 threadId);


	static ProfileSystemImpl& Get() { return static_cast< ProfileSystemImpl& >( ProfileSystem::Get()); }

	uint8      bFinalized : 1;
	uint8      bEnabled   : 1;
	uint8      bResampleRequested : 1;
private:


	friend class ThreadProfileData;

	ProfileFrameData mRecordFrames[2];

	uint64     mResetTime;
	uint64     mFrameStartTime;

	
};

struct TimeScopeResult
{
	~TimeScopeResult() = default;

	std::string name;
	TArray< std::unique_ptr< TimeScopeResult > > children;
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

static ProfileSystemImpl GSystem;
thread_local TArray< TimeScopeResult* > GTimeScopeStacks;
TArray< TimeScopeResult* >& TimeScope::GetResultStack()
{
	return GTimeScopeStacks;
}

int ProfileSampleNode::GetReadIndex()
{
	return 1 - GWriteIndex;
}

int ProfileSampleNode::GetWriteIndex()
{
	return GWriteIndex;
}

ProfileSystem& ProfileSystem::Get()
{
	return GSystem;
}

ProfileSampleScope::ProfileSampleScope(const char * name, unsigned flag)
{
	if (!GSystem.canSampling())
		return;

	ProfileSystemImpl::GetCurrentThreadData()->tryStartTiming(name, flag);
}

ProfileSampleScope::~ProfileSampleScope(void)
{
	if (!GSystem.canSampling())
		return;

	ProfileSystemImpl::GetCurrentThreadData()->stopTiming();
}

#endif //CORE_SHARE_CODE


ProfileSystemImpl::ProfileSystemImpl(char const* rootName)
	:mResetTime(0)
{
	for (int i = 0; i < 2; ++i) 
	{
		mRecordFrames[i].duration = 0;
		mRecordFrames[i].durationSinceReset = 0;
	}

	bResampleRequested = false;
	bFinalized = false;
	bEnabled = true;
}

ProfileSystemImpl::~ProfileSystemImpl()
{
	cleanup();
}

std::unordered_map< uint32, ThreadProfileData* > ThreadDataMap;

void ProfileSystemImpl::cleanup()
{
	bFinalized = true;
	for (auto& pair : ThreadDataMap)
	{
		pair.second->cleanup();
		delete pair.second;
	}
	ThreadDataMap.clear();
}

void ProfileSystemImpl::resetSample()
{
	GProfileClock.reset();
	GFrameCount = 0;
	GWriteIndex = 0;
	Profile_GetTicks(&mResetTime);
	//ThreadData reset
	for (auto& pair : ThreadDataMap)
	{
		pair.second->resetSample(mResetTime);
	}
}

void ProfileSystemImpl::incrementFrameCount()
{

	uint64 tick;
	Profile_GetTicks(&tick);

	auto& frameData = mRecordFrames[ProfileSampleNode::GetWriteIndex()];
	frameData.durationSinceReset = double(tick - mResetTime) / Profile_GetTickRate();
	frameData.duration = double(tick - mFrameStartTime) / Profile_GetTickRate();
	for (auto& pair : ThreadDataMap)
	{
		pair.second->endFrame(frameData, tick);
	}

	++GFrameCount;
	GWriteIndex = 1 - GWriteIndex;
	mFrameStartTime = tick;

	for (auto& pair : ThreadDataMap)
	{
		pair.second->startFrame(tick);
	}
}

thread_local ThreadProfileData* GThreadDataLocal = nullptr;

ThreadProfileData* ProfileSystemImpl::GetCurrentThreadData()
{
	if( GThreadDataLocal == nullptr )
	{
		GThreadDataLocal = new ThreadProfileData("Root");
		GThreadDataLocal->mThreadId = Thread::GetCurrentThreadId();
		ThreadDataMap.insert({ GThreadDataLocal->mThreadId , GThreadDataLocal });
	}
	return GThreadDataLocal;
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
	,mLastRecordFrame(0)
	,mStartTime(0)
	,mRecursionCounter(0)
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
	for (int i = 0; i < 2; ++i)
	{
		mRecoredData[i].reset();
	}

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


void ProfileSampleNode::resetFrame()
{
	auto const& readData = mRecoredData[GetReadIndex()];
	auto& writeData = mRecoredData[GetWriteIndex()];

	writeData.resetFrame();
	writeData.callCountTotal = readData.callCountTotal;
	writeData.execTimeTotal = readData.execTimeTotal;
}

void ProfileSampleNode::notifyCall()
{
	auto& writeData = mRecoredData[GetWriteIndex()];
	if (mRecursionCounter == 0) 
	{
		if (mLastRecordFrame != GFrameCount)
		{
			mLastRecordFrame = GFrameCount;
			resetFrame();
		}

		Profile_GetTicks(&mStartTime);
		writeData.callCount += 1;
		ProfileTimestamp timestamp;
		timestamp.start = mStartTime;
		timestamp.end = mStartTime;
		writeData.timestamps.push_back(timestamp);
		writeData.frame = GFrameCount;
	}

	++mRecursionCounter;
	writeData.callCountTotal += 1;
}

bool ProfileSampleNode::notifyReturn()
{
	--mRecursionCounter;
	if (mRecursionCounter != 0)
		return false;

	{ 
		uint64 tick;
		Profile_GetTicks(&tick);	
		double time = (tick - mStartTime ) / Profile_GetTickRate();
		auto& writeData = mRecoredData[GetWriteIndex()];

		writeData.timestamps.back().end = tick;

		writeData.execTime += time;
		writeData.execTimeTotal += time;
	}
	return true;
}

void ProfileSampleNode::notifyPause(uint64 tick)
{
	double time = (tick - mStartTime) / Profile_GetTickRate();
	auto& writeData = mRecoredData[GetWriteIndex()];

	writeData.timestamps.back().end = tick;
	writeData.execTime += time;
	writeData.execTimeTotal += time;
	mStartTime = tick;
}

void ProfileSampleNode::notifyResume()
{
	mLastRecordFrame = GFrameCount;
	resetFrame();

	auto& writeData = mRecoredData[GetWriteIndex()];
	ProfileTimestamp timestamp;
	timestamp.start = mStartTime;
	timestamp.end = mStartTime;
	writeData.timestamps.push_back(timestamp);
	writeData.frame = GFrameCount;
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
	for (int i = 0; i < 2; ++i)
	{
		mRootSample->mRecoredData[i].callCount = 1;
		ProfileTimestamp timestamp;
		timestamp.start = 0;
		timestamp.end = 0;
		mRootSample->mRecoredData[i].timestamps.push_back(timestamp);
	}

	mCurSample = mRootSample.get();
}


void ThreadProfileData::resetSample(uint64 tick)
{
	mRootSample->reset();
	mRootSample->mStartTime = tick;
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

	mRootSample.clear();
	mCurSample = nullptr;
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
