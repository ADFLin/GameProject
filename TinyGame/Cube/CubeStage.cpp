#include "CubePCH.h"
#include "CubeStage.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

namespace Cube
{

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		mCamera.setPos( Vec3f( 0 , 0 , 80 ) );
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

}//namespace Cube