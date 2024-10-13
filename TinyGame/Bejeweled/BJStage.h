#ifndef BJStage_h__
#define BJStage_h__

#include "BJScene.h"
#include "StageBase.h"
#include "GameRenderSetup.h"

namespace Bejeweled
{
	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		bool onInit();
		void postInit();
		void onUpdate(GameTimeSpan deltaTime);
		void onRender( float dFrame );

		void restart();
		void tick();
		void updateFrame( int frame );

		MsgReply onMouse( MouseMsg const& msg );
		MsgReply onKey(KeyMsg const& msg);

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
		{
			systemConfigs.bWasUsedPlatformGraphics = true;
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}


		bool onWidgetEvent(int event, int id, GWidget* ui) override;


		bool setupRenderResource(ERenderSystem systemName) override;

	protected:
		Scene mScene;
	};

}




#endif // BJStage_h__
