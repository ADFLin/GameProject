#include "ProjectPCH.h"
#include "CSceneLevel.h"

#include "CEntity.h"
#include "CRenderComponent.h"
#include "TResManager.h"
#include "GameObjectComponent.h"
#include "SpatialComponent.h"

#include "TriangleMeshData.h"

#include "CGFContent.h"
#include "RenderSystem.h"
#include "ISystem.h"

#include "CFScene.h"

CFScene* CSceneLevel::sSaveModelScene = nullptr;

CSceneLevel::CSceneLevel( char const* name , unsigned flag )
	:mName( name )
{
	init();
	mFlagBit = flag;
	if ( !( mFlagBit & SLF_USE_MAP ) )
		mRenderScene = gEnv->renderSystem->createRenderScene();
}

CSceneLevel::~CSceneLevel()
{
	_loseSceneMap();
	PhysicsSystem::getInstance().destroyWorld( mPhyWorld );

	if ( !useSceneMap() )
		mRenderScene->release();
}


void CSceneLevel::init()
{
	mIsPrepared = false;
	mIsLostMap  = false;
	mNavMesh = NULL;
	mCGFContent.reset( new CGFContent );

	mPhyWorld = PhysicsSystem::getInstance().createWorld();
	mPhyWorld->setGravity( Vec3D(0,0,-98) );

	if ( sSaveModelScene == NULL )
	{

	}
}


bool CSceneLevel::loadSceneMap( char const* name )
{
	if ( mMap.mMapRes || !useSceneMap() )
		return false;

	if ( !mMap.load( name ) ) 
		return false;

	if ( mFlagBit & SLF_USE_MAP_NAVMESH )
	{
		mNavMesh = mMap.getNavMesh();
	}

	PhyRigid* mapPhyComp = mPhyWorld->createRigidComponent( mMap.getTerrainShape() , 0 , 
		COF_TERRAIN , COF_SOILD & ~(COF_STATIC|COF_TERRAIN ) );

	mRenderScene = mMap.getCFScene();
	return true;
}


void CSceneLevel::addObject( ILevelObject* object )
{
	if ( object->mSceneLevel )
	{
		object->mSceneLevel->removeObject( object );
	}	
	object->mSceneLevel = this;
	object->onSpawn();
	mLevelObjectList.push_back( object );
}

bool CSceneLevel::removeObject( ILevelObject* object )
{
	if ( object->mSceneLevel != this )
		return false;

	mLevelObjectList.remove( object );
	object->onDestroy();
	object->getEntity()->addFlag( EF_DESTROY );

	return true;
}


void CSceneLevel::_loseSceneMap()
{
	for( LevelObjectList::iterator iter = mLevelObjectList.begin();
		 iter != mLevelObjectList.end() ; ++iter )
	{
		CEntity* entity = (*iter)->getEntity();
		CRenderComponent* renderComp =  entity->getComponentT< CRenderComponent >( COMP_RENDER );

		mRenderScene = sSaveModelScene;
	}
}

void CSceneLevel::update( long time )
{
	mPhyWorld->update( time );
}

ILevelObject* CSceneLevel::findObject( Cookie& cookie , Vec3D const& pos , float radiusSqure )
{
	while ( cookie.mIter != mLevelObjectList.end() )
	{
		ILevelObject* comp = *cookie.mIter;
		++cookie.mIter;

		Vec3D const& ePos = comp->getPosition();

		if ( Math::DistanceSqure( pos , ePos ) < radiusSqure )
			return comp;
	}
	return nullptr;
}

NavMesh* CSceneLevel::getNavMesh()
{
	return mNavMesh;
}

CFScene* CSceneLevel::getRenderScene()
{
	return mRenderScene;
}

bool CSceneLevel::prepare()
{
	if ( mIsPrepared )
		return true;

	if ( useSceneMap() )
	{
		if ( !mMap.prepare( this ) )
			return false;
	}

	mIsPrepared = true;
	return mIsPrepared;
}

void CSceneLevel::active()
{
	if ( useSceneMap() )
	{
		mMap.active( this );
	}
}

void CSceneLevel::enableFlag( SceneLevelFlag flag , bool enable )
{
	bool preSet = ( mFlagBit & flag ) != 0;
	if ( enable == preSet )
		return;

	switch( flag )
	{
	case SLF_USE_MAP: 
		assert( 0 ); 
		break;
	case SLF_USE_MAP_NAVMESH:
		assert( 0 ); 
		break;
	}

	if ( enable )
		mFlagBit |= flag;
	else
		mFlagBit &= ~flag;
}

bool CSceneMap::load( char const* name )
{
	mMapRes = static_cast< SceneMapRes* >(
		TResManager::getInstance().getResData( RES_SCENE_MAP , name ) );

	if ( !mMapRes )
		return false;

	//FIXME fill sceneobj

	return true;
}

bool CSceneMap::prepare( CSceneLevel* level )
{
	if ( !mMapRes )
		return true;

	if ( !TResManager::getInstance().checkResLoaded( mMapRes ) )
		return false;

	return true;
}

void CSceneMap::active( CSceneLevel* level )
{
	if ( !mMapRes )
		return;

	if ( mMapRes->user != level )
	{
		if ( mMapRes->user )
			mMapRes->user->_loseSceneMap();
	}
	mMapRes->user = level;
}

CFScene* CSceneMap::getCFScene()
{
	assert( mMapRes );
	return mMapRes->scene;
}

NavMesh* CSceneMap::getNavMesh()
{
	assert( mMapRes );
	return &mMapRes->navMesh;
}

PhyShapeHandle CSceneMap::getTerrainShape()
{
	return mMapRes->terrainShape;
}

CSceneLevel* LevelManager::createEmptyLevel( char const* levelName , unsigned flag  )
{
	LevelMap::iterator iter = mLevelMap.find( levelName );
	if ( iter != mLevelMap.end() )
		return NULL;

	CSceneLevel* level = new CSceneLevel( levelName , flag );

	mLevelMap.insert( std::make_pair( level->getName() , level ) );
	return level;
}

CSceneLevel* LevelManager::loadLevel( char const* path )
{


	return NULL;
}

bool LevelManager::changeLevelName( CSceneLevel* level , char const* name )
{


	return true;
}

bool LevelManager::destroyLevel( char const* name )
{
	LevelMap::iterator iter = mLevelMap.find( name );
	if ( iter == mLevelMap.end() )
		return true;

	if ( mRunningLevel == iter->second )
	{
		mRunningLevel = NULL;
	}

	delete iter->second;
	mLevelMap.erase( iter );
	return true;
}

void LevelManager::changeRunningLevel( char const* name )
{
	LevelMap::iterator iter = mLevelMap.find( name );
	if ( iter == mLevelMap.end() )
		return;

	if ( mRunningLevel )
	{

	}

	mRunningLevel = iter->second;

	if ( mRunningLevel )
	{
		mRunningLevel->prepare();
		mRunningLevel->active();
	}
}
