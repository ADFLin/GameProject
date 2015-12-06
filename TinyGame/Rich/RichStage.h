#ifndef RichStage_h__
#define RichStage_h__

#include "StageBase.h"


#include "RichLevel.h"
#include "RichScene.h"

#include "RichEditInterface.h"
#include "RichWorld.h"
#include "RichPlayer.h"



namespace Rich
{

	enum
	{
		UI_MOVE_BUTTON = UI_GAME_ID ,
		UI_BUY_LAND ,
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
					ReplyData data;
					data.intVal = mValue;
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
			UI_EVAL_MOVESTEP_BUTTON = UI_WEIGET_ID ,
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

	class UserController : public IController
		                 , public IMsgListener
	{
	public:
		virtual void queryAction( RequestID id , PlayerTurn& turn , ReqData const& data );
		virtual bool onWidgetEvent( int event , int id , GWidget* widget );
		virtual void onWorldMsg( WorldMsg const& msg );

		Level* mLevel;
	};

	
	enum StageState
	{
		SS_SETUP_GAME ,
		SS_MAP ,
		SS_STORE ,
	};

	class LevelStage : public StageBase
		             , public IMsgListener
					 , public ITurnControl
	{
		typedef StageBase BaseClass;
	public:
		enum 
		{
			UI_WORLD_EDITOR = BaseClass::NEXT_UI_ID ,
		};

		LevelStage();
		bool onInit();

		Player* createUserPlayer();

		void onRender( float dFrame )
		{
			Graphics2D& g = ::Global::getGraphics2D();
			mScene.render( g );
		}

		void restart()
		{
			mLevel.restart();

		}

		void tick()
		{
			if ( mEditor )
				return;

			mLevel.tick();
		}

		void updateFrame( int frame )
		{
			int time = frame * gDefaultTickTime;
			mScene.update( time );
		}

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();

			updateFrame( frame );
		}

		virtual void onWorldMsg( WorldMsg const& msg )
		{
			switch ( msg.id )
			{

			}
		}

		virtual bool needRunTurn()
		{
			return !mScene.haveAnimation();
		}

		virtual bool onWidgetEvent( int event , int id , GWidget* widget );

		virtual bool onMouse( MouseMsg const& msg )
		{  
			if ( mEditor && !mEditor->onMouse( msg ) )
				return false;
			return true;  
		}
		virtual bool onKey( unsigned key , bool isDown ){  return true;  }

		UserController mUserCtrler;

		IEditor*   mEditor;
		Scene      mScene;
		Level      mLevel;
	};


}//namespace Rich

#endif // RichStage_h__
