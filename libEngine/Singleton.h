#pragma once
#ifndef Singleton_H_F86B29AF_4177_4F6E_B1CD_1CE60D5366B2
#define Singleton_H_F86B29AF_4177_4F6E_B1CD_1CE60D5366B2

#include "CppVersion.h"
#include "CoreShare.h"

template< class T >
class SingletonT
{
public:
	static T& Get()
	{
		if ( !_instance )
			_instance = new T;
		return *_instance;
	}

	static void ReleaseInstance()
	{
		if ( _instance )
		{
			delete _instance;
			_instance = 0;
		}
	}
protected:
	SingletonT() = default;

	FUNCTION_DELETE(SingletonT(SingletonT const& sing));
	FUNCTION_DELETE(SingletonT& operator = (SingletonT const& sing));

private:
	static T* _instance;
};

template< class T >
T* SingletonT< T >::_instance = 0;

#endif // Singleton_H_F86B29AF_4177_4F6E_B1CD_1CE60D5366B2
