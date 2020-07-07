#pragma once
#ifndef ComUtility_H_66793974_BDBF_45BB_BC40_E0A745733BD5
#define ComUtility_H_66793974_BDBF_45BB_BC40_E0A745733BD5

#define CHECK_RETRUN( EXPR , RT_VALUE )\
	if( HRESULT hr = (EXPR) < 0 )\
		return RT_VALUE;

struct ReleaseDeleter
{
	template< class T >
	static void Destroy(T* ptr) { ptr->Release(); }
};

template< class T, class Deleter = ReleaseDeleter >
class TComPtr
{
public:

	TComPtr()
	{
		mPtr = nullptr;
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

	TComPtr* address() { return this; }
	T*   release() { T* ptr = mPtr; mPtr = nullptr; return ptr; }

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
	T* get() { return mPtr; }

	bool operator == (void* ptr) const { return mPtr == ptr; }
	bool operator != (void* ptr) const { return mPtr != ptr; }

	operator T*  () { return mPtr; }
	operator bool() const { return mPtr; }
	T*  operator->() { return mPtr; }
	T** operator& () { return &mPtr; }

	T* mPtr;
};

#endif // ComUtility_H_66793974_BDBF_45BB_BC40_E0A745733BD5
