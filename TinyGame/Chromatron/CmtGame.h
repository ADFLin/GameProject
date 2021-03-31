#ifndef CmtGame_h__
#define CmtGame_h__

#include "GameModule.h"

#include "CmtDefine.h"

namespace Chromatron
{

	class GameModule : public IGameModule
	{
	public:
		bool  initialize() override{ return true; }
		void  cleanup() override{}
		void  enter() override{}
		void  exit() override{} 
		void  deleteThis() override{ delete this; }
		//
		void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
	public:
		char const*           getName() override{ return CHROMATRON_NAME;   }
		GameController&       getController() override{ return IGameModule::getController(); }
		StageBase*            createStage( unsigned id ) override;
		SettingHepler*        createSettingHelper( SettingHelperType type ) override{ return nullptr; }
		bool                  queryAttribute( GameAttribute& value ) override;

		//old replay version
		ReplayTemplate*       createReplayTemplate( unsigned version ) override{ return nullptr; }
	};


}//namespace Chromatron

#endif // CmtGame_h__
