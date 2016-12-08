#include "TinyGamePCH.h"
#include "Big2Stage.h"

#include "Big2Scene.h"

#include "InputManager.h"
#include "INetEngine.h"

#include "NetGameStage.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"

#include "DataTransferImpl.h"


namespace Poker { namespace Big2 {


	LevelStage::LevelStage()
	{
		mServerLevel = NULL;
		mClientLevel = NULL;
		mScene = NULL;
		mTestUI = NULL;
	}

	bool LevelStage::onInit()
	{


		return true;
	}

	void LevelStage::onEnd()
	{
		if ( mServerLevel )
		{
			for( int i = 0 ; i < PlayerNum ; ++i )
				delete mServerLevel->getSlotBot( i );

			delete mServerLevel->getTransfer();
		}
		if ( mClientLevel )
			delete mClientLevel->getTransfer();

		delete mServerLevel;
		delete mClientLevel;
		delete mScene;
	}

	void LevelStage::tick()
	{
		updateTestUI();
	}

	void LevelStage::updateFrame( int frame )
	{
		mScene->updateFrame( frame );
	}

	void LevelStage::onRestart( uint64 seed , bool beInit )
	{
		if ( beInit )
			::Global::RandSeedNet( seed );

		if ( mServerLevel )
			mServerLevel->restrat( mRand , beInit );

		mClientLevel->restart( beInit );
	}

	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{
		for( int i = 0 ; i < 4 ; ++i )
		{
			GamePlayer* player = playerManager.createPlayer( i );
			player->getInfo().slot = i;
			player->getInfo().actionPort = i;
			player->getInfo().type = ( i == 0 ) ? PT_PLAYER : PT_COMPUTER;
		}
		playerManager.setUserID( 0 );
	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		if ( ( getModeType() == SMT_NET_GAME && ::Global::GameNet().getNetWorker()->isServer() ) ||
			   getModeType() == SMT_SINGLE_GAME )
		{
			mServerLevel = new ServerLevel;
		}
		mClientLevel = new ClientLevel;

		NetWorker* netWorker = ::Global::GameNet().getNetWorker();

		bool haveBot = false;
		if ( mServerLevel )
		{
			for( IPlayerManager::Iterator iter = playerManager.getIterator();
				iter.haveMore() ; iter.goNext() )
			{
				GamePlayer* player = iter.getElement();
				int slotId = player->getInfo().slot;
				player->getInfo().actionPort = slotId;

				assert( player->getType() != PT_SPECTATORS );
				if ( player->getType() == PT_COMPUTER )
				{
					//mServerLevel->setSlotBot( slotId , new CBot );
					haveBot = true;
				}

				SlotStatus& status = mServerLevel->getSlotStatus( slotId );
				status.playerId = player->getId();
			}
		}


		int userSlotId;
		for( IPlayerManager::Iterator iter = playerManager.getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			assert( player->getType() != PT_SPECTATORS );

			mClientLevel->getSlotStatus( player->getSlot() ).playerId = player->getId();
			player->getInfo().actionPort = player->getSlot();
			if ( player->getId() == playerManager.getUserID() )
			{
				userSlotId = player->getSlot();
				mClientLevel->setPlayerSlotId( userSlotId );
			}
		}

		if ( getModeType() == SMT_NET_GAME )
		{
			ComWorker* worker = static_cast< NetLevelStageMode* >( getStageMode() )->getWorker();
			if ( mServerLevel  )
			{
				mServerLevel->setupTransfer( new CSVWorkerDataTransfer(::Global::GameNet().getNetWorker(), 4 ) );
			}
			mClientLevel->setupTransfer( new CWorkerDataTransfer( worker , userSlotId ) );
		}
		else if ( getModeType() == SMT_SINGLE_GAME )
		{
			CTestDataTransfer* sv = new CTestDataTransfer;
			CTestDataTransfer* cl = new CTestDataTransfer;
			sv->slotId = SLOT_SERVER;
			sv->conTransfer = cl;

			cl->slotId = userSlotId;
			cl->conTransfer = sv;

			mServerLevel->setupTransfer( sv );
			mClientLevel->setupTransfer( cl );
		}



		::Global::GUI().cleanupWidget();

		mScene = new Scene( *mClientLevel , playerManager );
		
		mScene->setupUI();

		if ( mServerLevel && haveBot )
		{
			Vec2i frameSize( 360 , mScene->getCardSize().y + 60 );
			Vec2i sSize = ::Global::getDrawEngine()->getScreenSize();
			mServerLevel->setEventListener( this );

			GFrame* frame = new GFrame( UI_ANY , Vec2i( ( sSize.x - frameSize.x ) / 2 , 350 ) , frameSize , NULL );
			frame->setAlpha( 0.9f );
			GWidget* ui = new CardListUI( mServerLevel->getSlotOwnCards( 0 ) , Vec2i( frameSize.x / 2 , 15 ) , frame );

			GButton* button;
			Vec2i btnSize( 60 , 20 );
			int offset = btnSize.x + 2;
			Vec2i pos = Vec2i( frameSize.x / 2 - offset , frameSize.y - btnSize.y - 5 );

			button = new GButton( UI_TEST_PASS , pos , btnSize , frame );
			button->setTitle( "Pass" );
			pos.x += offset;
			button = new GButton( UI_TEST_SHOW_CARD , pos , btnSize , frame );
			button->setTitle( "Show" );

			mTestUI = frame;
			mTestUI->show( false );
			::Global::GUI().addWidget( mTestUI );
		}

		WidgetUtility::createDevFrame();
	}

	void LevelStage::onRender( float dFrame )
	{
		::Graphics2D& g = Global::getGraphics2D();		
		mScene->render( g );
	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch( id )
		{
		case UI_TEST_PASS:
			mServerLevel->procSlotPass( mServerLevel->getNextShowSlot() );
			return false;
		case UI_TEST_SHOW_CARD:
			{
				int num;
				int* pIndex = static_cast< CardListUI* >( mTestUI->getChild() )->getSelcetIndex( num );
				mServerLevel->procSlotShowCard( mServerLevel->getNextShowSlot() , pIndex , num );
			}
			return false;
		}
		return BaseClass::onWidgetEvent( event , id , ui );
	}

	bool LevelStage::onMouse( MouseMsg const& msg )
	{
		return true;
	}

	void LevelStage::updateTestUI()
	{
		static bool canPass = true;
		InputManager& manager = InputManager::getInstance();
		if ( mServerLevel )
		{
			if (  manager.isKeyDown( 'R' ) )
			{
				if ( canPass )
				{
					getStageMode()->restart( false );
					canPass = false;
				}
			}
			else
			{
				canPass = true;
			}
		}
	}

	bool LevelStage::setupNetwork( NetWorker* worker , INetEngine** engine )
	{
		return true;
	}

	void LevelStage::refreshTestUI()
	{
		int nextSlot = mServerLevel->getNextShowSlot();
		if ( nextSlot != -1 )
		{
			GamePlayer* player = getStageMode()->getPlayerManager()->getPlayer( mServerLevel->getSlotStatus( nextSlot ).playerId );
			if ( player->getType() == PT_COMPUTER )
			{
				static_cast< CardListUI* >( mTestUI->getChild() )->setClientCards( mServerLevel->getSlotOwnCards( nextSlot ) );
				mTestUI->show( true );
				return;
			}
		}

		mTestUI->show( false );
	}

	void LevelStage::onSlotTurn( int slotId , bool beShow )
	{
		refreshTestUI();
	}

	void LevelStage::onRoundInit()
	{
		refreshTestUI();
	}
	void LevelStage::onRoundEnd( bool isGameOver )
	{
		refreshTestUI();
		if ( !isGameOver )
		{
			TaskBase* task = new DelayTask( 5000 );
			task->setNextTask( TaskUtility::createMemberFunTask( this, &LevelStage::nextRound ) );
			addTask( task );
		}
	}

	void LevelStage::onNewCycle()
	{
		refreshTestUI();
	}

	bool LevelStage::nextRound( long time )
	{
		mServerLevel->startNewRound( mRand );
		return false;
	}


}//namespace Big2
}//namespace Poker