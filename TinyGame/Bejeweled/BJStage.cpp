#include "TinyGamePCH.h"
#include "BJStage.h"

#include "DrawEngine.h"
#include "GameGUISystem.h"
#include "Widget/WidgetUtility.h"

#include "RHI/RHICommand.h"

namespace Bejeweled
{
	using namespace Render;

	bool TestStage::onInit()
	{
		int len = Scene::CellLength * Scene::BoardSize;
		Vec2i pos = ( ::Global::GetScreenSize() - Vec2i( len , len ) ) / 2;
		pos.y -= 15;
		mScene.setBoardPos( pos );

		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_RESTART_GAME, "Restart");
		frame->addCheckBox("Auto Play", mScene.bAutoPlay);
		frame->addCheckBox("Draw Debug", mScene.bDrawDebugForce);
		return true;
	}

	void TestStage::postInit()
	{
		restart();
	}

	void TestStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);

		int frame = long(deltaTime) / gDefaultTickTime;
		for( int i = 0 ; i < frame ; ++i )
			tick();

		updateFrame( frame );
	}

	void TestStage::onRender( float dFrame )
	{
		IGraphics2D& g = Global::GetIGraphics2D();
		g.beginRender();
		mScene.render( g );
		g.endRender();
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

	bool TestStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		if (!BaseClass::onWidgetEvent(event, id, ui))
			return false;
		switch (id)
		{
		case UI_RESTART_GAME:
			restart();
			return false;
		}
		return true;
	}

	bool TestStage::setupRenderResource(ERenderSystem systemName)
	{
		mScene.initializeRHI();
		return true;
	}

}//namespace Bejeweled
