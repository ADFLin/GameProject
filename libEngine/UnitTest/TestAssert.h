#ifndef UT_TestAssert_h__
#define UT_TestAssert_h__

#include "Config.h"

#include <cassert>

namespace UnitTest
{

#define UT_ASSERT_EQ( A , B )

#define UT_ASSERT( EXPR )\
	assert( EXPR )

} //namespace UnitTest

#endif // UT_TestAssert_h__
