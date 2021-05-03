#include "MiscTestRegister.h"
#include "MarcoCommon.h"
#include <iostream>
#include "ProfileSystem.h"

enum
{
	A = 0, B, C, D, E, F, Count,
};

void TestCirminal()
{
	TIME_SCOPE("TestCirminal");

	for (uint32 posibleBits = 0; posibleBits < BIT(Count); ++posibleBits)
	{
		auto Value = [posibleBits](uint32 index) -> uint32
		{
			return !!(posibleBits & BIT(index));
		};

		auto CheckCondition = [&Value]() -> bool
		{
#define CHECK_CONDITION( code ) if (!(code) ) return false;
			CHECK_CONDITION(Value(A) + Value(B) >= 1);
			CHECK_CONDITION(Value(A) + Value(E) + Value(F) >= 2);
			CHECK_CONDITION(Value(A) + Value(D) < 2);
#if 0
			CHECK_CONDITION((Value(B) && Value(C)) || !(Value(B) || Value(C)));
#else
			CHECK_CONDITION(!(Value(B) ^ Value(C)));
#endif
			CHECK_CONDITION((Value(C) + Value(D)) == 1);
			CHECK_CONDITION((!(Value(D) == 0)) || (Value(E) == 0));

#undef CHECK_CONDITION
			return true;
		};
		if (CheckCondition())
		{
			for (int i = 0; i < Count; ++i)
			{
				std::cout << Value(i) << " ";
			}
			std::cout << std::endl;
		}
	}
}

REGISTER_MISC_TEST_ENTRY("Criminal Test", TestCirminal);