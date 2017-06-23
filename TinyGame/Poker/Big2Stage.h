#ifndef Big2Stage_h__
#define Big2Stage_h__

#include "GameStage.h"

#include "Big2Level.h"

class ComWorker;
class LocalPlayerManager;

namespace Poker { namespace Big2 {

	class Scene;
	class CardListUI;

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

		enum
		{
			UI_TEST_SHOW_CARD = NEXT_UI_ID ,
			UI_TEST_PASS ,
			UI_NEXT_ROUND ,
		};
		void onRestart( uint64 seed , bool beInit );
		bool onInit();
		void onEnd();
		void onRender( float dFrame );
		bool onWidgetEvent( int event , int id , GWidget* ui );
		bool onMouse(MouseMsg const& msg);

		void tick();
		void updateFrame( int frame );

		void setupLocalGame( LocalPlayerManager& playerManager );
		void setupScene( IPlayerManager& playerManager );

		//<ServerLevel::Listener>
		virtual void onRoundInit();
		virtual void onSlotTurn( int slotId , bool beShow );
		virtual void onRoundEnd( bool isGameOver );
		virtual void onNewCycle();
		//</ServerLevel::Listener>

		bool nextRound( long time );

		void updateTestUI();
		void refreshTestUI();
		bool setupNetwork( NetWorker* worker , INetEngine** engine );
		GWidget*       mTestUI;
		CRandom        mRand;
		ServerLevel*   mServerLevel;
		ClientLevel*   mClientLevel;
		Scene*         mScene;

	};


}//namespace Big2
}//namespace Poker

#endif // Big2Stage_h__
