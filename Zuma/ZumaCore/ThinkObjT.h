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
	typedef void (T::*ThinkFunc)( ThinkData* );

	bool haveThink()
	{
		return ( mContent.func != NULL || mContent.time != NEVER_THINK );
	}

	void processThink( unsigned time )
	{
		if ( !haveThink() )
			return;

		if ( mContent.time < time )
		{
			mContent.time = NEVER_THINK;
			( _this()->*mContent.func )( mContent.data );
		}
	}

	void  setThink( ThinkFunc func , ThinkData* data = NULL )
	{
		mContent.func = func;

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
			func = NULL;
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
		ThinkFunc   func;
	};
	
	ThinkContent mContent;

	inline T* _this(){ return static_cast< T* >( this ); }
};
#endif // ThinkObjT_h__
