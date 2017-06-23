#include "GameStage.h"

namespace GreedySnake
{
	class Scene;
	class Mode;

	class LevelStage : public GameStageBase
	{
		typedef GameStageBase BaseClass;
	public:

		LevelStage();
		~LevelStage();

		bool onInit();
		void onEnd(){}
		void onRestart( uint64 seed , bool beInit );
		
		
		void setupLocalGame( LocalPlayerManager& playerManager );
		void setupScene( IPlayerManager& playerManager );

		void tick();
		void updateFrame( int frame );
		void onChangeState( GameState state );
	
		bool onWidgetEvent( int event , int id , GWidget* ui );
		bool onKey(unsigned key, bool isDown);
		void onRender( float dFrame );

		bool                  getAttribValue( AttribValue& value );
		bool                  setupAttrib( AttribValue const& value );
		IFrameActionTemplate* createActionTemplate( unsigned version );
		bool                  setupNetwork( NetWorker* netWorker , INetEngine** engine );

		TPtrHolder< Scene >  mScene;
		Mode*  mGameMode;

		

	};

}//namespace GreedySnake