#ifndef CubeGame_h__
#define CubeGame_h__

#include "GameModule.h"

namespace Cube
{

	class GameModule : public IGameModule
	{
	public:
		char const*     getName(){  return "CubeWorld"; }

		InputControl& getInputControl(){  return IGameModule::getInputControl(); }
		//SettingHepler*  createSettingHelper( SettingHelperType type );
		//ReplayTemplate* createReplayTemplate( unsigned version );
		//GameSubStage*   createSubStage( unsigned id );
		StageBase*      createStage( unsigned id );
		bool            queryAttribute( GameAttribute& value );

		virtual void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
		virtual void enter();
		virtual void exit();

	};


}

#endif // CubeGame_h__
