#ifndef BubbleStage_h__
#define BubbleStage_h__

#include "GameStage.h"
#include "GameModule.h"

#include "BubbleMode.h"

namespace Bubble
{
	class Mode;

	class LevelStage : public GameStageBase
		             , public IActionLanucher
	{
		typedef GameStageBase BaseClass;
	public:
		LevelStage();

		//IActionLanucher
		void  fireAction( ActionTrigger& trigger );
		//GameSubStage
		void  tick();
		void  updateFrame( int frame );
		virtual bool getAttribValue( AttribValue& value );
		virtual bool setupAttrib( AttribValue const& value )
		{ 
			switch( value.id )
			{
			case ATTR_REPLAY_INFO:
				return true;
			case ATTR_GAME_INFO:
				return true;
			}
			return BaseClass::setupAttrib( value ); 
		}

		bool onWidgetEvent( int event , int id , GWidget* ui );

		virtual bool onInit();
		virtual void onEnd();
		virtual void onRestart( bool beInit )
		{

		}
		void onRender( float dFrame );
		void setupLocalGame( LocalPlayerManager& playerManager );
		void setupScene( IPlayerManager& playerManager );
		virtual void onChangeState( GameState state )
		{

		}

		IFrameActionTemplate* createActionTemplate( unsigned version );
		bool                  setupNetwork( NetWorker* netWorker , INetEngine** engine );
		PlayerDataManager mDataManager;
		Mode*   mMode;
		Scene*  scene;
	};

}// namespace Bubble

#endif // BubbleStage_h__
