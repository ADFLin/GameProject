#include "TTStage.h"

#include "RHI/RHIGraphics2D.h"
#include "InputManager.h"
#include "RHI/D3D11Command.h"

namespace TripleTown
{
	using namespace Render;

	D3D11System* mD3D11System;

	bool LevelStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		VERIFY_RETURN_FALSE(Global::GetDrawEngine().initializeRHI(RHITargetName::D3D11));

		srand(generateRandSeed());

		GameWindow& window = ::Global::GetDrawEngine().getWindow();
		if (GRHISystem->getName() == RHISytemName::D3D11)
		{
			mD3D11System = static_cast<D3D11System*>(GRHISystem);
			//::Global::GetDrawEngine().bUsePlatformBuffer = true;
		}

		if( !mScene.init() )
			return false;

		mScene.setupLevel(mLevel);
		mLevel.setPlayerData(*this);
		onRestart(true);


		FileSystem::FindFiles("TripleTown", ".tex", mFileIterator);
		mScene.loadPreviewTexture(mFileIterator.getFileName());

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart");
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Show preview texture"), mScene.bShowPreviewTexture);
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Show TexAtlas"), mScene.bShowTexAtlas);
		return true;
	}

	void LevelStage::onRender(float dFrame)
	{
		GameWindow& window = ::Global::GetDrawEngine().getWindow();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetViewport(commandList, 0, 0, window.getWidth(), window.getHeight());
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

		TRenderRT<RTVF_XY_CA>::DrawIndexed(commandList, EPrimitive::TriangleList, vertices, 4, indices , 6);

		RenderUtility::SetBrush(g, EColor::White);
		g.setPen(Color3ub(255, 0,0));
		g.drawRect(Vec2i(200, 200), Vec2i(100, 100));
		g.drawCircle(Vector2(400, 400), 20);


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

		if (GRHISystem->getName() == RHISytemName::D3D11)
		{
			if (::Global::GetDrawEngine().bUsePlatformBuffer)
			{
				Graphics2D& g = Global::GetGraphics2D();
				mD3D11System->mSwapChain->BitbltToDevice(g.getRenderDC());
			}
		}
	}

	void LevelStage::onEnd()
	{
		::Global::GetDrawEngine().shutdownRHI();
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
		case EKeyCode::Z:
			{
				auto nextTarget = ::Global::GetDrawEngine().isOpenGLEnabled() ? RHITargetName::D3D11 : RHITargetName::OpenGL;
				cleanupRHI();
				
				::Global::GetDrawEngine().shutdownRHI(false);
				::Global::GetDrawEngine().initializeRHI(nextTarget);

				initializeRHI();
			}
			break;
		}
		return true;
	}

	void LevelStage::cleanupRHI()
	{
		mScene.releaseResource();
	}

	void LevelStage::initializeRHI()
	{
		mScene.loadResource();
	}

}