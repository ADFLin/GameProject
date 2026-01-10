
#include "StageBase.h"
#include "GameRenderSetup.h"

#include "Renderer/RenderTransform2D.h"
#include "CarTrain/GameFramework.h"
#include "Collision/SimpleCollision.h"
#include "RHI/RHIGraphics2D.h"
#include "RandomUtility.h"
#include "ProfileSystem.h"

#include <chrono>

namespace CollisionBenchmark
{
	using namespace Render;
	using namespace CarTrain;

	// ============================================
	// Monster Entity - Used for Custom Collision System
	// ============================================
	class Monster
	{
	public:
		Vector2 mPos;
		Vector2 mVelocity;
		float mRadius = 8.0f;
		float mSpeed = 100.0f;

		Monster(Vector2 const& pos, float radius = 8.0f)
			: mPos(pos), mRadius(radius)
		{
		}
		\
		float getMass() const { return 1.0; }
		bool isStatic() const { return false; }
		// Non-virtual functions (Direct Call)
		Vector2 getPosition() const { return mPos; }
		void setPosition(Vector2 const& pos) { mPos = pos; }

		float getRadius() const { return mRadius; }


		int getSpatialIndex() { return mIndex; }
		void setSpatialIndex(int index) { mIndex = index; }

		int mIndex;
		void moveToward(Vector2 const& target, float deltaTime)
		{
			Vector2 dir = target - mPos;
			float dist = dir.length();
			if (dist > 1.0f)
			{
				dir /= dist;
				mPos += dir * mSpeed * deltaTime;
			}
		}
	};

	// ============================================
	// Box2D Monster - Uses CarTrain Physics System
	// ============================================
	class Box2DMonster
	{
	public:
		IPhysicsBody* mBody = nullptr;
		float mSpeed = 100.0f;

		Box2DMonster(IPhysicsScene* scene, Vector2 const& pos, float radius = 8.0f)
		{
			CircleObjectDef def;
			def.radius = radius;
			def.bEnablePhysics = true;
			def.bCollisionDetection = true;
			def.bCollisionResponse = true;
			def.mass = 1.0f;
			def.restitution = 0.0f;
			def.friction = 0.5f;

			mBody = scene->createCircle(def, XForm2D(pos));
		}

		Vector2 getPosition() const
		{
			return mBody->getTransform().getPos();
		}

		float getRadius() const
		{
			auto* def = static_cast<CircleObjectDef*>(mBody->getDefine());
			return def ? def->radius : 8.0f;
		}

		void moveToward(Vector2 const& target, float deltaTime)
		{
			Vector2 pos = getPosition();
			Vector2 dir = target - pos;
			float dist = dir.length();
			if (dist > 1.0f)
			{
				dir /= dist;
				mBody->setLinearVel(dir * mSpeed);
			}
			else
			{
				mBody->setLinearVel(Vector2::Zero());
			}
		}
	};

	// ============================================
	// Benchmark Stage
	// ============================================
	class BenchmarkStage : public StageBase, public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		BenchmarkStage() {}

		// Custom Collision System (Left Side)
		SimpleCollision::SpatialHashSystem<Monster> mSpatialHash;
		TArray<std::unique_ptr<Monster>> mLeftMonsters;
		Vector2 mLeftTarget;
		float mLeftCollisionTime = 0.0f;

		// Box2D Physics System (Right Side)
		GameWorld mBox2DWorld;
		TArray<std::unique_ptr<Box2DMonster>> mRightMonsters;
		Vector2 mRightTarget;
		float mRightCollisionTime = 0.0f;

		// Screen settings
		Vec2i mScreenSize;
		int mHalfWidth;
		float mWorldScale = 1.0f;

		// Statistics
		float mAccumulatedLeftTime = 0.0f;
		float mAccumulatedRightTime = 0.0f;
		int mFrameCount = 0;
		float mAvgLeftTime = 0.0f;
		float mAvgRightTime = 0.0f;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			mScreenSize = ::Global::GetScreenSize();
			mHalfWidth = mScreenSize.x / 2;

			// Initialize Left Side Target
			mLeftTarget = Vector2(mHalfWidth / 2.0f, mScreenSize.y / 2.0f);
				// Initialize Right Side Target (Mirrored)
			mRightTarget = Vector2(mHalfWidth + mHalfWidth / 2.0f, mScreenSize.y / 2.0f);

			// Initialize Box2D World
			mBox2DWorld.initialize();
			mBox2DWorld.getPhysicsScene()->setGravity(Vector2(0, 0)); // No gravity

			// Set Spatial Hash Cell Size and Bounds
			mSpatialHash.setCellSize(32.0f);
			// Since there are no walls now, set a large enough boundary to cover expansion
			// Left side area was 0 to mHalfWidth, expanded to allow monsters to be pushed out
			mSpatialHash.setBounds(Vector2(-2000, -2000), Vector2(4000, 4000));

			// Configure Collision Resolution Parameters (Strong Stability Mode)
			SimpleCollision::CollisionSettings settings;
			settings.relaxation = 0.8f;       // Increase force to resolve overlap (Stable with high value since Shuffle is off)
			settings.maxPushDistance = 100.0f;
			settings.minSeparation = 0.02f;
			settings.enableMassRatio = true;
			settings.shufflePairs = false;    // Keep false to prevent jitter
			mSpatialHash.setSettings(settings);

			// Create Wall Boundaries (Right Side Box2D)
			// createBox2DWalls(); // Removed walls

			::Global::GUI().cleanupWidget();

			return true;
		}

		void createBox2DWalls()
		{
			BoxObjectDef wallDef;
			wallDef.bEnablePhysics = false;
			wallDef.bCollisionDetection = true;
			wallDef.bCollisionResponse = true;

			float wallThickness = 10.0f;
			float halfHeight = mScreenSize.y / 2.0f;
			float quarterWidth = mHalfWidth / 2.0f;

			// Top Wall
			wallDef.extend = Vector2(mHalfWidth, wallThickness);
			mBox2DWorld.getPhysicsScene()->createBox(wallDef, 
				XForm2D(Vector2(mHalfWidth + quarterWidth, wallThickness / 2)));

			// Bottom Wall
			mBox2DWorld.getPhysicsScene()->createBox(wallDef, 
				XForm2D(Vector2(mHalfWidth + quarterWidth, mScreenSize.y - wallThickness / 2)));

			// Left Wall (Middle Divider)
			wallDef.extend = Vector2(wallThickness, mScreenSize.y);
			mBox2DWorld.getPhysicsScene()->createBox(wallDef, 
				XForm2D(Vector2(mHalfWidth + wallThickness / 2, halfHeight)));

			// Right Wall
			mBox2DWorld.getPhysicsScene()->createBox(wallDef, 
				XForm2D(Vector2(mScreenSize.x - wallThickness / 2, halfHeight)));
		}

		void onEnd() override
		{
			mLeftMonsters.clear();
			mRightMonsters.clear();
			mBox2DWorld.clearnup();
			BaseClass::onEnd();
		}

		void spawnMonster(Vector2 const& pos, bool isLeftSide)
		{
			float radius = RandFloat(6.0f, 12.0f);

			if (isLeftSide)
			{
				// Left Side: Use Custom Collision
				auto monster = std::make_unique<Monster>(pos, radius);
				mLeftMonsters.push_back(std::move(monster));
			}
			else
			{
				// Right Side: Use Box2D
				auto monster = std::make_unique<Box2DMonster>(
					mBox2DWorld.getPhysicsScene(), pos, radius);
				mRightMonsters.push_back(std::move(monster));
			}
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			float dt = deltaTime.value;

			// ========== Left Side: Custom Collision System ==========
			{
				auto start = std::chrono::high_resolution_clock::now();

				// Move Monsters
				for (auto& monster : mLeftMonsters)
				{
					monster->moveToward(mLeftTarget, dt);
				}

				// Spatial Hash Collision Resolution
				mSpatialHash.clear();
				for (auto& monster : mLeftMonsters)
				{
					if (monster)
					    mSpatialHash.insert(monster.get());
				}
				mSpatialHash.resolveCollisions(32); // High iterations for stability without sub-stepping

				auto end = std::chrono::high_resolution_clock::now();
				mLeftCollisionTime = std::chrono::duration<float, std::milli>(end - start).count();
				
				// Exponential Moving Average (Lerp alpha = 0.9)
				mAvgLeftTime = mAvgLeftTime * 0.9f + mLeftCollisionTime * 0.1f;
			}


			// ========== Right Side: Box2D Physics System ==========
			{
				auto start = std::chrono::high_resolution_clock::now();

				// Move Monsters (By setting velocity)
				for (auto& monster : mRightMonsters)
				{
					monster->moveToward(mRightTarget, dt);
				}

				// Box2D Physics Update
				mBox2DWorld.getPhysicsScene()->tick(dt);

				auto end = std::chrono::high_resolution_clock::now();
				mRightCollisionTime = std::chrono::duration<float, std::milli>(end - start).count();
				
				// Exponential Moving Average (Lerp alpha = 0.9)
				mAvgRightTime = mAvgRightTime * 0.9f + mRightCollisionTime * 0.1f;
			}

		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			RHICommandList& commandList = g.getCommandList();
			RHISetViewport(commandList, 0, 0, mScreenSize.x, mScreenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.1f, 0.1f, 0.1f, 1), 1);

			g.beginRender();

			// Draw Middle Divider
			RenderUtility::SetPen(g, EColor::White);
			g.drawLine(Vector2(mHalfWidth, 0), Vector2(mHalfWidth, mScreenSize.y));

			// ========== Left Side: Custom Collision ==========
			RenderUtility::SetBrush(g, EColor::Blue);
			RenderUtility::SetPen(g, EColor::Cyan);
			for (auto& monster : mLeftMonsters)
			{
				g.drawCircle(monster->mPos, monster->mRadius);
			}

			// Draw Left Target
			RenderUtility::SetBrush(g, EColor::Yellow);
			g.drawCircle(mLeftTarget, 5.0f);

			// ========== Right Side: Box2D ==========
			RenderUtility::SetBrush(g, EColor::Red);
			RenderUtility::SetPen(g, EColor::Orange);
			for (auto& monster : mRightMonsters)
			{
				g.drawCircle(monster->getPosition(), monster->getRadius());
			}

			// Draw Right Target
			RenderUtility::SetBrush(g, EColor::Yellow);
			g.drawCircle(mRightTarget, 5.0f);

			// ========== Display Stats ==========
			g.setTextColor(Color3ub(255, 255, 255));

			InlineString<256> leftInfo;
			leftInfo.format("Spatial Hash\nMonsters: %d\nFrame: %.3f ms\nAvg: %.3f ms",
				(int)mLeftMonsters.size(), mLeftCollisionTime, mAvgLeftTime);
			g.drawText(10, 10, leftInfo);

			InlineString<256> rightInfo;
			rightInfo.format("Box2D\nMonsters: %d\nFrame: %.3f ms\nAvg: %.3f ms",
				(int)mRightMonsters.size(), mRightCollisionTime, mAvgRightTime);
			g.drawText(mHalfWidth + 10, 10, rightInfo);

			// Performance Comparison
			if (mAvgLeftTime > 0 && mAvgRightTime > 0)
			{
				float speedup = mAvgRightTime / mAvgLeftTime;
				InlineString<128> comparison;
				comparison.format("Speedup: %.1fx faster", speedup);
				g.drawText(mScreenSize.x / 2 - 60, mScreenSize.y - 30, comparison);
			}

			// Controls Instructions
			g.drawText(10, mScreenSize.y - 50, "Left Click: Spawn monsters | R: Reset | Space: Spawn 100");

			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			Vector2 mousePos = msg.getPos();

			if (msg.onMoving())
			{
				// Update Target Position
				if (mousePos.x < mHalfWidth)
				{
					// Mouse on Left Side
					mLeftTarget = mousePos;
					// Mirror to Right Side
					mRightTarget = Vector2(mHalfWidth + mousePos.x, mousePos.y);
				}
				else
				{
					// Mouse on Right Side
					mRightTarget = mousePos;
					// Mirror to Left Side
					mLeftTarget = Vector2(mousePos.x - mHalfWidth, mousePos.y);
				}
			}

			if (msg.onLeftDown())
			{
				// Spawn Monster on Click
				if (mousePos.x < mHalfWidth)
				{
					// Click Left: Spawn Left, Mirror Right
					spawnMonster(mousePos, true);
					Vector2 mirrorPos(mHalfWidth + mousePos.x, mousePos.y);
					spawnMonster(mirrorPos, false);
				}
				else
				{
					// Click Right: Spawn Right, Mirror Left
					spawnMonster(mousePos, false);
					Vector2 mirrorPos(mousePos.x - mHalfWidth, mousePos.y);
					spawnMonster(mirrorPos, true);
				}
			}

			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R:
					// Reset
					mLeftMonsters.clear();
					mRightMonsters.clear();
					mAccumulatedLeftTime = 0;
					mAccumulatedRightTime = 0;
					mFrameCount = 0;
					mAvgLeftTime = 0;
					mAvgRightTime = 0;
					break;

				case EKeyCode::Space:
					// Batch Spawn 100 Monsters
					for (int i = 0; i < 100; ++i)
					{
						Vector2 leftPos(
							RandFloat(20, mHalfWidth - 20),
							RandFloat(20, mScreenSize.y - 20)
						);
						spawnMonster(leftPos, true);

						Vector2 rightPos(
							RandFloat(mHalfWidth + 20, mScreenSize.x - 20),
							RandFloat(20, mScreenSize.y - 20)
						);
						spawnMonster(rightPos, false);
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			return BaseClass::onWidgetEvent(event, id, ui);
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1600;
			systemConfigs.screenHeight = 800;
		}

	};

	REGISTER_STAGE_ENTRY("Collision Benchmark", BenchmarkStage, EExecGroup::Test, "Physics|Benchmark");

} // namespace CollisionBenchmark
