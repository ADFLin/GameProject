#ifndef UT_TestAssert_h__
#define UT_TestAssert_h__

#include "Config.h"
#include "LogSystem.h"

#include <cassert>

namespace UnitTest
{



#define UT_LOG(...) LogMsg(__VA_ARGS__)

#define UT_ASSERT_EQ( A , B ) \
	do { \
		auto valA = (A); \
		auto valB = (B); \
		if( !( valA == valB ) ) { \
			UT_LOG("Assert Failed: %s == %s", #A, #B); \
			UT_LOG("  Value A: %s", std::to_string(valA).c_str()); /* Might fail if not toString-able, keep simple for now */ \
			UT_LOG("  Value B: %s", std::to_string(valB).c_str()); \
			assert(false); \
		} \
	} while(0)

#define UT_ASSERT( EXPR ) \
	do { \
		if( !(EXPR) ) { \
			UT_LOG("Assert Failed: %s", #EXPR); \
			assert(false); \
		} \
	} while(0)

#define UT_ASSERT_TRUE( EXPR ) UT_ASSERT(EXPR)
#define UT_ASSERT_FALSE( EXPR ) UT_ASSERT(!(EXPR))
#define UT_ASSERT_NE( A, B ) \
	do { \
		if( (A) == (B) ) { \
			UT_LOG("Assert Failed: %s != %s", #A, #B); \
			assert(false); \
		} \
	} while(0)

} //namespace UnitTest

#endif // UT_TestAssert_h__
