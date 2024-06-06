#include "DDStage.h"
#include "CardDraw.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"

#include "GameGUISystem.h"
#include "RenderDebug.h"


namespace DouDizhu
{
	using namespace Render;

	LevelStage::LevelStage()
	{

	}

	void LevelStage::buildServerLevel(GameLevelInfo& info)
	{

	}

	void LevelStage::setupScene(IPlayerManager& playerManager)
	{
		::Global::GUI().cleanupWidget();
	}

	void LevelStage::setupLocalGame(LocalPlayerManager& playerManager)
	{

	}

	void LevelStage::onRestart(bool beInit)
	{

	}

	void LevelStage::onEnd()
	{

	}

	void LevelStage::onRender(float dFrame)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1, 1, 0);


		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		g.setBrush(Color3f(1,1,1));
		g.setBlendAlpha(1.0f);
		cardDraw->draw(::Global::GetIGraphics2D(), Vec2i(0, 0), Card(Card::eCLUBS, 0));

		g.drawTexture(*mUVTexture, Vec2i(300, 300), Vec2i(300, 300));
		g.drawTexture(*mTestTexture, Vec2i(  0, 300), Vec2i(300, 300));

		g.endRender();
	}

	void LevelStage::tick()
	{

	}

	ERenderSystem LevelStage::getDefaultRenderSystem()
	{
		return ERenderSystem::D3D11;
	}

	bool LevelStage::setupRenderResource(ERenderSystem systemName)
	{

		cardDraw = ICardDraw::Create(ICardDraw::eWin7);

		VERIFY_RETURN_FALSE(mUVTexture = RHIUtility::LoadTexture2DFromFile("Texture/UVChecker.png"));
		mTestTexture = &cardDraw->getResource()->getTexture();

		GTextureShowManager.registerTexture("UVChecker", mUVTexture);
		GTextureShowManager.registerTexture("Test", mTestTexture);
		return true;

	}

	void LevelStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		if (cardDraw)
		{
			cardDraw->release();
		}
	}

}