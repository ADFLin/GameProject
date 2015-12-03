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

typedef fastdelegate::FastDelegate1< ThinkContent& , void > ThinkFun;
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
	ThinkFun      fun;
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
	
	

	template< class T , class Fun >
	ThinkContent* setupThink( T* obj , Fun fun , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 )
	{
		ThinkFun thinkFun;
		thinkFun.bind( obj , fun );
		return setupThinkInternal( thinkFun , mode , loopTime );
	}
	template<  class Fun >
	ThinkContent* setupThink( Fun fun , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 )
	{
		ThinkFun thinkFun;
		thinkFun.bind( fun );
		return setupThinkInternal( thinkFun , mode , loopTime );
	}

private:

	ThinkContent* setupThinkInternal( ThinkFun const& fun , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 );
	
	static void setupContent( ThinkContent& content , ThinkMode mode , float loopTime );
	typedef std::list< ThinkContent > ContentList;
	ContentList mContentList;

};
#endif // ThinkableComponent_h__