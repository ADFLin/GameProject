#ifndef RichGame_h__
#define RichGame_h__

#include "GameModule.h"
#include "GameRenderSetup.h"

namespace Rich
{

	class GameModule : public IGameModule
		             , public IGameRenderSetup
	{
	public:


		void  enter() override {}
		void  exit() override {}
		//
		void beginPlay( StageManager& manger, EGameStageMode modeType ) override;
		void endPlay() override;

		bool queryAttribute(GameAttribute& value) override;

		//IGameRenderSetup
		ERenderSystem getDefaultRenderSystem()
		{
			return ERenderSystem::OpenGL;
		}
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.bWasUsedPlatformGraphics = true;
		}
		bool setupRenderResource(ERenderSystem systemName) override
		{
			return true;
		}
		void preShutdownRenderSystem(bool bReInit) override
		{

		}

		//~IGameRenderSetup
	public:
		virtual char const*           getName(){ return "Rich"; }
		virtual InputControl&         getInputControl(){ return IGameModule::getInputControl(); }
		virtual StageBase*            createStage( unsigned id );
		virtual SettingHepler*        createSettingHelper( SettingHelperType type ){ return nullptr; }

		//old replay version
		virtual ReplayTemplate*       createReplayTemplate( unsigned version ){ return nullptr; }
	};

}//namespace Rich

#endif // RichGame_h__
