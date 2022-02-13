#pragma once
#ifndef Optional_H_9BF4B338_1987_4808_A493_4DE55A4E7FB9
#define Optional_H_9BF4B338_1987_4808_A493_4DE55A4E7FB9

#include "TypeConstruct.h"

template< class T >
struct TMemoryStorage
{
	uint8 mData[sizeof(T)];
};

template< class T >
class TOptional
{
public:
	bool isSet() const { return mIsSet; }
	TOptional(T const& rhs)
	{
		mIsSet = true;
		TypeDataHelper::Construct(getValuePtr(), rhs);
	}

	TOptional()
	{
		mIsSet = false;
	}

	~TOptional()
	{
		if (mIsSet)
		{
			TypeDataHelper::Destruct(getValuePtr());
		}
	}

	operator T& () { return *getValuePtr(); }
	operator T const& () const { return *getValuePtr(); }

	template< class Q >
	TOptional& operator = (Q&& rhs)
	{
		if (mIsSet)
		{
			TypeDataHelper::Destruct(mValue.mData);
		}
		TypeDataHelper::Construct(getValuePtr(), std::forward<Q>(rhs));
		return *this;
	}

	void clear()
	{
		if (mIsSet)
		{
			TypeDataHelper::Destruct(getValuePtr());
			mIsSet = false;
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
