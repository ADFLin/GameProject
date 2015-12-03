#ifndef BMGame_h__
#define BMGame_h__

#include "GamePackage.h"
#include "GameControl.h"

#define BOMBER_MAN_NAME "BomberMan"

namespace BomberMan
{

	class CGamePackage : public IGamePackage
	{

	public:
		virtual bool  create(){ return true; }
		virtual void  cleanup(){}
		virtual bool  load(){ return true; }
		virtual void  release(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void  enter( StageManager& manger );
	public:

		virtual char const*           getName(){ return BOMBER_MAN_NAME; }
		virtual GameController&       getController(){ return mController;  }
		virtual GameSubStage*         createSubStage( unsigned id );
		virtual StageBase*            createStage( unsigned id ){ return NULL; }
		virtual SettingHepler*        createSettingHelper( SettingHelperType type );
		virtual bool                  getAttribValue( AttribValue& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }


		SimpleController mController;
	};


}//namespace BomberMan


#endif // BMGame_h__
