#include "TTStage.h"

#include "RHI/RHIGraphics2D.h"
#include "InputManager.h"


namespace TripleTown
{
	using namespace Render;

	bool LevelStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		srand(GenerateRandSeed());

		if( !mScene.init() )
			return false;

		mScene.setupLevel(mLevel);
		mLevel.setPlayerData(*this);
		onRestart(true);

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart");
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Show preview texture"), mScene.bShowPreviewTexture);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Show TexAtlas"), mScene.bShowTexAtlas);
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
			mLevel.setupLand(LT_TEST, false);
		}
		else
		{
			mLevel.restart();
		}
	}

	MsgReply LevelStage::onMouse(MouseMsg const& msg)
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
			
			return MsgReply::Handled();
		}
		else if( msg.onMoving() )
		{
			mScene.peekObject(msg.getPos());
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		auto SetQueueObject = [&](ObjectId id)
		{
			mLevel.setQueueObject(id);
			mScene.repeekObject();
		};
		if( msg.isDown() )
		{
			switch (msg.getCode())
			{
			case EKeyCode::S: SetQueueObject(OBJ_BEAR); return MsgReply::Handled();
			case EKeyCode::A: SetQueueObject(OBJ_GRASS); return MsgReply::Handled();
			case EKeyCode::Q: SetQueueObject(OBJ_CRYSTAL); return MsgReply::Handled();
			case EKeyCode::W: SetQueueObject(OBJ_ROBOT); return MsgReply::Handled();
			case EKeyCode::E: SetQueueObject(OBJ_NINJA); return MsgReply::Handled();
			case EKeyCode::X:
				if (mFileIterator.haveMore())
				{
					mFileIterator.goNext();
					mScene.loadPreviewTexture(mFileIterator.getFileName());
				}
				break;
			case EKeyCode::Up:   mLevel.undo(); return MsgReply::Handled();
			case EKeyCode::Down: mLevel.redo(); return MsgReply::Handled();
			case EKeyCode::N:
			case EKeyCode::M:
				{
					ObjectId const items[] =
					{
						OBJ_GRASS,
						OBJ_BUSH,
						OBJ_TREE,
						OBJ_HUT,
						OBJ_HOUSE,
						OBJ_MANSION,
						OBJ_CASTLE,
						OBJ_FLOATING_CASTLE,
						OBJ_TRIPLE_CASTLE,
					};


					int index = FindIndex(items, items + ARRAY_SIZE(items), mLevel.getQueueObject());
					if (index == INDEX_NONE)
					{
						SetQueueObject((msg.getCode() == EKeyCode::N) ? OBJ_GRASS : OBJ_TRIPLE_CASTLE );
					}
					else
					{
						if ((msg.getCode() == EKeyCode::N))
						{
							index = ( index - 1 + ARRAY_SIZE(items) ) % ARRAY_SIZE(items);
						}
						else
						{
							index = (index + 1) % ARRAY_SIZE(items);
						}

						SetQueueObject(items[index]);
					}
				}
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

}