#include "RichPCH.h"
#include "RichStage.h"

#include "RichWorldEditor.h"

#include "GameGUISystem.h"
#include "WidgetUtility.h"

namespace Rich
{

	LevelStage::LevelStage()
	{
		mEditor = nullptr;
	}


	bool LevelStage::onInit()
	{
		::Global::getGUI().cleanupWidget();

		mUserCtrler.mLevel = &mLevel;
		mLevel.mTurn.mControl = this;

		srand( 12313 );
		mLevel.init();
		mScene.setupLevel( mLevel );

		mLevel.getWorld().addMsgListener( *this );
		mLevel.getWorld().addMsgListener( mUserCtrler );

		Player* player;
		player = createUserPlayer();
		player->setRole( 0 );

		player = createUserPlayer();
		player->setRole( 1 );

		DevFrame* frame = WidgetUtility::createDevFrame();
		frame->addButton( UI_WORLD_EDITOR , "World Edit" );
		restart();

		return true;
	}

	bool LevelStage::onEvent( int event , int id , GWidget* widget )
	{
		if ( mEditor && !mEditor->onEvent( event , id , widget ) )
			return false;

		if ( !mUserCtrler.onWidgetEvent( event , id , widget ) )
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

	Player* LevelStage::createUserPlayer()
	{
		Player* player = mLevel.createPlayer();
		PlayerRenderComp* comp = mScene.createComponentT< PlayerRenderComp >( player );
		player->setController( mUserCtrler );
		player->changePos( MapCoord(0,0) );
		return player;
	}

	void UserController::queryAction( RequestID id , PlayerTurn& turn , ReqData const& data )
	{
		switch( id )
		{
		case REQ_BUY_LAND:
			{
				GWidget* widget = ::Global::getGUI().showMessageBox( UI_BUY_LAND , "Buy Land?" );
				widget->setUserData( intptr_t( &turn ) );
			}
			break;
		case REQ_UPGRADE_LAND:
			{
				GWidget* widget = ::Global::getGUI().showMessageBox( UI_BUY_LAND , "Upgrade Land?" );
				widget->setUserData( intptr_t( &turn ) );
			}
			break;
		case REQ_ROMATE_DICE_VALUE:
			{
				RomateDiceFrame* frame = new RomateDiceFrame( UI_ANY , Vec2i( 100 , 100 ) , nullptr );
				frame->mLevel    = mLevel;
				frame->mTurn     = &turn;
				::Global::getGUI().addWidget( frame );
			}
			break;
		}
	}

	bool UserController::onWidgetEvent( int event , int id , GWidget* widget )
	{
		switch( id )
		{
		case UI_BUY_LAND:
		case UI_UPGRADE_LAND:
			{
				PlayerTurn* turn = reinterpret_cast< PlayerTurn* >( widget->getUserData() );
				ReplyData data;
				data.intVal = ( event == EVT_BOX_YES ) ? 1 : 0;
				turn->replyAction( data );
				widget->destroy();
			}
			return false;
		}
		return true;
	}

	void UserController::onWorldMsg( WorldMsg const& msg )
	{
		switch ( msg.id )
		{
		case MSG_TURN_START:
			{
				MoveFrame* frame = new MoveFrame( UI_ANY , Vec2i( 100 , 100 ) , nullptr );
				frame->mLevel    = mLevel;
				frame->mTurn     = msg.turn;
				frame->setMaxPower( mLevel->getActivePlayer()->getMovePower() );
				::Global::getGUI().addWidget( frame );
			}
			break;
		}
	}

}