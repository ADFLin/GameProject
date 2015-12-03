#ifndef CubeGame_h__
#define CubeGame_h__

#include "GamePackage.h"

namespace Cube
{

	class CGamePackage : public IGamePackage
	{
	public:
		char const*     getName(){  return "CubeWorld"; }

		GameController& getController(){  return IGamePackage::getController(); }
		//SettingHepler*  createSettingHelper( SettingHelperType type );
		//ReplayTemplate* createReplayTemplate( unsigned version );
		//GameSubStage*   createSubStage( unsigned id );
		StageBase*      createStage( unsigned id );
		//bool            getAttribValue( AttribValue& value );

		virtual void    enter( StageManager& manger );
		virtual bool    load();
		virtual void    release();
		virtual void    deleteThis(){ delete this; }
	};


}

#endif // CubeGame_h__
