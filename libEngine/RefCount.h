#pragma once
#ifndef RefCount_H_AE7BB79E_FF29_450C_9F52_A7F3B2CF78A2
#define RefCount_H_AE7BB79E_FF29_450C_9F52_A7F3B2CF78A2

#include "Meta/MetaBase.h"

template < class T >
class TRefCountPtr;

template < class T >
class RefCountedObjectT
{
public:
	typedef TRefCountPtr< T > RefCountPtr;
	RefCountedObjectT():mRefCount(0){}
	void destroyThis(){ delete static_cast< T*>( this ); }
	int  getRefCount(){ return mRefCount; }

public:
	bool  decRef()
	{ 
		--mRefCount;
		if ( mRefCount <= 0 )
		{
			return true;
		}
		return false; 
	}
	void  incRef(){ ++mRefCount; }
private:
	T*  _this(){ return static_cast< T* >( this ); }
	friend class TRefCountPtr<T>;

	int  mRefCount;
};

template < class T >
class TRefCountPtr
{
public:
	TRefCountPtr():mPtr(0){}
	TRefCountPtr( T* ptr ){  init( ptr );  }
	TRefCountPtr( TRefCountPtr const& other ){  init( other.mPtr ); }
	~TRefCountPtr(){  cleanup(); }

	template< class Q >
	TRefCountPtr( TRefCountPtr< Q > const& other ) { init(other.mPtr); }


	T*       operator->()       { return mPtr; }
	T const* operator->() const { return mPtr; }
	T&       operator *( void )       { return *mPtr; }
	T const& operator *( void ) const { return *mPtr; }

	operator T* ( void )       { return mPtr; }
	operator T* ( void ) const { return mPtr; }

	void     reset(T* ptr) { assign(ptr);  }
	T*       get()       { return mPtr; }
	T const* get() const { return mPtr; }

	bool     isValid() const { return mPtr != nullptr; }
	void     release() 
	{ 
		cleanup();
	}

	TRefCountPtr& operator = ( TRefCountPtr const& rcPtr ){  assign( rcPtr.mPtr ); return *this;  }
	template< class Q >
	TRefCountPtr& operator = ( TRefCountPtr< Q > const& rcPtr) { assign(rcPtr.mPtr); return *this; }

	TRefCountPtr& operator = ( T* ptr ){  assign( ptr ); return *this;  }
private:
	void init( T* ptr ){ mPtr = ptr; if ( mPtr ) mPtr->incRef(); }
	void cleanup()
	{
		if( mPtr )
		{
			if ( mPtr->decRef() )
				mPtr->destroyThis();

			mPtr = nullptr;
		}  
	}
	void assign( T* ptr )
	{
		if ( mPtr == ptr )
			return;
		cleanup();
		init( ptr );
	}
	template< class Q >
	friend class TRefCountPtr;
	T* mPtr;
};

#endif // RefCount_H_AE7BB79E_FF29_450C_9F52_A7F3B2CF78A2
