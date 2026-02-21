#ifndef SurvivorsStage_h__
#define SurvivorsStage_h__

#include "StageBase.h"
#include "GameRenderSetup.h"

#include "SurvivorsCommon.h"
#include "SurvivorsEntity.h"
#include "SurvivorsMonster.h"
#include "SurvivorsHero.h"
#include "SurvivorsRenderer.h"

#include "ParallelCollision/ParallelCollision.h"
#include "Async/AsyncWork.h"
#include "Renderer/RenderThread.h"

#include <vector>
#include <memory>

namespace Survivors
{
	class SurvivorsStage : public StageBase, public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		SurvivorsStage() : mParallelManager(mThreadPool) {}

		// --- Accessors for game state (used by Hero, Monster) ---
		Vector2 getHeroPos() const { return mHero ? mHero->mPos : Vector2::Zero(); }
		int getHeroLevel() const { return mHero ? mHero->getLevel() : 1; }
		ParallelCollision::ParallelCollisionManager& getParallelManager() { return mParallelManager; }
		void addMonster(std::unique_ptr<Monster> m) { mMonsters.push_back(std::move(m)); }
		void addBullet(std::unique_ptr<GameBullet> b) { mBullets.push_back(std::move(b)); }
		void addVisual(VisualEffect const& ve) { mVisuals.push_back(ve); }
		const std::vector<std::unique_ptr<Monster>>& getMonsters() const { return mMonsters; }


		SurvivorsRenderer& getRenderer() { return mRenderer; }

		// --- StageBase overrides ---
		bool onInit() override;
		void onEnd() override;
		void onUpdate(GameTimeSpan deltaTime) override;
		void onRender(float dFrame) override;
		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;

		// --- IGameRenderSetup ---
		ERenderSystem getDefaultRenderSystem() override { return ERenderSystem::None; }
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;

	private:
		void spawnMonster();
		void monsterFire();

		QueueThreadPool mThreadPool;
		ParallelCollision::ParallelCollisionManager mParallelManager;
		std::unique_ptr<Hero> mHero;
		std::vector<std::unique_ptr<Monster>> mMonsters;
		std::vector<std::unique_ptr<GameBullet>> mBullets;
		std::vector<Gem> mGems;
		std::vector<VisualEffect> mVisuals;

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

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
		bool mbPaused = false;

		// Virtual Joystick
		bool mVJoystickActive = false;
		Vector2 mVJoystickCenter;
		Vector2 mVJoystickCurrentPos;
		float mVJoystickRadius = 60.0f;

		// Camera
		Vector2 mCameraPos = Vector2::Zero();
		float mCameraZoom = 1.0f;
		float mCameraZoomTarget = 1.0f;
		float mCameraFollowSpeed = 4.0f;

		SurvivorsRenderer mRenderer;

		bool mbInUpdate = false;
	};

}//namespace Survivors

#endif // SurvivorsStage_h__
