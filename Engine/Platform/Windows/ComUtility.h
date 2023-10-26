#pragma once
#ifndef ComUtility_H_66793974_BDBF_45BB_BC40_E0A745733BD5
#define ComUtility_H_66793974_BDBF_45BB_BC40_E0A745733BD5

#include "LogSystem.h"

#define CHECK_RESULT_CODE( EXPR , CODE )\
	if( HRESULT hr = (EXPR) < 0 )\
	{\
		CODE;\
	}
#define CHECK_RETURN( EXPR , RT_VALUE )\
	if( HRESULT hr = (EXPR) < 0 )\
	{\
		::LogWarning(0, "hr = %d", hr);\
		return RT_VALUE;\
	}

struct ReleaseDeleter
{
	template< class T >
	static void Destroy(T* ptr) { ptr->Release(); }
};

template< typename T, typename Deleter = ReleaseDeleter >
class TComPtr
{
public:

	TComPtr()
	{
		mPtr = nullptr;
	}

	explicit TComPtr(T* ptr)
		:mPtr(ptr)
	{

	}

	~TComPtr()
	{
		if( mPtr )
		{
			Deleter::Destroy(mPtr);
		}
	}


	TComPtr(TComPtr<T>&& other)
	{
		mPtr = other.mPtr;
		other.mPtr = nullptr;
	}
#if 0
	template< typename Q >
	TComPtr(TComPtr<Q>&& other)
	{
		mPtr = other.mPtr;
		other.mPtr = nullptr;
	}
#endif


	TComPtr(TComPtr<T> const& other)
	{
		mPtr = other.mPtr;
		if( mPtr )
		{
			mPtr->AddRef();
		}
	}

	void reset()
	{
		if( mPtr )
		{
			Deleter::Destroy(mPtr);
			mPtr = nullptr;
		}
	}

	T**   address() { return &mPtr; }
	T*    detach() { T* ptr = mPtr; mPtr = nullptr; return ptr; }

	void assign(T* ptr)
	{
		if (mPtr)
		{
			Deleter::Destroy(mPtr);
		}
		mPtr = ptr;
		if (mPtr)
		{
			mPtr->AddRef();
		}
	}

	template< typename U >
	bool castTo( TComPtr<U>& p) const throw()
	{
		if (mPtr == nullptr)
			return false;

		p.reset();
		return mPtr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(&p)) == 0;
	}

	void initialize(T* ptr)
	{
		assert(mPtr == nullptr);
		mPtr = ptr;
	}

	TComPtr& operator = (TComPtr const& rhs)
	{
		if( mPtr ) 
		{ 
			Deleter::Destroy(mPtr); 
		}
		mPtr = rhs.mPtr;
		if( mPtr )
		{
			mPtr->AddRef();
		}
		return *this;
	}

	int getRefCount() const
	{
		if ( mPtr )
		{
			mPtr->AddRef();
			return mPtr->Release();
		}
		return 0;
	}
	T*   get() { return mPtr; }
	bool isValid() const { return !!mPtr; }

	bool operator == (void* ptr) const { return mPtr == ptr; }
	bool operator != (void* ptr) const { return mPtr != ptr; }

	operator T*  () { return mPtr; }
	operator bool() const { return mPtr; }
	T*  operator->() { return mPtr; }
	T** operator& () { return &mPtr; }

	T* mPtr;
};

#endif // ComUtility_H_66793974_BDBF_45BB_BC40_E0A745733BD5
