#ifndef RichGame_h__
#define RichGame_h__

#include "GamePackage.h"

namespace Rich
{

	class GamePackage : public IGamePackage
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
