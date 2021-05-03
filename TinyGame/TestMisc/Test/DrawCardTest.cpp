
#include "MiscTestRegister.h"


void TestDrawCard()
{
	auto RunTest = [&]()
	{
		int cards[9];
		for (int i = 0; i < ARRAY_SIZE(cards); ++i)
		{
			cards[i] = i;
		}

		for( int n = 0 ; n < 10; ++n )
		{
			for (int i = 0; i < ARRAY_SIZE(cards); ++i)
			{
				int idx1 = rand() % ARRAY_SIZE(cards);
				std::swap(cards[i], cards[idx1]);
			}
		}

		int count = 0;
		for (;;)
		{
			if (cards[count] == 0)
				break;
			++count;
		}
		return count + 1;
	};

	int countAcc = 0;
	int const roundNum = 200000;

	for (int i = 0; i < roundNum; ++i)
	{
		countAcc += RunTest();
	}

	LogMsg("Result = %f", float(countAcc) / roundNum);
}


REGISTER_MISC_TEST_ENTRY("Draw Card Test", TestDrawCard);