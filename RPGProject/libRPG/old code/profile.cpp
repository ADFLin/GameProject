#include "profile.h"
#include "TMessageShow.h"



TClock   g_profileClock;

inline void Profile_Get_Ticks(unsigned long int * ticks)
{
	*ticks = g_profileClock.getTimeMicroseconds();
}

inline float Profile_Get_Tick_Rate(void)
{
	//	return 1000000.f;
	return 1000.f;
}


TProfileNode::TProfileNode( const char * name, TProfileNode * parent ) 
	:Name( name )
	,TotalCalls( 0 )
	,TotalTime( 0 )
	,StartTime( 0 )
	,RecursionCounter( 0 )
	,Parent( parent )
	,Child( NULL )
	,Sibling( NULL )
	,m_isShowChild(false)
{
	reset();
}


void	TProfileNode::cleanupMemory()
{
	delete ( Child);
	Child = NULL;
	delete ( Sibling);
	Sibling = NULL;
}

TProfileNode::~TProfileNode( void )
{
	delete ( Child );
	delete ( Sibling );
}


TProfileNode * TProfileNode::getSubNode( const char * name )
{
	// Try to find this sub node
	TProfileNode * child = Child;
	while ( child ) {
		if ( child->Name == name ) {
			return child;
		}
		child = child->Sibling;
	}

	// We didn't find it, so add it
	
	TProfileNode * node = new TProfileNode( name, this );
	node->Sibling = Child;
	Child = node;
	return node;
}


void	TProfileNode::reset( void )
{
	TotalCalls = 0;
	TotalTime = 0.0f;

	if ( Child ) {
		Child->reset();
	}
	if ( Sibling ) {
		Sibling->reset();
	}
}


void	TProfileNode::OnCall( void )
{
	TotalCalls++;
	if (RecursionCounter++ == 0) {
		Profile_Get_Ticks(&StartTime);
	}
}


bool	TProfileNode::OnReturn( void )
{
	if ( --RecursionCounter == 0 && TotalCalls != 0 ) { 
		unsigned long int time;
		Profile_Get_Ticks(&time);
		time-=StartTime;
		TotalTime += (float)time / Profile_Get_Tick_Rate();
	}
	return ( RecursionCounter == 0 );
}

void TProfileNode::showAllChild( bool beShow )
{
	TProfileNode* node = getChild();

	showChild( beShow  );
	while ( node )
	{
		node->showAllChild( beShow );
		node = node->getSibling();
	}
}
TProfileIterator::TProfileIterator( TProfileNode * start )
{
	parent = start;
	curNode = parent->getChild();
}


void	TProfileIterator::First(void)
{
	curNode = parent->getChild();
}


void	TProfileIterator::Next(void)
{
	curNode = curNode->getSibling();
}


bool	TProfileIterator::isDone(void)
{
	return curNode == NULL;
}


void	TProfileIterator::enterChild( int index )
{
	curNode = parent->getChild();
	while ( (curNode != NULL) && (index != 0) ) {
		index--;
		curNode = curNode->getSibling();
	}

	if ( curNode != NULL ) {
		parent = curNode;
		curNode = parent->getChild();
	}
}

void TProfileIterator::enterChild()
{
	TProfileNode* nextChild = curNode->getChild();

	parent = curNode;
	curNode  = nextChild;
}

void	TProfileIterator::enterParent( void )
{
	if ( parent->getParent() != NULL ) {
		parent = parent->getParent();
	}
	curNode = parent->getChild();
}

void TProfileIterator::Before()
{
	TProfileNode* node = parent->getChild();

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

TProfileNode	TProfileManager::Root( "Root", NULL );
TProfileNode *	TProfileManager::CurrentNode = &TProfileManager::Root;
int				TProfileManager::FrameCounter = 0;
unsigned long int			TProfileManager::ResetTime = 0;


/***********************************************************************************************
 * CProfileManager::Start_Profile -- Begin a named profile                                    *
 *                                                                                             *
 * Steps one level deeper into the tree, if a child already exists with the specified name     *
 * then it accumulates the profiling; otherwise a new child node is added to the profile tree. *
 *                                                                                             *
 * INPUT:                                                                                      *
 * name - name of this profiling record                                                        *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 * The string used is assumed to be a static string; pointer compares are used throughout      *
 * the profiling code for efficiency.                                                          *
 *=============================================================================================*/
void	TProfileManager::startProfile( const char * name )
{
	if (name != CurrentNode->getName()) {
		CurrentNode = CurrentNode->getSubNode( name );
	} 
	
	CurrentNode->OnCall();
}


/***********************************************************************************************
 * CProfileManager::Stop_Profile -- Stop timing and record the results.                       *
 *=============================================================================================*/
void	TProfileManager::stopProfile( void )
{
	// Return will indicate whether we should back up to our parent (we may
	// be profiling a recursive function)
	if (CurrentNode->OnReturn()) {
		CurrentNode = CurrentNode->getParent();
	}
}



void	TProfileManager::reset( void )
{ 
	g_profileClock.reset();
	Root.reset();
    Root.OnCall();
	FrameCounter = 0;
	Profile_Get_Ticks(&ResetTime);
}

void TProfileManager::incrementFrameCount( void )
{
	FrameCounter++;
}


float TProfileManager::getTimeSinceReset( void )
{
	unsigned long int time;
	Profile_Get_Ticks(&time);
	time -= ResetTime;
	return (float)time / Profile_Get_Tick_Rate();
}

#include <stdio.h>

void	TProfileManager::dumpRecursive(TProfileIterator* profileIterator, int spacing)
{
	profileIterator->First();
	if (profileIterator->isDone())
		return;

	float accumulated_time=0,parent_time = profileIterator->isRoot() ? TProfileManager::getTimeSinceReset() : profileIterator->getCurrentParentTotalTime();
	int i;
	int frames_since_reset = TProfileManager::getFrameCountSinceReset();
	for (i=0;i<spacing;i++)	printf(".");
	printf("----------------------------------\n");
	for (i=0;i<spacing;i++)	printf(".");
	printf("Profiling: %s (total running time: %.3f ms) ---\n",	profileIterator->getCurrentParentName(), parent_time );
	float totalTime = 0.f;

	
	int numChildren = 0;
	
	for (i = 0; !profileIterator->isDone(); i++,profileIterator->Next())
	{
		numChildren++;
		float current_total_time = profileIterator->getCurrentTotalTime();
		accumulated_time += current_total_time;
		float fraction = parent_time > SIMD_EPSILON ? (current_total_time / parent_time) * 100 : 0.f;
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
	printf("%s (%.3f %%) :: %.3f ms\n", "Unaccounted:",parent_time > SIMD_EPSILON ? ((parent_time - accumulated_time) / parent_time) * 100 : 0.f, parent_time - accumulated_time);
	
	for (i=0;i<numChildren;i++)
	{
		profileIterator->enterChild(i);
		dumpRecursive(profileIterator,spacing+3);
		profileIterator->enterParent();
	}
}



void	TProfileManager::dumpAll()
{
	TProfileIterator* profileIterator = 0;
	profileIterator = TProfileManager::createIterator();

	dumpRecursive(profileIterator,0);

	TProfileManager::releaseIterator(profileIterator);
}

void TProfileManager::cleanupMemory()
{
	Root.cleanupMemory();
	resetIter = true;
}

bool TProfileManager::resetIter = false;



void showProfileInfo( TProfileIterator* profIter , TMessageShow& msgShow , TProfileIterator* curProfIter )
{
	static double time_since_reset = 0.f;

	time_since_reset = TProfileManager::getTimeSinceReset();

	//recompute profiling data, and store profile strings
	double totalTime = 0;
	int frames_since_reset = TProfileManager::getFrameCountSinceReset();

	profIter->First();

	double parent_time = profIter->isRoot() ? time_since_reset : profIter->getCurrentParentTotalTime();

	if ( profIter->isRoot() )
	{
		msgShow.push( "--- Profiling: %s (total running time: %.3f ms) ---" , profIter->getCurrentParentName(), parent_time );
	}


	double accumulated_time = 0.f;

	int numChildren = 0;
	for (int i = 0; !profIter->isDone(); profIter->Next())
	{
		++numChildren;

		double current_total_time = profIter->getCurrentTotalTime();
		accumulated_time += current_total_time;
		double fraction = parent_time > SIMD_EPSILON ? (current_total_time / parent_time) * 100 : 0.f;

		if ( curProfIter && profIter->getCurNode() == curProfIter->getCurNode() )
		{
			msgShow.push( "o-> %d -- %s (%.2f %%) :: %.3f ms / frame (%d calls)",
				++i, profIter->getCurrentName(), fraction,
				(current_total_time / (double)frames_since_reset),
				profIter->getCurrentTotalCalls() );
		}
		else
		{
			msgShow.push( "|-> %d -- %s (%.2f %%) :: %.3f ms / frame (%d calls)",
				++i, profIter->getCurrentName(), fraction,
				(current_total_time / (double)frames_since_reset),
				profIter->getCurrentTotalCalls());
		}



		if ( profIter->getCurNode()->isShowChild() )
		{
			TProfileIterator iter = *profIter;
			iter.enterChild();
			msgShow.offsetPos(  20 , 0 );
			showProfileInfo( &iter , msgShow , curProfIter );
			msgShow.offsetPos( -20 , 0 );
			iter.enterParent();
		}

		totalTime += current_total_time;
	}

	if ( numChildren != 0 )
	{
		msgShow.push( "|-> %s (%.3f %%) :: %.3f ms / frame", "Other",
			// (min(0, time_since_reset - totalTime) / time_since_reset) * 100);
			parent_time > SIMD_EPSILON ? ((parent_time - accumulated_time) / parent_time) * 100 : 0.f , 
			(parent_time - accumulated_time) / (double)frames_since_reset  );
		msgShow.push( "-------------------------------------------------" );
	}
}

unsigned long int TClock::getTimeMilliseconds()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	LONGLONG elapsedTime = currentTime.QuadPart - 
		mStartTime.QuadPart;

	// Compute the number of millisecond ticks elapsed.
	unsigned long msecTicks = (unsigned long)(1000 * elapsedTime / 
		mClockFrequency.QuadPart);

	// Check for unexpected leaps in the Win32 performance counter.  
	// (This is caused by unexpected data across the PCI to ISA 
	// bridge, aka south bridge.  See Microsoft KB274323.)
	unsigned long elapsedTicks = GetTickCount() - mStartTick;
	signed long msecOff = (signed long)(msecTicks - elapsedTicks);
	if (msecOff < -100 || msecOff > 100)
	{
		// Adjust the starting time forwards.
		LONGLONG msecAdjustment = mymin(msecOff * 
			mClockFrequency.QuadPart / 1000, elapsedTime - 
			mPrevElapsedTime);
		mStartTime.QuadPart += msecAdjustment;
		elapsedTime -= msecAdjustment;

		// Recompute the number of millisecond ticks elapsed.
		msecTicks = (unsigned long)(1000 * elapsedTime / 
			mClockFrequency.QuadPart);
	}

	// Store the current elapsed time for adjustments next time.
	mPrevElapsedTime = elapsedTime;

	return msecTicks;
}

unsigned long int TClock::getTimeMicroseconds()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	LONGLONG elapsedTime = currentTime.QuadPart - 
		mStartTime.QuadPart;

	// Compute the number of millisecond ticks elapsed.
	unsigned long msecTicks = (unsigned long)(1000 * elapsedTime / 
		mClockFrequency.QuadPart);

	// Check for unexpected leaps in the Win32 performance counter.  
	// (This is caused by unexpected data across the PCI to ISA 
	// bridge, aka south bridge.  See Microsoft KB274323.)
	unsigned long elapsedTicks = GetTickCount() - mStartTick;
	signed long msecOff = (signed long)(msecTicks - elapsedTicks);
	if (msecOff < -100 || msecOff > 100)
	{
		// Adjust the starting time forwards.
		LONGLONG msecAdjustment = mymin(msecOff * 
			mClockFrequency.QuadPart / 1000, elapsedTime - 
			mPrevElapsedTime);
		mStartTime.QuadPart += msecAdjustment;
		elapsedTime -= msecAdjustment;
	}

	// Store the current elapsed time for adjustments next time.
	mPrevElapsedTime = elapsedTime;

	// Convert to microseconds.
	unsigned long usecTicks = (unsigned long)(1000000 * elapsedTime / 
		mClockFrequency.QuadPart);

	return usecTicks;
}