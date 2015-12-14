#ifndef RichGame_h__
#define RichGame_h__

#include "GamePackage.h"

namespace Rich
{

	class GamePackage : public IGamePackage
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
		virtual char const*           getName(){ return "Rich"; }
		virtual GameController&       getController(){ return IGamePackage::getController(); }
		virtual GameSubStage*         createSubStage( unsigned id ){ return nullptr; }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }
	};

}//namespace Rich

#endif // RichGame_h__
