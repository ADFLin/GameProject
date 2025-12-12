#ifndef UT_TestClass_h__
#define UT_TestClass_h__

#include "CoreShare.h"
#include "UnitTest/Config.h"


enum EUTRunResult
{
	RR_SUCCESS,
	RR_FAIL,
};

namespace UnitTest 
{
	class TestResult
	{






	};

	class Component
	{
	public:
		CORE_API Component(const char* name);
		const char* getName() const { return mName; }

		virtual EUTRunResult run() = 0;
		CORE_API static bool RunTest();
	private:
		const char* mName;
		Component* mNext;
	};


	template< class T >
	class TClassTest : public Component
	                 , private T
	{
	public:
		TClassTest(const char* name):Component(name){}
		EUTRunResult run()
		{ 
			return T::run();
		}
	};

	template < EUTRunResult (*Func)() >
	class TFuncTest : public Component
	{
	public:
		TFuncTest(const char* name):Component(name){}
		EUTRunResult run()
		{ 
			return (*Func)();
		}
	};

	template< typename T >
	bool RegisterTest( char const* name )
	{
		static UnitTest::TClassTest< T > StaticInstance(name);
		return true;
	}

	template< EUTRunResult (*Func)() >
	bool RegisterTest( char const* name )
	{
		static UnitTest::TFuncTest< Func > StaticInstance(name);
		return true;
	}

}//namespace UnitTest


#define UT_REG_TEST( ARG , NAME )\
	static bool ANONYMOUS_VARIABLE(GTestRegister) = UnitTest::RegisterTest< ARG >(NAME);

#define UT_RUN_TEST()\
	UnitTest::Component::RunTest();


#endif // UT_TestClass_h__