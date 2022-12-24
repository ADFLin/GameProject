#pragma once
#ifndef ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75
#define ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75

#include "CoreShare.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"
#include "LogSystem.h"
#include "Clock.h"

#include <unordered_map>

#ifndef USE_PROFILE
#define USE_PROFILE 1
#endif


#if USE_PROFILE
#	define	PROFILE_ENTRY( name )         ProfileSampleScope ANONYMOUS_VARIABLE(Proflie)( name );
#	define	PROFILE_ENTRY2( name , flag ) ProfileSampleScope ANONYMOUS_VARIABLE(Proflie)( name , flag );
#   define	PROFILE_FUNCTION()             ProfileSampleScope ANONYMOUS_VARIABLE(ProflieFunc)( __FUNCTION__);
#else
#	define	PROFILE_ENTRY( name )
#	define	PROFILE_ENTRY2( name , flag )
#   define	PROFILE_FUNCTION()
#endif //USE_PROFILE


#include "Holder.h"
#include "Singleton.h"

#define  CLOCK_EPSILON 1e-6

class ProfileSystem;


enum ProfileFlag
{
	PROF_RECURSIVE_ENTRY  = 1,
	PROF_DISABLE_THIS     = 2,
	PROF_DISABLE_CHILDREN = 4,
	PROF_FORCCE_ENABLE    = 8,
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
	int	           getTotalCalls() const { return mTotalCalls; }
	int            getFrameCalls() const { return mFrameCalls; }
	double         getTotalTime() const { return mTotalTime; }
	double         getLastFrameTime() const { return mLastFrameTime; }
	void           showChild(bool beShow) { mIsShowChild = beShow; }
	bool           isShowChild() const { return mIsShowChild; }
	void           showAllChild(bool beShow);

protected:

	void  addSubNode(ProfileSampleNode* node)
	{
		CHECK(node);
		node->mSibling = mChild;
		mChild = node;
	}

	void				unlink();
	void				reset();
	void				notifyCall();
	bool				notifyReturn();

	int                 mLastCallFrame;
	int                 mFrameCalls;
	int				    mTotalCalls;
	double			    mTotalTime;
	double              mLastFrameTime;
	uint64           	mStartTime;

	const char *	    mName;
	uint64              mTimestamp[2];

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

	virtual void   cleanup() = 0;
	virtual void   resetSample() = 0;

	virtual void   incrementFrameCount() = 0;
	virtual int	   getFrameCountSinceReset() = 0;
	virtual double getDurationSinceReset() = 0;
	virtual double getLastFrameDuration() = 0;
	virtual ProfileSampleNode* getRootSample(uint32 ThreadId = 0) = 0;
};

class  ProfileSampleScope
{
public:
	CORE_API  ProfileSampleScope( const char * name , unsigned flag = 0 );
	CORE_API ~ProfileSampleScope( void );
};

struct TimeScopeResult;
struct TimeScope
{
	TimeScope(char const* name , bool bUseStack = true);
	~TimeScope();

private:
	uint64 startTime;
	CORE_API std::vector< TimeScopeResult* >& GetResultStack();

	char const* mName;
	TimeScopeResult* mResult;
};

#define TIME_SCOPE( NAME , ... ) TimeScope ANONYMOUS_VARIABLE(timeScope)( NAME , ##__VA_ARGS__ );



#endif // ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75
