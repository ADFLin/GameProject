#ifndef BMGame_h__
#define BMGame_h__

#include "GameModule.h"
#include "GameControl.h"

#define BOMBER_MAN_NAME "BomberMan"

namespace BomberMan
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
		virtual void beginPlay(StageManager& manger, EGameStageMode modeType) override;
	public:

		virtual char const*           getName(){ return BOMBER_MAN_NAME; }
		virtual InputControl&         getInputControl(){ return mInputControl;  }
		virtual StageBase*            createStage(unsigned id);
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  queryAttribute( GameAttribute& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }


		DefaultInputControl mInputControl;
	};


}//namespace BomberMan


#endif // BMGame_h__
