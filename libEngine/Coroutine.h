#ifndef Coroutine_h__236D6542_36DD_4B46_8899_164DE1E3E983
#define Coroutine_h__236D6542_36DD_4B46_8899_164DE1E3E983

#include "Cpp11StdConfig.h"

#include "boost/coroutine/all.hpp"
#include "boost/ref.hpp"
#include "boost/bind.hpp"
#include "boost/function.hpp"

//Useful for visualize debug data
class FunctionJumper
{

	typedef boost::coroutines::asymmetric_coroutine< void >::pull_type ImplType;
	typedef boost::coroutines::asymmetric_coroutine< void >::push_type YeildType;

public:

	template< class Fun >
	void start( Fun fun )
	{
		mbYeild = false;
		mImpl = ImplType( std::bind( &FunctionJumper::execEntry< Fun > , this , std::placeholders::_1 , fun ) );
	}

	void jump()
	{
		if ( !isRunning() )
			return;
		mbYeild = !mbYeild;
		if ( mbYeild )
			(*mYeild)();
		else
			mImpl();
	}

	bool isRunning() const
	{
		return mYeild != NULL;
	}

private:

	template< class Fun >
	void execEntry( YeildType& type , Fun fun )
	{

		mYeild = &type;
		fun();
		mYeild = NULL;

	}
	bool       mbYeild;
	YeildType* mYeild;
	ImplType   mImpl;

};

#if 0
template< class T >
class Coroutine
{
public:
	typedef boost::coroutines::asymmetric_coroutine< T >::pull_type ImplType;
	typedef boost::coroutines::asymmetric_coroutine< T >::push_type YeildType;

	template< class Fun >
	void start( Fun fun )
	{

	}
	bool isRunning(){ return mImpl; }
	void call();
	void yeild( T const& );

private:
	ImplType mImpl;
};

#endif

#endif // Coroutine_h__236D6542_36DD_4B46_8899_164DE1E3E983
