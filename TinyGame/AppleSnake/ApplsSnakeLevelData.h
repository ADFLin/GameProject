#pragma once
#ifndef ApplsSnakeLevelData_H_25755E7C_9CB5_41D7_A0B8_4A0C659A393A
#define ApplsSnakeLevelData_H_25755E7C_9CB5_41D7_A0B8_4A0C659A393A

#include "AppleSnakeLevel.h"

#include "Template/ArrayView.h"

namespace AppleSnake
{

#define B ETile::Block
#define S ETile::SnakeBody
#define H ETile::SnakeHead
#define A ETile::Apple
#define G ETile::Goal
#define R ETile::Rock
#define T ETile::Trap
#define P ETile::Portal

	struct LevelData
	{
		Vec2i size;
		int   border;

		TArrayView< uint8 const > data;
		TArrayView< uint8 const > meta;
	};

	LevelData LvTest =
	{
		Vec2i(10, 5), 1,
		ARRAY_VIEW_REAONLY_DATA(uint8,
			S,S,H,0,0,0,0,0,0,G,
			B,B,B,B,0,0,0,0,0,B,
			B,0,0,B,0,0,0,0,0,B,
			B,0,0,0,0,A,0,0,0,B,
			B,B,B,B,0,0,0,B,B,B,
		)
	};

	LevelData LvTest2 =
	{
		Vec2i(12, 6), 1,
		ARRAY_VIEW_REAONLY_DATA(uint8,
			S,H,0,0,0,0,0,0,0,0,0,0,
			S,B,0,0,A,0,0,B,0,0,0,0,
			0,B,0,0,0,0,0,R,0,0,0,G,
			0,B,0,0,0,0,0,R,0,0,0,0,
			0,B,B,0,0,0,0,B,0,0,0,0,
			0,0,0,0,0,0,B,B,B,0,0,0,
		)
	};

	LevelData LvTest20 =
	{
		Vec2i(12, 9), 1,
		ARRAY_VIEW_REAONLY_DATA(uint8,
			0,0,0,0,B,0,0,0,0,0,0,0,
			0,0,0,0,B,0,0,0,0,0,0,0,
			0,S,H,0,B,0,0,0,0,0,0,0,
			0,S,R,0,B,0,R,0,0,0,0,G,
			0,0,B,B,B,B,B,0,0,0,0,B,
			0,A,0,0,0,0,0,0,0,0,0,B,
			0,0,0,0,0,0,0,0,0,0,0,B,
			B,0,B,B,B,B,B,B,B,0,0,B,
			B,B,B,0,0,0,0,0,B,B,B,B,
		)
	};

	LevelData LvTest21 =
	{
		Vec2i(9, 4), 1,
		ARRAY_VIEW_REAONLY_DATA(uint8,
			0,0,0,0,A,0,0,0,0,
			0,0,0,0,0,0,0,0,0,
			S,H,0,0,T,0,0,0,G,
			S,B,B,B,B,B,B,B,B,
		)
	};

	LevelData LvPortalTest =
	{
		Vec2i(9, 4), 1,
		ARRAY_VIEW_REAONLY_DATA(uint8,
			0,0,0,0,B,0,0,0,0,
			0,0,0,0,B,0,0,0,0,
			S,H,R,P,B,P,0,0,G,
			S,B,B,B,B,B,B,B,B,
		),
		ARRAY_VIEW_REAONLY_DATA(uint8,0x00,0x02)
	};


#undef B
#undef S
#undef H
#undef A
#undef G
#undef R
#undef P

}//namespace AppleSnake

#endif // ApplsSnakeLevelData_H_25755E7C_9CB5_41D7_A0B8_4A0C659A393A