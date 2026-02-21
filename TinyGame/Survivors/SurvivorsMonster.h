#ifndef SurvivorsMonster_h__
#define SurvivorsMonster_h__

#include "SurvivorsEntity.h"

namespace Survivors
{
	class SurvivorsStage;

	class Monster : public IPhysicsEntity
	{
	public:
		MonsterType mType = MonsterType::Golem;
		Vector2 mPos;
		Vector2 mVel = Vector2::Zero();
		Vector2 mKnockbackVel = Vector2::Zero();
		float mStunTimer = 0;
		float mRadius = 10.0f;
		float mScale = 1.0f;
		float mSpeed = 80.0f;
		int mHP = 50;
		bool mIsDead = false;
		SurvivorsStage* mStage = nullptr;

		Monster(Vector2 const& pos, SurvivorsStage* stage, MonsterType type = MonsterType::Golem)
			: mPos(pos), mStage(stage), mType(type)
		{
			if (type == MonsterType::Golem) { mSpeed = 50.0f; mHP = 500; mRadius = 30.0f; }
			else if (type == MonsterType::Slime) { mSpeed = 130.0f; mHP = 20; mRadius = 10.0f; }
		}

		void handleCollisionEnter(IPhysicsEntity* other, int category) override;
		void handleCollisionStay(IPhysicsEntity* other, int category) override;
		void handleCollisionExit(IPhysicsEntity* other, int category) override {}

		void handleCollisionEnter(IPhysicsBullet* bullet, int category) override;
		void handleCollisionStay(IPhysicsBullet* bullet, int category) override {}
		void handleCollisionExit(IPhysicsBullet* bullet, int category) override {}

		void applyKnockback(float force, float duration);

		void update(Vector2 const& target, float dt)
		{
			if (mStunTimer > 0)
			{
				mStunTimer -= dt;
				mVel = mKnockbackVel;
				mKnockbackVel *= std::max(0.0f, 1.0f - 4.0f * dt);
			}
			else
			{
				Vector2 dir = target - mPos;
				float dist = dir.length();
				if (dist > 1.0f) mVel = (dir / dist) * mSpeed;
				else mVel = Vector2::Zero();
			}
		}
	};

}//namespace Survivors

#endif // SurvivorsMonster_h__
