#include "ProjectPCH.h"
#include "GameObject.h"
#include "GameFramework.h"

GameObject::GameObject( GameObjectSystem* system ) 
	:IComponent( COMP_GAME_OBJECT )
	,mObjectSystem( system )
{

}

GameObject::~GameObject()
{
	for( ExtensionVec::iterator iter = mExtensions.begin(); 
		iter != mExtensions.end() ; ++iter )
	{
		IGameObjectExtension* extension = (*iter);
		delete extension;
	}
}

IGameObjectExtension* GameObject::queryExtension( char const* extName )
{
	GameObjectSystem::ExtensionID id = mObjectSystem->getExtensionID( extName );
	if ( id == GameObjectSystem::InvialdExtensionID )
		return NULL;

	for( ExtensionVec::iterator iter = mExtensions.begin(); 
		iter != mExtensions.end(); ++iter )
	{
		IGameObjectExtension* ext = *iter;
		if ( ext->mID == id )
			return ext;
	}
	return NULL;
}

bool GameObject::init( CEntity* entity , EntitySpawnParams const& params )
{
	for( ExtensionVec::iterator iter = mExtensions.begin();
		iter != mExtensions.end() ; ++iter )
	{
		IGameObjectExtension* extension = *iter;
		extension->setGameObject( this );
		if ( !extension->init( this , *params.helper ) )
		{
			return false;
		}
		extension->postInit();
	}
	return true;
}

void GameObject::update( long time )
{
	for( ExtensionVec::iterator iter = mExtensions.begin();
		iter != mExtensions.end() ; ++iter )
	{
		IGameObjectExtension* extension = *iter;
		extension->update( time );
	}
}

IGameObjectExtension* GameObject::acquireExtension( char const* extName )
{



	return NULL;
}

GameObjectSystem::ExtensionID GameObjectSystem::getExtensionID( char const* extName )
{
	ExtensionMap::iterator iter = mExtensionMap.find( extName );
	if ( iter == mExtensionMap.end() )
		return InvialdExtensionID;
	return iter->second.id;
}


class CObjectEntityClass : public IEntityClass 
	                     , public IEntityConstructor
{

public:

	virtual void                release()        { delete this;  }
	virtual IEntityConstructor* getConstructor() { return this; }
	virtual IEntityScript*      getEntityScript(){ return entityScript; }
	virtual char const*         getName() const  { return className.c_str();  }
	virtual IScriptTable*       getScriptTable() { return scriptTable;  }

	virtual bool construct( CEntity& entity , EntitySpawnParams& params )
	{
		GameObjectSystem* OBS = gEnv->framework->getGameObjectSystem();

		char const* extNames[1];
		extNames[0] = extName.c_str();

		GameObject* gameObject = OBS->createObject( entity , extNames , 1 );

		if ( !gameObject )
			return false;
		if ( !gameObject->init( &entity , params ) )
			return false;
		return true;
	}
	virtual void destroy( CEntity& entity )
	{

	}

	String         extName;
	String         className;
	IEntityScript* entityScript;
	IScriptTable*  scriptTable;
};

bool GameObjectSystem::registerExtension( char const* extName , IGameObjectExtensionFactory* factory , EntityClassDesc* desc )
{
	ExtensionMap::iterator iter = mExtensionMap.find( extName );
	if ( iter != mExtensionMap.end() )
	{
		return false;
	}
	ExternsionInfo info;
	info.id      = ++mNextID;
	info.factory = factory;
	mExtensionMap.insert( std::make_pair( String( extName ) , info ) );

	if ( desc && !gEnv->entitySystem->findClass( desc->name ) )
	{
		CObjectEntityClass* objectClass = new CObjectEntityClass;
		objectClass->className    = desc->name;
		objectClass->entityScript = desc->entityScript;
		objectClass->scriptTable  = desc->scriptTable;
		objectClass->extName      = extName;
		gEnv->entitySystem->registerClass( objectClass );
	}
	return true;
}

GameObject* GameObjectSystem::createObject( CEntity& entity , char const* extNames[] , int num )
{
	GameObject* object = new GameObject( this );

	entity.addComponent( object );
	for( int i = 0 ; i < num ; ++i )
	{
		ExtensionMap::iterator iter = mExtensionMap.find( extNames[i] );
		if ( iter == mExtensionMap.end() )
			continue;
		ExternsionInfo& info = iter->second;
		IGameObjectExtension* extension =  info.factory->create();
		extension->mID = info.id;
		object->mExtensions.push_back( extension );
	}
	mObjectList.push_back( object );
	return object;
}

void GameObjectSystem::update( long time )
{
	for( ObjectList::iterator iter = mObjectList.begin();
		iter != mObjectList.end() ; ++iter )
	{
		GameObject* object = *iter;
		object->update( time );
	}
}

