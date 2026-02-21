#include "SurvivorsHero.h"
#include "SurvivorsMonster.h"
#include "SurvivorsStage.h"
#include "InputManager.h"

namespace Survivors
{
	void Hero::update(float dt)
	{
		// Movement
		Vector2 moveDir = Vector2::Zero();
		if (InputManager::Get().isKeyDown(EKeyCode::W)) moveDir.y -= 1;
		if (InputManager::Get().isKeyDown(EKeyCode::S)) moveDir.y += 1;
		if (InputManager::Get().isKeyDown(EKeyCode::A)) moveDir.x -= 1;
		if (InputManager::Get().isKeyDown(EKeyCode::D)) moveDir.x += 1;

		// Virtual joystick overrides keyboard if active
		if (mJoystickDir.length2() > 0.01f)
		{
			moveDir = mJoystickDir;
		}

		if (moveDir.length2() > 0.1f)
		{
			if (moveDir.length2() > 1.0f)
				moveDir.normalize();
			mFacing = moveDir;
			mPos += moveDir * 300.0f * dt;
			mMoveTarget = mPos;
		}

		// Fire Bullets
		mBulletTimer += dt;
		if (mBulletRate > 0 && mBulletTimer > 1.0f / mBulletRate) { mBulletTimer = 0; fireBullet(); }

		updateOrbs(dt);

		if (mDamageFlashTimer > 0) mDamageFlashTimer -= dt;
		if (mDamageTimer > 0) mDamageTimer -= dt;
	}

	void Hero::updateOrbs(float dt)
	{
		if (!mEnableOrbs)
		{
			for (auto& b : mOrbBullets) mStage->getParallelManager().unregisterBullet(b.get());
			mOrbBullets.clear();
			return;
		}

		if (mOrbBullets.empty())
		{
			for (int i = 0; i < mOrbCount; ++i)
			{
				auto bullet = std::make_unique<GameBullet>(mPos, Vector2::Zero(), Category_HeroAbility);
				bullet->mRadius = 25.0f; // Visual radius matches physics
				CollisionBulletData bdata;
				bdata.position = bullet->mPos;
				bdata.categoryId = Category_HeroAbility;
				bdata.targetMask = Category_Monster;
				bdata.bQueryBulletCollision = false;
				bdata.shapeType = ShapeType::Circle;
				bdata.shapeParams = Vector3(bullet->mRadius, 0, 0);
				mStage->getParallelManager().registerBullet(bullet.get(), bdata);
				mOrbBullets.push_back(std::move(bullet));
			}
		}

		mOrbRotation += 220.0f * dt;
		for (int i = 0; i < mOrbBullets.size(); ++i)
		{
			float angle = mOrbRotation + i * (360.0f / mOrbBullets.size());
			float rad = angle * Math::PI / 180.0f;
			Vector2 offset(Math::Cos(rad), Math::Sin(rad));
			mOrbBullets[i]->mPos = mPos + offset * 100.0f;
			auto& data = mStage->getParallelManager().getBulletData(mOrbBullets[i]->mBulletId);
			data.position = mOrbBullets[i]->mPos;
			data.rotation = rad + Math::PI / 2; // Face tangent
		}
	}

	void Hero::fireBullet()
	{
		const auto& monsters = mStage->getMonsters();
		if (monsters.empty()) return;

		Monster* nearest = nullptr;
		float minDist = 1e10f;
		for (auto& m : monsters)
		{
			float d = (m->mPos - mPos).length2();
			if (d < minDist) { minDist = d; nearest = m.get(); }
		}

		if (nearest)
		{
			Vector2 dir = nearest->mPos - mPos;
			dir.normalize();
			auto bullet = std::make_unique<GameBullet>(mPos, dir * 600.0f, Category_Bullet);
			bullet->mRadius = 18.0f; // Larger visual and collision size
			CollisionBulletData bdata;
			bdata.position = bullet->mPos;
			bdata.velocity = bullet->mVel;
			bdata.rotation = std::atan2(dir.y, dir.x);
			bdata.categoryId = Category_Bullet;
			bdata.targetMask = Category_Monster | Category_MonsterBullet;
			bdata.bQueryBulletCollision = true;
			bdata.shapeType = ShapeType::Rect;
			bdata.shapeParams = Vector3(20, 5, 0); // Half-extent X, Y
			mStage->getParallelManager().registerBullet(bullet.get(), bdata);
			mStage->addBullet(std::move(bullet));
		}
	}

	void Hero::performSlash(Vector2 const& mousePos)
	{
		Vector2 dir = mousePos - mPos;
		float angle;
		if (dir.length2() < 25.0f) // 5px distance threshold
		{
			angle = std::atan2(mFacing.y, mFacing.x);
		}
		else
		{
			dir.normalize();
			mFacing = dir; // Face the slash direction
			angle = std::atan2(dir.y, dir.x);
		}

		auto bullet = std::make_unique<GameBullet>(mPos, Vector2::Zero(), Category_HeroAbility);
		bullet->mLifeTime = 0.12f;
		CollisionBulletData bdata;
		bdata.position = mPos;
		bdata.rotation = angle;
		bdata.categoryId = Category_HeroAbility;
		bdata.targetMask = Category_Monster;
		bdata.bQueryBulletCollision = false;
		bdata.shapeType = ShapeType::Arc;
		bdata.shapeParams = Vector3(120.0f, 60.0f * Math::PI / 180.0f, 0);
		mStage->getParallelManager().registerBullet(bullet.get(), bdata);
		mStage->addBullet(std::move(bullet));

		VisualEffect ve;
		ve.pos = mPos; ve.rotation = angle; ve.radius = 120.0f; ve.angle = 60.0f;
		ve.timer = ve.duration = 0.12f; ve.type = VisualType::Slash;
		mStage->addVisual(ve);
	}

	void Hero::performShockwave()
	{
		auto bullet = std::make_unique<GameBullet>(mPos, Vector2::Zero(), Category_HeroAbility);
		bullet->mLifeTime = 0.2f;
		CollisionBulletData bdata;
		bdata.position = mPos;
		bdata.categoryId = Category_HeroAbility; bdata.targetMask = Category_Monster;
		bdata.bQueryBulletCollision = false; bdata.shapeType = ShapeType::Circle;
		bdata.shapeParams = Vector3(200.0f, 0, 0);
		mStage->getParallelManager().registerBullet(bullet.get(), bdata);
		mStage->addBullet(std::move(bullet));

		VisualEffect ve;
		ve.pos = mPos; ve.radius = 200.0f;
		ve.timer = ve.duration = 0.3f; ve.type = VisualType::Shockwave;
		mStage->addVisual(ve);
	}

	void Hero::reset()
	{
		mOrbBullets.clear();
		mBulletTimer = 0;
		mOrbRotation = 0;
		mDamageFlashTimer = 0;
		mDamageTimer = 0;
		mHP = 1000;
	}

}//namespace Survivors
