#include "TTStage.h"

#include "GLGraphics2D.h"
#include "InputManager.h"

namespace TripleTown
{

	bool LevelStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		VERIFY_RETURN_FALSE(Global::GetDrawEngine().initializeRHI(RHITargetName::OpenGL));


		srand(generateRandSeed());

		GameWindow& window = ::Global::GetDrawEngine().getWindow();

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
		mScene.render();

		GLGraphics2D& g = Global::GetRHIGraphics2D();

		g.beginRender();
		SimpleTextLayout layout;
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
		}
		return true;
	}

}