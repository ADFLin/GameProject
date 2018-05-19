#include "ProjectPCH.h"
#include "TResManager.h"

#include "RenderSystem.h"
#include "CFWorld.h"
#include "CFScene.h"

#include <iostream>
#include "serialization.h"



#include "TriangleMeshData.h"

template < class Archive >
void PhysicsData::serialize( Archive & ar , const unsigned int version )
{
	ar & mass;
	boost::serialization::serializeColShape( ar , shape , Archive::is_loading() );
}

template < class Archive >
void IResBase::serialize( Archive & ar , const unsigned int version )
{
	ar & resType & resName;
}

template < class Archive >
void ObjectModelRes::serialize( Archive & ar , const unsigned int version )
{
	ar & boost::serialization::base_object<IResBase>(*this);
	ar & phyData;
}

template < class Archive >
void ActorModelRes::serialize( Archive & ar , const unsigned int version )
{
	ar & boost::serialization::base_object<IResBase>(*this);
	ar & phyData;
	ar & animTable & modelOffset;
}

namespace boost{
namespace serialization{

template < class Archive >
void serialize( Archive & ar , PhyShapeParams& info )
{
	ar & info.type;
	switch( info.type )
	{
	case CSHAPE_BOX:
		ar & info.box.x;
		ar & info.box.y;
		ar & info.box.z;
		break;
	case CSHAPE_SPHERE:
		ar & info.sphere.radius;
		break;
	case CSHAPE_CYLINDER:
		ar & info.cylinder.radius;
		ar & info.cylinder.height;
		break;
	case CSHAPE_TRIANGLE_MESH:
		break;
	}
}

template < class Archive >
void serializeColShape( Archive & ar , PhyShapeHandle& shape , true_type )
{
	PhyShapeParams info;
	boost::serialization::serialize( ar , info );
	if ( info.type != CSHAPE_TRIANGLE_MESH )
		shape = PhysicsSystem::Get().createShape( info );
	else
		shape = 0;
}

template < class Archive >
void serializeColShape( Archive & ar , PhyShapeHandle& shape , false_type )
{
	PhyShapeParams info;
	PhysicsSystem::Get().getShapeParamInfo( shape , info );
	boost::serialization::serialize( ar , info );
}

} // namespace serialization 
} // namespace boost




static char const* ResTypStr[] =
{
	"RES_OBJECT" ,
	"RES_ACTOR" ,
	"RES_FX"    ,
	"RES_MODEL" ,
	"RES_LEVEL" ,

	"RES_TEXTURE" ,

	"RES_AUDIO" ,
	"RES_3DAUDIO" ,
};

TResManager::TResManager()
{


}

void TResManager::init()
{
	mWorld   = gEnv->renderSystem->getCFWorld();
	mBGScene = mWorld->createScene( 1 );
	mCurScene = NULL;

	//m_fyWorld.SetAudioPath( "Data/Audio");
	//m_fyWorld.SetShaderPath( "Data/Shaders" );
}


ResId  TResManager::resigterRes( ResType type , IResBase* data )
{
	unsigned id;
	if ( m_FreeResID.size() != 0 )
	{
		id = m_FreeResID.back();
		m_resDataVec[id] = data;
	}
	else 
	{
		id = m_resDataVec.size();
		m_resDataVec.push_back( data );	
	}
	data->resID = id;
	m_resIDMap[ type ].insert( std::make_pair( data->resName.c_str() , ResId(id) ) );

	return ResId( id );
}

//eF3DFX* TResManager::cloneFx( char const* name )
//{
//	unsigned id = checkResID( RES_FX , name );
//	TFxResData* data = ( TFxResData*) m_resDataVec[ id ];
//
//	m_fyBGScene.SetCurrentRenderGroup( RG_FX );
//
//	eF3DFX* newFx =  data->fx->Clone();
//
//	newFx->SwitchScene( m_fyCurScene.Object() );
//	newFx->Reset();
//
//	m_fyBGScene.SetCurrentRenderGroup( 0 );
//
//	return newFx;
//}


//OBJECTid TResManager::cloneModel( char const* name ,bool beCopy)
//{
//	unsigned id = checkResID( RES_MODEL , name );
//	StaticModelRes* data = ( StaticModelRes*) m_resDataVec[ id ];
//
//	FnObject model;
//	model.Object( data->flyID );
//	FnObject obj;
//
//	m_fyBGScene.SetCurrentRenderGroup( RG_DYNAMIC_OBJ );
//
//	obj.Object( model.Instance( beCopy ) );
//	obj.ChangeScene( m_fyCurScene.Object() );
//	
//
//	return obj.Object();
//}

//CFObject* TResManager::cloneObject( char const* name )
//{
//	unsigned id = checkResID( RES_OBJECT , name );
//	ObjectModelRes* data = (ObjectModelRes*) m_resDataVec[ id ];
//
//	FnObject model;
//	model.Object( data->flyID );
//	FnObject obj;
//
//	m_fyBGScene.SetCurrentRenderGroup( RG_DYNAMIC_OBJ );
//	obj.Object( model.Instance( FALSE ) );
//	obj.ChangeScene( m_fyCurScene.Object() );
//
//	return obj.Object();
//}

//AUDIOid TResManager::cloneAudio( char const* name )
//{
//	unsigned id = checkResID( RES_AUDIO, name );
//	TAudioResData* data = (TAudioResData*) m_resDataVec[ id ];
//
//	return data->audio.Instance();
//}


//OBJECTid TResManager::clone3DAudio( char const* name )
//{
//	unsigned id = checkResID( RES_3DAUDIO, name );
//	T3DAudioResData* data = (T3DAudioResData*) m_resDataVec[ id ];
//
//	assert( data->audio.GetType() == AUDIO );
//
//	Fn3DAudio audio;
//	audio.Object( data->audio.Instance() );
//	audio.ChangeScene( m_fyCurScene.Object() );
//	audio.LoadSound( (char*)data->resName.c_str() );
//
//	return audio.Object();
//}

EquipModelRes::EquipModelRes() 
	:IResBase( RES_EQUIP_MODEL )
{
	model = nullptr;
}


ObjectModelRes::ObjectModelRes() 
	:IResBase( RES_OBJ_MODEL )
{
	model = nullptr;
}

ActorModelRes::ActorModelRes()
	:IResBase( RES_ACTOR_MODEL )
{
	model = nullptr;
	modelOffset.setValue(0,0,0);
}


IResBase* TResManager::findResData( ResType type , char const* name )
{
	ResIDMap::iterator iter = m_resIDMap[ type ].find( name );

	if ( iter != m_resIDMap[ type ].end() )
	{
		return m_resDataVec[ iter->second ];
	}
	return NULL;
}

unsigned TResManager::getResID( ResType type , char const* name )
{
	if ( IResBase* data = findResData( type , name ) )
	{
		if ( checkResLoaded( data ) )
			return data->resID;
	}
	return RES_ERROR_ID;
}

bool TResManager::checkResLoaded( IResBase* resData , bool beWait )
{
	switch( resData->state )
	{
	case RS_LOAD:
		return true;
	case RS_LOADING:
		if ( !beWait )
			return false;

	case RS_EMPTY:
		{
			bool result = resData->load();
			if ( result )
			{
				resData->state = RS_LOAD;
			}
			else
			{
				LogError("Can't load Res : type = %s , name = %s" , 
					ResTypStr[ resData->resType ] , 
					resData->resName.c_str() );
			}
			return result;
		}
		
	}
	return false;
}




unsigned TResManager::queryResID( char const* name , ResType type )
{
	ResIDMap::iterator iter = m_resIDMap[ type ].find( name );
	if ( iter != m_resIDMap[ type ].end() )
	{
		return iter->second;
	}
	return RES_ERROR_ID;
}

void TResManager::visitRes( ResType type , TResVisitCallBack& callback )
{
	for ( ResIDMap::iterator iter = m_resIDMap[ type ].begin();
		iter != m_resIDMap[ type ].end() ; ++iter )
	{
		IResBase* data = m_resDataVec[ iter->second ];
		callback.visit( data );
	}
}

IResBase* TResManager::getResData( ResType type ,char const* name , bool needLoad )
{
	unsigned id = checkResID( type , name , needLoad );

	if ( id != RES_ERROR_ID )
		return m_resDataVec[ id ];

	return NULL;
}

unsigned TResManager::checkResID( ResType type , char const* name ,  bool needLoad )
{
	ResId id = getResID( type , name );

	if ( id != RES_ERROR_ID )
		return id;

	IResBase* data = NULL;
	switch( type )
	{
	case RES_ACTOR_MODEL:
		return addNewActorModel( name );
	case RES_OBJ_MODEL: data = new ObjectModelRes; break;
	case RES_SCENE_MAP: data = new SceneMapRes; break;
	case RES_EQUIP_MODEL: data = new EquipModelRes; break;
	//case RES_FX:      data = new TFxResData;      break;
	//case RES_AUDIO:   data = new TAudioResData;   break;
	//case RES_3DAUDIO: data = new T3DAudioResData; break;
	}

	assert( data );
	data->resName = name;

	if ( needLoad )
		checkResLoaded( data );

	return resigterRes( type , data );
}

bool TResManager::saveRes( char const* name )
{
	try 
	{
		std::ofstream fs(name);
		boost::archive::text_oarchive ar(fs);

		ar.register_type(static_cast<ActorModelRes *>(NULL));
		ar.register_type(static_cast<ObjectModelRes *>(NULL));

		struct SaveResCallBack : public TResVisitCallBack
		{
			SaveResCallBack( boost::archive::text_oarchive& ar )
				:ar( ar ){}
			void visit( IResBase* data )
			{
				ar & data;
			}
			boost::archive::text_oarchive& ar;
		}callback( ar );

		
		size_t num = getResNum( RES_ACTOR_MODEL ) + getResNum( RES_OBJ_MODEL );
		ar & num;

		visitRes( RES_ACTOR_MODEL , callback );
		visitRes( RES_OBJ_MODEL , callback );

	}
	catch( ... )
	{
		return false;
	}

	return true;
}

bool TResManager::loadRes( char const* name )
{

	try 
	{
		std::ifstream fs(name);
		boost::archive::text_iarchive ar(fs);

		size_t num;

		ar.register_type< ActorModelRes >();
		ar.register_type< ObjectModelRes >();

		ar & num;
		for ( int i = 0 ; i < num ; ++ i )
		{
			IResBase* resData;
			ar & resData;
			resigterRes( resData->resType , resData );
		}
	}
	catch( ... )
	{
		return false;
	}

	return true;

}

ResId  TResManager::addNewActorModel( char const* name )
{
	ActorModelRes* data = new ActorModelRes;
	data->resName = name;
	data->initData();
	return resigterRes( data->resType , data );
}



void TResManager::addDefultData()
{
	{
		ActorModelRes* res;
		PhyShapeParams info;

#define ACTOR_MODEL_RES( name , offset )                     \
	res = new ActorModelRes;                                 \
	res->resName = name;                                     \
	res->phyData.mass = 0;                                   \
	res->phyData.shape = PhysicsSystem::Get().createShape( info );\
	res->modelOffset = offset;                                 \
	for( int i = 0 ; i < ActorModelRes::MAX_POSTABLE_NUM; ++i )\
	res->animTable[i] = table[i];                          \
	resigterRes( RES_ACTOR_MODEL , res );                      \

		{
			int table[] = { 2, 4 ,-1, 1, 5, 6, 6, -1 ,3, 3 ,-1, -1, -1 ,-1 ,-1 ,-1 ,-1 ,-1 };
			info.setBoxShape( Vec3D(25,25,85) );
			ACTOR_MODEL_RES( "Angel" , Vec3D(0 ,0 ,-40) );
		}
		{
			int table[] = { 2 ,4, 13 ,-1, 5, 6, 14 ,-1, 3, -1, -1, -1, -1, -1, -1, -1 ,-1 ,-1 };
			info.setBoxShape( Vec3D(40, 100, 60) );
			ACTOR_MODEL_RES( "Basilisk" , Vec3D(0, 10, -30 ) );
		}
		{
			int table[] = { 1, 2, 3, 21, 7 ,28, 4, 5, 10, 12, 12 ,14 ,19 ,19 ,-1, -1, -1, -1 };
			info.setBoxShape( Vec3D(25, 25, 85) );
			ACTOR_MODEL_RES( "Hero" , Vec3D(0 ,0 ,-43) );
		}

#undef ACTOR_MODEL_RES
	}
	{

		ObjectModelRes* res;
		PhyShapeParams info;

#define OBJ_MODEL_RES( NAME , MASS )                          \
	res = new ObjectModelRes;                                 \
	res->resName = NAME;                                      \
	res->phyData.mass = MASS;                                 \
	res->phyData.shape = PhysicsSystem::Get().createShape( info );\
	resigterRes( RES_OBJ_MODEL , res );                       \

		{
			info.setBoxShape( Vec3D(20 ,20 ,20) );
			OBJ_MODEL_RES( "box" , 1 );
		}
		{
			info.setBoxShape( Vec3D(20 ,20 ,20) );
			OBJ_MODEL_RES( "box1" , 1 );
		}
		{
			info.setBoxShape( Vec3D(90 , 90 , 90) );
			OBJ_MODEL_RES( "box2" , 1 );
		}

#undef OBJ_MODEL_RES
	}
}

//bool TAudioResData::load()
//{
//	FnWorld& world = TResManager::instance().getWorld();
//
//	audio.Object(  world.CreateAudio() );
//	return  audio.Load( (char*)resName.c_str() );
//}

//bool TFxResData::loadRes()
//{
//	FnScene& scene = TResManager::instance().getBGScene();
//
//	scene.SetCurrentRenderGroup( RG_FX );
//
//	fx  = new eF3DFX( scene.Object() );
//	fx->SetWorkPath( FX_DATA_DIR );
//	fx->SetModelPath( FX_MODEL_DATA_DIR );
//	fx->SetTexturePath( FX_TEXTURE_DATA_DIR );
//
//
//	bool result = fx->Load( (char*) resName.c_str() );
//	
//	scene.SetCurrentRenderGroup( 0 );
//	return result;
//}

//TFxResData::TFxResData() 
//	:ResBase( RES_FX )
//{
//	fx = NULL;
//}

bool EquipModelRes::load()
{
	CFWorld* world = TResManager::Get().getWorld();
	CFScene* scene = TResManager::Get().getBGScene();

	world->setDir( CFly::DIR_OBJECT , ITEM_DATA_DIR );
	world->setDir( CFly::DIR_TEXTURE , ITEM_DATA_DIR );
	//scene.SetCurrentRenderGroup( RG_DYNAMIC_OBJ );

	if ( !model )
		model = scene->createObject();

	String path = resName + ".cw3";
	bool result = model->load( path.c_str() );

	return result;
}

void EquipModelRes::release()
{
	if ( model )
		model->release();
}

bool ObjectModelRes::load()
{
	CFWorld* world = TResManager::Get().getWorld();
	CFScene* scene = TResManager::Get().getBGScene();

	world->setDir( CFly::DIR_OBJECT , OBJECT_DATA_DIR );
	//world->setDir( CFly::DIR_TEXTURE , ITEM_DATA_DIR );
	//scene.SetCurrentRenderGroup( RG_DYNAMIC_OBJ );

	if ( !model )
		model = scene->createObject();

	String path = resName + ".cw3";
	bool result = model->load( path.c_str() );

	if ( result)
	{
		//objectModel->addNormalData();
	}
	return result;
}



void ObjectModelRes::release()
{
	if ( model )
		model->release();
}

CObjectModel* ObjectModelRes::createModel( CFScene* scene )
{
	CFObject* object = model->instance( false );
	object->changeScene( scene );
	CObjectModel* comp = new CObjectModel( object );
	return comp;
}

bool ActorModelRes::load()
{
	CFWorld* world = TResManager::Get().getWorld();
	CFScene* scene = TResManager::Get().getBGScene();

	String path = ACTOR_DATA_DIR + resName;
	world->setDir( CFly::DIR_ACTOR , path.c_str() );
	world->setDir( CFly::DIR_OBJECT , path.c_str() );
	//world->setDir( CFly::DIR_TEXTURE , path.c_str() );

	//scene.SetCurrentRenderGroup( RG_ACTOR );

	path = resName + ".cwc";
	model = scene->createActorFromFile( path.c_str() );

	if ( !model )
		return false;

	return true;
}

void ActorModelRes::initData()
{
	phyData.setDefult();
	modelOffset = Vec3D(0,0,0);
	for ( int i = 0 ; i < MAX_POSTABLE_NUM ; ++i )
	{
		animTable[i] = -1;
	}
}

void ActorModelRes::release()
{
	if ( model )
		model->release();
}

CActorModel* ActorModelRes::createModel( CFScene* scene )
{
	CFActor* actor = model->instance( false );
	actor->changeScene( scene );
	CActorModel* comp = new CActorModel( actor );

	for( int i = 0 ; i < ANIM_TYPE_NUM ; ++i )
	{
		if ( animTable[i] == -1 )
			continue;
		char poseName[ 32 ];
		sprintf_s( poseName , sizeof( poseName ) , "%d" , animTable[i] - 1 );
		comp->setAnimationState( poseName , AnimationType(i) );
	}
	comp->restoreAnim();
	return comp;
}


//bool T3DAudioResData::loadRes()
//{
//	FnScene& scene = TResManager::instance().getBGScene();
//	audio.Object( scene.Create3DAudio() );
//
//	return audio.LoadSound((char*)resName.c_str() );
//}



bool SceneMapRes::load()
{
	String mapDir = String( MAP_DATA_DIR ) + resName;
	CFWorld* world = TResManager::Get().getWorld();

	scene = world->createScene( 5 );

	world->setDir( CFly::DIR_SCENE , mapDir.c_str() );

	String fileName = resName +".cwn";

	bool result = true;
	if ( !scene->load( fileName.c_str() ) )
	{
		goto load_fail;
	}

	terrainMesh = new TriangleMeshData;
	fileName = mapDir + "/" + resName + ".ter";
	if ( !terrainMesh->load( fileName.c_str() ) )
	{
		LogError( "cant Load TerrainData! , map = %s" , resName.c_str() );
		goto load_fail;
	}

	PhyShapeParams info;
	terrainMesh->fillShapeParams( info );
	terrainShape = PhysicsSystem::Get().createShape( info );

	fileName = mapDir + "/" + resName + ".nav";
	if ( !navMesh.load( fileName.c_str() ) )
	{
		LogError( "cant Load NavMesh! , map = %s" , resName.c_str() ); 
		goto load_fail;
	}

	return true;

load_fail:
	releaseInternal();
	return false;
}

void SceneMapRes::releaseInternal()
{
	if ( scene )
		scene->release();
	delete terrainMesh;
	PhysicsSystem::Get().destroyShape( terrainShape );
	navMesh.releaseData();

	scene = nullptr;
	terrainMesh = nullptr;
	terrainShape = 0;
}

