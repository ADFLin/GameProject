#pragma once
#ifndef Optional_H_9BF4B338_1987_4808_A493_4DE55A4E7FB9
#define Optional_H_9BF4B338_1987_4808_A493_4DE55A4E7FB9

#include "TypeMemoryOp.h"

template< class T >
struct TMemoryStorage
{
	uint8 mData[sizeof(T)];
};

template< class T >
class TOptional
{
public:
	template< typename ...TArgs >
	void construct(TArgs&& ...args)
	{
		CHECK(!isSet());
		FTypeMemoryOp::Construct(getValuePtr(), std::forward<TArgs>(args)...);
		mIsSet = true;
	}

	void release()
	{
		CHECK(isSet());
		FTypeMemoryOp::Destruct(getValuePtr());
		mIsSet = false;
	}

	bool isSet() const { return mIsSet; }
	TOptional(T const& rhs)
	{
		mIsSet = true;
		FTypeMemoryOp::Construct(getValuePtr(), rhs);
	}

	TOptional()
	{
		mIsSet = false;
	}

	~TOptional()
	{
		clear();
	}

	operator T& () { return *getValuePtr(); }
	operator T const& () const { return *getValuePtr(); }
	T& operator*() { CHECK(mIsSet); return *getValuePtr(); }

	template< class Q >
	TOptional& operator = (Q&& rhs)
	{
		clear();
		FTypeMemoryOp::Construct(getValuePtr(), std::forward<Q>(rhs));
		return *this;
	}

	void clear()
	{
		if (mIsSet)
		{
			release();
		}
	}
protected:
	T* getValuePtr()
	{
		return reinterpret_cast<T*>(mValue.mData);
	}
	T const* getValuePtr() const
	{
		return reinterpret_cast<T const*>(mValue.mData);
	}
	TMemoryStorage<T> mValue;
	bool mIsSet;
};


#endif // Optional_H_9BF4B338_1987_4808_A493_4DE55A4E7FB9
