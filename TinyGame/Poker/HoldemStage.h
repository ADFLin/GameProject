#ifndef HoldemStage_h__
#define HoldemStage_h__

#include "GameStage.h"
#include "HoldemLevel.h"

#include "THolder.h"
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

	class LevelStage : public GameSubStage
		             , public ServerLevel::Listener
	{
	public:
		LevelStage();

		void buildServerLevel( GameLevelInfo& info );
		void setupScene( IPlayerManager& playerManager );
		void setupLocalGame( LocalPlayerManager& playerManager );

		void onRestart( uint64 seed , bool beInit );
		void onEnd();
		void onRender( float dFrame );

		CRandom  mRandom;

		TPtrHolder< ServerLevel >  mServerLevel;
		TPtrHolder< ClientLevel >  mClientLevel;
		TPtrHolder< Scene >        mScene;
	};

}//namespace Holdem
}//namespace Poker


#endif // HoldemStage_h__
