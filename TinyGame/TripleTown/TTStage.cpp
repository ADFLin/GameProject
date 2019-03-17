#include "TTStage.h"

namespace TripleTown
{

	bool LevelStage::onInit()
	{

		::Global::GUI().cleanupWidget();

		if( !::Global::GetDrawEngine().startOpenGL() )
			return false;

		GameWindow& window = ::Global::GetDrawEngine().getWindow();

		if( !mScene.init() )
			return false;

		mScene.setupLevel(mLevel);
		onRestart(true);

		WidgetUtility::CreateDevFrame();
		return true;
	}

}