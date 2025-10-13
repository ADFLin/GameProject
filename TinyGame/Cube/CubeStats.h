#pragma once
#ifndef CubeStats_H_F10E0388_7D7B_4FF2_9D40_BA87EFF7FAAF
#define CubeStats_H_F10E0388_7D7B_4FF2_9D40_BA87EFF7FAAF

namespace Cube
{
	struct Stats
	{
		double time;
		double timeAcc;
		int    count;
	};



	struct ScopedStats
	{
		ScopedStats(Stats& record)
			:record(record)
		{



		}

		~ScopedStats()
		{



		}

		Stats& record;
	};

#define STATS(Stats)\
	
}//namespace Cube

#endif // CubeStats_H_F10E0388_7D7B_4FF2_9D40_BA87EFF7FAAF
