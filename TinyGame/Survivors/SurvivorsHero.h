#ifndef SurvivorsHero_h__
#define SurvivorsHero_h__

#include "SurvivorsEntity.h"
#include <vector>
#include <memory>

namespace Survivors
{
	class SurvivorsStage;

	class Hero : public IPhysicsEntity
	{
	public:
		Vector2 mPos;
		Vector2 mMoveTarget;
		float mRadius = 15.0f;
		int mMaxHP = 1000;
		int mHP = 1000;
		int mLevel = 1;
		int mXP = 0;
		int mNextLevelXP = 100;
		Vector2 mFacing = Vector2(1, 0);
		Vector2 mJoystickDir = Vector2::Zero();
		SurvivorsStage* mStage = nullptr;
		
		// Skill States
		float mBulletRate = 6.0f; // Actions per sec
		float mBulletTimer = 0;
		int   mOrbCount = 4;
		bool  mEnableOrbs = false;
		float mOrbRotation = 0;
		std::vector<std::unique_ptr<GameBullet>> mOrbBullets;
		
		float mDamageFlashTimer = 0;
		float mDamageTimer = 0;
		bool  mbInvincible = false;


		Hero(Vector2 const& pos, SurvivorsStage* stage) : mPos(pos), mMoveTarget(pos), mStage(stage) {}
		
		void takeDamage(int amount)
		{
			if (mbInvincible) return;
			if (mDamageTimer > 0) return;
			mHP -= amount;
			if (mHP < 0) mHP = 0;
			mDamageFlashTimer = 0.2f;
			mDamageTimer = 0.2f; // Reduced invincibility for better feedback
		}
		
		int getLevel() { return mLevel; }
		
		void handleCollisionEnter(IPhysicsEntity* other, int category) override 
		{
			if (category == Category_Monster) takeDamage(20);
		}
		void handleCollisionStay(IPhysicsEntity* other, int category) override 
		{ 
			if (category == Category_Monster) takeDamage(20);
		}
		void handleCollisionExit(IPhysicsEntity* other, int category) override {}
		
		void handleCollisionEnter(IPhysicsBullet* bullet, int category) override 
		{
			if (category == Category_MonsterBullet)
			{
				takeDamage(10);
			}
		}
		void handleCollisionStay(IPhysicsBullet* bullet, int category) override {}
		void handleCollisionExit(IPhysicsBullet* bullet, int category) override {}
		
		void update(float dt);
		void updateOrbs(float dt);
		void fireBullet();
		void performSlash(Vector2 const& mousePos);
		void performShockwave();
		void reset();
	};

}//namespace Survivors

#endif // SurvivorsHero_h__
