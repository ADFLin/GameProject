#pragma once
#ifndef DDStage_H_476AA87B_D366_4E76_9E8A_4791AEEB7A39
#define DDStage_H_476AA87B_D366_4E76_9E8A_4791AEEB7A39

#include "GameStage.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommon.h"

#include "PokerBase.h"

namespace Poker
{
	class ICardDraw;
}

namespace DouDizhu
{
	using namespace Poker;
	using namespace Render;

	enum class EGroupTrick
	{
		Solo,
		Pair,
		Trio,
		Airplane,
		Four,
	};

	struct TrickInfo
	{

	};

	class Scene;

	class CRandom : public IRandom
	{
	public:
		int getInt()
		{
			return ::Global::Random();
		}
	};

	class LevelStage : public GameStageBase
		             , public IGameRenderSetup
		             //, public ServerLevel::Listener
	{
		typedef GameStageBase BaseClass;
	public:
		LevelStage();

		void buildServerLevel(GameLevelInfo& info);
		void setupScene(IPlayerManager& playerManager);
		void setupLocalGame(LocalPlayerManager& playerManager);

		void onRestart(bool beInit);
		void onEnd();
		void onRender(float dFrame);
		void tick();

		//ServerLevel::Listener
		CRandom  mRandom;

		//TPtrHolder< ServerLevel >  mServerLevel;
		//TPtrHolder< ClientLevel >  mClientLevel;
		//TPtrHolder< Scene >        mScene;
		//class DevBetPanel*         mDevPanel;


		RHITexture2DRef mUVTexture;

		ERenderSystem getDefaultRenderSystem() override;
		bool setupRenderSystem(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;

		ICardDraw* cardDraw = nullptr;

	};

}//namespace Holdem

#endif // DDStage_H_476AA87B_D366_4E76_9E8A_4791AEEB7A39