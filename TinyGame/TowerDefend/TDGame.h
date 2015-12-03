#ifndef TDGame_h__
#define TDGame_h__

#include "GamePackage.h"
#include "TDController.h"


#define TOWER_DEFEND_NAME "TowerDefend"

namespace TowerDefend
{
	class CGamePackage : public IGamePackage
	{
	public:
		char const*     getName()      {  return TOWER_DEFEND_NAME;  }
		GameController& getController(){  return mController;  }
		SettingHepler*  createSettingHelper( SettingHelperType type )
		{
			return NULL;
		}
		GameSubStage*   createSubStage( unsigned id );
		bool            getAttribValue( AttribValue& value ){ return false; }

		void            enter( StageManager& manger );
		virtual void    deleteThis(){ delete this; }

	private:
		Controller    mController;
	};

}//namespace TowerDefend


#endif // TDGame_h__
