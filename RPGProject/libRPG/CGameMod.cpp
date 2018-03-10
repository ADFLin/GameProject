#include "ProjectPCH.h"
#include "CGameMod.h"

#include "GameFramework.h"

#include "CNPCBase.h"
#include "CHero.h"
#include "CRigidObject.h"

#include "TResManager.h"
#include "TItemSystem.h"
#include "TEventSystem.h"
#include "CUISystem.h"

namespace
{
	template< class T >
	class TGameObjectFactory : public IGameObjectExtensionFactory
	{
	public:
		virtual T*   create(){  return new T();  }
		virtual void release(){ delete this; }
		static  TGameObjectFactory* createFactory(){ return new TGameObjectFactory;  }
	};
	template< class ES >
	ES* getEntityScript()
	{
		static ES es;
		return &es;
	}
}


bool CGameMod::init( GameFramework* gameFramework )
{
	mFramework = gameFramework;

	TResManager::Get().init();

	if ( !TResManager::Get().loadRes( RES_DATA_PATH ) )
	{
		LogErrorF( "can't load Res" );
		TResManager::Get().addDefultData();
		TResManager::Get().saveRes( RES_DATA_PATH );
	}

	if ( !TItemManager::Get().loadItemData( ITEM_DATA_PATH ) )
		return false;


	registerFactory();
	return true;
}


void CGameMod::registerFactory()
{
	GameObjectSystem* OBS = gEnv->framework->getGameObjectSystem();

#define OBJECT_FACTORY( OBJECT_NAME , OEJECT_CLASS , ENTITY_NAME , ENTITY_SCRIPT , SCRIPT_TABLE )\
	{\
		EntityClassDesc desc;\
		desc.entityScript = getEntityScript< ENTITY_SCRIPT >();\
		desc.scriptTable  = SCRIPT_TABLE;\
		desc.name         = ENTITY_NAME;\
		OBS->registerExtension( OBJECT_NAME  , TGameObjectFactory<OEJECT_CLASS>::createFactory() , &desc );\
	}

	OBJECT_FACTORY( "NPC"         , CNPCBase     , "Actor:NPC"   , CActorScript , NULL )
	OBJECT_FACTORY( "Hero"        , CHero        , "Actor:Hero"  , CActorScript , NULL )
	OBJECT_FACTORY( "RigidObject" , CRigidObject , "RigidObject" , CRigidScript , NULL )
}

int CGameMod::update( long time )
{
	TEventSystem::Get().updateFrame();
	CUISystem::Get().updateFrame();
	mFramework->update( time );
	return 1;
}
