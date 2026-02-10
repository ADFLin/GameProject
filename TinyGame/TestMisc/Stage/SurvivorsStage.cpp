
#include "StageBase.h"
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "RandomUtility.h"
#include "ProfileSystem.h"
#include "ParallelCollision/ParallelCollision.h"
#include "Async/AsyncWork.h"
#include "Widget/WidgetUtility.h"
#include "RenderUtility.h"
#include "SystemPlatform.h"
#include "InputManager.h" // Added for InputManager

#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

namespace Survivors
{
	using namespace ParallelCollision;
	using namespace Math;
	using namespace Render;

	enum CollisionCategory
	{
		Category_Monster = 1,
		Category_Hero = 2,
		Category_Bullet = 4,
		Category_MonsterBullet = 8,
		Category_HeroAbility = 16,
		Category_Fence = 32,
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

	class SurvivorsStage;

	class Monster : public IPhysicsEntity
	{
	public:
		Vector2 mPos;
		Vector2 mVel = Vector2::Zero();
		Vector2 mKnockbackVel = Vector2::Zero();
		float mStunTimer = 0;
		float mRadius = 10.0f;
		float mSpeed = 80.0f;
		int mHP = 50;
		bool mIsDead = false;
		SurvivorsStage* mStage = nullptr;

		Monster(Vector2 const& pos, SurvivorsStage* stage) : mPos(pos), mStage(stage) {}

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

	class GameBullet : public IPhysicsBullet
	{
	public:
		Vector2 mPos;
		Vector2 mVel;
		float mRadius = 4.0f;
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

	class Hero : public IPhysicsEntity
	{
	public:
		Vector2 mPos;
		Vector2 mMoveTarget;
		float mRadius = 15.0f;
		int mMaxHP = 1000;
		int mHP = 1000;
		Vector2 mFacing = Vector2(1, 0);
		SurvivorsStage* mStage = nullptr;

		// Skill States
		float mBulletRate = 6.0f; // Actions per sec
		float mBulletTimer = 0;
		bool mEnableOrbs = false;
		float mOrbRotation = 0;
		std::vector<std::unique_ptr<GameBullet>> mOrbBullets;

		float mDamageFlashTimer = 0;
		float mDamageTimer = 0;

		Hero(Vector2 const& pos, SurvivorsStage* stage) : mPos(pos), mMoveTarget(pos), mStage(stage) {}

		void takeDamage(int amount)
		{
			if (mDamageTimer > 0) return;
			mHP -= amount;
			if (mHP < 0) mHP = 0;
			mDamageFlashTimer = 0.2f;
			mDamageTimer = 0.2f; // Reduced invincibility for better feedback
		}

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

	class SurvivorsStage : public StageBase, public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		SurvivorsStage() : mParallelManager(mThreadPool) {}

		Vector2 getHeroPos() const { return mHero->mPos; }
		ParallelCollisionManager& getParallelManager() { return mParallelManager; }
		void addMonster(std::unique_ptr<Monster> m) { mMonsters.push_back(std::move(m)); }
		void addBullet(std::unique_ptr<GameBullet> b) { mBullets.push_back(std::move(b)); }
		void addVisual(VisualEffect const& ve) { mVisuals.push_back(ve); }
		const std::vector<std::unique_ptr<Monster>>& getMonsters() const { return mMonsters; }

		bool onInit() override
		{
			if (!BaseClass::onInit()) return false;

			mScreenSize = ::Global::GetScreenSize();
			mHero = std::make_unique<Hero>(Vector2(mScreenSize.x / 2.0f, mScreenSize.y / 2.0f), this);

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

			::Global::GUI().cleanupWidget();
			DevFrame* frame = WidgetUtility::CreateDevFrame();
			
			// Use FWidgetProperty::Bind or manual binding if WidgetUtility::Bind is unavailable
			// Based on WidgetUtility.h reading, FWidgetProperty::Bind exists, but maybe not WidgetUtility::Bind?
			// Checking WidgetUtility.h again...
			// Oops, they are static members of FWidgetProperty. 
			
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

			return true;
		}

		void onEnd() override
		{
			if (mHero) mHero->mOrbBullets.clear();
			mParallelManager.cleanup();
			mMonsters.clear();
			mBullets.clear();
			mThreadPool.waitAllWorkComplete();
			BaseClass::onEnd();
		}

		void spawnMonster()
		{
			Vector2 pos;
			float side = RandFloat(0, 4);
			float margin = 150.0f;
			if (side < 1) pos = Vector2(RandFloat(0, mScreenSize.x), -margin);
			else if (side < 2) pos = Vector2(RandFloat(0, mScreenSize.x), mScreenSize.y + margin);
			else if (side < 3) pos = Vector2(-margin, RandFloat(0, mScreenSize.y));
			else pos = Vector2(mScreenSize.x + margin, RandFloat(0, mScreenSize.y));

			auto monster = std::make_unique<Monster>(pos, this);
			CollisionEntityData data;
			data.position = pos;
			data.velocity = Vector2::Zero();
			data.isStatic = false;
			data.bQueryCollision = true;
			data.categoryId = Category_Monster;
			data.targetMask = Category_Hero | Category_HeroAbility | Category_Bullet;

			if (RandFloat(0, 1) < 0.01f) // 1% chance for Large Monster
			{
				monster->mRadius = 30.0f;
				monster->mHP = 500;
				data.mass = 20.0f;
			}
			else
			{
				data.mass = 1.0f;
			}
			
			data.radius = monster->mRadius;
			mParallelManager.registerEntity(monster.get(), data);
			mMonsters.push_back(std::move(monster));
		}

		void monsterFire()
		{
			for (auto& m : mMonsters)
			{
				if (RandFloat(0, 1) < 0.02f)
				{
					Vector2 dir = mHero->mPos - m->mPos;
					dir.normalize();
					auto bullet = std::make_unique<GameBullet>(m->mPos, dir * 200.0f, Category_MonsterBullet);
					CollisionBulletData bdata;
					bdata.position = bullet->mPos;
					bdata.velocity = bullet->mVel;
					bdata.categoryId = Category_MonsterBullet;
					bdata.targetMask = Category_Hero | Category_Bullet;
					bdata.bQueryBulletCollision = true;
					bdata.shapeType = ShapeType::Circle;
					bdata.shapeParams = Vector3(6.0f, 0, 0);
					mParallelManager.registerBullet(bullet.get(), bdata);
					mBullets.push_back(std::move(bullet));
				}
			}
		}

		void onUpdate(GameTimeSpan deltaTime) override
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

			mSpawnTimer += dt;
			if (mSpawnRate > 0 && mSpawnTimer > 1.0f / mSpawnRate) { mSpawnTimer = 0; spawnMonster(); }

			mMonsterFireTimer += dt;
			if (mMonsterFireRate > 0 && mMonsterFireTimer > 1.0f / mMonsterFireRate) { mMonsterFireTimer = 0; monsterFire(); }

			Vector2 heroPos = mHero ? mHero->mPos : Vector2::Zero();

			// Update Logic
			if (mHero) mHero->update(dt);
			for (auto& m : mMonsters) m->update(heroPos, dt);
			for (auto& b : mBullets) b->update(dt);

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
				auto& data = mParallelManager.getBulletData(b->mBulletId);
				data.position = b->mPos;
				data.velocity = b->mVel;
				if (b->mVel.length2() > 0.001f)
                    data.rotation = std::atan2(b->mVel.y, b->mVel.x);
			}

			// Clean up
			mMonsters.erase(std::remove_if(mMonsters.begin(), mMonsters.end(), [this](auto& m) {
				if (m->mIsDead) { mParallelManager.unregisterEntity(m.get()); return true; }
				return false;
			}), mMonsters.end());

			mBullets.erase(std::remove_if(mBullets.begin(), mBullets.end(), [this](auto& b) {
				if (b->mIsDead) { mParallelManager.unregisterBullet(b.get()); return true; }
				return false;
			}), mBullets.end());

			mParallelManager.endFrame(dt);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			RHICommandList& commandList = g.getCommandList(); // Render namespace needed
			RHISetViewport(commandList, 0, 0, (float)mScreenSize.x, (float)mScreenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.04f, 0.04f, 0.08f, 1), 1);

			g.beginRender();
			
			if (mHero)
			{
				if (mHero->mDamageFlashTimer > 0)
					RenderUtility::SetBrush(g, EColor::Red);
				else
					RenderUtility::SetBrush(g, EColor::Green);

				RenderUtility::SetPen(g, EColor::White);
				g.drawCircle(mHero->mPos, mHero->mRadius);

				// Draw Health Bar
				{
					float barWidth = 60.0f;
					float barHeight = 6.0f;
					Vector2 barPos = mHero->mPos + Vector2(-barWidth * 0.5f, mHero->mRadius + 10.0f);
					
					// Border
					g.setPen(Color4f(0,0,0,1), 1);
					g.setBrush(Color4f(0,0,0,1));
					g.drawRect(barPos - Vector2(1, 1), Vector2(barWidth + 2, barHeight + 2));

					// Background
					g.enablePen(false);
					g.setBrush(Color3f(0.3f, 0.1f, 0.1f));
					g.drawRect(barPos, Vector2(barWidth, barHeight));

					// Fill
					float hpRange = (float)mHero->mHP / mHero->mMaxHP;
					g.setBrush(Color3f(1.0f - hpRange, hpRange, 0.0f));
					g.drawRect(barPos, Vector2(barWidth * hpRange, barHeight));
					g.enablePen(true);
				}

				RenderUtility::SetBrush(g, EColor::Cyan);
				for (auto& b : mHero->mOrbBullets) g.drawCircle(b->mPos, b->mRadius);
			}


			RenderUtility::SetPen(g, EColor::Black);
			for (auto& m : mMonsters)
			{
				if (m->mPos.x < -m->mRadius || m->mPos.x > mScreenSize.x + m->mRadius ||
					m->mPos.y < -m->mRadius || m->mPos.y > mScreenSize.y + m->mRadius)
					continue;

				if (m->mStunTimer > 0) RenderUtility::SetBrush(g, EColor::White);
				else RenderUtility::SetBrush(g, EColor::Red);
				g.drawCircle(m->mPos, m->mRadius);
			}

			for (auto& b : mBullets)
			{
				if (b->mPos.x < -b->mRadius || b->mPos.x > mScreenSize.x + b->mRadius ||
					b->mPos.y < -b->mRadius || b->mPos.y > mScreenSize.y + b->mRadius)
					continue;

				if (b->mCategory == Category_Bullet) { g.setBrush(Color3f(1, 1, 0)); g.setPen(Color3f(1, 1, 0)); }
				else { g.setBrush(Color3f(1, 0.5f, 0)); g.setPen(Color3f(1, 0.5f, 0)); }
				g.drawCircle(b->mPos, b->mRadius);
			}
            
			for (auto& v : mVisuals)
			{
				float alpha = v.timer / v.duration;
				if (v.type == VisualType::Slash)
				{
					g.setPen(Color3f(1, 0.9f, 0.2f), 4);
					g.drawArcLine(v.pos, v.radius, v.rotation - 1.0f, 2.0f);
				}
				else if (v.type == VisualType::Shockwave)
				{
					float curR = v.radius * (1.0f - alpha);
					g.setPen(Color3f(0.2f, 0.8f, 1.0f), 5);
					g.drawCircle(v.pos, curR);
				}
				else if (v.type == VisualType::HitSpark)
				{
					g.setPen(Color3f(1, 1, 1), 1);
					g.setBrush(Color3f(1, 1, 1));
					g.drawCircle(v.pos, 5.0f * alpha);
				}
			}

			// Debug Drawing
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
				for (int i = 0; i < solver.mEntityCount; ++i)
				{
					g.drawCircle(solver.positions[i], solver.radii[i]);
				}
			}

			if (mShowBullets)
			{
				g.setPen(Color3f(1, 1, 0), 1);
				g.enableBrush(false);
				for (int i = 0; i < solver.mBulletCount; ++i)
				{
					Vector2 pos = solver.bulletPositions[i];
					ShapeType type = solver.bulletShapeTypes[i];
					Vector3 params = solver.bulletShapeParams[i];
					float rot = 0;
					if (solver.bulletVelocities[i].length2() > 0.001f)
						rot = std::atan2(solver.bulletVelocities[i].y, solver.bulletVelocities[i].x);

					switch (type)
					{
					case ShapeType::Circle:
						g.drawCircle(pos, params.x);
						break;
					case ShapeType::Rect:
						{
							Vector2 halfSize = Vector2(params.x, params.y) * 0.5f;
							Vector2 corners[4] = {
								Vector2(-halfSize.x, -halfSize.y),
								Vector2(halfSize.x, -halfSize.y),
								Vector2(halfSize.x, halfSize.y),
								Vector2(-halfSize.x, halfSize.y)
							};
							float c = std::cos(rot), s = std::sin(rot);
							for (int k = 0; k < 4; ++k)
							{
								float nx = corners[k].x * c - corners[k].y * s;
								float ny = corners[k].x * s + corners[k].y * c;
								corners[k] = pos + Vector2(nx, ny);
							}
							g.drawPolygon(corners, 4);
						}
						break;
					case ShapeType::Arc:
						g.drawArcLine(pos, params.x, rot - params.y * 0.5f, params.y);
						break;
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
							{
								g.drawLine(posA, mParallelManager.mEntityDataBuffer[other->mPhysicsId].position);
							}
						}
						for (int bHandle : rec.eventData->prevBullets)
						{
							IPhysicsBullet* bullet = mParallelManager.getBulletFromHandle(bHandle);
							if (bullet && bullet->mBulletId >= 0 && bullet->mBulletId < mParallelManager.mBulletCount)
							{
								g.drawLine(posA, mParallelManager.mBulletDataBuffer[bullet->mBulletId].position);
							}
						}
					}
				}
				for (int id : mParallelManager.mLastActiveBulletIndices)
				{
					if (id < 0 || id >= mParallelManager.mBulletCount) continue;
					auto const& rec = mParallelManager.mBulletReceivers[id];
					Vector2 posA = mParallelManager.mBulletDataBuffer[id].position;
					if (rec.eventData)
					{
						for (int otherBHandle : rec.eventData->prevBullets)
						{
							IPhysicsBullet* bullet = mParallelManager.getBulletFromHandle(otherBHandle);
							if (bullet && bullet->mBulletId >= 0 && bullet->mBulletId < mParallelManager.mBulletCount)
							{
								g.drawLine(posA, mParallelManager.mBulletDataBuffer[bullet->mBulletId].position);
							}
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
					g.enableBrush(true);
					g.enablePen(false);
					g.beginBlend(0.15f);
					g.drawRect(wall.min, wall.max - wall.min);
					g.endBlend();
					g.enableBrush(false);
					g.enablePen(true);
					g.drawRect(wall.min, wall.max - wall.min);
				}
				g.enableBrush(true);
				g.enablePen(true);
			}


			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<512> info;
			info.format("Hero HP: %d | Monsters: %d | Bullets: %d\nCollision: %.3f ms | Iters: %d\n[WASD] Move | [Q] Orbs | [R-Click] Slash | [M-Click] Shockwave\n[R] Reset Simulation", 
				mHero ? mHero->mHP : 0, (int)mMonsters.size(), (int)mBullets.size() + (mHero ? (int)mHero->mOrbBullets.size() : 0), 
				mParallelManager.getLastSolveTime(), mParallelManager.getSolver().settings.iterations);
			g.drawText(20, 20, info);

			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (mHero)
			{
				if (msg.onMoving() || msg.onLeftDown()) mHero->mMoveTarget = msg.getPos();
				if (msg.onRightDown()) mHero->performSlash(msg.getPos());
				if (msg.onMiddleDown()) mHero->performShockwave();
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::Q: if (mHero) mHero->mEnableOrbs = !mHero->mEnableOrbs; break;
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

		ERenderSystem getDefaultRenderSystem() override { return ERenderSystem::None; }
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) 
		{ 
			systemConfigs.screenWidth = 1280; systemConfigs.screenHeight = 720; 
		}

	private:
		QueueThreadPool mThreadPool;
		ParallelCollisionManager mParallelManager;
		std::unique_ptr<Hero> mHero;
		std::vector<std::unique_ptr<Monster>> mMonsters;
		std::vector<std::unique_ptr<GameBullet>> mBullets;
		std::vector<VisualEffect> mVisuals;

		Vec2i mScreenSize;
		float mSpawnRate = 5.0f; // Actions per sec
		float mSpawnTimer = 0;
		float mMonsterFireRate = 0.5f; // Actions per sec
		float mMonsterFireTimer = 0;

		bool mShowCells = false;
		bool mShowEntities = false;
		bool mShowBullets = false;
		bool mShowEvents = false;
		bool mShowWalls = false;
	};

	// Implement Monster methods after SurvivorsStage definition
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
			int dmg = (category == Category_HeroAbility) ? 10 : 2;
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

	// Hero Implementations
	void Hero::update(float dt)
	{
		// Movement
		Vector2 moveDir = Vector2::Zero();
		if (InputManager::Get().isKeyDown(EKeyCode::W)) moveDir.y -= 1;
		if (InputManager::Get().isKeyDown(EKeyCode::S)) moveDir.y += 1;
		if (InputManager::Get().isKeyDown(EKeyCode::A)) moveDir.x -= 1;
		if (InputManager::Get().isKeyDown(EKeyCode::D)) moveDir.x += 1;
		if (moveDir.length2() > 0.1f)
		{
			moveDir.normalize();
			mFacing = moveDir;
			mPos += moveDir * 300.0f * dt;
			mMoveTarget = mPos;
		}
		else
		{
			Vector2 dir = mMoveTarget - mPos;
			float dist = dir.length();
			if (dist > 5.0f)
			{
				mFacing = dir / dist;
				mPos += mFacing * 300.0f * dt;
			}
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
			for (int i = 0; i < 4; ++i)
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
		for (int i = 0; i < 4; ++i)
		{
			float angle = mOrbRotation + i * 90.0f;
			float rad = angle * Math::PI / 180.0f;
			Vector2 offset(Math::Cos(rad), Math::Sin(rad));
			mOrbBullets[i]->mPos = mPos + offset * 100.0f;
			auto& data = mStage->getParallelManager().getBulletData(mOrbBullets[i]->mBulletId);
			data.position = mOrbBullets[i]->mPos;
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
			CollisionBulletData bdata;
			bdata.position = bullet->mPos;
			bdata.velocity = bullet->mVel;
			bdata.categoryId = Category_Bullet;
			bdata.targetMask = Category_Monster | Category_MonsterBullet;
			bdata.bQueryBulletCollision = true;
			bdata.shapeType = ShapeType::Circle;
			bdata.shapeParams = Vector3(bullet->mRadius, 0, 0);
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

	REGISTER_STAGE_ENTRY("Survivors Stage", SurvivorsStage, EExecGroup::Test, "Physics|Benchmark");
}
