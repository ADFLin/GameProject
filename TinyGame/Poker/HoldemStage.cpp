#include "TinyGamePCH.h"
#include "HoldemStage.h"
#include "HoldemScene.h"

#include "DataTransferImpl.h"
#include "GameWorker.h"
#include "NetGameMode.h"

#include "GameGUISystem.h"
#include "DrawEngine.h"
#include "Widget/WidgetUtility.h"
#include "TaskBase.h"

namespace Holdem {

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
				mServer->procBetRequest( pos , EBetAction::Fold , 0 );
				break;
			case UI_ALL_IN:
				mServer->procBetRequest( pos ,EBetAction::AllIn , 0 );
				break;
			case UI_RISE:
				mServer->procBetRequest( pos ,EBetAction::Raise , mServer->getRule().bigBlind );
				break;
			case UI_CALL:
				mServer->procBetRequest( pos , EBetAction::Call , 0 );
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
		mDevPanel = nullptr;
	}

	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{
		for( int i = 0 ; i < 6 ; ++i )
		{
			GamePlayer* player = playerManager.createPlayer( i );
			player->setSlot( i );
			player->setActionPort( i );
			player->setType( (i == 0 ) ? PT_PLAYER : PT_COMPUTER );
		}
		playerManager.setUserID( 0 );

		GameLevelInfo info;
		configLevelSetting( info );
	}

	void LevelStage::configLevelSetting( GameLevelInfo& info )
	{
		mServerLevel.reset( new ServerLevel );
		mServerLevel->setListener(this);

		IPlayerManager& playerManager = *getStageMode()->getPlayerManager();

		for( auto iter = playerManager.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			int slotId = player->getSlot();
			player->setActionPort( slotId );

			assert( player->getType() != PT_SPECTATORS );
			int money = 200 /*+ 50 * player->getId()*/;
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

		mScene.reset(new Scene(*mClientLevel, *getStageMode()->getPlayerManager()));

		NetWorker* netWorker = ::Global::GameNet().getNetWorker();

		int userSlotId;

		for( auto iter = playerManager.createIterator(); iter; ++iter )
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

		if ( getModeType() == EGameStageMode::Net )
		{
			ComWorker* worker = static_cast< NetLevelStageMode* >( getStageMode() )->getWorker();
			if ( mServerLevel )
			{
				mServerLevel->setupTransfer( new CSVWorkerDataTransfer(::Global::GameNet().getNetWorker() ) );
			}
			mClientLevel->setupTransfer( new CWorkerDataTransfer( worker , userSlotId ) );
		}
		else if ( getModeType() == EGameStageMode::Single )
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
		mScene->setupWidget();
		if ( mServerLevel && getModeType() == EGameStageMode::Single )
		{
			mDevPanel = new DevBetPanel( UI_ANY , Vec2i( 100 , 300 ) , NULL );
			mDevPanel->mServer = mServerLevel;
			::Global::GUI().addWidget(mDevPanel);
		}
		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart" );
	}

	void LevelStage::onEnd()
	{
		if ( mServerLevel )
		{
			delete &mServerLevel->getTransfer();
		}

		if ( mClientLevel )
		{
			delete &mClientLevel->getTransfer();
		}

		mServerLevel.clear();
		mClientLevel.clear();
		mScene.clear();

		BaseClass::onEnd();
	}

	void LevelStage::onRender( float dFrame )
	{
		IGraphics2D& g = ::Global::GetIGraphics2D();
		g.beginRender();
		mScene->render( g );
		g.endRender();
	}

	void LevelStage::tick()
	{
		BaseClass::tick();
		if( mDevPanel )
		{
			mDevPanel->show(mServerLevel->isPlaying());
		}
	}

	void LevelStage::onRoundEnd()
	{
		auto func = [this](long time)->bool
		{
			mServerLevel->startNewRound(mRandom);
			return false;
		};

		DelayTask* delay = new DelayTask(2000);
		delay->setNextTask( TaskUtility::DelegateFunc(func) );
		addTask(delay);
	}

	void LevelStage::onPlayerLessBetMoney(int posSlot)
	{
		if( posSlot % 2 )
		{
			SlotInfo& slot = mServerLevel->getSlotInfo(posSlot);
			slot.ownMoney = 100;
		}
	}

	void LevelStage::setupCardDraw(ICardDraw* cardDraw)
	{
		mScene->setupCardDraw(cardDraw);
	}

	void LevelStage::onRestart(bool beInit)
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