#ifndef CubeGame_h__
#define CubeGame_h__

#include "GameInstance.h"

namespace Cube
{

	class GameInstance : public IGameInstance
	{
	public:
		char const*     getName(){  return "CubeWorld"; }

		GameController& getController(){  return IGameInstance::getController(); }
		//SettingHepler*  createSettingHelper( SettingHelperType type );
		//ReplayTemplate* createReplayTemplate( unsigned version );
		//GameSubStage*   createSubStage( unsigned id );
		StageBase*      createStage( unsigned id );
		//bool            getAttribValue( AttribValue& value );

		virtual void beginPlay( StageModeType type, StageManager& manger );
		virtual void enter();
		virtual void exit();
		virtual void deleteThis(){ delete this; }
	};


}

#endif // CubeGame_h__
