#include "TinyGamePCH.h"
#include "BJStage.h"

#include "DrawEngine.h"
#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

namespace Bejeweled
{
	bool TestStage::onInit()
	{
		int len = Scene::CellLength * Scene::BoardSize;
		Vec2i pos = ( ::Global::GetScreenSize() - Vec2i( len , len ) ) / 2;
		mScene.setBoardPos( pos );

		::Global::GUI().cleanupWidget();

		restart();
		WidgetUtility::CreateDevFrame();
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
		Graphics2D& g = Global::GetGraphics2D();
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

	MsgReply TestStage::onMouse( MouseMsg const& msg )
	{
		MsgReply reply = mScene.procMouseMsg( msg );
		if (reply.isHandled())
			return reply;

		return BaseClass::onMouse(msg);
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			default:
				mScene.procKey(msg);
			}
		}
		return BaseClass::onKey(msg);
	}

}//namespace Bejeweled
