#include "TinyGamePCH.h"
#include "HoldemStage.h"
#include "HoldemScene.h"

#include "PokerNet.h"
#include "GameWorker.h"
#include "NetGameStage.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"

namespace Poker { namespace Holdem {

	class DevBetPanel : public BetPanelBase
	{
		typedef BetPanelBase BaseClass;
	public:
		DevBetPanel( int id , Vec2i const& pos , GWidget* parent )
			:BaseClass( id , pos , parent ){}

		bool onChildEvent( int event , int id , GWidget* ui )
		{
			int pos = mServer->getCurBetPos();
			switch( id )
			{
			case UI_FOLD:
				mServer->procBetRequest( pos , BET_FOLD , 0 );
				break;
			case UI_ALL_IN:
				mServer->procBetRequest( pos ,BET_ALL_IN , 0 );
				break;
			case UI_RISE:
				mServer->procBetRequest( pos ,BET_RAISE , mServer->getRule().bigBlind );
				break;
			case UI_CALL:
				mServer->procBetRequest( pos , BET_CALL , 0 );
				break;
			case UI_RISE_MONEY:
				break;
			}
			return BaseClass::onChildEvent( event , id , ui );
		}

		void refreshWidget()
		{
			doRefreshWidget( *mServer , mServer->getCurBetPos() );
		}
		ServerLevel* mServer;
	};

	LevelStage::LevelStage()
	{

	}

	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{
		for( int i = 0 ; i < 6 ; ++i )
		{
			GamePlayer* player = playerManager.createPlayer( i );
			player->getInfo().slot = i;
			player->getInfo().actionPort = i;
			player->getInfo().type = ( i == 0 ) ? PT_PLAYER : PT_COMPUTER;
		}
		playerManager.setUserID( 0 );

		setupServerLevel();
	}

	void LevelStage::setupServerLevel()
	{
		mServerLevel.reset( new ServerLevel );

		IPlayerManager& playerManager = *getPlayerManager();

		for( IPlayerManager::Iterator iter = playerManager.getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			int slotId = player->getInfo().slot;
			player->getInfo().actionPort = slotId;

			assert( player->getType() != PT_SPECTATORS );
			int money = 100 + 50 * player->getId();
			switch ( player->getType() )
			{
			case PT_COMPUTER:
				mServerLevel->addPlayer( player->getId() , slotId , money );
				break;
			case PT_PLAYER:
				mServerLevel->addPlayer( player->getId() , slotId , money );
				break;
			case PT_SPECTATORS:
				break;
			}
			//SlotStatus& status = mServerLevel->getSlotStatus( slotId );
			//status.playerId = player->getId();
		}

	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		mClientLevel.reset( new ClientLevel );

		mScene.reset( new Scene( *mClientLevel , *getPlayerManager() ) );

		NetWorker* netWorker = getManager()->getNetWorker();

		int userSlotId;
		for( IPlayerManager::Iterator iter = playerManager.getIterator();
			iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			assert( player->getType() != PT_SPECTATORS );
			//mClientLevel->getSlotStatus( player->getSlot() ).playerId = player->getId();
			//player->getInfo().actionPort = player->getSlot();
			if ( player->getId() == playerManager.getUserID() )
			{
				userSlotId = player->getActionPort();
				mClientLevel->setPlayerPos( userSlotId );
			}
		}

		if ( getGameType() == GT_NET_GAME )
		{
			ComWorker* worker = static_cast< GameNetLevelStage* >( getStage() )->getWorker();
			if ( mServerLevel )
			{
				mServerLevel->setupTransfer( new CSVWorkerDataTransfer( getManager()->getNetWorker() , MaxPlayerNum ) );
			}
			mClientLevel->setupTransfer( new CWorkerDataTransfer( worker , userSlotId ) );
		}
		else if ( getGameType() == GT_SINGLE_GAME )
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

		::Global::getGUI().cleanupWidget();
		mScene->setupWidget();
		if ( mServerLevel && getGameType() == GT_SINGLE_GAME )
		{
			DevBetPanel* panel = new DevBetPanel( UI_ANY , Vec2i( 100 , 300 ) , NULL );
			panel->mServer = mServerLevel;
			::Global::getGUI().addWidget( panel );
		}
		DevFrame* frame = WidgetUtility::createDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart" );
	}

	void LevelStage::onEnd()
	{
		if ( mServerLevel )
		{
			delete mServerLevel->getTransfer();
		}

		if ( mClientLevel )
		{
			delete mClientLevel->getTransfer();
		}

		mServerLevel.clear();
		mClientLevel.clear();
		mScene.clear();
	}

	void LevelStage::onRender( float dFrame )
	{
		Graphics2D& g = ::Global::getGraphics2D();
		mScene->render( g );
	}

	void LevelStage::onRestart( uint64 seed , bool beInit )
	{
		if ( mServerLevel )
		{
			if ( beInit )
			{
				Rule rule;
				rule.bigBlind   = 10;
				rule.smallBlind = 5;
				mServerLevel->setupGame( rule );
			}
			mServerLevel->startNewRound( mRandom );
		}
	}

}//namespace Holdem
}//namespace Poker