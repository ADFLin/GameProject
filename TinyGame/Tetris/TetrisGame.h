#ifndef TetrisGame_h__
#define TetrisGame_h__

#include "GameModule.h"
#include "GameControl.h"

#define TETRIS_NAME "Tetris"



namespace Tetris
{
	class RecordManager;
	RecordManager& GetRecordManager();

	union GameInfoData;	
	typedef SimpleController MyController;

	class GameModule : public IGameModule
	{
	public:
		GameModule();
		char const*     getName(){  return TETRIS_NAME;  }
		GameController& getController(){  return mController;  }
		SettingHepler*  createSettingHelper( SettingHelperType type );
		ReplayTemplate* createReplayTemplate( unsigned version );
		StageBase*      createStage( unsigned id );
		bool            getAttribValue( AttribValue& value );

		virtual void    beginPlay( StageManager& manger, StageModeType modeType ) override;
		virtual void    enter();
		virtual void    exit(); 
		virtual void    deleteThis(){ delete this; }
	private:
		MyController  mController;
	};

}//namespace Tetris


#endif // TetrisGame_h__
