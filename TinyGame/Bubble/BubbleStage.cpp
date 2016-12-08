#include "BubblePCH.h"
#include "BubbleStage.h"

#include "BubbleGame.h"
#include "BubbleAction.h"

#include "GameSingleStage.h"
#include "CSyncFrameManager.h"

#include "GameGUISystem.h"
#include "GameWidgetID.h"
#include "WidgetUtility.h"

namespace Bubble
{
	LevelStage::LevelStage()
	{
		mMode = NULL;
	}

	bool LevelStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		if ( mMode == NULL )
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
		PlayerData* mainViewPlayer = NULL;
		for( IPlayerManager::Iterator iter = playerManager.getIterator();
			 iter.haveMore() ; iter.goNext() )
		{
			GamePlayer* player = iter.getElement();
			if ( player->getType() != PT_SPECTATORS )
			{
				++numLevel;
				PlayerData* data = mDataManager.createData();
				player->getInfo().actionPort = data->getId();

				if ( player->getId() == playerManager.getUserID() )
				{
					mainViewPlayer = data;
					controller.setPortControl( data->getId() , 0 );
				}
				else if ( mainViewPlayer == NULL )
				{
					mainViewPlayer = data;
				}
			}
		}

		switch( numLevel )
		{
		case 1:
			mainViewPlayer->getScene().setSurfacePos( Vec2i( 200 , 30 ) );
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			break;

		}

		::Global::GUI().cleanupWidget();

		WidgetUtility::createDevFrame();
	}

	void LevelStage::onRender( float dFrame )
	{
		Vec2f pos = Vec2f( 100 , 100 );

		Graphics2D& g = Global::getGraphics2D();

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
			if ( getGameState() != GS_END )
			{
				::Global::GUI().showMessageBox( 
					UI_RESTART_GAME , LAN("Do you Want to Stop Current Game?") );
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
		return NULL;

	}

	void LevelStage::tick()
	{
		switch( getGameState() )
		{
		case GS_START:
			changeState( GS_RUN );
			break;
		case GS_RUN:
			mDataManager.tick();
			break;
		}
	}

	void LevelStage::updateFrame( int frame )
	{
		mDataManager.updateFrame( frame );
	}

	bool LevelStage::getAttribValue( AttribValue& value )
	{
		return BaseClass::getAttribValue( value );
	}


	bool LevelStage::setupNetwork( NetWorker* netWorker , INetEngine** engine )
	{
		IFrameActionTemplate* actionTemplate = createActionTemplate( LAST_VERSION );
		INetFrameManager* netFrameMgr;
		if ( netWorker->isServer() )
		{
			CServerFrameGenerator* frameGenerator = new CServerFrameGenerator;
			netFrameMgr = new SVSyncFrameManager( netWorker , actionTemplate , frameGenerator );
		}
		else
		{
			CClientFrameGenerator* frameGenerator = new CClientFrameGenerator;
			netFrameMgr = new CLSyncFrameManager( netWorker , actionTemplate , frameGenerator );

		}
		*engine = new CFrameActionEngine( netFrameMgr );
		return true;
	}

}// namespace Bubble