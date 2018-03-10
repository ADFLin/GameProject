#include "ProjectPCH.h"
#include "GameFramework.h"

#include "ConsoleSystem.h"
#include "PhysicsSystem.h"
#include "CUISystem.h"
#include "TResManager.h"

#include "CEntity.h"
#include "GameObject.h"

#include "CFWorld.h"
#include "common.h"

#include "IScriptTable.h"
#include "RenderSystem.h"
#include "LevelEditor.h"
#include "CSceneLevel.h"


GameFramework::GameFramework()
	:mObjectSystem( new GameObjectSystem )
	,mSystem(nullptr)
{

}

GameFramework::~GameFramework()
{

}


class CSystem : public ISystem
{
public:
	bool  init( SSystemInitParams& params )
	{
		mEvt.renderSystem = new RenderSystem;

		if ( !mEvt.renderSystem->init( params ) )
			return false;

		mEvt.entitySystem = new EntitySystem;
		mEvt.scriptSystem = new ScriptSystem;

		return true;
	}
	void release()
	{
		delete mEvt.renderSystem;
		delete mEvt.entitySystem;
		delete mEvt.scriptSystem;
	}
};



bool GameFramework::init( SSystemInitParams& params )
{
	mSystem = new CSystem;
	if ( !mSystem->init( params ) )
		return false;

	gEnv->framework = this;

	if ( !ConsoleSystem::Get().init() )
		return false;
	if ( !PhysicsSystem::Get().initSystem() )
		return false;
	if ( !CUISystem::Get().initSystem() )
		return false;

	mLevelManager.reset( new LevelManager );

	if ( params.isDesigner )
	{
		mLevelEditor.reset( new LevelEditor( mLevelManager ) );
		if ( !mLevelEditor->init( params ) )
			return false;
	}

	return true;
}

void GameFramework::update( long time )
{
	g_GlobalVal.updateFrame();

	EntitySystem* entitySystem = getSystem()->getEntitySystem();
	entitySystem->update( time );
	//getGameObjectSystem()->update( time );
}

