#ifndef TDGame_h__
#define TDGame_h__

#include "GameModule.h"
#include "TDController.h"


#define TOWER_DEFEND_NAME "TowerDefend"

namespace TowerDefend
{
	class GameModule : public IGameModule
	{
	public:
		char const*     getName()      {  return TOWER_DEFEND_NAME;  }
		GameController& getController(){  return mController;  }
		SettingHepler*  createSettingHelper( SettingHelperType type )
		{
			return NULL;
		}
		StageBase*      createStage( unsigned id );
		bool            getAttribValue( AttribValue& value ){ return false; }

		void beginPlay( StageManager& manger, StageModeType modeType ) override;
		virtual void    deleteThis(){ delete this; }

	private:
		Controller    mController;
	};

}//namespace TowerDefend


#endif // TDGame_h__
