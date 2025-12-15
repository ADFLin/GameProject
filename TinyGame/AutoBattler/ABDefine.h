#ifndef AutoBattlerDefine_h__
#define AutoBattlerDefine_h__

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"

#define AUTO_BATTLER_NAME "AutoBattler"

// Game Constants
#define AB_MAX_PLAYER_NUM 8

// Board
#define AB_MAP_SIZE_X 8
#define AB_MAP_SIZE_Y 4
#define AB_BENCH_SIZE 8

// Player State
#define AB_INITIAL_GOLD 2
#define AB_INITIAL_HP 100
#define AB_MAX_LEVEL 10
#define AB_SHOP_SLOT_COUNT 5
#define AB_SHOP_REFRESH_COST 2
#define AB_BUY_XP_COST 4
#define AB_XP_PER_BUY 4

// Timings
#define AB_PHASE_PREP_TIME 30.0f
#define AB_PHASE_COMBAT_TIME 40.0f

// Visual Sizing (TFT-style proportions)
// Board Cell - hex grid cell size in world units
#define AB_CELL_SIZE 80
// Bench - same size as board cells for visual consistency
#define AB_BENCH_SLOT_SIZE AB_CELL_SIZE
#define AB_BENCH_GAP 0
#define AB_BENCH_OFFSET_X -20.0f
#define AB_BENCH_OFFSET_Y -15.0f
// Unit model scale relative to cell/slot size
#define AB_UNIT_SCALE_BOARD 1.0f
#define AB_UNIT_SCALE_BENCH 1.0f
// Board tile scale relative to cell size
#define AB_TILE_SCALE 0.90f
// Spacing between player boards
#define AB_BOARD_PADDING 150



namespace AutoBattler
{
	using Math::Vector2;
	using Math::Vector3;
	using Math::Vector4;
	using Math::Matrix4;

	enum class ECoordType
	{
		None,
		Board,
		Bench,
	};

	struct UnitLocation
	{
		ECoordType type = ECoordType::None;
        union
        {
            Vec2i pos;
            int index;
        };

		bool isValid() const { return type != ECoordType::None; }
		bool isBoard() const { return type == ECoordType::Board; }
		bool isBench() const { return type == ECoordType::Bench; }
	};

}

#endif // AutoBattlerDefine_h__
