#include "QAGame.h"

#include "Game.h"
#include "MenuStage.h"

#include "StageBase.h"

EXPORT_GAME_MODULE( QuadAssaultModule )


class ProxyStage : public StageBase
{
public:

	Game* mGame;

	virtual bool onInit() override
	{
		mGame = new Game();
		if( !mGame->init("config.txt") )
			return false;
		mGame->addStage(new MenuStage(), false);
		return true;
	}

	virtual void onEnd() override
	{
		mGame->exit();
	}

	virtual void onUpdate(long time) override
	{
		mGame->tick(float(time) / gDefaultTickTime);
	}


	virtual void onRender(float dFrame) override
	{
		mGame->render();
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

GameController& QuadAssaultModule::getController()
{
	return IGameModule::getController();
}

bool QuadAssaultModule::getAttribValue(AttribValue& value)
{
	switch( value.id )
	{
	case ATTR_SINGLE_SUPPORT:
		value.iVal = 1;
		return true;
	}
	return false;
}

void QuadAssaultModule::beginPlay(StageModeType type, StageManager& manger)
{
	ProxyStage* stage = new ProxyStage;
	manger.setNextStage(stage);
}

void QuadAssaultModule::endPlay()
{
	
}
