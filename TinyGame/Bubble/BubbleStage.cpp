#include "BubblePCH.h"
#include "BubbleStage.h"

#include "BubbleGame.h"
#include "BubbleAction.h"

#include "GameStageMode.h"
#include "CSyncFrameManager.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "Widget/WidgetUtility.h"

#include <vector>

namespace Bubble
{
	LevelStage::LevelStage()
	{
		mMode = nullptr;
	}

	bool LevelStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if ( mMode == nullptr )
		{
			mMode = new TestMode;
		}

		mDataManager.setMode( mMode );
		getActionProcessor().setLanucher( this );

		return true;
	}

	void LevelStage::onEnd()
	{
		getActionProcessor().setLanucher( nullptr );
		delete mMode;

		BaseClass::onEnd();
	}


	void LevelStage::setupLocalGame( LocalPlayerManager& playerManager )
	{
		playerManager.createPlayer( 0 );
		playerManager.setUserID( 0 );
	}

	void LevelStage::setupScene( IPlayerManager& playerManager )
	{
		Controller& controller = static_cast< Controller& >(	
			getGame()->getController() );
		
		int numLevel = 0;
		PlayerData* mainViewPlayer = nullptr;
		std::vector< PlayerData* > otherPlayersData;
		for( auto iter = playerManager.createIterator(); iter; ++iter )
		{
			GamePlayer* player = iter.getElement();
			if ( player->getType() != PT_SPECTATORS )
			{
				++numLevel;
				PlayerData* data = mDataManager.createData();
				player->setActionPort( data->getId() );

				if ( player->getId() == playerManager.getUserID() )
				{
					mainViewPlayer = data;
					controller.setPortControl( data->getId() , 0 );
				}
				else if ( playerManager.getUser()->getType() == PT_SPECTATORS && mainViewPlayer == nullptr )
				{
					mainViewPlayer = data;
				}
				else
				{
					otherPlayersData.push_back(data);
				}
			}
		}

		switch( numLevel )
		{
		case 1:
			mainViewPlayer->getScene().setSurfacePos(Vec2i(200 , 30 ) );
			break;
		case 2:
			mainViewPlayer->getScene().setSurfacePos(Vec2i(100, 30));
			otherPlayersData[0]->getScene().setSurfacePos(Vec2i(450, 30));
			break;
		case 3:
		case 4:
		case 5:
			break;

		}

		::Global::GUI().cleanupWidget();

		WidgetUtility::CreateDevFrame();
	}

	void LevelStage::onRender( float dFrame )
	{
		Vector2 pos = Vector2( 100 , 100 );

		Graphics2D& g = Global::GetGraphics2D();

		mDataManager.render( g );
	}

	void LevelStage::fireAction( ActionTrigger& trigger )
	{
		mDataManager.fireAction( trigger );
	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* ui )
	{
		switch ( id )
		{
		case UI_RESTART_GAME:
			if ( getGameState() != GameState::End )
			{
				::Global::GUI().showMessageBox( 
					UI_RESTART_GAME , LOCTEXT("Do you Want to Stop Current Game?") );
			}
			else
			{
				getStageMode()->restart( false ); 
			}
			return false;

		}

		return BaseClass::onWidgetEvent( event , id , ui );
	}

	IFrameActionTemplate* LevelStage::createActionTemplate( unsigned version )
	{
		switch( version )
		{
		case LAST_VERSION:
		case CFrameActionTemplate::LastVersion:
			{
				CFrameActionTemplate* actionTemp = new CFrameActionTemplate( mDataManager );
				return actionTemp;
			}
		}
		return nullptr;

	}

	void LevelStage::tick()
	{
		switch( getGameState() )
		{
		case GameState::Start:
			changeState( GameState::Run );
			break;
		case GameState::Run:
			mDataManager.tick();
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mDataManager.updateFrame( frame );
	}

	bool LevelStage::queryAttribute( GameAttribute& value )
	{
		return BaseClass::queryAttribute( value );
	}


	bool LevelStage::setupNetwork( NetWorker* netWorker , INetEngine** engine )
	{
		IFrameActionTemplate* actionTemplate = createActionTemplate( LAST_VERSION );
		INetFrameManager* netFrameMgr;
		if ( netWorker->isServer() )
		{
			CServerFrameCollector* frameCollector = new CServerFrameCollector;
			netFrameMgr = new SVSyncFrameManager( netWorker , actionTemplate , frameCollector );
		}
		else
		{
			CClientFrameGenerator* frameCollector = new CClientFrameGenerator;
			netFrameMgr = new CLSyncFrameManager( netWorker , actionTemplate , frameCollector );

		}
		*engine = new CFrameActionEngine( netFrameMgr );
		return true;
	}

}// namespace Bubble