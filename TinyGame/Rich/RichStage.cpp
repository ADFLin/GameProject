#include "RichPCH.h"
#include "RichStage.h"

#include "GameStageMode.h"

#include "RichWorldEditor.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

namespace Rich
{
	namespace SimpleMap
	{
		uint8 const EE = 0;
		uint8 const TE = 1;
		int const SizeX = 8;
		int const SizeY = 6;
		uint8 Data[] =
		{
			TE,TE,TE,TE,TE,TE,TE,TE,
			TE,EE,EE,EE,EE,EE,EE,TE,
			TE,EE,EE,EE,EE,EE,EE,TE,
			TE,EE,EE,EE,EE,EE,EE,TE,
			TE,EE,EE,EE,EE,EE,EE,TE,
			TE,TE,TE,TE,TE,TE,TE,TE,
		};
	}

	LevelStage::LevelStage()
	{
		mEditor = nullptr;
	}


	bool LevelStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		mUserController.mLevel = &mLevel;
		mLevel.mTurn.mControl = this;

		srand( 12313 );
		mLevel.init();
		mScene.setupLevel( mLevel );

		mLevel.getWorld().addMsgListener( *this );
		mLevel.getWorld().addMsgListener( mUserController );

		Area* area = new LandArea;

		int idx = 0;
		for( int j = 0 ; j < SimpleMap::SizeY ; ++j )
		{
			for( int i = 0; i < SimpleMap::SizeX; ++i )
			{
				switch( SimpleMap::Data[idx] )
				{
				case SimpleMap::TE:
					mLevel.getWorld().addTile(MapCoord(i, j), EMPTY_AREA_ID);
					break;
				}
				++idx;
			}
		}

		Player* player;
		player = createUserPlayer();
		player->initPos(MapCoord(0, 0), MapCoord(0, 1));
		player->setRole( 0 );
		

		player = createUserPlayer();
		player->initPos(MapCoord(0, 0), MapCoord(0, 1));
		player->setRole( 1 );


		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_WORLD_EDITOR , "World Edit" );
		restart();

		return true;
	}

	bool LevelStage::onWidgetEvent( int event , int id , GWidget* widget )
	{
		if ( mEditor && !mEditor->onWidgetEvent( event , id , widget ) )
			return false;

		if ( !mUserController.onWidgetEvent( event , id , widget ) )
			return false;

		switch( id )
		{
		case UI_WORLD_EDITOR:
			if ( mEditor )
			{
				delete mEditor;
			}
			mEditor = new WorldEditor;
			mEditor->setup( mLevel , mScene );
			break;
		}

		return true;
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{
	
		switch( getStageMode()->getModeType() )
		{
		case EGameStageMode::Single:
		case EGameStageMode::Net:

			break;
		}
	}

	Player* LevelStage::createUserPlayer()
	{
		Entity* entity = new Entity;
		Player* player = mLevel.createPlayer();
		entity->addComponent(COMP_ACTOR, player);
		player->setController(mUserController);
		PlayerRenderComp* comp = mScene.createComponentT< PlayerRenderComp >(entity);
		return player;
	}

	void GameInputController::queryAction( ActionRequestID id , PlayerTurn& turn , ActionReqestData const& data )
	{
		switch( id )
		{
		case REQ_BUY_LAND:
			{
				GWidget* widget = ::Global::GUI().showMessageBox( UI_BUY_LAND , "Buy Land?" );
				widget->setUserData( intptr_t( &turn ) );
			}
			break;
		case REQ_UPGRADE_LAND:
			{
				GWidget* widget = ::Global::GUI().showMessageBox( UI_UPGRADE_LAND, "Upgrade Land?" );
				widget->setUserData( intptr_t( &turn ) );
			}
			break;
		case REQ_ROMATE_DICE_VALUE:
			{
				RomateDiceFrame* frame = new RomateDiceFrame( UI_ANY , Vec2i( 100 , 100 ) , nullptr );
				frame->mLevel    = mLevel;
				frame->mTurn     = &turn;
				::Global::GUI().addWidget( frame );
			}
			break;
		}
	}

	bool GameInputController::onWidgetEvent( int event , int id , GWidget* widget )
	{
		switch( id )
		{
		case UI_BUY_LAND:
		case UI_UPGRADE_LAND:
			{
				PlayerTurn* turn = reinterpret_cast< PlayerTurn* >( widget->getUserData() );
				ActionReplyData data;
				data.addParam( ( event == EVT_BOX_YES ) ? 1 : 0 );
				turn->replyAction( data );
				widget->destroy();
			}
			return false;
		}
		return true;
	}

	void GameInputController::onWorldMsg( WorldMsg const& msg )
	{
		switch ( msg.id )
		{
		case MSG_TURN_START:
			{
				MoveFrame* frame = new MoveFrame( UI_ANY , Vec2i( 100 , 100 ) , nullptr );
				frame->mLevel    = mLevel;
				frame->mTurn     = msg.getParam<PlayerTurn*>();
				frame->setMaxPower( mLevel->getActivePlayer()->getMovePower() );
				::Global::GUI().addWidget( frame );
			}
			break;
		}
	}

}