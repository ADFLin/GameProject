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
		void onEnd();
		void onRestart( bool beInit );
		
		
		void setupLocalGame( LocalPlayerManager& playerManager );
		void setupScene( IPlayerManager& playerManager );

		void tick();
		void updateFrame( int frame );
		void onChangeState( EGameState state );
	
		bool onWidgetEvent( int event , int id , GWidget* ui );
		bool onKey(KeyMsg const& msg);
		void onRender( float dFrame );

		bool                  queryAttribute( GameAttribute& value );
		bool                  setupAttribute( GameAttribute const& value );
		IFrameActionTemplate* createActionTemplate( unsigned version );
		bool                  setupNetwork( NetWorker* netWorker , INetEngine** engine );

		TPtrHolder< Scene >  mScene;
		Mode*  mGameMode;

		

	};

}//namespace GreedySnake