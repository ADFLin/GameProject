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
		InputControl&   getInputControl(){  return mInputControl;  }
		SettingHepler*  createSettingHelper( SettingHelperType type )
		{
			return NULL;
		}
		StageBase*      createStage( unsigned id );
		bool            queryAttribute( GameAttribute& value ){ return false; }

		void beginPlay( StageManager& manger, EGameStageMode modeType ) override;

	private:
		CInputControl    mInputControl;
	};

}//namespace TowerDefend


#endif // TDGame_h__
