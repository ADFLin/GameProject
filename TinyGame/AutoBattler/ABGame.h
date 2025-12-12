#ifndef ABGame_h__
#define ABGame_h__

#include "GameModule.h"
#include "ABDefine.h"
#include "GameControl.h"

namespace AutoBattler
{
	class GameModule : public IGameModule
	{
	public:
		char const*     getName() override {  return AUTO_BATTLER_NAME;  }
		InputControl&   getInputControl() override {  return mInputControl;  }
		SettingHepler*  createSettingHelper( SettingHelperType type ) override;
		StageBase*      createStage( unsigned id ) override;
		bool            queryAttribute( GameAttribute& value ) override;

		void beginPlay( StageManager& manger, EGameStageMode modeType ) override;

		void  doRelease() { delete this; }
	private:
		DefaultInputControl    mInputControl;
	};

}//namespace AutoBattler

#endif // ABGame_h__
