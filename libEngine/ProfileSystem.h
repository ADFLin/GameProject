#pragma once
#ifndef ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75
#define ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75

#include "CoreShare.h"

#include "Core/IntegerType.h"
#include "MarcoCommon.h"

#include <unordered_map>

#ifndef USE_PROFILE
#define USE_PROFILE 1
#endif


#if USE_PROFILE
#	define	PROFILE_ENTRY( name )         ProfileSampleScope ANONYMOUS_VARIABLE(Proflie)( name );
#	define	PROFILE_ENTRY2( name , flag ) ProfileSampleScope ANONYMOUS_VARIABLE(Proflie)( name , flag );
#else
#	define	PROFILE_ENTRY( name )
#	define	PROFILE_ENTRY2( name , flag )
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

class	ProfileSampleNode
{
public:
	ProfileSampleNode(const char * name, ProfileSampleNode * parent);
	~ProfileSampleNode();

	ProfileSampleNode*    getSubNode(const char * name);
	ProfileSampleNode*    getParent() { return mParent; }
	ProfileSampleNode*    getSibling() { return mSibling; }
	ProfileSampleNode*    getChild() { return mChild; }

	char const*    getName() { return mName; }
	int	           getTotalCalls() { return TotalCalls; }
	float          getTotalTime() { return TotalTime; }

	void           showChild(bool beShow) { mIsShowChild = beShow; }
	bool           isShowChild() const { return mIsShowChild; }
	void           showAllChild(bool beShow);

protected:

	void                addSubNode(ProfileSampleNode* node)
	{
		node->mSibling = mChild;
		mChild = node;
	}


	void				unlink();
	void				reset(void);
	void				onCall(void);
	bool				onReturn(void);


	const char *	    mName;
	int				    TotalCalls;
	float			    TotalTime;
	unsigned long int	StartTime;
	int				    RecursionCounter;

	bool                mIsShowChild;
	unsigned            mPrevFlag;
	unsigned            mStackNum;

	ProfileSampleNode*	mParent;
	ProfileSampleNode*	mChild;
	ProfileSampleNode*	mSibling;


	friend class ThreadProfileData;
};


class ProfileSampleIterator
{
public:
	ProfileSampleIterator(){}
	ProfileSampleIterator(ProfileSampleNode * start);

	void		    first();
	void			next();
	void            before();

	bool			isDone(void);
	bool            isRoot(void) { return (parent->getParent() == 0); }

	void            enterChild();
	void			enterChild(int index);
	void			enterParent();

	// Access the current child
	const char*	    getCurrentName() { return curNode->getName(); }
	int				getCurrentTotalCalls() { return curNode->getTotalCalls(); }
	float			getCurrentTotalTime() { return curNode->getTotalTime(); }

	// Access the current parent
	const char *	getCurrentParentName() { return parent->getName(); }
	float			getCurrentParentTotalTime() { return parent->getTotalTime(); }

	ProfileSampleNode*  getCurNode() { return curNode; }
	ProfileSampleNode*  getParent() { return parent; }
	ProfileSampleNode*  getChild() { return curNode->getChild(); }

protected:
	ProfileSampleNode*	parent;
	ProfileSampleNode*	curNode;

	friend	class	 ProfileSystem;
};

class ThreadProfileData
{
public:
	ThreadProfileData(char const* rootName);
	
	
	uint32                    mThreadId;
	ProfileSampleNode*	      mCurSample;
	unsigned                  mCurFlag;
	unsigned                  mCurStackNum;
	TPtrHolder< ProfileSampleNode >  mRootSample;


	void resetSample();
	void tryStartTiming(const char * name, unsigned flag);
	void stopTiming();

	void cleanup();


	ProfileSampleIterator getSampleIterator()
	{
		return ProfileSampleIterator(mRootSample.get());
	}

	static void cleanupNode(ProfileSampleNode* node)
	{
		if( node->mChild )
			cleanupNode(node->mChild);

		if( node->mSibling )
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

class ProfileSystem
{
public:
	CORE_API static ProfileSystem& Get();

	virtual void  cleanup() = 0;
	virtual void  resetSample() = 0;

	virtual void  incrementFrameCount() = 0;
	virtual int	  getFrameCountSinceReset() = 0;
	virtual float  getTimeSinceReset() = 0;

	virtual ProfileSampleIterator getSampleIterator(uint32 ThreadId = 0) = 0;

#if 0
	virtual void   dumpRecursive(ProfileSampleIterator* profileIterator, int spacing) = 0;
	virtual void   dumpAll(uint32 ThreadId) = 0;
#endif
};




template < class T >
class ProfileNodeVisitorT
{
	T* _this(){ return static_cast< T* >( this ); }
public:

	typedef ProfileSampleIterator SampleIterator;
	typedef ProfileSampleNode     SampleNode;

	void onRoot      ( SampleNode* node ){}
	void onNode      ( SampleNode* node , double parentTime ){}
	bool onEnterChild( SampleNode* node ){ return true; }
	void onEnterParent( SampleNode* node , int numChildren , double accTime ){}


	void visitNodes( uint32 threadId = 0)
	{
		SampleIterator iter = ProfileSystem::Get().getSampleIterator(threadId);
		visitRecursive( &iter );
	}

	void visitRecursive( SampleIterator* profIter )
	{
		SampleNode* parent = profIter->getParent();

		if ( !_this()->onEnterChild( parent ) )
			return;

		double parentTime;
		if ( parent->getParent() != 0 )
			parentTime = parent->getTotalTime();
		else
			parentTime = ProfileSystem::Get().getTimeSinceReset();

		if ( profIter->isRoot() )
		{
			_this()->onRoot( parent );
		}


		double accTime = 0;
		int    numChildren = 0;


		for ( ; !profIter->isDone(); profIter->next() )
		{
			SampleNode* curNode = profIter->getCurNode();

			_this()->onNode( curNode , parentTime );

			++numChildren;
			accTime += curNode->getTotalTime();

			SampleIterator iter = *profIter;

			iter.enterChild();
			iter.first();
			visitRecursive( &iter );
			iter.enterParent();
		}

		_this()->onEnterParent( parent , numChildren , accTime );
	}
};


class  ProfileSampleScope
{
public:
	CORE_API  ProfileSampleScope( const char * name , unsigned flag = 0 );
	CORE_API ~ProfileSampleScope( void );
};


#endif // ProfileSystem_H_246AD834_B5C3_4C3C_B24C_D6ECCC926F75
