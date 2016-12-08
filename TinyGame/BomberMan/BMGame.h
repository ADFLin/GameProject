#ifndef BMGame_h__
#define BMGame_h__

#include "GameInstance.h"
#include "GameControl.h"

#define BOMBER_MAN_NAME "BomberMan"

namespace BomberMan
{

	class GameInstance : public IGameInstance
	{
	public:
		virtual bool  initialize(){ return true; }
		virtual void  cleanup(){}
		virtual void  enter(){}
		virtual void  exit(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void beginPlay( StageModeType type, StageManager& manger );
	public:

		virtual char const*           getName(){ return BOMBER_MAN_NAME; }
		virtual GameController&       getController(){ return mController;  }
		virtual StageBase*            createStage(unsigned id);
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  getAttribValue( AttribValue& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }


		SimpleController mController;
	};


}//namespace BomberMan


#endif // BMGame_h__
