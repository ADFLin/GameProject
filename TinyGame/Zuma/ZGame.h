#ifndef ZGame_h__
#define ZGame_h__

#include "GameModule.h"

#define ZUMA_NAME "Zuma"
namespace Zuma
{
	class GameCore;

	class GameModule : public IGameModule
	{
	public:
		GameModule();
		virtual bool  initialize(){ return true; }
		virtual void  cleanup(){}
		virtual void  enter();
		virtual void  exit(); 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
	public:
		virtual char const*       getName(){ return ZUMA_NAME; }
		virtual InputControl&     getInputControl(){ return IGameModule::getInputControl(); }
		virtual StageBase*        createStage( unsigned id );
		virtual SettingHepler*    createSettingHelper( SettingHelperType type ){ return NULL; }
		virtual bool              queryAttribute( GameAttribute& value );

		GameCore* mCore;
	};
}
#endif // ZGame_h__
