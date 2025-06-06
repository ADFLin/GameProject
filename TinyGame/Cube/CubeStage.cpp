#include "CubePCH.h"
#include "CubeStage.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

namespace Cube
{

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		mCamera.setPos( Vec3f( -10 , 0 , 0 ) );


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