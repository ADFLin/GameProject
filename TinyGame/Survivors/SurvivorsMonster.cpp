#include "SurvivorsMonster.h"
#include "SurvivorsHero.h"
#include "SurvivorsStage.h"
#include "RandomUtility.h"

namespace Survivors
{
	void Monster::handleCollisionEnter(IPhysicsEntity* other, int category)
	{
		if (category == Category_HeroAbility)
		{
			// applyKnockback removed from Enter to reduce sensitivity
		}
		else if (category == Category_Hero)
		{
			static_cast<Hero*>(other)->takeDamage(20);
		}
	}

	void Monster::handleCollisionStay(IPhysicsEntity* other, int category)
	{
		if (category == Category_Monster)
		{
			Monster* otherMonster = static_cast<Monster*>(other);
			float otherSpeedSq = otherMonster->mKnockbackVel.length2();
			float mySpeedSq = mKnockbackVel.length2();
			if (otherSpeedSq > 500.0f && otherSpeedSq > mySpeedSq)
			{
				mKnockbackVel = otherMonster->mKnockbackVel * 0.8f;
			}
		}
		else if (category == Category_Hero)
		{
			static_cast<Hero*>(other)->takeDamage(20);
		}
	}

	void Monster::handleCollisionEnter(IPhysicsBullet* bullet, int category)
	{
		if (category == Category_Bullet || category == Category_HeroAbility)
		{
			// Damage scaling with level (roughly)
			int baseDmg = (category == Category_HeroAbility) ? 10 : 2;
			int levelBonus = mStage->getHeroLevel() / 2;
			int dmg = baseDmg + levelBonus;
			mHP -= dmg;
			if (mHP <= 0) mIsDead = true;

			if (category == Category_HeroAbility)
			{
				applyKnockback(500.0f, 0.8f);
			}

			VisualEffect ve;
			ve.pos = mPos + Vector2(RandFloat(-5, 5), RandFloat(-5, 5));
			ve.timer = ve.duration = 0.1f;
			ve.type = VisualType::HitSpark;
			mStage->addVisual(ve);
		}
	}

	void Monster::applyKnockback(float force, float duration)
	{
		Vector2 pushDir = mPos - mStage->getHeroPos();
		if (pushDir.normalize() < 0.001f) pushDir = Vector2(1, 0);
		mKnockbackVel = pushDir * force;
		mStunTimer = duration;
	}

}//namespace Survivors
