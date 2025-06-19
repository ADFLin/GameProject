#include "CubePCH.h"
#include "CubeStage.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

namespace Cube
{

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		int posX = 0 * ChunkSize;
		int posY = 0 * ChunkSize;
		mCamera.setPos( Vec3f(posX, posY, 80 ) );
		mDebugCamera.setPos(Vec3f(0, 0, 80));

		Level::InitializeData();

		mLevel = new Level;
		mLevel->setupWorld();
		Vec2i screenSize = ::Global::GetScreenSize();
		mScene = new Scene(screenSize.x, screenSize.y);
		mScene->changeWorld( mLevel->getWorld() );

		restart();
		return true;
	}

	void TestStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 800;
	}

}//namespace Cube