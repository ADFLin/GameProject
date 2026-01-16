#include "UnitTest.h"
#include "Math/BigFloat.h"
#include "Math/TVector2.h"
#include <string>
#include <algorithm>

class BigFloatTest
{
public:
	static EUTRunResult run()
	{
		typedef TBigFloat<8, 1> BF; // 256-bit mantissa (~77 decimal digits)
		double val;

		// 1. Basic Construction and Comparison
		{
			BF a = 1.0;
			BF b = 2.0;
			BF c = "1.0";

			UT_ASSERT(a < b);
			UT_ASSERT(b > a);
			UT_ASSERT(a >= c);
			UT_ASSERT(a <= c);
			UT_ASSERT(!(a > c));
			UT_ASSERT(!(a < c));
		}

		// 2. Arithmetic: Addition & Subtraction
		{
			BF a = 1.5;
			BF b = 2.25;
			BF res = a;
			res.add(b); // result = a + b = 3.75

			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 3.75);

			res.sub(a); // res = 3.75 - 1.5 = 2.25
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 2.25);
		}

		// 3. Multiplication & Division
		{
			BF a = 3.0;
			BF b = 4.0;
			BF res = a;
			res.mul(b); // res = 12.0

			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 12.0);

			res.div(a); // res = 4.0
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 4.0);
		}

		// 4. Precision Test (10^50 level)
		{
			// Test if we can represent and perform math at e^50 level
			// Bit count for 10^50 is ~166 bits. Our BF<8, 1> has 256 bits.
			BF base = "100000000000000000000000000000000000000000000000000.0"; // 10^50
			BF offset = "0.000000000000001"; // small offset

			BF res = base;
			res.add(offset);

			UT_ASSERT(res > base);

			res.sub(base);
			// After subtracting base, we should get offset back (approximately, within bits)
			std::string str;
			res.getString(str);
			// The string output might be technical, but it should not be "0"
			UT_ASSERT(!res.isZero());
		}

		// 5. Transcendental: ln & exp (Basic check)
		{
			BF a = 1.0;
			a.ln(); // ln(1) = 0
			UT_ASSERT(a.isZero());

			BF b = 0.0;
			BF e_val;
			e_val.exp(b); // exp(0) = 1
			UT_ASSERT(e_val.convertToDouble(val));
			UT_ASSERT(val == 1.0);
		}

		// 6. Extreme Magnitudes (10^500 level)
		{
			// Construct 10^100 by repeated multiplication
			BF ten = 10.0;
			BF res = 1.0;
			for(int i = 0; i < 100; ++i) res.mul(ten);
			
			// Verify it's huge
			BF one = 1.0;
			UT_ASSERT(res > one);

			// Construct 10^-100
			BF small = 1.0;
			small.div(res);
			UT_ASSERT(small < one);
			UT_ASSERT(!small.isZero());

			// Scale it back
			small.mul(res);
			UT_ASSERT(small.convertToDouble(val));
			UT_ASSERT(std::abs(val - 1.0) < 1e-15);
		}

		// 7. Negative Arithmetic
		{
			BF a = 10.0;
			BF b = -5.0;
			BF res = a;
			res.add(b); // 10 + (-5) = 5
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 5.0);

			res = b;
			res.sub(a); // -5 - 10 = -15
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == -15.0);

			res = a;
			res.mul(b); // 10 * -5 = -50
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == -50.0);

			res.div(b); // -50 / -5 = 10
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 10.0);
		}

		// 8. String Round-trip & Precision
		{
			std::string input = "1.23456789012345678901234567890123456789";
			BF a = input.c_str();
			std::string output;
			a.getString(output);
			
			// Check if the significant digits match
			std::string digits = output;
			digits.erase(std::remove(digits.begin(), digits.end(), '.'), digits.end());
			UT_ASSERT(digits.find("12345678901234567890123456789") != std::string::npos);
		}

		// 8.1. Scientific Round-trip
		{
			std::string input = "1.2345678E-50";
			BF a = input.c_str();
			std::string output;
			a.getString(output);

			std::string digits = output;
			digits.erase(std::remove(digits.begin(), digits.end(), '.'), digits.end());
			UT_ASSERT(digits.find("12345678") != std::string::npos);
			UT_ASSERT(output.find("E-50") != std::string::npos);

			BF b = output.c_str();
			BF diff = a;
			diff.sub(b);
			UT_ASSERT(diff.isZero() || diff < "1E-60");
		}

		// 9. Edge Cases: Zero
		{
			BF z = 0.0;
			BF a = 123.456;
			BF res = a;
			res.mul(z);
			UT_ASSERT(res.isZero());

			res = z;
			res.div(a);
			UT_ASSERT(res.isZero());

			res = a;
			res.add(z);
			UT_ASSERT(res.convertToDouble(val));
			UT_ASSERT(val == 123.456);
		}

		// 10. String Format Trailing Zero Removal
		{
			std::string str;
			BF val;

			// Case 1: Trailing zeros in fraction
			val.setFromString("1.250000");
			val.getString(str);
			UT_ASSERT(str == "1.25");

			// Case 2: Integer with dot
			val.setFromString("100.000"); // 1.0 E 2
			val.getString(str);
			UT_ASSERT(str == "1E2");

			// Case 3: Scientific notation with redundant zeros
			val.setFromString("1.50000e10");
			val.getString(str);
			UT_ASSERT(str == "1.5E10");

			// Case 4: Zero
			val.setZero();
			val.getString(str);
			UT_ASSERT(str == "0");

			val.setFromString("0.0000");
			val.getString(str);
			UT_ASSERT(str == "0");

			// Case 5: Just dot
			val.setFromString("1.");
			val.getString(str);
			UT_ASSERT(str == "1");
		}

		return RR_SUCCESS;
	}
};

UT_REG_TEST(BigFloatTest, "Math.BigFloat")
