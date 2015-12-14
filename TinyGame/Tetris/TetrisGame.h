#ifndef TetrisGame_h__
#define TetrisGame_h__

#include "GamePackage.h"
#include "GameControl.h"

#define TETRIS_NAME "Tetris"

class RecordManager;

namespace Tetris
{
	RecordManager& getRecordManager();
	union GameInfoData;	
	typedef SimpleController MyController;

	class CGamePackage : public IGamePackage
	{
	public:
		CGamePackage();
		char const*     getName(){  return TETRIS_NAME;  }
		GameController& getController(){  return mController;  }
		SettingHepler*  createSettingHelper( SettingHelperType type );
		ReplayTemplate* createReplayTemplate( unsigned version );
		GameSubStage*   createSubStage( unsigned id );
		StageBase*      createStage( unsigned id );
		bool            getAttribValue( AttribValue& value );

		virtual void    beginPlay( GameType type, StageManager& manger );
		virtual void    enter();
		virtual void    exit(); 
		virtual void    deleteThis(){ delete this; }
	private:
		MyController  mController;
	};

}//namespace Tetris


#endif // TetrisGame_h__
