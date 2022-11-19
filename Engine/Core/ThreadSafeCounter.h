#pragma once
#ifndef ThreadSafeCounter_H_F155A794_6EAA_43DD_82E4_FD1446243DD7
#define ThreadSafeCounter_H_F155A794_6EAA_43DD_82E4_FD1446243DD7

#include "SystemPlatform.h"

template< class T >
class TThreadSafeCounter
{
public:
	TThreadSafeCounter(T value):mValue(value) {}
	TThreadSafeCounter(){}

	T add(T value) { return SystemPlatform::AtomExchangeAdd(&mValue , value); }
	T set(T value) { return SystemPlatform::AtomExchange(&mValue , value); }
	T get() const { return mValue; }
	T reset(){ return SystemPlatform::AtomExchange(&mValue , 0); }

	volatile T mValue;
};

typedef TThreadSafeCounter<int32> ThreadSafeCounter;

#endif // ThreadSafeCounter_H_F155A794_6EAA_43DD_82E4_FD1446243DD7