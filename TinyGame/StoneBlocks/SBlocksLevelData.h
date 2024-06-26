#pragma once
#ifndef SBlocksLevelData_H_9D7C7557_7763_4853_BC6B_304EFFDDE3A1
#define SBlocksLevelData_H_9D7C7557_7763_4853_BC6B_304EFFDDE3A1

#include "SBlocksCore.h"

namespace SBlocks
{
	constexpr uint8 MakeByte(uint8 b0) { return b0; }
	template< typename ...Args >
	constexpr uint8 MakeByte(uint8 b0, Args ...args) { return b0 | (MakeByte(args...) << 1); }

#define M(...) __VA_ARGS__

	inline LevelDesc DefautlNewLevel =
	{
		//map
		{
			{
				5 ,
				{
					M(0,0,0,0,0),
					M(0,0,0,0,0),
					M(0,0,0,0,0),
					M(0,0,0,0,0),
					M(0,0,0,0,0),
				}
			}
		},

		//shape
		{

		},

		//piece
		{

		},
	};

#if 1
	inline LevelDesc ClassicalLevel =
	{
		//map
		{
			{
				8 ,
				{
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
				}
			}
		},

		//shape
		{
			{//0
				3,
				{
					M(1,0,1),
					M(1,1,1),
					M(1,0,1),
				},
				false,
			},
			{//1
				3,
				{
					M(1,1,1),
					M(1,1,1),
				},
				false,
			},
			{//2
				3,
				{
					M(1,0,0),
					M(1,1,1),
					M(0,1,0),
				},
				false,
			},
			{//3
				4,
				{
					M(1,1,1,1),
					M(0,1,0,0),
				},
				false,
			},
			{//4
				3,
				{
					M(1,1,1),
					M(1,0,0),
					M(1,0,0),
				},
				false,
			},
			{//5
				3,
				{
					M(1,0,1),
					M(1,1,1),
				},
				false,
			},
			{//6
				4,
				{
					M(1,1,1,0),
					M(0,0,1,1),
				},
				false,
			},
			{//7
				4,
				{
					M(1,1,1,1),
					M(1,0,0,0),
				},
				false,
			},
			{//8
				3,
				{
					M(1,1,0),
					M(0,1,0),
					M(0,1,1),
				},
				false,
			},
			{//9
				2,
				{
					M(1,1),
					M(1,1),
				},
				false,
			},
			{//10
				3,
				{
					M(1,1,1),
					M(0,1,0),
				},
				false,
			},
			{//11
				3,
				{
					M(1,1,0),
					M(0,1,1),
				},
				false,
			},
			{//12
				3,
				{
					M(1,1,1),
					M(1,0,0),
				},
				false,
			},
		},

		//piece
		{
			//{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},
			//{11},{10},{12},{4},{7},{1},{9},{6},{5},{3},{0},{8},{2},
			//{3},{9},{8},{0},{11},{12},{2},{4},{6},{10},{7},{1},{5},
			{9},{1},{5},{6},{8},{4},{0},{10},{3},{11},{2},{12},{7},
		},

		true,
	};
#else

	inline LevelDesc ClassicalLevel =
	{
		//map
		{
			{
				8 ,
				{
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
				}
			}
		},

		//shape
		{
			{//9
				2,
				{
					M(1,1),
					M(1,1),
				},
				false,
			},
			{//1
				3,
				{
					M(1,1,1),
					M(1,1,1),
				},
				false,
			},
			{//5
				3,
				{
					M(1,0,1),
					M(1,1,1),
				},
				false,
			},


			{//6
				4,
				{
					M(1,1,1,0),
					M(0,0,1,1),
				},
				false,
			},

			{//8
				3,
				{
					M(1,1,0),
					M(0,1,0),
					M(0,1,1),
				},
				false,
			},
			{//4
				3,
				{
					M(1,1,1),
					M(1,0,0),
					M(1,0,0),
				},
				false,
			},

			{//0
				3,
				{
					M(1,0,1),
					M(1,1,1),
					M(1,0,1),
				},
				false,
			},

			{//10
				3,
				{
					M(1,1,1),
					M(0,1,0),
				},
				false,
			},

			{//3
				4,
				{
					M(1,1,1,1),
					M(0,1,0,0),
				},
				false,
			},
			{//11
				3,
				{
					M(1,1,0),
					M(0,1,1),
				},
				false,
			},
			{//2
				3,
				{
					M(1,0,0),
					M(1,1,1),
					M(0,1,0),
				},
				false,
			},
			{//12
				3,
				{
					M(1,1,1),
					M(1,0,0),
				},
				false,
			},
			{//7
				4,
				{
					M(1,1,1,1),
					M(1,0,0,0),
				},
				false,
			},
		},

		//piece
		{
			{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},
			//{11},{10},{12},{4},{7},{1},{9},{6},{5},{3},{0},{8},{2},
			//{3},{9},{8},{0},{11},{12},{2},{4},{6},{10},{7},{1},{5},
			//{9},{1},{5},{6},{8},{4},{0},{10},{3},{11},{2},{12},{7},
		},

		true,
	};
#endif

	inline LevelDesc DebugLevel =
	{
		//map
		{
			{
				3 ,
				{
					M(0,0,0),
					M(1,0,0),
					M(1,1,0),
				}
			}
		},

		//shape
		{
			{
				3,
				{
					M(1,0,0),
					M(1,1,0),
					M(1,1,1),
				},
				true, {1 , 1},
			}
		},

		//piece
		{
			{0},
		},
	};

	inline LevelDesc TestLv =
	{
		//map
		{
			{
				5 ,
				{
					M(0,0,0,0,0),
					M(0,0,0,0,0),
					M(0,0,0,1,0),
					M(0,0,0,0,0),
					M(0,0,0,0,0),
				}
			}
		} ,

		//shape
		{
			{
				3,
				{
					M(1,1,1),
					M(0,1,0),
				},
				true, {1.5 , 0.5},
			},
			{
				3,
				{
					M(0,1,1),
					M(1,1,0),
				},
				true, {1.5 , 1.0},
			},
			{
				3,
				{
					M(1,0,0),
					M(1,1,1),
				},
				true, {1.5 , 1},
			},
			{
				2,
				{
					M(1,1),
				},
				true, {1 , 0.5},
			},
			{
				3,
				{
					M(1,1,1),
				},
				true, {1.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,0),
				},
				true, {0.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,1),
				},
				true, {1 , 1},
			},
		},

		//piece
		{
			{0},
			{1},
			{2},
			{3},
			{4},
			{5},
			{6},
		}
	};

	inline LevelDesc TestLv2 =
	{
		//map
		{
			{
				6 ,
				{
					M(0,0,0,0,0,0),
					M(0,0,0,0,0,0),
					M(0,0,1,0,0,0),
					M(0,0,0,0,0,0),
					M(0,0,0,0,0,1),
				}
			},
		} ,

		//shape
		{
			{
				3,
				{
					M(1,1,1),
					M(0,1,0),
				},
				true, {1.5 , 0.5},
			},
			{
				3,
				{
					M(0,1,1),
					M(1,1,0),
				},
				true, {1.5 , 1.0},
			},
			{
				3,
				{
					M(1,0,0),
					M(1,1,1),
				},
				true, {1.5 , 1},
			},
			{
				2,
				{
					M(1,1),
				},
				true, {1 , 0.5},
			},
			{
				3,
				{
					M(1,1,1),
				},
				true, {1.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,0),
				},
				true, {0.5 , 0.5},
			},
			{
				2,
				{
					M(1,1),
					M(1,1),
				},
				true, {1 , 1},
			},

			{
				5,
				{
					M(1,1,1,1),
				},
				true, {2.5 , 0.5},
			},
		},

		//piece
		{
			{7},
			{0},
			{1},
			{2},
			{3},
			{4},
			{5},
			{6},

		}
	};

	inline LevelDesc TestLv3 =
	{
		//map
		{
			{
				8 ,
				{
					M(0,0,0,0,0,0,0,1),
					M(0,0,0,0,0,0,0,1),
					M(1,0,0,0,0,0,1,1),
					M(1,0,1,0,0,0,1,1),
					M(1,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
					M(0,0,0,0,0,0,0,0),
				}
			},
		} ,

		//shape
		{
			{
				5,
				{
					M(1,1,0,0,0),
					M(0,1,1,1,1),
				},
				true, {2.5 , 1.0},
			},
			{
				4,
				{
					M(0,0,1,1),
					M(1,1,1,1),
				},
				true, {2.0 , 1.0},
			},
			{
				3,
				{
					M(0,1,1),
					M(0,1,1),
					M(1,1,1),
				},
				true, {1.5 , 1.5},
			},
			{
				3,
				{
					M(1,1,0),
					M(1,1,1),
					M(1,1,1),
				},
				true, {1.5 , 1.5},
			},
			{
				2,
				{
					M(0,1),
					M(0,1),
					M(1,1),
				},
				true, {1 , 1.5},
			},
			{
				4,
				{
					M(1,1,0,0),
					M(1,1,0,0),
					M(1,1,1,0),
					M(1,1,1,1),
					M(1,1,1,1),
				},
				true, {2 , 2.5},
			},
			{
				4,
				{
					M(0,0,1,1),
					M(0,0,1,1),
					M(1,1,1,1),
				},
				true, {2.5 , 1.5},
			},
		},

		//piece
		{
			{0},
			{1},
			{2},
			{3},
			{4},
			{5},
			{6},
			{7},
		}
	};

	inline LevelDesc TestLv4 =
	{
		//map
		{
			{
				6 ,
				{
					M(1,1,0,0,1,1),
					M(0,0,0,0,0,0),
					M(0,0,0,0,0,0),
					M(1,1,0,0,1,1),
					M(1,1,0,0,1,1),
				}
			},
		} ,

		//shape
		{
			{
				2,
				{
					M(1,1),
					M(1,1),
				},
				false,
			},
			{
				2,
				{
					M(1,1),
				},
				false,
			},
		},

		//piece
		{
			{0},
			{0},
			{1},
			{0},
			{0},
		}
	};

#undef M
}

#endif // SBlocksLevelData_H_9D7C7557_7763_4853_BC6B_304EFFDDE3A1