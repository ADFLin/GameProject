#ifndef TResManager_h__
#define TResManager_h__

#include "common.h"
#include "Singleton.h"
#include "TCombatSystem.h"
#include "TAblilityProp.h"
#include "PhysicsSystem.h"
#include "NavMesh.h"
#include "CActorModel.h"


typedef unsigned ResId;

//enum  ModelType;
typedef uint32 ModelType;
struct DamageInfo;
class TResManager;
class eF3DFX;

class CActorModel;
class CObjectModel;

#define RES_ERROR_ID 0xffffffff

#define RES_DATA_PATH    "Data/Res.txt"

#define ACTOR_DATA_DIR   "Data/NPC/"
#define OBJECT_DATA_DIR  "Data/Model/"
#define MAP_DATA_DIR     "Data/Map/"
#define ITEM_DATA_DIR    "Data/Model/Items"

#define FX_DATA_DIR         "Data/Fx/"
#define FX_MODEL_DATA_DIR   "Data/Fx/Textures"
#define FX_TEXTURE_DATA_DIR "Data/Fx/Models"


//enum RenderGroup
//{
//	RG_TERRAIN = 0,
//	RG_SHADOW ,
//	RG_VOLUME_LIGHT ,
//	RG_DYNAMIC_OBJ ,
//	RG_ACTOR = RG_DYNAMIC_OBJ,
//	RG_FX ,
//	RG_GROUP_NUM ,
//};


enum ResType
{
	
	RES_ACTOR_MODEL ,
	RES_OBJ_MODEL   ,
	RES_EQUIP_MODEL ,

	RES_SCENE_MAP   ,

	RES_AUDIO   ,
	RES_3DAUDIO ,

	RES_TEXTURE ,
	RES_TYPE_NUM ,
};


enum ResState
{
	RS_LOAD    ,
	RS_LOADING ,
	RS_EMPTY   ,
};

struct IResBase
{
	IResBase( ResType type )
	{
		resType = type;
		resID   = RES_ERROR_ID;
		state   = RS_EMPTY;
	}

	virtual ~IResBase(){}
	virtual bool load() = 0;
	virtual void release() = 0;

	bool    isLoaded(){ return state == RS_LOAD;  }

	ResId    resID;
	ResState state;

	String   resName;
	ResType  resType;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int version );
};



//struct TAudioResData : public ResBase
//{
//	TAudioResData():TResData( RES_AUDIO ){}
//
//	FnAudio  audio;
//	virtual bool loadRes();
//};

//struct T3DAudioResData : public ResBase
//{
//	T3DAudioResData():TResData( RES_3DAUDIO ){}
//	Fn3DAudio audio;
//	virtual bool loadRes();
//};


//struct TFxResData : public ResBase
//{
//	TFxResData();
//
//	eF3DFX* fx;
//	virtual bool loadRes();
//};

struct EquipModelRes : public IResBase
{
	EquipModelRes();
	CFObject*  model;
	//virtual 
	bool load();
	//virtual
	void release();
};

struct PhysicsData
{
	PhysicsData()
	{
		mass  = 0;
		shape = 0;
	}

	~PhysicsData()
	{
		PhysicsSystem::Get().destroyShape( shape );
	}

	void setDefult()
	{
		mass = 1.0;
		PhyShapeParams info;
		info.setBoxShape( Vec3D( 50 , 50 , 50 ) );
		PhysicsSystem::Get().destroyShape( shape );
		shape = PhysicsSystem::Get().createShape( info );
	}

	float   mass;
	PhyShapeHandle shape;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int version );
};

class ObjectModelRes : public IResBase
{
public:
	ObjectModelRes();
	CFObject*   model;
	PhysicsData phyData;
	//virtual
	bool load();
	//virtual
	void release();

	CObjectModel* createModel( CFScene* scene );
protected:
	void initData()
	{
		phyData.setDefult();
	}
	friend class TResManager;
	friend class TObject;

public:
	template < class Archive >
	void serialize( Archive & ar , const unsigned int version );

};

enum ActivityType;
class TActor;

class ActorModelRes : public IResBase
{
public:
	ActorModelRes();

	CFActor*    model;
	Vec3D       modelOffset;
	PhysicsData phyData;

	static int const MAX_POSTABLE_NUM = ANIM_TYPE_NUM;
	int   animTable[MAX_POSTABLE_NUM];
	//virtual
	bool load();
	//virtual
	void release();

	CActorModel* createModel( CFScene* scene );
protected:
	void initData();
	friend class TResManager;

public:
	template < class Archive >
	void serialize( Archive & ar , const unsigned int version );
};


class TStatNavMesh;
class TriangleMeshData;
class CSceneLevel;

class SceneMapRes : public IResBase
{
public:
	SceneMapRes():IResBase( RES_SCENE_MAP )
	{
		scene = nullptr;
		terrainMesh = nullptr;
		terrainShape = 0;

		user = nullptr;
	}

	bool load();

	//virtual
	void release(){  releaseInternal();  }

	CFScene*           scene;
	NavMesh            navMesh;
	TriangleMeshData*  terrainMesh;
	PhyShapeHandle     terrainShape;

	CSceneLevel*       user;
private:
	void releaseInternal();

};

class TResVisitCallBack
{
public:
	virtual ~TResVisitCallBack(){}
	virtual void visit( IResBase* data ) = 0;
};


class TResManager : public SingletonT<TResManager>
{
public:
	TResManager();
	void init();
	bool saveRes( char const* name );
	bool loadRes( char const* name );


	unsigned  queryResID( char const* name , ResType type );
	unsigned  getResID( ResType type , char const* name );
	
	IResBase*  findResData( ResType type , char const* name );
	void       freeRes( unsigned resID );
	IResBase*  getResData( ResType type ,char const* name , bool needLoad = true );
	void       visitRes( ResType type , TResVisitCallBack& callback );

	template< class T >
	T* getResDataT( ResType type ,char const* name , bool needLoad = true ){ return static_cast< T* >( getResData( type , name , needLoad ) ); }
	
	size_t    getResNum( ResType type ){  return m_resIDMap[type].size();  }

	ResId     getActorModelID( char const* name ){  return getResID( RES_ACTOR_MODEL , name );  }
	ResId     getObjectModelID( char const* name ){  return getResID( RES_OBJ_MODEL , name );  }

	ActorModelRes*  getActorModel( char const* name ){  return getResDataT< ActorModelRes >( RES_ACTOR_MODEL , name );  }
	ObjectModelRes* getObjectModel( char const* name ){  return getResDataT< ObjectModelRes >( RES_OBJ_MODEL , name );  }

	void       setCurScene( CFScene* scene){  mCurScene = scene;  }
	CFScene*   getBGScene(){ return mBGScene; }
	CFWorld*   getWorld(){ return mWorld; } 

	bool      checkResLoaded( IResBase* resData , bool beWait = true );
	void      addDefultData();


protected:

	
	ResId  addNewActorModel( char const* name );
	ResId  resigterRes( ResType type , IResBase* data );
	ResId  checkResID( ResType type , char const* name , bool needLoad );
	

	typedef std::map< ModelType , unsigned > ModelIDMap;

	struct StrCmp
	{
		bool operator()(const char* s1, const char* s2) const
		{
			return strcmp(s1, s2) < 0;
		}
	};
	typedef std::map< char const* , ResId , StrCmp > ResIDMap;
	typedef std::vector< IResBase* >  ResDataVec;

	CFWorld* mWorld;
	CFScene* mBGScene;
	CFScene* mCurScene;

	ModelIDMap  m_ModelIDMap;
	ResIDMap    m_resIDMap[ RES_TYPE_NUM ];

	std::vector< unsigned >    m_FreeResID;
	ResDataVec                 m_resDataVec;
};




#endif // TResManager_h__