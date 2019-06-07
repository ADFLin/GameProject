#include "TTStage.h"

#include "GLGraphics2D.h"
#include "InputManager.h"

namespace TripleTown
{

	bool LevelStage::onInit()
	{

		::Global::GUI().cleanupWidget();

		if( !::Global::GetDrawEngine().startOpenGL() )
			return false;


		srand(generateRandSeed());

		GameWindow& window = ::Global::GetDrawEngine().getWindow();

		if( !mScene.init() )
			return false;

		mScene.setupLevel(mLevel);
		mLevel.setPlayerData(*this);
		onRestart(true);


		FileSystem::FindFiles("TripleTown", ".tex", mIterator);
		mScene.loadPreviewTexture(mIterator.getFileName());

		auto frame = WidgetUtility::CreateDevFrame();
		frame->addButton( UI_RESTART_GAME , "Restart");
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
		g.endRender();
	}

	void LevelStage::onEnd()
	{
		::Global::GetDrawEngine().stopOpenGL();
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
			if( InputManager::Get().isKeyDown(Keyboard::eCONTROL) )
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

	bool LevelStage::onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return true;

		switch( key )
		{
		case 'S': mLevel.setQueueObject(OBJ_BEAR); return false;
		case 'A': mLevel.setQueueObject(OBJ_GRASS); return false;
		case 'Q': mLevel.setQueueObject(OBJ_CRYSTAL); return false;
		case 'W': mLevel.setQueueObject(OBJ_ROBOT); return false;
		case Keyboard::eX:
			if( mIterator.haveMore() )
			{
				mIterator.goNext();
				mScene.loadPreviewTexture(mIterator.getFileName());
			}
			break;
		}
		return true;
	}

}