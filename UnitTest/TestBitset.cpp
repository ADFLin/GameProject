#include "UnitTest.h"
#include "Bitset.h"

class BitsetTest
{
public:
	EUTRunResult run()
	{
		Bitset b( 15 , false );
		b.set( 0 , true );
		b.set( 10 , true );

		UT_ASSERT( b.count() == 2 );
		UT_ASSERT( b.test( 0 ) && b.test( 10 ) );
		return RR_SUCCESS; 
	}
};

EUTRunResult Test2()
{
	Bitset b(15, false);
	b.set(0, true);
	b.set(10, true);

	UT_ASSERT(b.count() == 2);
	UT_ASSERT(b.test(0) && b.test(10));
	return RR_SUCCESS;
}

UT_REG_TEST(BitsetTest, "Bitset")
UT_REG_TEST(Test2, "Test2")
