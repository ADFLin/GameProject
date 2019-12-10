#ifndef CmtStage_h__
#define CmtStage_h__

#include "StageBase.h"

#include "CmtScene.h"

namespace Chromatron
{

	class LevelStage : public StageBase
	{
		using BaseClass = StageBase;

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

		bool   changeLevel( int indexLevel ){ return changeLevelInternal(indexLevel, false ); }
	protected:
		bool   onInit() override;
		void   onEnd() override;
		void   onUpdate( long time ) override;
		void   onRender( float dFrame ) override;
		bool   onWidgetEvent( int event , int id , GWidget* ui ) override;
		bool   onKey(KeyMsg const& msg) override;
		bool   onMouse( MouseMsg const& msg ) override;

		void   tick();
		void   updateFrame( int frame );

		void   cleanupGameData();
		bool   loadGameData( int idxPackage , bool loadState );
		bool   saveGameLevelState();
		bool   loadGameLevelState();

		Level& getCurLevel(){ return mScene.getLevel(); }
		bool   tryUnlockLevel();
		bool   changeLevelInternal( int indexLevel, bool haveChangeData , bool beCreateMode = false );
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

		struct LevelData
		{
			Level* level;
			State  state;
		};
		LevelData mLevels[MaxNumLevel];
		int    mNumLevel;
	};

}//namespace Chromatron

#endif // CmtStage_h__
