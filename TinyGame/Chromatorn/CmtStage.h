#ifndef CmtStage_h__
#define CmtStage_h__

#include "StageBase.h"

#include "CmtScene.h"

namespace Chromatron
{

	class LevelStage : public StageBase
	{
		typedef StageBase BaseClass;

		enum
		{
			UI_LEVEL_CHOICE = BaseClass::NEXT_UI_ID ,
			UI_RESTART_LEVEL     ,
			UI_NEXT_LEVEL_BUTTON ,
			UI_PREV_LEVEL_BUTTON ,
			UI_PREV_UNSOLVED_LEVEL_BUTTON ,
			UI_GO_UNSOLVED_LEVEL_BUTTON ,
			UI_GAME_PACKAGE_CHOICE ,
		};

	public:
		LevelStage();

		bool   changeLevel( int level ){ return changeLevelInternal( level , false ); }
	protected:
		bool   onInit();
		void   onEnd();
		void   onUpdate( long time );
		void   onRender( float dFrame );
		bool   onWidgetEvent( int event , int id , GWidget* ui );
		bool   onKey( unsigned key , bool isDown );
		bool   onMouse( MouseMsg const& msg );

		void   tick();
		void   updateFrame( int frame );

		void   cleanupGameData();
		bool   loadGameData( int idxPackage , bool loadState );
		bool   saveGameLevelState();
		bool   loadGameLevelState();

		Level& getCurLevel(){ return mScene.getLevel(); }
		bool   tryUnlockLevel();
		bool   changeLevelInternal( int level , bool haveChangeData , bool beCreateMode = false );
		void   drawStatusPanel( GWidget* widget );

	private:
		enum State
		{
			eLOCK     ,
			eUNSOLVED ,
			eSOLVED   ,
			eHAVE_SOLVED ,
		};

		GChoice* mLevelChoice;
		GButton* mNextLevelButton;
		GButton* mPrevLevelButton;
		int      mMinChoiceLevel;

		static int const MaxNumLevel = 50;
		Scene  mScene;
		int    mIndexGamePackage;
		int    mIndexLevel;

		int    mNumLevelSolved;

		Level* mLevelStorage[ MaxNumLevel ];
		State  mLevelState[ MaxNumLevel ];
		int    mNumLevel;
	};

}//namespace Chromatron

#endif // CmtStage_h__
