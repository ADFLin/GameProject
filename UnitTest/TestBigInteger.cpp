#include "UnitTest.h"
#include "Math/BigInteger.h"
#include <cassert>

template<int N>
struct TestBU : public TBigUint<N>
{
	using TBigUint<N>::shiftLeftBit;
	using TBigUint<N>::shiftRightBit;
	using TBigUint<N>::setZero;
	using TBigUint<N>::elements;
};

class BigIntTest
{
public:
	static EUTRunResult run()
	{
		// 1. Bit Utilities Verification
		{
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(0) == 0);
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(1) == 1);
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(2) == 2);
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(3) == 2);
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(0xFFFFFFFF) == 32);
			
			// Critical check for our standardize issue:
			// 0x20000000 is 0010 0000 ... (bit 30 set, 1-based index)
			// It should return 30.
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(0x20000000) == 30);
			UT_ASSERT(BigUintBase::findHighestNonZeroBit(0x80000000) == 32);

			UT_ASSERT(BigUintBase::findLowestNonZeroBit(0) == 0);
			UT_ASSERT(BigUintBase::findLowestNonZeroBit(1) == 1);
			UT_ASSERT(BigUintBase::findLowestNonZeroBit(2) == 2);
			UT_ASSERT(BigUintBase::findLowestNonZeroBit(0x80000000) == 32);
		}

		// 2. TBigUint<1> Shift Operations
		{
			typedef TestBU<1> BU;
			BU a;
			a.setZero(); a.elements[0] = 1;
			a.shiftLeftBit(1, 0);
			UT_ASSERT(a.elements[0] == 2);
			
			// Test the standardization shift case:
			// 0x20000000 << 2 should be 0x80000000
			a.setZero(); a.elements[0] = 0x20000000;
			a.shiftLeftBit(2, 0);
			UT_ASSERT(a.elements[0] == 0x80000000);

			a.setZero(); a.elements[0] = 0x80000000;
			a.shiftRightBit(31, 0);
			UT_ASSERT(a.elements[0] == 1);
		}

		// 3. TBigUint<2> Cross-Word Shift Operations
		{
			typedef TestBU<2> BU;
			BU a; a.setZero();
			// Set highest bit of lower word
			a.elements[0] = 0x80000000; 
			
			// Shift left 1 bit -> should move to lowest bit of higher word
			a.shiftLeftBit(1, 0);
			UT_ASSERT(a.elements[0] == 0);
			UT_ASSERT(a.elements[1] == 1);
			
			// Shift back
			a.shiftRightBit(1, 0);
			UT_ASSERT(a.elements[0] == 0x80000000);
			UT_ASSERT(a.elements[1] == 0);

			// Test massive shift
			a.setZero(); a.elements[0] = 1;
			a.shiftLeftBit(32, 0);
			UT_ASSERT(a.elements[0] == 0);
			UT_ASSERT(a.elements[1] == 1);
		}

		// 4. Arithmetic Basic
		{
			typedef TestBU<2> BU;
			BU a;
			BU b;
			a.setZero(); a.elements[0] = 10;
			b.setZero(); b.elements[0] = 20;

			BU res;
			// Note: operator+ returns TBigUint, which cannot count as TestBU directly unless casted or using members
			// So we use .add method which modifies in place for TBigUint, but here we can check operators:
			// TBigUint::add returns carry.
			
			// Let's use internal add
			a.add(b);
			UT_ASSERT(a.elements[0] == 30);
			
			// Check overflow logic
			BU maxVal; maxVal.setZero();
			maxVal.elements[0] = 0xFFFFFFFF; maxVal.elements[1] = 0;
			BU one; one.setZero(); one.elements[0] = 1;
			
			maxVal.add(one); // 0xFFFFFFFF + 1 -> 0 with carry to next word
			UT_ASSERT(maxVal.elements[0] == 0);
			UT_ASSERT(maxVal.elements[1] == 1);
		}

		return RR_SUCCESS;
	}
};

UT_REG_TEST(BigIntTest, "Math.BigInt")
