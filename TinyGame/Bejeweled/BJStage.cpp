#include "TinyGamePCH.h"
#include "BJStage.h"

#include "DrawEngine.h"
#include "GameGUISystem.h"
#include "WidgetUtility.h"

namespace Bejeweled
{
	bool TestStage::onInit()
	{
		int len = Scene::CellLength * Scene::BoardSize;
		Vec2i pos = ( Global::getDrawEngine()->getScreenSize() - Vec2i( len , len ) ) / 2;
		mScene.setBoardPos( pos );

		::Global::GUI().cleanupWidget();
		WidgetUtility::CreateDevFrame();

		restart();
		return true;
	}

	void TestStage::onUpdate( long time )
	{
		BaseClass::onUpdate( time );

		int frame = time / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void TestStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();
		mScene.render( g );
	}

	void TestStage::restart()
	{
		mScene.restart();
	}

	void TestStage::tick()
	{
		mScene.tick();
	}

	void TestStage::updateFrame( int frame )
	{
		mScene.updateFrame( frame );
	}

	bool TestStage::onMouse( MouseMsg const& msg )
	{
		if ( !BaseClass::onMouse( msg ) )
			return false;
		return mScene.procMouseMsg( msg );
	}

	bool TestStage::onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return false;

		switch( key )
		{
		case Keyboard::eR: restart(); break;
		default:
			mScene.procKey( key , isDown );
		}
		return false;
	}

}//namespace Bejeweled
