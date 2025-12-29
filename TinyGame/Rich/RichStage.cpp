#include "RichStage.h"

#include "GameMode.h"

#include "RichWorldEditor.h"
#include "RichAI.h"

#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

namespace Rich
{

	void LoadMap(Scene& scene);

	LevelStage::LevelStage()
	{
		mEditor = nullptr;
	}

	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();
		mScene.initializeRHI();
		mUserController.mLevel = &mLevel;

		mLevel.init();
		mScene.setupLevel(mLevel);
		mLevel.getWorld().addMsgListener( *this );


		LoadMap(mScene);

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_WORLD_EDITOR , "World Edit" );
		frame->addButton( UI_RESTART_GAME , "Restart");
		frame->addCheckBox("Pause", mbPauseGame );
		frame->addCheckBox("ShowDebugDraw", mbShowDebugDraw);
		frame->addCheckBox("FollowPlayer", mbFollowPlayer);

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

		if (!BaseClass::onWidgetEvent(event, id, widget))
			return false;

		return true;
	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{
	
		switch( getStageMode()->getModeType() )
		{
		case EGameMode::Single:
		case EGameMode::Net:

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

	AIController* LevelStage::createAIController(Player& player)
	{
		AIController* aiController = new AIController;
		controllers.push_back(aiController);
		aiController->process(player);
		return aiController;
	}

	void GameInputController::startTurn(PlayerTurn& turn)
	{
		MoveFrame* frame = new MoveFrame(UI_MOVE_FRAME, Vec2i(100, 100), nullptr);
		frame->mLevel = mLevel;
		frame->mTurn = &turn;
		frame->setMaxPower(mLevel->getActivePlayer()->getMovePower());
		::Global::GUI().addWidget(frame);
		CO_YEILD(this);
	}


	void GameInputController::endTurn(PlayerTurn& turn)
	{

	}

	void GameInputController::queryAction(ActionRequestID id, PlayerTurn& turn, ActionReqestData const& data)
	{
		switch( id )
		{
		case REQ_BUY_LAND:
			{
				GWidget* widget = ::Global::GUI().showMessageBox( UI_BUY_LAND , "Buy Land?" );
				auto reply = CO_YEILD< ActionReplyData >(this);
				turn.replyAction(reply);
			}
			break;
		case REQ_BUY_STATION:
			{
				GWidget* widget = ::Global::GUI().showMessageBox( UI_BUY_STATION, "Buy Land?");
				auto reply = CO_YEILD< ActionReplyData >(this);
				turn.replyAction(reply);
			}
			break;
		case REQ_UPGRADE_LAND:
			{
				GWidget* widget = ::Global::GUI().showMessageBox( UI_UPGRADE_LAND, "Upgrade Land?" );
				auto reply = CO_YEILD< ActionReplyData >(this);
				turn.replyAction(reply);
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
		case UI_BUY_STATION:
		case UI_UPGRADE_LAND:
			{
				ActionReplyData data;
				data.addParam( ( event == EVT_BOX_YES ) ? 1 : 0 );
				mLevel->resumeLogic(data);
				widget->destroy();
			}
			return false;
		case UI_MOVE_FRAME:
			if (event == EVT_EXIT_UI)
			{
				mLevel->resumeLogic();
			}
			return false;
		}
		return true;
	}



}