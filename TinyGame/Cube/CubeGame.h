#ifndef CubeGame_h__
#define CubeGame_h__

#include "GameModule.h"

namespace Cube
{

	class GameModule : public IGameModule
	{
	public:
		char const*     getName(){  return "CubeWorld"; }

		GameController& getController(){  return IGameModule::getController(); }
		//SettingHepler*  createSettingHelper( SettingHelperType type );
		//ReplayTemplate* createReplayTemplate( unsigned version );
		//GameSubStage*   createSubStage( unsigned id );
		StageBase*      createStage( unsigned id );
		bool            getAttribValue( AttribValue& value );

		virtual void beginPlay( StageManager& manger, StageModeType modeType ) override;
		virtual void enter();
		virtual void exit();
		virtual void deleteThis(){ delete this; }
	};


}

#endif // CubeGame_h__
