#ifndef SurvivorsEntity_h__
#define SurvivorsEntity_h__

#include "SurvivorsCommon.h"
#include <vector>
#include <memory>

namespace Survivors
{
	class GameBullet : public IPhysicsBullet
	{
	public:
		Vector2 mPos;
		Vector2 mVel;
		float mRadius = 12.0f;
		float mLifeTime = 5.0f;
		bool mIsDead = false;
		int mCategory = Category_Bullet;

		GameBullet(Vector2 const& pos, Vector2 const& vel, int category = Category_Bullet) : mPos(pos), mVel(vel), mCategory(category) {}

		void handleCollisionEnter(IPhysicsEntity* other, int category) override
		{
			// Pierce through monsters
			// if (mCategory == Category_Bullet && category == Category_Monster) mIsDead = true;
			if (mCategory == Category_MonsterBullet && category == Category_Hero) mIsDead = true;
		}
		void handleCollisionStay(IPhysicsEntity* other, int category) override {}
		void handleCollisionExit(IPhysicsEntity* other, int category) override {}

		void handleCollisionEnter(IPhysicsBullet* other, int category) override
		{
			if (mCategory == Category_Bullet && category == Category_MonsterBullet) mIsDead = true;
			else if (mCategory == Category_MonsterBullet && category == Category_Bullet) mIsDead = true;
		}
		void handleCollisionStay(IPhysicsBullet* other, int category) override {}
		void handleCollisionExit(IPhysicsBullet* other, int category) override {}

		void update(float dt)
		{
			mPos += mVel * dt;
			mLifeTime -= dt;
			if (mLifeTime <= 0) mIsDead = true;
		}
	};

}//namespace Survivors

#endif // SurvivorsEntity_h__
