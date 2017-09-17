#ifndef ThinkObjT_h__
#define ThinkObjT_h__

#include "ZBase.h"

#include <functional>

#define NEVER_THINK -1

struct ThinkData
{
	virtual ~ThinkData(){}
	virtual void release() = 0;
};

template < class T >
class ThinkObjT
{
public:
	typedef void (T::*ThinkFun)( ThinkData* );

	bool haveThink()
	{
		return ( mContent.fun != NULL || mContent.time != NEVER_THINK );
	}

	void processThink( unsigned time )
	{
		if ( !haveThink() )
			return;

		if ( mContent.time < time )
		{
			mContent.time = NEVER_THINK;
			( _this()->*mContent.fun )( mContent.data );
		}
	}

	void  setThink( ThinkFun fun , ThinkData* data = NULL )
	{
		mContent.fun = fun;

		if ( mContent.data )
			mContent.data->release();

		mContent.data = data;
	}
	void  setNextThinkTime( unsigned time ){  mContent.time = time;  }

private:

	struct ThinkContent
	{
		ThinkContent()
		{
			fun  = NULL;
			time = NEVER_THINK;
			data = NULL;
		}
		~ThinkContent()
		{
			if( data )
			{
				data->release();
			}
		}

		ThinkData*  data;
		unsigned    time;
		ThinkFun    fun;
	};
	
	ThinkContent mContent;

	inline T* _this(){ return static_cast< T* >( this ); }
};
#endif // ThinkObjT_h__
