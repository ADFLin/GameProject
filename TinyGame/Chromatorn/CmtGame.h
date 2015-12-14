#ifndef CmtGame_h__
#define CmtGame_h__

#include "GamePackage.h"

#include "CmtDefine.h"

namespace Chromatron
{

	class CGamePackage : public IGamePackage
	{
	public:
		virtual bool  initialize(){ return true; }
		virtual void  cleanup(){}
		virtual void  enter(){}
		virtual void  exit(){} 
		virtual void  deleteThis(){ delete this; }
		//
		virtual void beginPlay( GameType type, StageManager& manger );
	public:
		virtual char const*           getName(){ return CHROMATRON_NAME;   }
		virtual GameController&       getController(){ return IGamePackage::getController(); }
		virtual GameSubStage*         createSubStage( unsigned id );
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return NULL; }
		virtual bool                  getAttribValue( AttribValue& value );

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return NULL; }
	};


}//namespace Chromatron

#endif // CmtGame_h__
