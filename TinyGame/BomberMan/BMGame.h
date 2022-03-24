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
		void  startupModule() override {}
		void  shutdownModule() override {}
		void  enter() override {}
		void  exit() override {}
		void  deleteThis() override { delete this; }
		//
		void beginPlay(StageManager& manger, EGameStageMode modeType) override;
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
