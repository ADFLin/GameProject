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

	Game* mGame;

	virtual bool onInit() override
	{
		if( !BaseClass::onInit() )
			return false;

		Render::ShaderManager::Get().setBaseDir("QuadAssault/shader/");
		::Global::GUI().cleanupWidget();

		Vec2i screenSize = ::Global::GetScreenSize();
		mGame = new Game();
		mGame->mGraphics2D = &::Global::GetRHIGraphics2D();
		if( !mGame->init("config.txt" , screenSize , false ) )
			return false;

#if 1
		mGame->addStage(new MenuStage(), false);
#else
		gLevelFileName = "test.lv";
		gMapFileName = "test.map";
		gIdxCurLevel = 0;
		mGame->addStage(new LevelStage(), false);
#endif
		return true;
	}

	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::OpenGL;
	}


	bool setupRenderSystem(ERenderSystem systemName) override
	{
		if (!mGame->initRenderSystem())
			return false;

		return true;
	}


	virtual void onEnd() override
	{
		mGame->exit();
		Render::ShaderManager::Get().setBaseDir("");
	}

	virtual void onUpdate(long time) override
	{
		mGame->tick(float(time) / 1000);
	}

	virtual void onRender(float dFrame) override
	{
		mGame->render();
	}

	virtual MsgReply onChar(unsigned code) override
	{
		return mGame->onChar(code);
	}

	virtual MsgReply onMouse(MouseMsg const& msg) override
	{
		return mGame->onMouse(msg);
	}

	virtual MsgReply onKey(KeyMsg const& msg) override
	{
		return mGame->onKey(msg);
	}
};


void QuadAssaultModule::deleteThis()
{
	delete this;
}

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
