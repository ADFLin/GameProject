#include "ProfileSystem.h"

//#include "TMessageShow.h"

#include "Clock.h"
#include "PlatformThread.h"

#include <atomic>

struct ProfileFrameData
{
	double durationSinceReset;
	double duration;
};

CORE_API extern HighResClock  GProfileClock;
CORE_API extern int    GWriteIndex;
CORE_API extern uint64 GFrameCount;
CORE_API extern RWLock GDataIndexLock;

class ThreadProfileData
{
public:
	ThreadProfileData(char const* rootName);

	std::string               mName;
	uint32                    mThreadId;
	ProfileSampleNode*	      mCurSample;
	unsigned                  mCurFlag;
	unsigned                  mCurStackNum;
	TPtrHolder< ProfileSampleNode >  mRootSample;


	void resetSample(uint64 tick);
	void tryStartTiming(const char * name, unsigned flag);
	void stopTiming();
	void setName(char const* name)
	{
		mName = name;
		mRootSample->mName = mName.c_str();
	}
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

	uint64 getFrameCountSinceReset() override { return GFrameCount; }
	double getDurationSinceReset() override
	{
		return mRecordFrames[ProfileSampleNode::GetReadIndex()].durationSinceReset;
	}


	double getLastFrameDuration() override
	{
		return mRecordFrames[ProfileSampleNode::GetReadIndex()].duration;
	}



	void   readLock() override
	{
		GDataIndexLock.readLock();
	}

	void   readUnlock() override
	{
		GDataIndexLock.readUnlock();
	}

	ProfileSampleNode* getRootSample(uint32 threadId = 0) override
	{
		if (threadId)
		{
			auto threadData = GetThreadData(threadId);
			if (threadData)
			{
				return threadData->getRootSample();
			}
			return nullptr;
		}
		else
		{
			return GetCurrentThreadData()->getRootSample();
		}
	}
	void   setThreadName(uint32 threadId, char const* name) override;
	void   getAllThreadIds(TArray<uint32>& outIds) override;
	bool   canSampling()
	{
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

HighResClock  GProfileClock;
int    GWriteIndex = 0;
uint64 GFrameCount = 0;
RWLock GDataIndexLock;

void Profile_GetTicks(uint64 * ticks)
{
	*ticks = GProfileClock.getTimeMicroseconds();
}

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

std::unordered_map< uint32, ThreadProfileData* > GThreadDataMap;
RWLock GThreadDataMapLock;

void ProfileSystemImpl::cleanup()
{
	return;

	RWLock::WriteLocker locker(GThreadDataMapLock);
	bFinalized = true;
	for (auto& pair : GThreadDataMap)
	{
		pair.second->cleanup();
		delete pair.second;
	}
	GThreadDataMap.clear();
}

void ProfileSystemImpl::resetSample()
{
	RWLock::ReadLocker locker(GThreadDataMapLock);

	GProfileClock.reset();
	GFrameCount = 0;
	GWriteIndex = 0;
	Profile_GetTicks(&mResetTime);
	//ThreadData reset
	for (auto& pair : GThreadDataMap)
	{
		pair.second->resetSample(mResetTime);
	}
}


void ProfileSystemImpl::incrementFrameCount()
{
	RWLock::WriteLocker locker(GDataIndexLock);

	uint64 tick;
	Profile_GetTicks(&tick);

	auto& frameData = mRecordFrames[ProfileSampleNode::GetWriteIndex()];
	frameData.durationSinceReset = double(tick - mResetTime) / Profile_GetTickRate();
	frameData.duration = double(tick - mFrameStartTime) / Profile_GetTickRate();

	{
		RWLock::ReadLocker  DataMaplocker(GThreadDataMapLock);
		for (auto& pair : GThreadDataMap)
		{
			pair.second->endFrame(frameData, tick);
		}

		++GFrameCount;
		GWriteIndex = 1 - GWriteIndex;
		mFrameStartTime = tick;

		for (auto& pair : GThreadDataMap)
		{
			pair.second->startFrame(tick);
		}
	}
}
void ProfileSystemImpl::setThreadName(uint32 threadId, char const* name)
{
	ThreadProfileData* threadData = GetThreadData(threadId);
	if (threadData)
	{
		threadData->setName(name);
	}
}

void ProfileSystemImpl::getAllThreadIds(TArray<uint32>& outIds)
{
	RWLock::ReadLocker  DataMaplocker(GThreadDataMapLock);

	for (auto& pair : GThreadDataMap)
	{
		outIds.push_back(pair.first);
	}
}

thread_local ThreadProfileData* GThreadDataLocal = nullptr;

ThreadProfileData* ProfileSystemImpl::GetCurrentThreadData()
{
	if( GThreadDataLocal == nullptr )
	{
		GThreadDataLocal = new ThreadProfileData(PlatformThread::GetThreadName(PlatformThread::GetCurrentThreadId()).c_str());
		GThreadDataLocal->mThreadId = PlatformThread::GetCurrentThreadId();
	
		{
			RWLock::WriteLocker locker(GThreadDataMapLock);
			GThreadDataMap.insert({ GThreadDataLocal->mThreadId , GThreadDataLocal });
		}
	}
	return GThreadDataLocal;
}

ThreadProfileData* ProfileSystemImpl::GetThreadData(uint32 threadId)
{
	RWLock::ReadLocker locker(GThreadDataMapLock);

	auto iter = GThreadDataMap.find(threadId);
	if (iter == GThreadDataMap.end())
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
	RWLock::ReadLocker locker(GDataIndexLock);

	auto& writeData = mRecoredData[GWriteIndex];
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
		{
			RWLock::ReadLocker locker(GDataIndexLock);
			auto& writeData = mRecoredData[GWriteIndex];

			writeData.timestamps.back().end = tick;

			writeData.execTime += time;
			writeData.execTimeTotal += time;
		}
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
	:mName(rootName)
{
	mCurFlag = 0;

	mRootSample.reset(CreateNode(mName.c_str(), nullptr));
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
				if( name == node->getName() )
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
