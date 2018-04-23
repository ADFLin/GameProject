#include "ProfileSystem.h"

//#include "TMessageShow.h"

#include "Clock.h"
HighResClock   g_profileClock;

inline void Profile_Get_Ticks(unsigned long int * ticks)
{
	*ticks = g_profileClock.getTimeMicroseconds();
}

inline float Profile_Get_Tick_Rate(void)
{
	//	return 1000000.f;
	return 1000.f;
}


ProfileSystem::SampleNode::SampleNode( const char * name, SampleNode * parent ) 
	:mName( name )
	,TotalCalls( 0 )
	,TotalTime( 0 )
	,StartTime( 0 )
	,RecursionCounter( 0 )
	,mParent( parent )
	,mChild( NULL )
	,mSibling( NULL )
	,mIsShowChild( true )
{
	reset();
}


void	ProfileSystem::SampleNode::cleanup()
{
	mChild = NULL;
	mSibling = NULL;
}

ProfileSystem::SampleNode::~SampleNode()
{

}


ProfileSystem::SampleNode * ProfileSystem::SampleNode::getSubNode( const char * name )
{
	// Try to find this sub node
	ProfileSystem::SampleNode * child = mChild;
	while ( child ) 
	{
		if ( child->mName == name ) 
		{
			return child;
		}
		child = child->mSibling;
	}
	return NULL;

}


void	ProfileSystem::SampleNode::reset( void )
{
	TotalCalls = 0;
	TotalTime = 0.0f;

	if ( mChild ) {
		mChild->reset();
	}
	if ( mSibling ) {
		mSibling->reset();
	}
}


void	ProfileSystem::SampleNode::onCall( void )
{
	TotalCalls++;
	if (RecursionCounter++ == 0) {
		Profile_Get_Ticks(&StartTime);
	}
}


bool	ProfileSystem::SampleNode::onReturn( void )
{
	if ( --RecursionCounter == 0 && TotalCalls != 0 ) { 
		unsigned long int time;
		Profile_Get_Ticks(&time);
		time-=StartTime;
		TotalTime += (float)time / Profile_Get_Tick_Rate();
	}
	return ( RecursionCounter == 0 );
}

void ProfileSystem::SampleNode::showAllChild( bool beShow )
{
	ProfileSystem::SampleNode* node = getChild();

	showChild( beShow  );
	while ( node )
	{
		node->showAllChild( beShow );
		node = node->getSibling();
	}
}
ProfileSystem::SampleIterator::SampleIterator( ProfileSystem::SampleNode * start )
{
	parent = start;
	curNode = parent->getChild();
}


void	ProfileSystem::SampleIterator::first()
{
	curNode = parent->getChild();
}


void	ProfileSystem::SampleIterator::next()
{
	curNode = curNode->getSibling();
}

bool	ProfileSystem::SampleIterator::isDone()
{
	return curNode == NULL;
}

void	ProfileSystem::SampleIterator::enterChild( int index )
{
	curNode = parent->getChild();
	while ( (curNode != NULL) && (index != 0) ) 
	{
		index--;
		curNode = curNode->getSibling();
	}

	if ( curNode != NULL ) 
	{
		parent = curNode;
		curNode = parent->getChild();
	}
}

void ProfileSystem::SampleIterator::enterChild()
{
	ProfileSystem::SampleNode* nextChild = curNode->getChild();

	parent = curNode;
	curNode  = nextChild;
}

void	ProfileSystem::SampleIterator::enterParent()
{
	if ( parent->getParent() != NULL ) 
	{
		parent = parent->getParent();
	}
	curNode = parent->getChild();
}

void ProfileSystem::SampleIterator::before()
{
	ProfileSystem::SampleNode* node = parent->getChild();

	if ( node == curNode )
		return;

	while( node )
	{
		if ( node->getSibling() == curNode )
			break;
		node = node->getSibling();
	}
	curNode = node;
}



ProfileSystem::ProfileSystem( char const* rootName ) 
	:mFrameCounter(0)
	,ResetTime(0)
	,mResetIterator( false )
{

	mCurFlag = 0;

	mRootSample.reset( createSampleNode( rootName , NULL ) );
	mRootSample->mPrevFlag = 0;
	mRootSample->mStackNum = 0;

	mCurSample = mRootSample.get();
}

ProfileSystem& ProfileSystem::Get()
{
	static ProfileSystem sInstance;
	return sInstance;
}

ProfileSystem::~ProfileSystem()
{
	cleanup();
}

void	ProfileSystem::startProfile( const char * name , unsigned flag )
{

	if ( flag & PROF_FORCCE_ENABLE )
	{
		flag |= ( mCurFlag & ~( PROF_DISABLE_THIS | PROF_DISABLE_CHILDREN ) );
		flag &= ~PROF_FORCCE_ENABLE ;
	}
	else
	{
		flag |= mCurFlag;
	}
	
	if ( flag & PROF_DISABLE_THIS )
	{
		mCurFlag = flag;
		return;
	}

	bool needCall = true;
	if ( name != mCurSample->getName() ) 
	{
		if ( flag & PROF_DISABLE_CHILDREN )
			flag |= PROF_DISABLE_THIS;

		SampleNode* node;
		if ( flag & PROF_RECURSIVE_ENTRY )
		{
			node = mCurSample->getParent();
			while( node )
			{
				if ( name = node->getName() )
					break;
				node = node->getParent();
			}

			if ( node )
			{
				while( node != mCurSample )
				{
					stopProfile();
				}
			}
		}
		else
		{
			node = mCurSample->getSubNode( name );
		}


		if ( node == NULL )
		{
			node = createSampleNode( name, mCurSample );
			mCurSample->addSubNode( node );
		}
		mCurSample = node;		
	} 

	mCurSample->onCall();
	mCurSample->mPrevFlag = mCurFlag;
	mCurFlag = flag;
}


void	ProfileSystem::stopProfile( void )
{	
	if ( mCurSample->onReturn())
	{
		mCurFlag = mCurSample->mPrevFlag;
		mCurSample = mCurSample->getParent();
	}
}



void	ProfileSystem::reset( )
{ 
	g_profileClock.reset();
	mRootSample->reset();
    mRootSample->onCall();
	mFrameCounter = 0;
	Profile_Get_Ticks(&ResetTime);
}

void ProfileSystem::incrementFrameCount( )
{
	mFrameCounter++;
}


float ProfileSystem::getTimeSinceReset( )
{
	unsigned long int time;
	Profile_Get_Ticks(&time);
	time -= ResetTime;
	return (float)time / Profile_Get_Tick_Rate();
}

#include <stdio.h>

void	ProfileSystem::dumpRecursive(ProfileSystem::SampleIterator* profileIterator, int spacing)
{
	profileIterator->first();
	if (profileIterator->isDone())
		return;

	float accumulated_time=0,parent_time = profileIterator->isRoot() ? ProfileSystem::getTimeSinceReset() : profileIterator->getCurrentParentTotalTime();
	int i;
	int frames_since_reset = ProfileSystem::getFrameCountSinceReset();
	for (i=0;i<spacing;i++)	printf(".");
	printf("----------------------------------\n");
	for (i=0;i<spacing;i++)	printf(".");
	printf("Profiling: %s (total running time: %.3f ms) ---\n",	profileIterator->getCurrentParentName(), parent_time );
	float totalTime = 0.f;

	
	int numChildren = 0;
	
	for (i = 0; !profileIterator->isDone(); i++,profileIterator->next())
	{
		numChildren++;
		float current_total_time = profileIterator->getCurrentTotalTime();
		accumulated_time += current_total_time;
		float fraction = parent_time > CLOCK_EPSILON ? (current_total_time / parent_time) * 100 : 0.f;
		{
			int i;	for (i=0;i<spacing;i++)	printf(".");
		}
		printf("%d -- %s (%.2f %%) :: %.3f ms / frame (%d calls)\n",i, profileIterator->getCurrentName(), fraction,(current_total_time / (double)frames_since_reset),profileIterator->getCurrentTotalCalls());
		totalTime += current_total_time;
		//recurse into children
	}

	if (parent_time < accumulated_time)
	{
		printf("what's wrong\n");
	}
	for (i=0;i<spacing;i++)	printf(".");
	printf("%s (%.3f %%) :: %.3f ms\n", "Unaccounted:",parent_time > CLOCK_EPSILON ? ((parent_time - accumulated_time) / parent_time) * 100 : 0.f, parent_time - accumulated_time);
	
	for (i=0;i<numChildren;i++)
	{
		profileIterator->enterChild(i);
		dumpRecursive(profileIterator,spacing+3);
		profileIterator->enterParent();
	}
}



void	ProfileSystem::dumpAll()
{
	ProfileSystem::SampleIterator profileIterator =
		ProfileSystem::getSampleIterator();

	dumpRecursive( &profileIterator ,0 );
}



void ProfileSystem::cleanup( SampleNode* node )
{
	if ( node->mChild )
		cleanup( node->mChild );

	if ( node->mSibling )
		cleanup( node->mSibling );

	node->cleanup();
	destorySampleNode( node );
}

void ProfileSystem::cleanup()
{
	if ( mRootSample->mChild )
		cleanup( mRootSample->mChild );
	if ( mRootSample->mSibling )
		cleanup( mRootSample->mSibling );

	mRootSample->mChild = NULL;
	mRootSample->mSibling = NULL;
	reset();
	mResetIterator = true;
}
void ProfileSystem::destorySampleNode( SampleNode* node )
{
	delete node;
}

ProfileSystem::SampleIterator ProfileSystem::getSampleIterator()
{
	return SampleIterator( mRootSample.get() );
}

ProfileSystem::SampleNode* ProfileSystem::createSampleNode( char const* name , SampleNode* parent )
{
	return new SampleNode( name  , parent );
}

ProfileSample::ProfileSample( const char * name , unsigned flag )
{
	ProfileSystem::Get().startProfile( name , flag );
}

ProfileSample::~ProfileSample( void )
{
	ProfileSystem::Get().stopProfile();
}
