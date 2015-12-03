#ifndef GameObject_h__
#define GameObject_h__

#include "CEntity.h"

class GameObjectSystem;
class IGameObjectExtension;

class GameObject : public IComponent
{
public:
	GameObject( GameObjectSystem* system );
	~GameObject();

	bool    init( CEntity* entity , EntitySpawnParams const& params );
	IGameObjectExtension*   queryExtension( char const* extName );
	IGameObjectExtension*   acquireExtension( char const* extName );

	void     update( long time );
private:
	friend class GameObjectSystem;
	typedef std::vector< IGameObjectExtension* > ExtensionVec;
	ExtensionVec      mExtensions;
	GameObjectSystem* mObjectSystem;
};

class IGameObjectExtensionFactory
{
public:
	virtual IGameObjectExtension* create() = 0;
	virtual void release() = 0;
};

struct EntityClassDesc;

class GameObjectSystem
{
public:
	GameObjectSystem()
	{
		mNextID = 0;
	}
	typedef unsigned ExtensionID;
	static  ExtensionID const InvialdExtensionID = ~ExtensionID(0);

	
	ExtensionID getExtensionID( char const* extName );
	bool        registerExtension( char const* extName , IGameObjectExtensionFactory* factory , EntityClassDesc* desc );

	void        update( long time );
	GameObject* createObject( CEntity& entity , char const* extNames[] , int num );
private:
	struct ExternsionInfo 
	{
		ExtensionID id;
		IGameObjectExtensionFactory* factory;
	};

	struct StrCmp
	{
		bool operator()( char const* s1 , char const* s2 ) const { return strcmp( s1 , s2 ) < 0;  }
	};
	typedef std::map< String , ExternsionInfo > ExtensionMap;
	typedef std::list< GameObject* > ObjectList;

	
	ObjectList    mObjectList;
	ExtensionMap  mExtensionMap;
	ExtensionID   mNextID;
};

class IGameObjectExtension
{
public:
	CEntity*     getEntity() const { return mGameObject->getEntity(); }
	GameObject*  getGameObject(){ return mGameObject;  }

	virtual bool init( GameObject* gameObj , GameObjectInitHelper const& helper ){ return true; }
	virtual void postInit(){}
	virtual void release(){}
	virtual void update( long time ){}
private:
	void         setGameObject( GameObject* gameObj ){  mGameObject = gameObj;  }
	friend class GameObjectSystem;
	friend class GameObject;
	typedef GameObjectSystem::ExtensionID ExtensionID;
	ExtensionID  mID;
	GameObject*  mGameObject;
};


#endif // GameObject_h__
