#include "StageBase.h"
#include "SurvivorsStage.h"
#include "SurvivorsRenderer.h"

#include "RandomUtility.h"
#include "ProfileSystem.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"

#include "SystemPlatform.h"
#include "InputManager.h"

#include <algorithm>
#include <cmath>

#include "RHI/RHIGraphics2D.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGlobalResource.h"

#include "DrawEngine.h"
#include "GameGUISystem.h"
#include "StageRegister.h"

namespace Survivors
{
	using namespace ParallelCollision;

	bool SurvivorsStage::onInit()
	{
		if (!BaseClass::onInit()) return false;

		mScreenSize = ::Global::GetScreenSize();
		mHero = std::make_unique<Hero>(Vector2(mScreenSize.x / 2.0f, mScreenSize.y / 2.0f), this);
		mCameraPos = mHero->mPos;

		// Initialize renderer (loads all textures)
		mRenderer.init();

		mParallelManager.init(Vector2(-2000, -2000), Vector2(mScreenSize.x + 2000, mScreenSize.y + 2000), 32.0f);

		{
			CollisionEntityData data;
			data.position = mHero->mPos;
			data.radius = mHero->mRadius;
			data.velocity = Vector2::Zero();
			data.mass = 10000.0f;
			data.isStatic = false;
			data.bQueryCollision = true;
			data.categoryId = Category_Hero;
			data.targetMask = Category_Monster | Category_MonsterBullet;
			mParallelManager.registerEntity(mHero.get(), data);
		}

		mParallelManager.addWall(Vector2(-500, -500), Vector2(mScreenSize.x + 500, -400));
		mParallelManager.addWall(Vector2(-500, mScreenSize.y + 400), Vector2(mScreenSize.x + 500, mScreenSize.y + 500));
		mParallelManager.addWall(Vector2(-500, -500), Vector2(-400, mScreenSize.y + 500));
		mParallelManager.addWall(Vector2(mScreenSize.x + 400, -500), Vector2(mScreenSize.x + 500, mScreenSize.y + 500));

		mThreadPool.init(std::max(1u, (uint32)4));
		mParallelManager.getSolver().settings.frameTargetSubStep = 1.0f / 120.0f;

		::Global::GUI().cleanupWidget();
		DevFrame* frame = WidgetUtility::CreateDevFrame();

		FWidgetProperty::Bind(frame->addSlider("Spawn Rate"), mSpawnRate, 0.5f, 100.0f);
		FWidgetProperty::Bind(frame->addSlider("Bullet Rate"), mHero->mBulletRate, 1.0f, 20.0f);
		FWidgetProperty::Bind(frame->addSlider("Monster Fire Rate"), mMonsterFireRate, 0.1f, 10.0f);

		frame->addCheckBox("Enable Orbs (Q)", mHero->mEnableOrbs);

		FWidgetProperty::Bind(frame->addSlider("Iterations"), mParallelManager.getSolver().settings.iterations, 1, 20);

		frame->addCheckBox("Show Grid", mShowCells);
		frame->addCheckBox("Show Entities", mShowEntities);
		frame->addCheckBox("Show Bullets", mShowBullets);
		frame->addCheckBox("Show Events", mShowEvents);
		frame->addCheckBox("Show Walls", mShowWalls);
		FWidgetProperty::Bind(frame->addSlider("Golem Draw Scale"), mRenderer.monsterDrawScale((int)MonsterType::Golem), 1.0f, 10.0f);
		FWidgetProperty::Bind(frame->addSlider("Slime Draw Scale"), mRenderer.monsterDrawScale((int)MonsterType::Slime), 1.0f, 10.0f);

		return true;
	}

	void SurvivorsStage::onEnd()
	{
		if (mHero) mHero->mOrbBullets.clear();
		mParallelManager.cleanup();
		mMonsters.clear();
		mBullets.clear();
		mThreadPool.waitAllWorkComplete();
		BaseClass::onEnd();
	}

	void SurvivorsStage::spawnMonster()
	{
		Vector2 pos;
		float side = RandFloat(0, 4);
		float margin = 150.0f;
		if (side < 1) pos = Vector2(RandFloat(0, mScreenSize.x), -margin);
		else if (side < 2) pos = Vector2(RandFloat(0, mScreenSize.x), mScreenSize.y + margin);
		else if (side < 3) pos = Vector2(-margin, RandFloat(0, mScreenSize.y));
		else pos = Vector2(mScreenSize.x + margin, RandFloat(0, mScreenSize.y));

		MonsterType type = MonsterType::Slime;
		if (RandFloat(0, 1) < 0.01f)
		{
			type = MonsterType::Golem;
		}

		auto monster = std::make_unique<Monster>(pos, this, type);
		CollisionEntityData data;
		if (type == MonsterType::Golem)
		{
			monster->mRadius = 30.0f;
			monster->mHP = 500;
			data.mass = 20.0f;
		}
		else
		{
			data.mass = 1.0f;
		}

		data.position = pos;
		data.velocity = Vector2::Zero();
		data.isStatic = false;
		data.bQueryCollision = true;
		data.categoryId = Category_Monster;
		data.targetMask = Category_Hero | Category_HeroAbility | Category_Bullet;

		data.radius = monster->mRadius;
		data.mass = (monster->mType == MonsterType::Golem) ? 20.0f : 1.0f;
		mParallelManager.registerEntity(monster.get(), data);
		mMonsters.push_back(std::move(monster));
	}

	void SurvivorsStage::monsterFire()
	{
		for (auto& m : mMonsters)
		{
			if (RandFloat(0, 1) < 0.02f)
			{
				Vector2 dir = mHero->mPos - m->mPos;
				dir.normalize();
				auto bullet = std::make_unique<GameBullet>(m->mPos, dir * 200.0f, Category_MonsterBullet);
				bullet->mRadius = 6.0f;
				CollisionBulletData bdata;
				bdata.position = bullet->mPos;
				bdata.velocity = bullet->mVel;
				bdata.categoryId = Category_MonsterBullet;
				bdata.targetMask = Category_Hero | Category_Bullet;
				bdata.bQueryBulletCollision = true;
				bdata.shapeType = ShapeType::Circle;
				bdata.shapeParams = Vector3(bullet->mRadius, 0, 0);
				mParallelManager.registerBullet(bullet.get(), bdata);
				mBullets.push_back(std::move(bullet));
			}
		}
	}

	void SurvivorsStage::onUpdate(GameTimeSpan deltaTime)
	{
		mParallelManager.beginFrame();

		{
			PROFILE_ENTRY("SyncGameFormPhysics");
			if (mHero && mHero->mPhysicsId != -1)
			{
				auto& data = mParallelManager.getEntityData(mHero->mPhysicsId);
				mHero->mPos = data.position;
			}
			for (auto& m : mMonsters)
			{
				auto& data = mParallelManager.getEntityData(m->mPhysicsId);
				m->mPos = data.position;
			}
			for (auto& b : mBullets)
			{
				auto& data = mParallelManager.getBulletData(b->mBulletId);
				b->mPos = data.position;
			}
		}

		PROFILE_ENTRY("Game Tick");
		float dt = deltaTime.value;

		if (mbPaused || (mHero && mHero->mHP <= 0))
		{
			// Keep camera moving even in pause/gameover but stop simulation
		}
		else
		{
			mSpawnTimer += dt;
			if (mSpawnRate > 0 && mSpawnTimer > 1.0f / mSpawnRate) { mSpawnTimer = 0; spawnMonster(); }

			mMonsterFireTimer += dt;
			if (mMonsterFireRate > 0 && mMonsterFireTimer > 1.0f / mMonsterFireRate) { mMonsterFireTimer = 0; monsterFire(); }

			Vector2 heroPos = mHero ? mHero->mPos : Vector2::Zero();

			// Update Logic
			if (mHero) mHero->update(dt);
			for (auto& m : mMonsters) m->update(heroPos, dt);
			for (auto& b : mBullets) b->update(dt);
		}

		// Update Camera
		if (mHero)
		{
			float lerpSpeed = 1.0f - std::exp(-mCameraFollowSpeed * deltaTime.value);
			mCameraPos += (mHero->mPos - mCameraPos) * lerpSpeed;
		}
		{
			float zoomLerp = 1.0f - std::exp(-5.0f * deltaTime.value);
			mCameraZoom += (mCameraZoomTarget - mCameraZoom) * zoomLerp;
		}
		mWorldToScreen = RenderTransform2D::LookAt(Vector2(mScreenSize), mCameraPos, 0.0f, mCameraZoom);
		mScreenToWorld = mWorldToScreen.inverse();

		if (!mbPaused)
		{
			for (auto it = mVisuals.begin(); it != mVisuals.end();)
			{
				it->timer -= dt;
				if (it->type == VisualType::Slash && mHero) it->pos = mHero->mPos;
				if (it->timer <= 0) it = mVisuals.erase(it);
				else ++it;
			}

			// Sync FROM Objects TO Physics
			if (mHero && mHero->mPhysicsId != -1)
			{
				auto& data = mParallelManager.getEntityData(mHero->mPhysicsId);
				CHECK(mParallelManager.mReceivers[mHero->mPhysicsId].entity == mHero.get());
				data.position = mHero->mPos;
				data.radius = mHero->mRadius;
			}
			for (auto& m : mMonsters)
			{
				auto& data = mParallelManager.getEntityData(m->mPhysicsId);
				data.position = m->mPos;
				data.velocity = m->mVel;
			}
			for (auto& b : mBullets)
			{
				if (mHero && b->mCategory == Category_HeroAbility)
				{
					auto& data = mParallelManager.getBulletData(b->mBulletId);
					if (data.shapeType == ShapeType::Arc)
					{
						b->mPos = mHero->mPos;
					}
				}

				auto& data = mParallelManager.getBulletData(b->mBulletId);
				data.position = b->mPos;
				data.velocity = b->mVel;
				if (b->mVel.length2() > 0.001f)
					data.rotation = std::atan2(b->mVel.y, b->mVel.x);
			}

			// Clean up dead monsters
			mMonsters.erase(std::remove_if(mMonsters.begin(), mMonsters.end(), [this](auto& m) {
				if (m->mIsDead) {
					Gem gem;
					gem.pos = m->mPos;
					gem.value = (m->mType == MonsterType::Golem) ? 10 : 1;
					mGems.push_back(gem);
					mParallelManager.unregisterEntity(m.get());
					return true;
				}
				return false;
			}), mMonsters.end());

			// Clean up dead bullets
			mBullets.erase(std::remove_if(mBullets.begin(), mBullets.end(), [this](auto& b) {
				if (b->mIsDead) { mParallelManager.unregisterBullet(b.get()); return true; }
				return false;
			}), mBullets.end());

			// Gems Update
			if (mHero)
			{
				for (auto& gem : mGems)
				{
					float distSq = (gem.pos - mHero->mPos).length2();
					if (distSq < Square(200.0f))
					{
						Vector2 dir = mHero->mPos - gem.pos;
						gem.pos += dir * (8.0f * dt);
						if (distSq < Square(mHero->mRadius + 10.0f))
						{
							mHero->mXP += gem.value;
							gem.isDead = true;
							while (mHero->mXP >= mHero->mNextLevelXP)
							{
								mHero->mXP -= mHero->mNextLevelXP;
								mHero->mLevel++;
								mHero->mNextLevelXP = (int)(mHero->mNextLevelXP * 1.3f);
								VisualEffect ve;
								ve.pos = mHero->mPos;
								ve.timer = ve.duration = 0.5f;
								ve.type = VisualType::Shockwave;
								ve.radius = 400.0f;
								mVisuals.push_back(ve);

								// Level Up Benefits
								mHero->mHP = std::min(mHero->mMaxHP, mHero->mHP + (int)(mHero->mMaxHP * 0.2f));
								mHero->mBulletRate *= 1.1f;
								if (mHero->mLevel % 5 == 0 && mHero->mOrbCount < 24)
								{
									mHero->mOrbCount++;
									for (auto& b : mHero->mOrbBullets)
										mParallelManager.unregisterBullet(b.get());
									mHero->mOrbBullets.clear();
								}
								ve.radius = 400.0f;
								mVisuals.push_back(ve);
							}
						}
					}
				}
				mGems.erase(std::remove_if(mGems.begin(), mGems.end(), [](auto& g) { return g.isDead; }), mGems.end());
			}

			mParallelManager.endFrame(dt);
		}
	}

	void SurvivorsStage::onRender(float dFrame)
	{
		// Build scene snapshot for render thread
		auto snapshot = std::make_shared<SceneSnapshot>();
		snapshot->worldToScreen = mWorldToScreen;
		snapshot->screenToWorld = mScreenToWorld;
		snapshot->screenSize = Vector2(mScreenSize);

		snapshot->monsters.reserve(mMonsters.size());
		for (auto const& m : mMonsters) snapshot->monsters.push_back({ m->mPos, m->mRadius, m->mScale, m->mStunTimer, m->mType });

		snapshot->gems.reserve(mGems.size());
		for (auto const& gem : mGems) snapshot->gems.push_back({ gem.pos });

		snapshot->bullets.reserve(mBullets.size());
		for (auto const& b : mBullets) snapshot->bullets.push_back({ b->mPos, b->mVel, b->mRadius, b->mCategory });

		snapshot->visuals.reserve(mVisuals.size());
		for (auto const& v : mVisuals) snapshot->visuals.push_back({ v.pos, v.type, v.timer, v.duration, v.radius, v.angle, v.rotation });

		if (mHero)
		{
			snapshot->hero.pos = mHero->mPos;
			snapshot->hero.radius = mHero->mRadius;
			snapshot->hero.hpParams = (float)mHero->mHP / mHero->mMaxHP;
			snapshot->hero.damageFlashTimer = mHero->mDamageFlashTimer;
			snapshot->hero.facing = mHero->mFacing;
			snapshot->hero.orbs.reserve(mHero->mOrbBullets.size());
			for (auto const& b : mHero->mOrbBullets) snapshot->hero.orbs.push_back({ b->mPos, b->mVel, b->mRadius, b->mCategory });
		}

		RenderThread::AddCommand("RenderSurvivors", [this, snapshot]()
		{
			mRenderer.renderScene(*snapshot);
		});

		// Debug Drawing & UI on GameThread
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			g.beginRender();
			g.setBlendAlpha(1.0f);

			// Debug Drawing (World Space)
			g.transformXForm(mWorldToScreen, true);
			auto& solver = mParallelManager.getSolver();

			if (mShowCells)
			{
				float cellSize = solver.settings.cellSize;
				Vector2 minBound = solver.grid.minBound;
				int width = solver.grid.gridWidth;
				int height = solver.grid.gridHeight;
				g.setPen(Color3f(0, 1, 0), 1);
				g.enableBrush(false);
				for (int i = 0; i <= width; ++i)
				{
					float x = minBound.x + i * cellSize;
					g.drawLine(Vector2(x, minBound.y), Vector2(x, minBound.y + height * cellSize));
				}
				for (int i = 0; i <= height; ++i)
				{
					float y = minBound.y + i * cellSize;
					g.drawLine(Vector2(minBound.x, y), Vector2(minBound.x + width * cellSize, y));
				}
			}

			if (mShowEntities)
			{
				g.setPen(Color3f(0, 0, 1), 1);
				g.enableBrush(false);
				for (int i = 0; i < mParallelManager.mEntityCount; ++i)
				{
					auto const& data = mParallelManager.mEntityDataBuffer[i];
					g.drawCircle(data.position, data.radius);
				}
			}

			if (mShowBullets)
			{
				g.setPen(Color3f(1, 1, 0), 1);
				g.enableBrush(false);
				for (int i = 0; i < mParallelManager.mBulletCount; ++i)
				{
					auto const& data = mParallelManager.mBulletDataBuffer[i];
					Vector2 pos = data.position;
					ShapeType type = data.shapeType;
					Vector3 params = data.shapeParams;
					float rot = data.rotation;
					switch (type)
					{
					case ShapeType::Circle: g.drawCircle(pos, params.x); break;
					case ShapeType::Rect:
					{
						Vector2 halfSize = Vector2(params.x, params.y) * 0.5f;
						Vector2 corners[4] = { Vector2(-halfSize.x, -halfSize.y), Vector2(halfSize.x, -halfSize.y), Vector2(halfSize.x, halfSize.y), Vector2(-halfSize.x, halfSize.y) };
						float c = std::cos(rot), s = std::sin(rot);
						for (int k = 0; k < 4; ++k) { float nx = corners[k].x * c - corners[k].y * s; float ny = corners[k].x * s + corners[k].y * c; corners[k] = pos + Vector2(nx, ny); }
						g.drawPolygon(corners, 4);
					}
					break;
					case ShapeType::Arc: g.drawArcLine(pos, params.x, rot - params.y * 0.5f, params.y); break;
					}
				}
			}

			if (mShowEvents)
			{
				g.setPen(Color3f(1, 0, 0), 1);
				g.enableBrush(false);
				for (int id : mParallelManager.mLastActiveEntityIndices)
				{
					if (id < 0 || id >= mParallelManager.mEntityCount) continue;
					auto const& rec = mParallelManager.mReceivers[id];
					Vector2 posA = mParallelManager.mEntityDataBuffer[id].position;
					if (rec.eventData)
					{
						for (int otherHandle : rec.eventData->prevEntities)
						{
							IPhysicsEntity* other = mParallelManager.getEntityFromHandle(otherHandle);
							if (other && other->mPhysicsId >= 0 && other->mPhysicsId < mParallelManager.mEntityCount)
								g.drawLine(posA, mParallelManager.mEntityDataBuffer[other->mPhysicsId].position);
						}
						for (int bHandle : rec.eventData->prevBullets)
						{
							IPhysicsBullet* bullet = mParallelManager.getBulletFromHandle(bHandle);
							if (bullet && bullet->mBulletId >= 0 && bullet->mBulletId < mParallelManager.mBulletCount)
								g.drawLine(posA, mParallelManager.mBulletDataBuffer[bullet->mBulletId].position);
						}
					}
				}
			}

			if (mShowWalls)
			{
				g.setPen(Color3f(1, 1, 0), 1);
				g.setBrush(Color3f(1, 1, 0));
				for (auto const& wall : solver.walls)
				{
					g.enableBrush(true); g.enablePen(false); g.beginBlend(0.15f);
					g.drawRect(wall.min, wall.max - wall.min);
					g.endBlend(); g.enableBrush(false); g.enablePen(true);
					g.drawRect(wall.min, wall.max - wall.min);
				}
			}

			// UI & HUD (Screen Space)
			g.identityXForm();

			// Draw Joystick
			if (mVJoystickActive)
			{
				g.enableBrush(false); g.setPen(Color4f(1, 1, 1, 0.4f), 2);
				g.drawCircle(mVJoystickCenter, mVJoystickRadius);
				Vector2 offset = mVJoystickCurrentPos - mVJoystickCenter;
				float dist = offset.length();
				if (dist > mVJoystickRadius) offset = offset * (mVJoystickRadius / dist);
				g.enableBrush(true); g.setBrush(Color4f(1, 1, 1, 0.5f)); g.setPen(Color4f(1, 1, 1, 0.8f), 1);
				g.drawCircle(mVJoystickCenter + offset, 12.0f);
			}

			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<512> info;
			info.format("Hero HP: %d | Monsters: %d | Bullets: %d %s\nCollision: %.3f ms | Iters: %d\n[RenderThread Split Active]",
				mHero ? mHero->mHP : 0, (int)mMonsters.size(), (int)mBullets.size() + (mHero ? (int)mHero->mOrbBullets.size() : 0),
				(mHero && mHero->mbInvincible) ? "[GOD MODE ON]" : "",
				mParallelManager.getLastSolveTime(), solver.settings.iterations);
			g.drawText(20, 20, info);

			if (mHero && mHero->mHP <= 0)
			{
				g.setTextColor(Color3ub(255, 50, 50));
				g.drawText(Vector2(mScreenSize.x * 0.5f - 60, mScreenSize.y * 0.5f - 30), "GAME OVER");
				g.setTextColor(Color3ub(255, 255, 255));
				g.drawText(Vector2(mScreenSize.x * 0.5f - 80, mScreenSize.y * 0.5f + 10), "Press 'R' to Restart");
			}
			else if (mbPaused)
			{
				g.setTextColor(Color3ub(255, 100, 100));
				g.drawText(Vector2(mScreenSize.x * 0.5f - 50, mScreenSize.y * 0.5f - 20), "GAME PAUSED");
			}

			if (mHero)
			{
				g.setTextColor(Color3ub(50, 200, 255));
				InlineString<64> lvStr; lvStr.format("LV. %d", mHero->mLevel);
				g.drawText(20, mScreenSize.y - 55, lvStr);
				float xpW = (float)mHero->mXP / mHero->mNextLevelXP * (mScreenSize.x - 40);
				g.enablePen(false); g.enableBrush(true); g.setBrush(Color3f(0.05f, 0.05f, 0.05f));
				g.drawRect(Vector2(20, mScreenSize.y - 30), Vector2(mScreenSize.x - 40, 12));
				g.setBrush(Color3f(0.2f, 0.7f, 1.0f)); g.drawRect(Vector2(20, mScreenSize.y - 30), Vector2(xpW, 12));
				g.enablePen(true); g.enableBrush(false); g.setPen(Color4f(1, 1, 1, 0.8f), 1);
				g.drawRect(Vector2(20, mScreenSize.y - 30), Vector2(mScreenSize.x - 40, 12));
			}

			g.endRender();
		}
	}

	MsgReply SurvivorsStage::onMouse(MouseMsg const& msg)
	{
		if (mHero)
		{
			if (msg.onLeftDown())
			{
				mVJoystickActive = true;
				mVJoystickCenter = msg.getPos();
				mVJoystickCurrentPos = msg.getPos();
				mHero->mJoystickDir = Vector2::Zero();
			}
			else if (msg.onLeftUp())
			{
				mVJoystickActive = false;
				mHero->mJoystickDir = Vector2::Zero();
			}
			else if (msg.onMoving() && mVJoystickActive && msg.isLeftDown())
			{
				mVJoystickCurrentPos = msg.getPos();
				Vector2 offset = Vector2(msg.getPos()) - mVJoystickCenter;
				float dist = offset.length();
				if (dist > 5.0f)
				{
					float t = std::min(dist / mVJoystickRadius, 1.0f);
					mHero->mJoystickDir = (offset / dist) * t;
				}
				else
				{
					mHero->mJoystickDir = Vector2::Zero();
				}
			}

			if (msg.onWheelFront())
			{
				mCameraZoomTarget *= 1.1f;
				mCameraZoomTarget = std::min(mCameraZoomTarget, 5.0f);
			}
			else if (msg.onWheelBack())
			{
				mCameraZoomTarget *= 0.9f;
				mCameraZoomTarget = std::max(mCameraZoomTarget, 0.2f);
			}

			if (msg.onRightDown())
			{
				Vector2 worldPos = mWorldToScreen.transformInvPosition(Vector2(msg.getPos()));
				mHero->performSlash(worldPos);
			}
			if (msg.onMiddleDown()) mHero->performShockwave();
		}
		return BaseClass::onMouse(msg);
	}

	MsgReply SurvivorsStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::Q: if (mHero) mHero->mEnableOrbs = !mHero->mEnableOrbs; break;
			case EKeyCode::G: if (mHero) mHero->mbInvincible = !mHero->mbInvincible; break;
			case EKeyCode::P: mbPaused = !mbPaused; break;
			case EKeyCode::Space:
				for (int i = 0; i < 100; ++i) spawnMonster();
				break;
			case EKeyCode::R:
				mMonsters.clear();
				mBullets.clear();
				mVisuals.clear();
				if (mHero) mHero->reset();
				mParallelManager.cleanup();
				if (mHero)
				{
					CollisionEntityData data;
					data.position = mHero->mPos;
					data.radius = mHero->mRadius;
					data.mass = 10000.0f;
					data.categoryId = Category_Hero;
					data.bQueryCollision = true;
					data.targetMask = Category_Monster | Category_MonsterBullet;
					mParallelManager.registerEntity(mHero.get(), data);
				}
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

	void SurvivorsStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 720;
		systemConfigs.bUseRenderThread = true;
	}

	StageBase* CreateSurvivorsStage()
	{
		return new SurvivorsStage();
	}

	REGISTER_STAGE_ENTRY("Survivors Stage", SurvivorsStage, EExecGroup::Test, "Physics|Benchmark");
}//namespace Survivors
