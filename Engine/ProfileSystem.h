#pragma once
#ifndef ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75
#define ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75

#include "CoreShare.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"
#include "LogSystem.h"
#include "Clock.h"
#include "Holder.h"
#include "Singleton.h"
#include "DataStructure/Array.h"

#include <unordered_map>

#ifndef USE_PROFILE
#define USE_PROFILE 1
#endif


#if USE_PROFILE
#	define	PROFILE_ENTRY( NAME , ... )    ProfileSampleScope ANONYMOUS_VARIABLE(Proflie)( NAME, ##__VA_ARGS__);
#   define	PROFILE_FUNCTION()             ProfileSampleScope ANONYMOUS_VARIABLE(ProflieFunc)( __FUNCTION__);
#else
#	define	PROFILE_ENTRY( NAME )
#   define	PROFILE_FUNCTION()
#endif //USE_PROFILE




#define  CLOCK_EPSILON 1e-6

inline double Profile_GetTickRate()
{
	//	return 1000000.f;
	return 1000.f;
}
CORE_API void Profile_GetTicks(uint64* ticks);
class ProfileSystem;


enum ProfileFlag
{
	PROF_RECURSIVE_ENTRY  = 1,
	PROF_DISABLE_THIS     = 2,
	PROF_DISABLE_CHILDREN = 4,
	PROF_FORCCE_ENABLE    = 8,
};


struct ProfileTimestamp
{
	uint64 start;
	uint64 end;
};

class ProfileSampleNode
{
public:
	ProfileSampleNode(const char * name, ProfileSampleNode * parent);
	~ProfileSampleNode();


	bool haveParent() const { return !!mParent; }

	ProfileSampleNode*    getSubNode(const char * name);
	ProfileSampleNode*    getParent() { return mParent; }
	ProfileSampleNode*    getSibling() { return mSibling; }
	ProfileSampleNode*    getChild() { return mChild; }

	char const*    getName() const { return mName; }
	int	           getTotalCalls() const { return getReadData().callCountTotal; }
	int            getFrameCalls() const { return getReadData().callCount; }
	double         getTotalTime() const { return getReadData().execTimeTotal; }
	double         getFrameExecTime() const { return getReadData().execTime; }
	TArray<ProfileTimestamp> const& getFrameTimestamps() const { return getReadData().timestamps; }
	uint64         getLastFrame() const { return getReadData().frame; }
	void           showChild(bool beShow) { mIsShowChild = beShow; }
	bool           isShowChild() const { return mIsShowChild; }
	void           showAllChild(bool beShow);


	CORE_API static int GetReadIndex();
	CORE_API static int GetWriteIndex();

	char const*         mCategory = nullptr;

protected:


	void  addSubNode(ProfileSampleNode* node)
	{
		CHECK(node);
		node->mSibling = mChild;
		mChild = node;
	}

	void				unlink();
	void				reset();
	void                resetFrame();
	void				notifyCall();
	bool				notifyReturn();
	void                notifyPause(uint64 tick);
	void                notifyResume();
	

	struct FrameData
	{
		uint64 frame;
		int    callCount;
		int    callCountTotal;

		double execTime;
		double execTimeTotal;

		TArray<ProfileTimestamp> timestamps;

		void resetFrame()
		{
			execTime = 0;
			callCount = 0;
			timestamps.clear();
		}
		void reset()
		{
			resetFrame();
			execTimeTotal = 0;
			callCountTotal = 0;
		}
	};

	FrameData const& getReadData() const
	{
		return mRecoredData[GetReadIndex()];
	}


	FrameData           mRecoredData[2];


	uint64              mLastRecordFrame;
	uint64           	mStartTime;

	char const*	        mName;


	int				    mRecursionCounter;
	bool                mIsShowChild;
	unsigned            mPrevFlag;
	unsigned            mStackNum;

	ProfileSampleNode*	mParent;
	ProfileSampleNode*	mChild;
	ProfileSampleNode*	mSibling;

	friend class ThreadProfileData;
};

class ProfileSystem
{
public:
	CORE_API static ProfileSystem& Get();

	virtual bool   isEnabled() = 0;
	virtual void   setEnabled(bool bNewEnabled) = 0;
	virtual void   cleanup() = 0;
	virtual void   resetSample() = 0;

	virtual void   incrementFrameCount() = 0;
	virtual uint64 getFrameCountSinceReset() = 0;
	virtual double getDurationSinceReset() = 0;
	virtual double getLastFrameDuration() = 0;
	virtual void   readLock() = 0;
	virtual void   readUnlock() = 0;

	virtual ProfileSampleNode* getRootSample(uint32 threadId = 0) = 0;
	virtual void   setThreadName(uint32 threadId, char const* name) = 0;
	virtual void   getAllThreadIds(TArray<uint32>& outIds) = 0;
};

class ProfileReadScope
{
public:
	ProfileReadScope()
	{
		ProfileSystem::Get().readLock();
	}
	~ProfileReadScope()
	{
		ProfileSystem::Get().readUnlock();
	}
};
class  ProfileSampleScope
{
public:
	CORE_API  ProfileSampleScope(const char * name, unsigned flag = 0);
	CORE_API  ProfileSampleScope(const char * name, char const* category, unsigned flag = 0);
	CORE_API ~ProfileSampleScope( void );
};

struct TimeScopeResult;
struct TimeScope
{
	TimeScope(char const* name , bool bUseStack = true);
	~TimeScope();

private:
	uint64 startTime;
	CORE_API TArray< TimeScopeResult* >& GetResultStack();

	char const* mName;
	TimeScopeResult* mResult;
};

#define TIME_SCOPE( NAME , ... ) TimeScope ANONYMOUS_VARIABLE(timeScope)( NAME , ##__VA_ARGS__ );



#endif // ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75
