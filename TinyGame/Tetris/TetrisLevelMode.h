#ifndef TetrisLevelMode_h__
#define TetrisLevelMode_h__

#include "TetrisLevelManager.h"
#include "TetrisGameInfo.h"

#include "TVector2.h"
typedef TVector2< int > Vec2i;

class  CLPlayerManager;
struct Record;

namespace Tetris
{
	class GameWorld;

	class NormalModeData : public ModeData
	{
	public:
		short   idxCurLvG;
		int     score;
		short   scoreCombo;
		int     gravityLevel;

		void   calcScore( int numLayer );
		void   changeNextLevel( Level* level );
		void   updateGravityValue( Level* level );
		void   updateScoreCombo( int numLayer );
	};


	class ChallengeModeData : public NormalModeData
	{
	public:
		void reset( Mode* mode , LevelData& lvData  , bool beInit );
	};


	class ChallengeMode : public Mode
	{
	public:
		ChallengeMode()
		{
			mInfo.startGravityLevel = 0;
		}
		ChallengeMode( NormalModeInfo const& info )
			:mInfo( info )
		{

		}

		void        init();
		void        getGameInfo( GameInfo& info ){  info.modeNormal = mInfo; }
		ModeID      getModeID(){ return MODE_TS_CHALLENGE; } 
		void        fireAction( LevelData& lvData , ActionTrigger& trigger );
		ModeData*   createModeData(){  return new ChallengeModeData;  }
		void        setupScene( unsigned flag );
		void        setupSingleGame( MyController& controller );
		void        onLevelEvent( LevelData& lvData , LevelEvent const& event );
		void        renderStats( GWidget* ui );
		int         markRecord( GamePlayer* player , RecordManager& manager );


		NormalModeInfo mInfo;

	};


	class PracticeModeData : public NormalModeData
	{
	public:
		void reset( Mode* mode , LevelData& data , bool beInit );
	};

	class PracticeMode : public Mode
	{
	public:
		enum
		{
			UI_GRAVITY_VALUE_SLIDER = UI_LEVEL_MODE_ID ,
			UI_ENTRY_DELAY_SLIDER ,
			UI_CLEAR_LAYER_SLIDER ,
			UI_LOCK_PIECE_SLIDER ,
			UI_MAP_SIZE_X_SLIDER ,
			UI_MAP_SIZE_Y_SLIDER ,
		};

		ModeID      getModeID(){ return MODE_TS_PRACTICE; } 
		void        init();
		void        doRestart( bool beInit );
		void        fireAction( LevelData& lvData , ActionTrigger& trigger );

		void        setupScene( unsigned flag );
		void        setupSingleGame( MyController& controller );
		void        onLevelEvent( LevelData& lvData , LevelEvent const& event );
		void        onGameOver(){}

		bool        onWidgetEvent( int event , int id , GWidget* ui );

		ModeData*   createModeData(){  return new PracticeModeData;  }

		void        renderStats( GWidget* ui );
		void        renderControl( GWidget* ui );

		int   mGravityValue;
		int   mMaxClearLayerNum;
	};


	class BattleModeData : public ModeData
	{
	public:
		int  numAddLayer;
		int  numWinRound;

		void reset( Mode* mode , LevelData& lvData , bool beInit );
		void checkAddLayer( Level* level );
	};

	class BattleMode : public Mode
	{
	public:
		ModeID      getModeID(){ return MODE_TS_BATTLE; } 
		void        init();
		void        doRestart( bool beInit ){}
		void        setupScene( unsigned flag );
		void        fireAction( LevelData& lvData , ActionTrigger& trigger );
		ModeData*   createModeData(){  return new BattleModeData;  }
		void        setupSingleGame( MyController& controller );
		bool        checkOver();
		void        renderStats( GWidget* ui );
		void        onLevelEvent( LevelData& lvData , LevelEvent const& event );
	};

}//namespace Tetris


#endif // TetrisLevelMode_h__
