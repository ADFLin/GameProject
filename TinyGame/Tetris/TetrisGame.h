#ifndef TetrisGame_h__
#define TetrisGame_h__

#include "GameModule.h"
#include "GameControl.h"
#include "GameRenderSetup.h"

#define TETRIS_NAME "Tetris"



namespace Tetris
{
	class RecordManager;
	RecordManager& GetRecordManager();

	union GameInfoData;	
	typedef DefaultInputControl CInputControl;

	class GameModule : public IGameModule
		             , public IGameRenderSetup
	{
	public:
		GameModule();
		char const*     getName(){  return TETRIS_NAME;  }
		InputControl&   getInputControl(){  return mInputControl;  }
		SettingHepler*  createSettingHelper( SettingHelperType type );
		ReplayTemplate* createReplayTemplate( unsigned version );
		StageBase*      createStage( unsigned id );
		bool            queryAttribute( GameAttribute& value );

		void  beginPlay( StageManager& manger, EGameMode modeType ) override;
		void  endPlay() override;
		void  enter() override;
		void  exit() override;

		ERenderSystem getDefaultRenderSystem() override;
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;
		bool setupRenderResource(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;




	private:
		CInputControl  mInputControl;
	};

}//namespace Tetris


#endif // TetrisGame_h__
