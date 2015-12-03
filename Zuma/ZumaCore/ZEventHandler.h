#ifndef ZEventHandler_h__
#define ZEventHandler_h__

#include "ZBase.h"

#ifdef _DEBUG
#include <typeinfo>
#endif // _DEBUG

namespace Zuma
{

	class ZEventData;
	class ZEventHandler
	{
	public:
		ZEventHandler(){  mParent = NULL;  }
		virtual ~ZEventHandler(){}
		virtual bool onEvent( int evtID , ZEventData const& data , ZEventHandler* sender ) = 0;

		ZEventHandler* getParentHandler(){ return mParent; }
		void           setParentHandler( ZEventHandler* parent ){ mParent = parent; }
		void           processEvent( int evtID , ZEventData const& data );

	protected:
		ZEventHandler* mParent;
	};

	inline void ZEventHandler::processEvent( int evtID , ZEventData const& data )
	{
		ZEventHandler* cur = this;
		while( cur )
		{
			if ( !cur->onEvent( evtID , data , this ) )
				break;
			cur = cur->mParent;
		}
	}

	class ZEventData
	{
	public:
		template < class T >
		ZEventData( T  val ){  set( val ); }

		template < class T >
		T*  getPtr() const
		{
			assert( *type == typeid( T* ) ||
				type->before( typeid( T* ) ) );
			return (T*)val.ptr;
		}

		int getInt() const
		{
			assert( *type == typeid(int) );
			return val.intVal;
		}

		float getFloat() const
		{
			assert( *type == typeid(float) );
			return val.floatVal;
		}

		template < class T >
		void set( T* ptr )
		{
#ifdef _DEBUG
			type = &typeid( ptr );
#endif
			val.ptr = ptr;
		}

		void set( int v )
		{
#ifdef _DEBUG
			type = &typeid( v );
#endif
			val.intVal = v;
		}

		void set( float v )
		{
#ifdef _DEBUG
			type = &typeid( v );
#endif
			val.floatVal = v;
		}

	private:

		union
		{
			int   intVal;
			float floatVal;
			void* ptr;
		} val;
#ifdef  _DEBUG
		std::type_info const* type;
#endif
	};

}//namespace Zuma

#endif // ZEventHandler_h__
