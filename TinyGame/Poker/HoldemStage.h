#ifndef HoldemStage_h__
#define HoldemStage_h__

#include "GameStage.h"
#include "HoldemLevel.h"

#include "Holder.h"

namespace Poker { namespace Holdem {

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
		             , public ServerLevel::Listener
	{
		typedef GameStageBase BaseClass;
	public:
		LevelStage();

		void buildServerLevel(GameLevelInfo& info);
		void setupScene( IPlayerManager& playerManager );
		void setupLocalGame( LocalPlayerManager& playerManager );

		void onRestart( bool beInit );
		void onEnd();
		void onRender( float dFrame );
		void tick();

		//ServerLevel::Listener
		virtual void onRoundEnd();
		virtual void onPlayerLessBetMoney(int posSlot);
		CRandom  mRandom;

		TPtrHolder< ServerLevel >  mServerLevel;
		TPtrHolder< ClientLevel >  mClientLevel;
		TPtrHolder< Scene >        mScene;
		class DevBetPanel*         mDevPanel;

		

	};

}//namespace Holdem
}//namespace Poker


#endif // HoldemStage_h__
