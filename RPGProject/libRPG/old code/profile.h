#ifndef profile_h__
#define profile_h__

#include "LinearMath/btQuickprof.h"
class TMessageShow;

#define  USE_PROFILE

#ifdef USE_PROFILE
#define	TPROFILE( name )	TFlyProfileSample __profile( name )
#else
#define	TPROFILE( name )
#endif //USE_PROFILE


class TClock
{
public:
	TClock()
	{
		QueryPerformanceFrequency(&mClockFrequency);
		reset();
	}

	~TClock()
	{
	}

	/// Resets the initial reference time.
	void reset()
	{
		QueryPerformanceCounter(&mStartTime);
		mStartTick = GetTickCount();
		mPrevElapsedTime = 0;
	}

	/// Returns the time in ms since the last call to reset or since 
	/// the btClock was created.
	unsigned long int getTimeMilliseconds();
	unsigned long int getTimeMicroseconds();

private:
	LARGE_INTEGER mClockFrequency;
	DWORD mStartTick;
	LONGLONG mPrevElapsedTime;
	LARGE_INTEGER mStartTime;
};


class	TProfileNode 
{

public:
	TProfileNode( const char * name, TProfileNode * parent );
	~TProfileNode( void );

	TProfileNode* getSubNode( const char * name );
	TProfileNode* getParent( void )	{ return Parent; }
	TProfileNode* getSibling( void ){ return Sibling; }
	TProfileNode* getChild( void )	{ return Child; }

	void				cleanupMemory();
	void				reset( void );
	void				OnCall( void );
	bool				OnReturn( void );

	const char *	    getName( void )		    { return Name; }
	int				    getTotalCalls( void )	{ return TotalCalls; }
	float				getTotalTime( void )	{ return TotalTime; }

	void                showChild(bool beShow ){ m_isShowChild = beShow; }
	bool                isShowChild() const { return m_isShowChild; }
	void                showAllChild( bool beShow );
protected:

	const char *	    Name;
	int				    TotalCalls;
	float			    TotalTime;
	unsigned long int	StartTime;
	int				    RecursionCounter;

	bool  m_isShowChild;
	
	TProfileNode *	Parent;
	TProfileNode *	Child;
	TProfileNode *	Sibling;
};

class TProfileManager;
class TProfileIterator
{
public:
	TProfileIterator( TProfileNode * start );

	void		    First(void);
	void			Next(void);
	void            Before();

	bool			isDone(void);
	bool            isRoot(void) { return (parent->getParent() == 0); }

	void            enterChild();
	void			enterChild( int index );		// Make the given child the new parent
	void			enterLargestChild( void );	// Make the largest child the new parent
	void			enterParent( void );			// Make the current parent's parent the new parent

	// Access the current child
	const char *	getCurrentName( void )		{ return curNode->getName(); }
	int				getCurrentTotalCalls( void )	{ return curNode->getTotalCalls(); }
	float			getCurrentTotalTime( void )	{ return curNode->getTotalTime(); }

	// Access the current parent
	const char *	getCurrentParentName( void )			{ return parent->getName(); }
	int				getCurrentParentTotalCalls( void )	{ return parent->getTotalCalls(); }
	float			getCurrentParentTotalTime( void )	{ return parent->getTotalTime(); }


	TProfileNode*  getCurNode(){ return curNode; }
	TProfileNode*  getParent(){ return parent; }
	TProfileNode*  getCurNodeChild(){ return curNode->getChild(); }

protected:
	TProfileNode*	parent;
	TProfileNode*	curNode;

	friend	class	 TProfileManager;
};

class	TProfileManager 
{
public:
	static	void	startProfile( const char * name );
	static	void	stopProfile( void );

	static	void	cleanupMemory();

	static	void	reset( void );
	static	void	incrementFrameCount( void );
	static	int		getFrameCountSinceReset( void )		{ return FrameCounter; }
	static	float   getTimeSinceReset( void );

	static	TProfileIterator*	createIterator( void )	
	{ 
		return new TProfileIterator( &Root ); 
	}
	static void  releaseIterator( TProfileIterator* iterator ) { delete iterator; }
	static void  dumpRecursive(TProfileIterator* profileIterator, int spacing);
	static void	 dumpAll();

	static  bool  resetIter;

private:
	
	static	TProfileNode	  Root;
	static	TProfileNode *	  CurrentNode;
	static	int				  FrameCounter;
	static	unsigned long int ResetTime;
};




class	TFlyProfileSample {
public:
	TFlyProfileSample( const char * name )
	{ 
		TProfileManager::startProfile( name ); 
	}

	~TFlyProfileSample( void )					
	{ 
		TProfileManager::stopProfile(); 
	}
};

void showProfileInfo( TProfileIterator* profIter , TMessageShow& msgShow , TProfileIterator* curProfIter );
#endif // profile_h__
