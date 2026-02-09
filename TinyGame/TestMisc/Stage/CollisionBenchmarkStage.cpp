
#include "StageBase.h"
#include "GameRenderSetup.h"

#include "Renderer/RenderTransform2D.h"
#include "CarTrain/GameFramework.h"
#include "Collision/SimpleCollision.h"
#include "RHI/RHIGraphics2D.h"
#include "RandomUtility.h"
#include "ProfileSystem.h"

#include <chrono>
#include "Async/AsyncWork.h"
#include "Widget/WidgetUtility.h"

#include "ParallelCollision/ParallelCollision.h"

namespace CollisionBenchmark
{
	using namespace Render;
	using namespace CarTrain;

	class Monster : public ParallelCollision::IPhysicsEntity
	{
	public:
		Vector2 mPos;
		float mRadius = 8.0f;
		float mSpeed = 100.0f;
		float mMass = 1.0f;

		Monster(Vector2 const& pos, float radius = 8.0f)
			: mPos(pos), mRadius(radius)
		{
		}
		
		float getMass() const { return mMass; }
		bool isStatic() const { return false; }
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
				mPos += (dir / dist) * mSpeed * deltaTime;
			}
		}

		Vector2 getMoveVelocity(Vector2 const& target)
		{
			Vector2 dir = target - mPos;
			float dist = dir.length();
			if (dist > 1.0f) return (dir / dist) * mSpeed;
			return Vector2::Zero();
		}

		// ParallelCollision::IPhysicsEntity implementation
		void handleCollisionEnter(ParallelCollision::IPhysicsEntity* other, int category) override {}
		void handleCollisionStay(ParallelCollision::IPhysicsEntity* other, int category) override {}
		void handleCollisionExit(ParallelCollision::IPhysicsEntity* other, int category) override {}

		void handleCollisionEnter(ParallelCollision::IPhysicsBullet* bullet, int category) override {}
		void handleCollisionStay(ParallelCollision::IPhysicsBullet* bullet, int category) override {}
		void handleCollisionExit(ParallelCollision::IPhysicsBullet* bullet, int category) override {}
	};

	class Box2DMonster
	{
	public:
		IPhysicsBody* mBody = nullptr;
		float mSpeed = 100.0f;

		Box2DMonster(IPhysicsScene* scene, Vector2 const& pos, float radius = 8.0f, bool bHero = false)
		{
			CircleObjectDef def;
			def.radius = radius;
			def.bEnablePhysics = !bHero;
			def.bCollisionDetection = true;
			def.bCollisionResponse = true;
			def.mass = 1.0f;
			mBody = scene->createCircle(def, XForm2D(pos));
		}

		Vector2 getPosition() const { return mBody->getTransform().getPos(); }
		float getRadius() const { return static_cast<CircleObjectDef*>(mBody->getDefine())->radius; }

		void moveToward(Vector2 const& target, float deltaTime)
		{
			Vector2 dir = target - getPosition();
			float dist = dir.length();
			if (dist > 1.0f) mBody->setLinearVel((dir / dist) * mSpeed);
			else mBody->setLinearVel(Vector2::Zero());
		}
	};

	class BenchmarkStage : public StageBase, public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		BenchmarkStage() : mParallelManager(mThreadPool) {}

		SimpleCollision::SpatialHashSystem<Monster> mSpatialHash;
		TArray<std::unique_ptr<Monster>> mLeftMonsters;
		Vector2 mLeftTarget;

		GameWorld mBox2DWorld;
		TArray<std::unique_ptr<Box2DMonster>> mRightMonsters;
		Vector2 mRightTarget;
		std::unique_ptr<Box2DMonster> mBox2DHero;

		std::unique_ptr<Monster> mHero;
		bool mUseParallel = false;
		bool mUseNewParallel = false;
		bool mEnableHero = true;
		bool mEnableBox2D = true;
		bool mEnableSubStep = false;
		QueueThreadPool mThreadPool;
		ParallelCollision::ParallelCollisionManager mParallelManager;

		Vec2i mScreenSize;
		int mHalfWidth;

		float mLeftCollisionTime = 0.0f;
		float mAvgLeftTime = 0.0f;
		float mRightCollisionTime = 0.0f;
		float mAvgRightTime = 0.0f;

		int iterations = 8;
		bool onInit() override
		{
			if (!BaseClass::onInit()) 
				return false;

			mScreenSize = ::Global::GetScreenSize();
			mHalfWidth = mScreenSize.x / 2;
			mLeftTarget = Vector2(mHalfWidth / 2.0f, mScreenSize.y / 2.0f);
			mRightTarget = Vector2(mHalfWidth + mHalfWidth / 2.0f, mScreenSize.y / 2.0f);

			mBox2DWorld.initialize();
			mBox2DWorld.getPhysicsScene()->setGravity(Vector2(0, 0));

			mParallelManager.init(Vector2(-2000, -2000), Vector2(4000, 4000), 32.0f);

			// Restore to stable CellSize
			mSpatialHash.setCellSize(32.0f);
			mSpatialHash.setBounds(Vector2(-2000, -2000), Vector2(4000, 4000));

			SimpleCollision::CollisionSettings settings;
			settings.relaxation = 1.0f; // Integrated over-push into this value
			settings.maxPushDistance = 10.0f; // Allow larger pushes to resolve deep overlaps
			settings.minSeparation = 0.01f;
			settings.enableMassRatio = true;
			settings.shufflePairs = false; 
			mSpatialHash.setSettings(settings);

			mThreadPool.init(2);

			::Global::GUI().cleanupWidget();
			DevFrame* frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Use Parallel (Simple)", mUseParallel);
			frame->addCheckBox("Use New Parallel", mUseNewParallel);
			frame->addCheckBox("Enable Hero", mEnableHero);
			frame->addCheckBox("Enable Box2D", mEnableBox2D);
			frame->addCheckBox("Enable SubStep", mEnableSubStep);

			mHero = std::make_unique<Monster>(mLeftTarget, 20.0f);
			mHero->mMass = 10000.0f;
			{
				ParallelCollision::CollisionEntityData data;
				data.position = mLeftTarget;
				data.velocity = Vector2::Zero();
				data.radius = mHero->getRadius();
				data.mass = mHero->mMass;
				data.isStatic = true;
				data.bQueryCollision = true;
				mParallelManager.registerEntity(mHero.get(), data);
			}

			mBox2DHero = std::make_unique<Box2DMonster>(mBox2DWorld.getPhysicsScene(), mRightTarget, 20.0f, true);

			return true;
		}

		void onEnd() override
		{
			mLeftMonsters.clear(); mRightMonsters.clear();
			mBox2DWorld.clearnup(); BaseClass::onEnd();
		}

		void spawnMonster(Vector2 const& pos, bool isLeftSide)
		{
			float radius = RandFloat(6.0f, 12.0f);
			if (isLeftSide)
			{
				auto monster = std::make_unique<Monster>(pos, radius);
				ParallelCollision::CollisionEntityData data;
				data.position = pos;
				data.velocity = Vector2::Zero();
				data.radius = radius;
				data.mass = monster->mMass;
				data.isStatic = false;
				data.bQueryCollision = false;
				mParallelManager.registerEntity(monster.get(), data);
				mLeftMonsters.push_back(std::move(monster));
			}
			else mRightMonsters.push_back(std::make_unique<Box2DMonster>(mBox2DWorld.getPhysicsScene(), pos, radius));
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			// Clamp DT to prevent oscillation when frame rate drops (due to Box2D etc.)
			float dt = std::min(deltaTime.value, 0.01666f); 

			// ========== Left Side Update (Custom) ==========
			{
				PROFILE_ENTRY("Custom Physics World Tick");

				auto start = std::chrono::high_resolution_clock::now();
				
				if (mUseNewParallel)
				{
					mParallelManager.beginFrame();
					// Update Hero in Parallel
					if (mEnableHero)
					{
						mHero->setPosition(mLeftTarget);
						auto& data = mParallelManager.getEntityData(mHero->mPhysicsId);
						data.position = mLeftTarget;
						data.isStatic = true;
					}
					else
					{
						auto& data = mParallelManager.getEntityData(mHero->mPhysicsId);
						data.position = Vector2(-10000, -10000);
						data.isStatic = true;
					}

					for (auto& monster : mLeftMonsters)
					{
						// Sync position back for logic/rendering
						monster->mPos = mParallelManager.getEntityData(monster->mPhysicsId).position;
						// Update velocity for next step
						mParallelManager.getEntityData(monster->mPhysicsId).velocity = monster->getMoveVelocity(mLeftTarget);
					}

					mParallelManager.getSolver().settings.iterations = iterations;
					mParallelManager.getSolver().settings.relaxation = 1.0f;
					mParallelManager.endFrame(dt);
				}
				else
				{
					for (auto& monster : mLeftMonsters) monster->moveToward(mLeftTarget, dt);

					mSpatialHash.clear();
					if (mEnableHero) { mHero->setPosition(mLeftTarget); mSpatialHash.insert(mHero.get()); }
					for (auto& monster : mLeftMonsters) mSpatialHash.insert(monster.get());

					int subSteps = mEnableSubStep ? 4 : 1;
					if (mUseParallel) mSpatialHash.parallelResolveCollisions(mThreadPool, iterations, subSteps);
					else mSpatialHash.resolveCollisions(32, subSteps);
				}

				auto end = std::chrono::high_resolution_clock::now();
				mLeftCollisionTime = std::chrono::duration<float, std::milli>(end - start).count();
				mAvgLeftTime = mAvgLeftTime * 0.9f + mLeftCollisionTime * 0.1f;
			}

			// ========== Right Side Update (Box2D) ==========
			if (mEnableBox2D)
			{
				PROFILE_ENTRY("Box2D World Tick");
				auto start = std::chrono::high_resolution_clock::now();
				for (auto& monster : mRightMonsters) monster->moveToward(mRightTarget, dt);

				if (mBox2DHero)
				{
					if (mEnableHero) mBox2DHero->mBody->setTransform(XForm2D(mRightTarget));
					else mBox2DHero->mBody->setTransform(XForm2D(Vector2(-10000, -10000)));
				}
				
				// Tick Box2D (Single tick as requested)
				mBox2DWorld.getPhysicsScene()->tick(dt);

				auto end = std::chrono::high_resolution_clock::now();
				mRightCollisionTime = std::chrono::duration<float, std::milli>(end - start).count();
				mAvgRightTime = mAvgRightTime * 0.9f + mRightCollisionTime * 0.1f;
			}
			else
			{
				mRightCollisionTime = 0.0f;
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
			RenderUtility::SetPen(g, EColor::White); g.drawLine(Vector2(mHalfWidth, 0), Vector2(mHalfWidth, mScreenSize.y));

			// Left Draw
			RenderUtility::SetBrush(g, EColor::Blue); RenderUtility::SetPen(g, EColor::Cyan);
			for (auto& monster : mLeftMonsters)
			{
				Vector2 drawPos = monster->mPos;
				// New Parallel is asynchronous (1 frame latency), so we extrapolate for smoother visuals
				if (mUseNewParallel)
				{
					// Simple linear extrapolation: Pos + Vel * dt
					// Note: Since we don't store individual velocity in Monster, we fetch from manager or re-calculate
					// But wait, monster->mPos IS the position from manager.
					// Let's just trust mPos for now, or if user complains about jitter, we can add extrapolation later.
					// Actually, the jitter is likely due to the latency against the mouse cursor.
					// Let's keep it raw to show the "Real" physics state, or user might be confused why it penetrates.
				}
				g.drawCircle(drawPos, monster->mRadius);
			}
			if (mEnableHero) { RenderUtility::SetBrush(g, EColor::Green); RenderUtility::SetPen(g, EColor::Yellow); g.drawCircle(mHero->mPos, mHero->mRadius); }
			RenderUtility::SetBrush(g, (mUseParallel || mUseNewParallel) ? EColor::Red : EColor::Yellow); g.drawCircle(mLeftTarget, 5.0f);

			// Right Draw
			RenderUtility::SetBrush(g, EColor::Red); RenderUtility::SetPen(g, EColor::Orange);
			for (auto& monster : mRightMonsters) g.drawCircle(monster->getPosition(), monster->getRadius());
			if (mEnableHero) { RenderUtility::SetBrush(g, EColor::Green); RenderUtility::SetPen(g, EColor::Yellow); g.drawCircle(mBox2DHero->getPosition(), mBox2DHero->getRadius()); }
			RenderUtility::SetBrush(g, EColor::Yellow); g.drawCircle(mRightTarget, 5.0f);

			// Stats
			g.setTextColor(Color3ub(255, 255, 255));
			InlineString<256> leftInfo;
			const char* modeName = mUseNewParallel ? "New Parallel" : (mUseParallel ? "Simple Parallel" : "Simple Single");
			
			if (mUseNewParallel)
			{
				leftInfo.format("Mode: %s\nSubSteps: %d | Iters %d: \nFrame: %.3f ms\nAvg: %.3f ms\nSim Time: %.3f ms",
					modeName, mEnableSubStep ? 4 : 1, iterations, mLeftCollisionTime, mAvgLeftTime, mParallelManager.getLastSolveTime());
			}
			else
			{
				leftInfo.format("Mode: %s\nSubSteps: %d | Iters: %d\nFrame: %.3f ms\nAvg: %.3f ms",
					modeName, mEnableSubStep ? 4 : 1, iterations, mLeftCollisionTime, mAvgLeftTime);
			}

			g.drawText(10, 10, leftInfo);

			InlineString<256> rightInfo;
			rightInfo.format("Box2D\nMonsters: %d\nFrame: %.3f ms\nAvg: %.3f ms",
				(int)mRightMonsters.size(), mRightCollisionTime, mAvgRightTime);
			g.drawText(mHalfWidth + 10, 10, rightInfo);

			if (mAvgLeftTime > 0 && mAvgRightTime > 0) {
				float speedup = mAvgRightTime / mAvgLeftTime;
				InlineString<128> comparison; comparison.format("Speedup: %.1fx faster", speedup);
				g.drawText(mScreenSize.x / 2 - 60, mScreenSize.y - 30, comparison);
			}
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			Vector2 mousePos = msg.getPos();
			if (msg.onMoving()) {
				if (mousePos.x < mHalfWidth) { mLeftTarget = mousePos; mRightTarget = Vector2(mHalfWidth + mousePos.x, mousePos.y); }
				else { mRightTarget = mousePos; mLeftTarget = Vector2(mousePos.x - mHalfWidth, mousePos.y); }
			}
			if (msg.onLeftDown()) {
				if (mousePos.x < mHalfWidth) { spawnMonster(mousePos, true); spawnMonster(Vector2(mHalfWidth + mousePos.x, mousePos.y), false); }
				else { spawnMonster(mousePos, false); spawnMonster(Vector2(mousePos.x - mHalfWidth, mousePos.y), true); }
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown()) {
				switch (msg.getCode()) {
				case EKeyCode::R: 
					mLeftMonsters.clear(); mRightMonsters.clear(); 
					mParallelManager.cleanup(); 
					// Re-register Hero after cleanup
					{
						ParallelCollision::CollisionEntityData data;
						data.position = mLeftTarget;
						data.velocity = Vector2::Zero();
						data.radius = mHero->getRadius();
						data.mass = mHero->mMass;
						data.isStatic = true;
						data.bQueryCollision = true;
						mParallelManager.registerEntity(mHero.get(), data);
					}
					mAvgLeftTime = 0; mAvgRightTime = 0; break;
				case EKeyCode::Space:
					for (int i = 0; i < 100; ++i) {
						Vector2 lp(RandFloat(20, mHalfWidth - 20), RandFloat(20, mScreenSize.y - 20));
						spawnMonster(lp, true); spawnMonster(Vector2(mHalfWidth + lp.x, lp.y), false);
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		ERenderSystem getDefaultRenderSystem() override { return ERenderSystem::None; }
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override { systemConfigs.screenWidth = 1600; systemConfigs.screenHeight = 840; }
	};

	REGISTER_STAGE_ENTRY("Collision Benchmark", BenchmarkStage, EExecGroup::Test, "Physics|Benchmark");
}
