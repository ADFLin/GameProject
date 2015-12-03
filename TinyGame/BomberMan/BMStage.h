#ifndef BMStage_h__
#define BMStage_h__

#include "GameStage.h"

#include "BMMode.h"
#include "BMScene.h"


namespace BomberMan
{
	class Mode;

	class MenuStage : public StageBase
	{






	};

	

	class LevelStage : public GameSubStage
		             , public ActionEnumer
	{
		typedef GameSubStage BaseClass;
	public:
		LevelStage()
		{

		}

		enum StageStep
		{
			STEP_READY  ,
			STEP_RUN    ,
			STEP_ROUND_RESULT ,
			STEP_GAME_OVER    ,
		};

		void setMode( Mode* mode );

		//virtual 
		bool onInit();
		void onEnd();
		void onRender( float dFrame );
		void onRestart( uint64 seed , bool beInit );

		void setupLocalGame( LocalPlayerManager& playerManager );

		void setupScene( IPlayerManager& playerManager );
		void tick();
		void updateFrame( int frame );

		bool onKey( unsigned key , bool isDown )
		{
			if ( !isDown )
				return false;

			switch( key )
			{
			case 'R': getStage()->restart( false ); break;
			}
			return false;
		}
		void fireAction( ActionTrigger& trigger );
		void prevScanAction( bool beUpdateFrame );

		void setStep( StageStep step , long time );
	protected:
		//virtual 
		IFrameActionTemplate* createActionTemplate( unsigned version );
		bool                  setupNetwork( NetWorker* netWorker , INetEngine** engine );
		StageStep        mStep;
		Mode*            mMode;
		Scene            mScene;
		long             mGameTime;
		long             mNextStepTime;
	};


}//namespace BomberMan

#endif // BMStage_h__
