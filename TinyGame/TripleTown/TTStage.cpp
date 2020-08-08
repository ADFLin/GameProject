#include "TTStage.h"

#include "RHI/RHIGraphics2D.h"
#include "InputManager.h"


namespace TripleTown
{
	using namespace Render;

	bool LevelStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		srand(generateRandSeed());

		if( !mScene.init() )
			return false;

		mScene.setupLevel(mLevel);
		mLevel.setPlayerData(*this);
		onRestart(true);

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart");
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Show preview texture"), mScene.bShowPreviewTexture);
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Show TexAtlas"), mScene.bShowTexAtlas);
		return true;
	}

	void LevelStage::onRender(float dFrame)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		RHISetShaderProgram(commandList, nullptr);
		RHISetFrameBuffer(commandList, nullptr);
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

		float scaleItem = 0.8f;

		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(100 / 255.f, 100 / 255.f, 100 / 255.f, 1), 1, 1.0);

		struct Vertex
		{
			Vector2 pos;
			Color4f color;
		};

		Vertex vertices[] =
		{
			{Vector2(0,0) , Color4f(1,1,1,1)},
			{Vector2(0,400) , Color4f(1,1,1,1)},
			{Vector2(400,400) , Color4f(1,1,1,1)},
			{Vector2(400,0) , Color4f(1,1,1,1)},
		};

		int indices[] = { 0, 1, 2, 0 , 2 , 3 };
		mScene.render();

		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		g.beginRender();

		RenderUtility::SetBrush(g, EColor::White);

		SimpleTextLayout layout;
		RenderUtility::SetFont(g, FONT_S10);
		g.setTextColor(Color3ub(255, 255, 0));
		layout.show(g, "Points = %d", mPlayerPoints);
		layout.show(g, "Coins = %d", mPlayerCoins);

		if( mScene.bShowPreviewTexture )
		{
			layout.show(g, "Preview Texture = %s", mFileIterator.getFileName());
		}

		g.endRender();

	}

	void LevelStage::onEnd()
	{
		BaseClass::onEnd();
	}

	void LevelStage::onRestart(bool beInit)
	{
		if( beInit )
		{
			mPlayerCoins = 0;
			mPlayerPoints = 0;
			mLevel.setupLand(LT_STANDARD, true);
		}
		else
		{
			mLevel.restart();
		}
	}

	bool LevelStage::onMouse(MouseMsg const& msg)
	{
		mScene.setLastMousePos(msg.getPos());
		if( msg.onLeftDown() )
		{
			if( InputManager::Get().isKeyDown(EKeyCode::Control) )
			{
				ObjectId id = mScene.getObjectId(msg.getPos());
				if( id != OBJ_NULL )
				{
					mLevel.setQueueObject(id);
				}
			}
			else
			{
				mScene.click(msg.getPos());
			}
			
			return false;
		}
		else if( msg.onMoving() )
		{
			mScene.peekObject(msg.getPos());
		}
		return true;
	}

	bool LevelStage::onKey(KeyMsg const& msg)
	{
		if( !msg.isDown() )
			return true;

		switch(msg.getCode())
		{
		case EKeyCode::S: mLevel.setQueueObject(OBJ_BEAR); return false;
		case EKeyCode::A: mLevel.setQueueObject(OBJ_GRASS); return false;
		case EKeyCode::Q: mLevel.setQueueObject(OBJ_CRYSTAL); return false;
		case EKeyCode::W: mLevel.setQueueObject(OBJ_ROBOT); return false;
		case EKeyCode::X:
			if( mFileIterator.haveMore() )
			{
				mFileIterator.goNext();
				mScene.loadPreviewTexture(mFileIterator.getFileName());
			}
			break;
		}
		return true;
	}

}