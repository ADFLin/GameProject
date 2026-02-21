#ifndef SurvivorsCommon_h__
#define SurvivorsCommon_h__

#include "Math/Vector2.h"
#include "ParallelCollision/ParallelCollision.h"

namespace Survivors
{
	using namespace ParallelCollision;
	using namespace Math;

	enum CollisionCategory
	{
		Category_Monster = 1,
		Category_Hero = 2,
		Category_Bullet = 4,
		Category_MonsterBullet = 8,
		Category_HeroAbility = 16,
		Category_Fence = 32,
	};

	enum class MonsterType
	{
		Golem,
		Slime,
		Count
	};

	enum class VisualType
	{
		Slash,
		Shockwave,
		HitSpark,
	};

	struct VisualEffect
	{
		Vector2 pos;
		float rotation;
		float radius;
		float angle;
		float timer;
		float duration;
		VisualType type;
	};

	struct Gem
	{
		Vector2 pos;
		int     value;
		bool    isDead = false;
	};

}//namespace Survivors

#endif // SurvivorsCommon_h__
