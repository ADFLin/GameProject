#ifndef CEntity_h__
#define CEntity_h__

#include <cstdlib>
#include <cassert>
#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 

#include <vector>

class CEntity;
class IEntityClass;
class IScriptTable;

enum CompGroup
{
	COMP_UNKNOWN  ,
	COMP_SPATIAL  ,
	COMP_RENDER   ,
	COMP_PHYSICAL ,
	COMP_GAME_OBJECT ,
	COMP_GROUP_NUM ,
};

enum EntityEventType
{
	ENTITY_EVT_UPDATE ,
	ENTITY_EVT_XFORM  ,

};

struct EntityEvent
{
	EntityEventType type;
};


struct GameObjectInitHelper;

struct EntitySpawnParams
{
	IEntityClass* entityClass;
	Vec3D  pos;
	Quat   q;
	float  scale;

	GameObjectInitHelper* helper;
	IScriptTable* propertyTable;
	EntitySpawnParams()
		:entityClass( NULL )
		,pos(0,0,0)
		,q( 0,0,0,1 )
		,helper( NULL )
		,propertyTable( NULL ){}
};


class IComponent
{
public:

	enum
	{
		eLogic  = BIT(0) ,
		eRemove = BIT(1) ,
	};

	IComponent( CompGroup group = COMP_UNKNOWN );
	virtual ~IComponent(){}
	virtual bool init( CEntity* entity , EntitySpawnParams& params ){ return true; }
	virtual void update( long time ){}
	virtual void onLink(){}
	virtual void onUnlink(){}

	virtual void onEvent( EntityEvent const& event ){}

	CEntity* getEntity() const { return mOwner; }
	void     setPriority( unsigned value )
	{  
		mPriority = value; 
	}


	int     getPriority() const { return mPriority; }
	void    enableLogical(){  mFlag |= eLogic; }

	
private:
	CompGroup mGroup;
	unsigned  mPriority;
	unsigned  mFlag;
	CEntity*  mOwner;
	friend class CEntity;
};

enum EntityUpdatePolicy
{
	EUP_ALWAYS ,
	EUP_NEVER  ,
};

enum EntityFlag
{
	EF_DESTROY = 0x001, 
	EF_FREEZE  = 0x002,

	EF_GLOBAL  = 0x010,
	EF_LEVEL   = 0x020,
	EF_TEMP    = 0x040,

	EF_ZONE_MAK = 0x0f0 ,
	EF_MANAGING = 0x100 ,
};


class CEntity
{
public:
	CEntity();

	void removeComponent( IComponent* comp )
	{
		assert( comp->mOwner == this );
		comp->mFlag |= IComponent::eRemove;
	}
	void addComponent( IComponent* comp )
	{
		 addComponentInternal( comp );
	}
	void          update( long time );
	IComponent*   getComponent( CompGroup group );
	template< class T >
	T*            getComponentT( CompGroup group )
	{
#ifdef _DEBUG
		assert( dynamic_cast< T* >( getComponent( group ) ) );
#endif
		return static_cast< T* >( getComponent( group ) );
	}

	IEntityClass* getClass(){ return mClass; }

	void          setUpdatePolicy( EntityUpdatePolicy policy ){  mUpdatePolicy = policy;  }
	IScriptTable* getScriptTable(){ return mScriptTable;  }
	void          sendEvent( EntityEvent const& event );
public:
	unsigned getFlag(){ return mFlag; }
	void     addFlag( EntityFlag flag ){ mFlag |= flag; }
	void     removeFlag( EntityFlag flag ){ mFlag &= ~flag; }

private:
	void     addComponentInternal( IComponent* comp );

	typedef std::vector< IComponent* > ComponentList;

	IComponent*        mCompments[ COMP_GROUP_NUM ];

	unsigned           mFlag;
	ComponentList      mCompList;
	IEntityClass*      mClass;
	EntityUpdatePolicy mUpdatePolicy;
	IScriptTable*      mScriptTable;
	friend class EntitySystem;
};

class IScriptTable;
struct GameObjectInitHelper;


class IEntityConstructor
{
public:
	virtual bool construct( CEntity& entity , EntitySpawnParams& param ) = 0;
	virtual void destroy( CEntity& entity ){}
};

class IEntityScript
{
public:
	virtual void  createComponent( CEntity& entity , EntitySpawnParams& params ) = 0;
};

struct EntityClassDesc
{
	char const*         name;
	IEntityScript*      entityScript;
	IScriptTable*       scriptTable;
};

class IEntityClass
{
public:
	virtual void                release() = 0;
	virtual IEntityConstructor* getConstructor() = 0;
	virtual IEntityScript*      getEntityScript() = 0;
	virtual char const*         getName() const = 0;
	virtual IScriptTable*       getScriptTable() = 0;
};

class EntitySystem
{
public:

	bool           registerClass( IEntityClass* entityClass );
	IEntityClass*  findClass( char const* name );
	CEntity*       spawnEntity( EntitySpawnParams& params );

	void           update( long time );

private:

	struct StrCmp
	{
		bool operator()( char const* s1 , char const* s2 ) const { return strcmp( s1 , s2 ) < 0;  }
	};
	typedef std::list< CEntity* > EntityList;
	typedef std::map< char const* , IEntityClass* , StrCmp > EntityClassMap;
	EntityList     mEntityList;
	EntityClassMap mClassMap;
};


#endif // CEntity_h__
