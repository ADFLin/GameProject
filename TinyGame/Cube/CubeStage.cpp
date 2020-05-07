#include "CubePCH.h"
#include "CubeStage.h"

#include "GameGlobal.h"
#include "GameGUISystem.h"

namespace Cube
{

	bool TestStage::onInit()
	{
		VERIFY_RETURN_FALSE(Global::GetDrawEngine().initializeRHI(RHITargetName::OpenGL));

		GameWindow& window = Global::GetDrawEngine().getWindow();

		::Global::GUI().cleanupWidget();

		mCamera.setPos( Vec3f( -10 , 0 , 0 ) );


		Level::init();

		mLevel = new Level;
		mLevel->setupWorld();

		mScene = new Scene( window.getWidth() , window.getHeight() );
		mScene->changeWorld( mLevel->getWorld() );

		restart();
		return true;
	}

}//namespace Cube