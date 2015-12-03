#ifndef RefObject_h__
#define RefObject_h__

template < class T >
class RefPtrT;

template < class T >
class RefObjectT
{
public:
	typedef RefPtrT< T > RefPtr;
	RefObjectT():mRefCount(0){}
	void destroyThis(){ delete static_cast< T*>( this ); }
	int  getRefCount(){ return mRefCount; }
public:
	bool  decRef()
	{ 
		--mRefCount;
		if ( mRefCount <= 0 )
		{
			_this()->destroyThis();
			return true;
		}
		return false; 
	}
	void  incRef(){ ++mRefCount; }
private:
	T*  _this(){ return static_cast< T* >( this ); }
	friend class RefPtrT<T>;

	int  mRefCount;
};

template < class T >
class RefPtrT
{
public:
	RefPtrT():mPtr(0){}
	RefPtrT( T* ptr ){  init( ptr );  }
	RefPtrT( RefPtrT const& rcPtr ){  init( rcPtr.mPtr ); }
	~RefPtrT(){  cleanup(); }

	T*       operator->()       { return mPtr; }
	T const* operator->() const { return mPtr; }
	T&       operator *( void )       { return *mPtr; }
	T const& operator *( void ) const { return *mPtr; }

	operator T* ( void )       { return mPtr; }
	operator T* ( void ) const { return mPtr; }

	T*       get()       { return mPtr; }
	T const* get() const { return mPtr; }

	void     release() 
	{ 
		cleanup();
		mPtr = 0;
	}

	RefPtrT& operator = ( RefPtrT const& rcPtr ){  assign( rcPtr.mPtr ); return *this;  }
	RefPtrT& operator = ( T* ptr ){  assign( ptr ); return *this;  }
private:
	void init( T* ptr ){ mPtr = ptr; if ( mPtr ) mPtr->incRef(); }
	void cleanup(){  if ( mPtr ) mPtr->decRef();  }
	void assign( T* ptr )
	{
		if ( mPtr == ptr )
			return;
		cleanup();
		init( ptr );
	}
	T* mPtr;
};

#endif // RefObject_h__
