#ifndef TetrisGame_h__
#define TetrisGame_h__

#include "GameInstance.h"
#include "GameControl.h"

#define TETRIS_NAME "Tetris"



namespace Tetris
{
	class RecordManager;
	RecordManager& getRecordManager();

	union GameInfoData;	
	typedef SimpleController MyController;

	class GameInstance : public IGameInstance
	{
	public:
		GameInstance();
		char const*     getName(){  return TETRIS_NAME;  }
		GameController& getController(){  return mController;  }
		SettingHepler*  createSettingHelper( SettingHelperType type );
		ReplayTemplate* createReplayTemplate( unsigned version );
		StageBase*      createStage( unsigned id );
		bool            getAttribValue( AttribValue& value );

		virtual void    beginPlay( StageModeType type, StageManager& manger );
		virtual void    enter();
		virtual void    exit(); 
		virtual void    deleteThis(){ delete this; }
	private:
		MyController  mController;
	};

}//namespace Tetris


#endif // TetrisGame_h__
