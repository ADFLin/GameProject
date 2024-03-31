#pragma once
#ifndef TemplateMisc_H_89EFA76B_D261_47AE_83A7_DE05B2C8EA0D
#define TemplateMisc_H_89EFA76B_D261_47AE_83A7_DE05B2C8EA0D

#include "CppVersion.h"

class Noncopyable
{
public:
	Noncopyable() {}
private:
	FUNCTION_DELETE( Noncopyable(Noncopyable const& rhs) )
	FUNCTION_DELETE( Noncopyable& operator = (Noncopyable const& rhs) )
};


template< class T >
class TGuardValue : public Noncopyable
{
public:
	TGuardValue(T& valueRef, T newValue)
		:mRef(valueRef), mOldValue(valueRef)
	{
		mRef = newValue;
	}
	~TGuardValue()
	{
		mRef = mOldValue;
	}
private:

	T& mRef;
	T  mOldValue;
};

template< class T >
class TGuardLoadStore : public Noncopyable
{
public:
	TGuardLoadStore(T& valueRef)
		:mRef(valueRef)
	{
		mValue = valueRef;
	}

	~TGuardLoadStore()
	{
		mRef = mValue;
	}

	operator T& (){ return mValue; }
	TGuardLoadStore& operator = (T const& rhs)
	{
		mValue = rhs;
		return *this;
	}
private:

	T& mRef;
	T  mValue;
};

template< class T >
class TRef
{
public:
	explicit TRef(T& val) :mValue(val) {}
	operator T&      () { return mValue; }
	operator T const&() const { return mValue; }
private:
	T& mValue;
};

template< class T >
class TConstRef
{
public:
	explicit TConstRef(T const& val) :mValue(val) {}
	operator T const&() const { return mValue; }

private:
	T const& mValue;
};

template< class T >
TRef< T >      ref(T& val) { return TRef< T >(val); }
template< class T >
TConstRef< T > ref(T const& val) { return TConstRef< T >(val); }

template< typename ...TFunc >
struct TOverloaded : public TFunc...
{
	using TFunc::operator()...;
};

template< typename ...TFunc >
TOverloaded(TFunc ...func)->TOverloaded< TFunc... >;

#endif // TemplateMisc_H_89EFA76B_D261_47AE_83A7_DE05B2C8EA0D
