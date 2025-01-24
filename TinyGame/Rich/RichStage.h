#ifndef RichStage_h__
#define RichStage_h__

#include "GameStage.h"
//#include "StageBase.h"


#include "RichLevel.h"
#include "RichScene.h"

#include "RichEditInterface.h"
#include "RichWorld.h"
#include "RichPlayer.h"

#include "DataTransfer.h"

namespace Rich
{
	class AIController;

	enum
	{
		UI_MOVE_BUTTON = UI_GAME_ID ,
		UI_MOVE_FRAME ,
		UI_BUY_LAND ,
		UI_BUY_STATION ,
		UI_UPGRADE_LAND ,
	};

	class RomateDiceFrame : public GFrame
	{
	public:
		enum
		{
			UI_DICE_VALUE ,
		};
		RomateDiceFrame( int id , Vec2i const& pos  , GWidget* parent )
			:GFrame( id , pos , Vec2i( 200 , 70 ) , parent )
		{

			mValue = 1;
			GSlider* slider = new GSlider( UI_DICE_VALUE , Vec2i( 10 , 40 ) , 100 , true , this );
			slider->setRange( 1 , 6 );
			slider->setValue( mValue );
			slider->showValue();
			GButton* button = new GButton( UI_OK , Vec2i( 130 , 35 ) , Vec2i( 60 , 20 ) , this );
			button->setTitle( "OK" );
		}

		bool onChildEvent( int event , int id , GWidget* widget )
		{ 
			switch( id )
			{
			case UI_DICE_VALUE:
				{
					mValue = static_cast< GSlider* >( widget )->getValue();
				}
				return false;
			case UI_OK:
				{
					ActionReplyData data;
					data.addParam( mValue );
					mTurn->replyAction( data );
					destroy();
				}
				return false;
			}
			return true; 
		}

		Level*      mLevel;
		PlayerTurn* mTurn;
		int         mValue;
	};


	class MoveFrame : public GFrame
	{
	public:
		enum
		{
			UI_EVAL_MOVESTEP_BUTTON = UI_WIDGET_ID ,
			UI_POWER_BUTTON ,
		};

		MoveFrame( int id , Vec2i const& pos  , GWidget* parent )
			:GFrame( id , pos , Vec2i( 200 , 200 ) , parent )
		{
			for( int i = 0 ; i < MAX_MOVE_POWER ;++i )
			{
				//GButton* button = new GButton( UI_POWER_BUTTON , Vec2i( 20 , i * 30 ))
			}
			GButton* button = new GButton( UI_EVAL_MOVESTEP_BUTTON , Vec2i( 5 , 5 ) , Vec2i( 100 , 25 ) , this );
			button->setTitle( "Go" );
		}

		void setMaxPower( int power )
		{
			movePower = power;
			for( int i = power ; i < MAX_MOVE_POWER ;++i )
			{

			}
		}

		bool onChildEvent( int event , int id , GWidget* widget )
		{ 
			switch( id )
			{
			case UI_EVAL_MOVESTEP_BUTTON:
				{
					mTurn->goMoveByPower( movePower );
					sendEvent(EVT_EXIT_UI);
					destroy();
				}
				return false;
			}
			return true; 
		}
		PlayerTurn* mTurn;
		Level*      mLevel;
		int         movePower;
	};

	class GameInputController : public IPlayerController
							  , public IYieldInstruction
	{
	public:
		void startTurn(PlayerTurn& turn) override;
		void endTurn(PlayerTurn& turn) override;
		void queryAction(ActionRequestID id, PlayerTurn& turn, ActionReqestData const& data);

		virtual bool onWidgetEvent( int event , int id , GWidget* widget );

		void setupClientNetwork( Player* player )
		{
#if 0
			NetWorker* worker = ::Global::GameNet().getNetWorker();
			assert(!worker->isServer());

			mNetDataTranslater = new CWorkerDataTransfer(worker, player->g);
#endif
		}
		Level* mLevel;
		IDataTransfer* mNetDataTranslater;
	};

	
	enum StageState
	{
		SS_SETUP_GAME ,
		SS_MAP ,
		SS_STORE ,
	};

	class LevelStage : public GameStageBase
		             , public IWorldMessageListener
	{
		typedef GameStageBase BaseClass;
	public:
		enum 
		{
			UI_WORLD_EDITOR = BaseClass::NEXT_UI_ID ,
		};

		LevelStage();
		bool onInit();

		Player* createUserPlayer();


		std::vector< IPlayerController* > controllers;

		AIController* createAIController(Player& player);

		void onRender( float dFrame )
		{
			IGraphics2D& g = ::Global::GetIGraphics2D();
			g.beginRender();

			RenderParams renderParams;
			renderParams.bDrawDebug = mbShowDebugDraw;
			//mView.screenOffset = Vector2::Zero();
			mScene.mView.screenSize = ::Global::GetScreenSize();
			mScene.mView.zoom = 0.5;
			mScene.mView.update();
			mScene.render( g , renderParams );

			g.endRender();
		}


		virtual void onRestart(bool beInit) 
		{
			bool bAutoPlay = true;
			if (beInit)
			{
				for (int i = 0; i < 4; ++i)
				{
					Player* player = createUserPlayer();
					player->setRole(i);
				}

				if (bAutoPlay)
				{
					int i = 0;
					for (auto player : mLevel.getPlayerList())
					{
						if ( i != 0 )
							createAIController(*player);
						++i;
					}
				}
			}
			else
			{
				mScene.reset();
				mLevel.reset();
			}

			for( auto player : mLevel.mPlayerList )
			{
				player->initPos(MapCoord(0, 0), MapCoord(0, 1));
				player->mMoney = mLevel.getGameOptions().initialCash;
				player->mMovePower = 2;
			}

			srand(GenerateRandSeed());
			startGame();
		}

		void startGame()
		{
			mScene.initiliazeTiles();
			mLevel.start();
		}

		void tick()
		{
			if ( mEditor)
				return;

			mLevel.tick();
		}

		void updateFrame( int frame )
		{
			int time = frame * gDefaultTickTime;

			mScene.update( time );
			if (mbFollowPlayer)
			{
				Player* player = mLevel.getActivePlayer();
				if (player)
				{
					RenderView& view = mScene.mView;
					Vector2 pos = player->getOwner()->getComponentT<ActorRenderComp>(COMP_RENDER)->pos;
					Vector2 screenPos = view.convMapToScreenPos(pos);
					view.screenOffset += screenPos - 0.5 * view.screenSize;
				}
			}
		}

		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			if (mbPauseGame)
			{
				return;
			}

			int accFactor = 15;
			deltaTime.value *= 15;
			BaseClass::onUpdate(deltaTime);
		}

		virtual void onWorldMsg( WorldMsg const& msg )
		{
			switch ( msg.id )
			{

			}
		}

		virtual bool onWidgetEvent( int event , int id , GWidget* widget );

		virtual MsgReply onMouse( MouseMsg const& msg )
		{  
			if (mEditor)
			{
				MsgReply reply = mEditor->onMouse(msg);
				if (reply.isHandled())
					return reply;
			}
			static Vec2i StaticPrevPos;
			if (msg.onRightDown())
			{
				StaticPrevPos = msg.getPos();
			}

			if (msg.isRightDown() && msg.onMoving())
			{
				Vec2i offset = msg.getPos() - StaticPrevPos;
				mScene.mView.screenOffset -= Vector2(offset);
				StaticPrevPos = msg.getPos();
			}
			return BaseClass::onMouse(msg);  
		}
		virtual MsgReply onKey(KeyMsg const& msg)
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::Q:
					mScene.mView.dir = ViewDir((mScene.mView.dir + 1) % 4);
					return MsgReply::Handled();
				default:
					break;
				}

			}
			return MsgReply::Unhandled();
		}

		GameInputController mUserController;

		IDataTransfer* mSeverDataTransfer;
		IEditor*   mEditor;
		Scene      mScene;
		Level      mLevel;

		bool mbPauseGame = false;
		bool mbFollowPlayer = false;
		bool mbShowDebugDraw = false;

		virtual void setupScene(IPlayerManager& playerManager) override;

	};


}//namespace Rich

#endif // RichStage_h__
