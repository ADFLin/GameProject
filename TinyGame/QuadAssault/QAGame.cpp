#include "QAGame.h"

#include "Game.h"
#include "MenuStage.h"

#include "GlobalVariable.h"
#include "LevelStage.h"

#include "StageBase.h"
#include "GameGUISystem.h"
#include "GameRenderSetup.h"

#include "Widget/WidgetUtility.h"

#include "RHI/ShaderManager.h"



EXPORT_GAME_MODULE( QuadAssaultModule )


class ProxyStage : public StageBase
	             , public IGameRenderSetup
{
	typedef StageBase BaseClass;
public:

	Game* mGame = nullptr;

	virtual bool onInit() override
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();

		Vec2i screenSize = ::Global::GetScreenSize();
		mGame = new Game();
		mGame->mGraphics2D = &::Global::GetRHIGraphics2D();
		if( !mGame->init("config.txt" , screenSize) )
			return false;

#if 1
		mGame->addStage(new MenuStage(), false);
#else
		auto& levelInfo = mGame->getPlayingLevel();
		levelInfo.mapFile = "test.map";
		levelInfo.level.fileName = "test.lv";
		levelInfo.levelIndex = 0;
		mGame->addStage(new LevelStage(), false);
#endif
		return true;
	}

	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::None;
	}

	bool setupRenderResource(ERenderSystem systemName) override
	{
		if (!mGame->initRenderSystem())
			return false;

		return true;
	}

	virtual void onEnd() override
	{
		mGame->exit();
	}

	virtual void onUpdate(long time) override
	{
		if (mGame == nullptr)
			return;
		mGame->tick(float(time) / 1000);
	}

	virtual void onRender(float dFrame) override
	{
		if (mGame == nullptr)
			return;
		mGame->render();
	}

	virtual MsgReply onChar(unsigned code) override
	{
		if (mGame == nullptr)
			return MsgReply::Unhandled();
		return mGame->onChar(code);
	}

	virtual MsgReply onMouse(MouseMsg const& msg) override
	{
		if (mGame == nullptr)
			return MsgReply::Unhandled();
		return mGame->onMouse(msg);
	}

	virtual MsgReply onKey(KeyMsg const& msg) override
	{
		if (mGame == nullptr)
			return MsgReply::Unhandled();
		return mGame->onKey(msg);
	}

	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{		
		systemConfigs.screenWidth = 1024;
		systemConfigs.screenHeight = 768;
	}

};

char const* QuadAssaultModule::getName()
{
	return "QuadAssault";
}

InputControl& QuadAssaultModule::getInputControl()
{
	return IGameModule::getInputControl();
}

bool QuadAssaultModule::queryAttribute(GameAttribute& value)
{
	switch( value.id )
	{
	case ATTR_SINGLE_SUPPORT:
		value.iVal = 1;
		return true;
	}
	return false;
}

void QuadAssaultModule::beginPlay(StageManager& manger, EGameStageMode modeType)
{
	ProxyStage* stage = new ProxyStage;
	manger.setNextStage(stage);
}

void QuadAssaultModule::endPlay()
{
	
}
