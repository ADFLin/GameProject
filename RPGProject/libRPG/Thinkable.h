#ifndef Thinkable_h__
#define Thinkable_h__

#include "common.h"
#include <list>

#define NEVER_THINK (-1)

enum ThinkMode
{
	THINK_LOOP,
	THINK_ONCE,
	THINK_REMOVE,
};

class Thinkable;
struct ThinkContent;

typedef fastdelegate::FastDelegate< void ( ThinkContent& ) > ThinkFunc;
struct ThinkContent
{
public:
	float        loopTime;
	float        nextTime;

	union  
	{
		int      intData;
		unsigned uintData;
		float    floatData;
		void*    ptrData;
	};
	float         getLastTime(){ return lastTime; }

private:
	ThinkMode     mode;
	ThinkFunc     func;
	float         lastTime;

	friend class Thinkable;
};


class Thinkable
{
public:
	Thinkable()
	{
	}

	void update( long time );
	
	

	template< class T , class TFunc >
	ThinkContent* setupThink( T* obj , TFunc func , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 )
	{
		ThinkFunc thinkFunc;
		thinkFunc.bind( obj , func );
		return setupThinkInternal( thinkFunc , mode , loopTime );
	}
	template<  class TFunc >
	ThinkContent* setupThink( TFunc func , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 )
	{
		ThinkFunc thinkFunc;
		thinkFunc.bind( func );
		return setupThinkInternal( thinkFunc , mode , loopTime );
	}

private:

	ThinkContent* setupThinkInternal( ThinkFunc const& func , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 );
	
	static void setupContent( ThinkContent& content , ThinkMode mode , float loopTime );
	typedef std::list< ThinkContent > ContentList;
	ContentList mContentList;

};
#endif // ThinkableComponent_h__