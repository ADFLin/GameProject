#ifndef ProfileSystem_h__
#define ProfileSystem_h__

#include "CoreShare.h"
#include "MarcoCommon.h"

#ifndef USE_PROFILE
#define USE_PROFILE 1
#endif


#if USE_PROFILE
#	define	PROFILE_ENTRY( name )         ProfileSample ANONYMOUS_VARIABLE(Proflie)( name );
#	define	PROFILE_ENTRY2( name , flag ) ProfileSample ANONYMOUS_VARIABLE(Proflie)( name , flag );
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

class ProfileSystem
{
public:
	~ProfileSystem ();

	class SampleIterator;
	class SampleNode;

	ProfileSystem ( char const* rootName = "Root" );


	CORE_API static ProfileSystem& Get();

	CORE_API void startProfile( const char * name , unsigned flag = 0 );
	CORE_API void stopProfile();

	SampleNode* getRootSample(){ return mRootSample; }
	CORE_API void cleanup();

	CORE_API void	reset();
	CORE_API void	incrementFrameCount();
	int		getFrameCountSinceReset( )		{ return mFrameCounter; }
	CORE_API float   getTimeSinceReset( );

	CORE_API SampleIterator  getSampleIterator( );

	CORE_API void    dumpRecursive( SampleIterator* profileIterator, int spacing );
	CORE_API void    dumpAll();

private:

	SampleNode* createSampleNode(char const* name, SampleNode* parent);
	void        destorySampleNode(SampleNode* node);

	void cleanup( SampleNode* node );

	unsigned              mCurFlag;

	unsigned              mCurStackNum;

	bool                  mResetIterator;
	TPtrHolder< SampleNode > mRootSample;
	SampleNode*	          mCurSample;
	int				      mFrameCounter;
	unsigned long int     ResetTime;
};

class	ProfileSystem::SampleNode 
{

public:
	SampleNode( const char * name, SampleNode * parent );
	~SampleNode();

	SampleNode*    getSubNode( const char * name );
	SampleNode*    getParent()	   { return mParent; }
	SampleNode*    getSibling()    { return mSibling; }
	SampleNode*    getChild()	   { return mChild; }

	char const*    getName()	   { return mName; }
	int	           getTotalCalls() { return TotalCalls; }
	float          getTotalTime()  { return TotalTime; }

	void           showChild(bool beShow )  { mIsShowChild = beShow; }
	bool           isShowChild() const      {  return mIsShowChild; }
	void           showAllChild( bool beShow );

protected:

	void                addSubNode( SampleNode* node )
	{
		node->mSibling = mChild;
		mChild = node;
	}


	void				cleanup();
	void				reset( void );
	void				onCall( void );
	bool				onReturn( void );


	const char *	    mName;
	int				    TotalCalls;
	float			    TotalTime;
	unsigned long int	StartTime;
	int				    RecursionCounter;

	bool                mIsShowChild;
	unsigned            mPrevFlag;
	unsigned            mStackNum;

	SampleNode*	        mParent;
	SampleNode*	        mChild;
	SampleNode*	        mSibling;

	friend class ProfileSystem;
};

class ProfileSystem::SampleIterator
{
	typedef ProfileSystem::SampleNode SampleNode;
public:
	SampleIterator( ProfileSystem::SampleNode * start );

	void		    first();
	void			next();
	void            before();

	bool			isDone(void);
	bool            isRoot(void) { return (parent->getParent() == 0); }

	void            enterChild();
	void			enterChild( int index );
	void			enterParent();

	// Access the current child
	const char*	    getCurrentName()       { return curNode->getName(); }
	int				getCurrentTotalCalls() { return curNode->getTotalCalls(); }
	float			getCurrentTotalTime()  { return curNode->getTotalTime(); }

	// Access the current parent
	const char *	getCurrentParentName()	    { return parent->getName(); }
	float			getCurrentParentTotalTime()	{ return parent->getTotalTime(); }

	SampleNode*  getCurNode(){ return curNode; }
	SampleNode*  getParent() { return parent; }
	SampleNode*  getChild(){ return curNode->getChild(); }

protected:
	SampleNode*	parent;
	SampleNode*	curNode;

	friend	class	 ProfileSystem;
};


template < class T >
class ProfileNodeVisitorT
{
	T* _this(){ return static_cast< T* >( this ); }
public:

	typedef ProfileSystem::SampleIterator SampleIterator;
	typedef ProfileSystem::SampleNode     SampleNode;

	void onRoot      ( SampleNode* node ){}
	void onNode      ( SampleNode* node , double parentTime ){}
	bool onEnterChild( SampleNode* node ){ return true; }
	void onEnterParent( SampleNode* node , int numChildren , double accTime ){}


	void visitNodes()
	{
		SampleIterator iter = ProfileSystem::Get().getSampleIterator();
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


class CORE_API ProfileSample
{
public:
	ProfileSample( const char * name , unsigned flag = 0 );
	~ProfileSample( void );
};



#endif // ProfileSystem_h__
