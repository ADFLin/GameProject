
#include "common.h"
#include <vector>

#define MAX_LIGHT_NUM 4
#define LEVEL_DATA_PATH "Data/level/"

#include "PhysicsSystem.h"
#include "CGFContent.h"

class CEntity;
class NavMesh;
class SceneMapRes;
class ILevelObject;
class CGFContent;


typedef std::list< CFObject* > CFObjectList;


class CSceneLevel;

class CSceneMap
{
public:
	CSceneMap(){ mMapRes = NULL; }
	bool  load( char const* name );

	CFScene*        getCFScene();
	NavMesh*        getNavMesh();
	PhyShapeHandle  getTerrainShape();
	bool            prepare( CSceneLevel* scene );
	void            active( CSceneLevel* level );

	SceneMapRes*    mMapRes;
};

class ISceneLevelListener
{

};



enum SceneLevelFlag
{
	SLF_USE_MAP = BIT(1) ,
	SLF_USE_MAP_NAVMESH  = BIT(0) ,
};

class CSceneLevel
{
public:
	CSceneLevel( char const* name , unsigned flag );
	~CSceneLevel();

	NavMesh*   getNavMesh();
	CFScene*   getRenderScene();

	bool       prepare();
	void       active();

	char const*    getName(){ return mName.c_str();  }	
	PhysicsWorld*  getPhysicsWorld(){ return mPhyWorld; }

	void       enableFlag( SceneLevelFlag flag , bool enable );
	void       update( long time );
	void       _loseSceneMap();

	CGFContent&  getCGFContent(){ return *mCGFContent;  }
	bool         loadSceneMap( char const* name );

protected:
	bool                   mIsPrepared;
	bool                   mIsLostMap;

	CGFContructHelper      mHelper;
	String                 mName;
	unsigned               mFlagBit;
	TPtrHolder< CGFContent >  mCGFContent;
	CFScene*               mRenderScene;
	NavMesh*               mNavMesh;

	CSceneMap              mMap;
	PhysicsWorld*          mPhyWorld;


	void updateScene();
	bool useSceneMap(){  return ( mFlagBit & SLF_USE_MAP )  != 0;  }
	
public:
	void init();
	typedef std::list< ILevelObject* > LevelObjectList;

	struct Cookie
	{
	private:
		LevelObjectList::iterator mIter;
		friend CSceneLevel;
	};

	void          addObject( ILevelObject* object );
	bool          removeObject( ILevelObject* object );
	void          startFindObject( Cookie& cookie ){ cookie.mIter = mLevelObjectList.begin(); } 
	ILevelObject* findObject( Cookie& cookie , Vec3D const& pos , float radiusSqure );

	LevelObjectList      mLevelObjectList;

public:
	static  CFScene* sSaveModelScene;
};


enum LevelType
{





};

class LevelRoute
{





};


class LevelManager
{
public:
	LevelManager()
	{
		mRunningLevel = NULL;
	}

	CSceneLevel* loadLevel( char const* path );
	CSceneLevel* getRunningLevel(){ return mRunningLevel;  }
	CSceneLevel* createEmptyLevel( char const* levelName , unsigned flag = 0 );
	void         changeRunningLevel( char const* name );
	bool         destroyLevel( char const* name );
	bool         changeLevelName( CSceneLevel* level , char const* name );

private:
	struct StrCmp
	{
		bool operator()( char const* s1 , char const* s2 ) const { return ::strcmp( s1 , s2 ) < 0; }
	};
	typedef std::map< char const* , CSceneLevel* , StrCmp > LevelMap;
	LevelMap     mLevelMap;
	CSceneLevel* mRunningLevel;
};

