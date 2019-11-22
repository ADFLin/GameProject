#ifndef CmtGame_h__
#define CmtGame_h__

#include "GameModule.h"

#include "CmtDefine.h"

namespace Chromatron
{

	class GameModule : public IGameModule
	{
	public:
		virtual bool  initialize(){ return true; }
		virtual void  cleanup(){}
		virtual void  enter(){}
		virtual void  exit(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void beginPlay( StageManager& manger, StageModeType modeType ) override;
	public:
		virtual char const*           getName(){ return CHROMATRON_NAME;   }
		virtual GameController&       getController(){ return IGameModule::getController(); }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return NULL; }
		virtual bool                  queryAttribute( GameAttribute& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }
	};


}//namespace Chromatron

#endif // CmtGame_h__
