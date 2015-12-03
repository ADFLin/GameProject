#ifndef UT_TestClass_h__
#define UT_TestClass_h__

#include "UnitTest/Config.h"

namespace UnitTest 
{

	class TestResult
	{






	};

	class Component
	{
	public:
		Component();
		enum RunResult
		{
			RR_SUCCESS ,
		};
		virtual RunResult run() = 0;
		static bool RunTest();
	private:
		Component* mNext;
	};


	template< class T , class C >
	class ClassTestT : public Component
	{
	public:
		RunResult run(){ return RR_SUCCESS; }
	};



#define UT_REG_TEST_CLASS( CLASS )\
	static CLASS sTest_##CLASS; 
#define UT_RUN_TEST()\
	UnitTest::Component::RunTest();

}//namespace UnitTest

#endif // UT_TestClass_h__