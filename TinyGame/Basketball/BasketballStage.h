#ifndef BasketballStage_h__
#define BasketballStage_h__

#include "StageBase.h"
#include "GameGlobal.h"
#include "GameRenderSetup.h"
#include "RHI/DrawUtility.h"

namespace Basketball
{
	using namespace Render;	

	class LevelStage : public StageBase, public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		LevelStage();
		~LevelStage();

		virtual bool onInit() override;
		virtual void onUpdate(GameTimeSpan deltaTime) override;
		virtual void onRender(float dFrame) override;


		void drawGame(RHIGraphics2D& g);
		virtual MsgReply onKey(KeyMsg const& msg) override;

		virtual ERenderSystem getDefaultRenderSystem() override;
		virtual bool setupRenderResource(ERenderSystem systemName) override;
		virtual void preShutdownRenderSystem(bool bReInit = false) override;


		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;

	protected:
		void restart();
		void throwBall();
		void updateBalls(float dt);
		
		// Gameplay variables
		float mAimPhase; // 0.0 to 1.0 (oscillating)
		float mAimSpeed;
		bool  mbAimDirectionPositive;
		struct Ball
		{
			Vector2 pos;
			Vector2 velocity;
			float   rotation;
			bool    bHasScored;
			float   deadTimer; 
		};
		
		std::vector<Ball> mBalls;
		float mShootCooldown;

		float   mGravity;
		
		int mScore;
		int mHighScore;
		float mGameTimer;
		bool mbGameOver;

		// Visual constraints
		Vector2 mHoopPos;
		float   mHoopRadius;

		Vector2 mPlayerPos;
		float   mScoreTimer;
		Vector2 mScorePos;

		struct Particle
		{
			Vector2 pos;
			Vector2 velocity;
			float   life;
			Color3f color;
		};
		std::vector<Particle> mParticles;
		struct Star
		{
			Vector2 pos;
			float   size;
			float   brightness;
			float   twinkleSpeed;
		};
		std::vector<Star> mStars;

		// Resources (Texture IDs or simple shapes for now)
		// We will implement simple shape rendering first
	};
}

#endif // BasketballStage_h__
