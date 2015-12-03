#include "ProjectPCH.h"
#include "Thinkable.h"

void Thinkable::update( long time )
{
	for( ContentList::iterator iter = mContentList.begin();
		 iter != mContentList.end() ; ++iter )
	{
		ThinkContent& content = *iter;

		if ( content.mode == THINK_REMOVE ||
			 content.nextTime == NEVER_THINK )
			return;

		if ( content.nextTime <= g_GlobalVal.curtime )
		{
			float thinkTime = g_GlobalVal.curtime;

			if ( content.mode == THINK_ONCE )
				content.nextTime = NEVER_THINK;
			else if ( content.mode == THINK_LOOP )
				content.nextTime = thinkTime + content.loopTime;

			content.fun( content );
			content.lastTime = g_GlobalVal.curtime;
		}
	}
}



void Thinkable::setupContent( ThinkContent& content , ThinkMode mode , float loopTime )
{
	content.mode = mode;
	content.loopTime = loopTime;
	content.nextTime = NEVER_THINK;
}

ThinkContent* Thinkable::setupThinkInternal( ThinkFun const& fun , ThinkMode mode /*= THINK_ONCE */, float loopTime /*= 0.0 */ )
{
	ContentList::iterator removeIter = mContentList.end();
	for( ContentList::iterator iter = mContentList.begin();
		 iter != mContentList.end() ; ++iter )
	{
		ThinkContent& content = *iter;

		if ( content.fun == fun )
		{
			setupContent( content , mode , loopTime );
			return &content;
		}
		else if ( removeIter == mContentList.end() && content.mode == THINK_REMOVE )
		{
			removeIter = iter;
		}
	}

	if ( mode == THINK_REMOVE )
		return nullptr;

	if ( removeIter != mContentList.end() )
	{
		ThinkContent& content = *removeIter;
		content.fun = fun;
		setupContent( content , mode , loopTime );
		return &content;
	}

	ThinkContent content;
	content.fun = fun;
	setupContent( content , mode , loopTime );
	mContentList.push_back( content );
	return &mContentList.back();
}
