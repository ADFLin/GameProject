#ifndef TDWorld_h__
#define TDWorld_h__

#include "TDDefine.h"
#include "TDCollision.h"
#include "TDEntity.h"

#include "TGrid2D.h"

namespace TowerDefend
{

	class Building;

	enum 
	{
		BM_BUILDING    = BIT(0),
		BM_TERRAIN     = BIT(1),
		BM_USER_DEFINE = BIT(2),
	};

	enum CollisionLayer
	{
		CL_WALK ,
		CL_FLY  ,
		CL_BUILD ,

		NumCollsisionLayer ,
	};
	class WorldMap
	{
	public:

		struct TileData
		{
			unsigned    tileID;
			unsigned    tileParam;
			uint8       blockMask[ NumCollsisionLayer ];
			uint8       blockBlgMask;
			Building*   building;
		};

		void        setup( int cx , int cy );
		TileData&   getTile( Vec2i const& mapPos ){  return mMapData.getData( mapPos.x , mapPos.y ); }

		Building*   getBuilding( Vec2f const& wPos );
		bool        canBuild( ActorId type , Vec2f const& pos , Vec2i& mapPos );
		void        addBuiding( Building* building , Vec2i const& mapPos );
		void        removeBuilding( Building* building );

		bool        testCollisionX( Vec2f const& mapPos , float colRadius , CollisionLayer layer , float& offset );
		bool        testCollisionY( Vec2f const& mapPos , float colRadius , CollisionLayer layer , float& offset );
		bool        checkCollision( Unit* unit );
		bool        checkCollision( Vec2f const& pos , float radius , CollisionLayer layer );
		TGrid2D< TileData > mMapData;
	};

	enum EntityEvent
	{
		EVT_EN_SPAWN    ,
		EVT_EN_KILLED   ,
		EVT_EN_DESTROY  ,
		EVT_EN_BLG_CONSTRUCTED ,
		EVT_EN_UNIT_PRODUCTED ,
		EVT_EN_CHANGE_COM ,
		EVT_EN_PUSH_COM ,
		EVT_EN_KILL_ACTOR ,
	};

	enum PlayerRelationship
	{
		PR_SHARE_ACTOR ,
		PR_TEAM        ,
		PR_AllIANCE    ,
		PR_NEUTRAL     ,
		PR_EMPTY       ,
	};

	class World
	{
	public:
		World();
		WorldMap&         getMap()         { return mMap;  }
		EntityManager&    getEntityMgr()   { return mEntityMgr; }
		CollisionManager& getCollisionMgr(){ return mCollisionMgr; }

		PlayerRelationship  getRelationship( Actor* actor1 , Actor* actor2 );

		bool        canControl( PlayerInfo* player , Actor* actor );
		bool        produceUnit( ActorId type , PlayerInfo* pInfo , bool useRes );
		bool        tryPlaceUnit( Unit* unit , Building* building , Vec2f const& targetPos );
		Building*   constructBuilding( ActorId type , Unit* builder , Vec2f const& pos , bool useRes );
	protected:

		bool tryPlaceUnitInternal( Unit* unit , Vec2f const& startPos, Vec2f const& offsetDir, float maxOffset );
		bool canBuild( ActorId type , Unit* builder , Vec2f const& pos , Vec2i& mapPos , PlayerInfo* pInfo , bool needSolve );

		CollisionManager  mCollisionMgr;
		WorldMap          mMap;
		EntityManager     mEntityMgr;
	};

}//namespace TowerDefend

#endif // TDWorld_h__
