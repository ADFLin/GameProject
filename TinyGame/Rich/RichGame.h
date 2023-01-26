#ifndef RichGame_h__
#define RichGame_h__

#include "GameModule.h"

namespace Rich
{

	class GameModule : public IGameModule
	{
	public:


		void  enter() override {}
		void  exit() override {}
		//
		virtual void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
	public:
		virtual char const*           getName(){ return "Rich"; }
		virtual InputControl&         getInputControl(){ return IGameModule::getInputControl(); }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }
	};

}//namespace Rich

#endif // RichGame_h__
