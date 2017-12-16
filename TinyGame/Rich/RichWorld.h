#ifndef RichWorld_h__
#define RichWorld_h__

#include "RichBase.h"

#include "ParamCollection.h"
#include "DataStructure/Grid2D.h"

#include <vector>

namespace Rich
{
	typedef TVector2< int > Vec2i;

	AreaId const ERROR_AREA_ID = AreaId( 0xff );
	AreaId const EMPTY_AREA_ID = AreaId( 0 );

	typedef int LinkHandle;
	LinkHandle const EmptyLinkHandle = LinkHandle( -1 );
	int const MAX_DIR_NUM = 16;
	typedef int DistType;

	enum MsgId
	{
		MSG_TURN_START ,
		MSG_TURN_END  ,
		MSG_MOVE_START ,
		MSG_MOVE_END  ,
		MSG_THROW_DICE ,
		MSG_MODIFY_MONEY ,
		MSG_BUY_LAND ,
		MSG_PAY_PASS_TOLL ,
		MSG_ENTER_STORE ,
		MSG_MOVE_STEP ,
	};


	struct WorldMsg : ParamCollection< 4 >
	{
		MsgId id;
	};

	class IWorldMessageListener
	{
	public:
		virtual void onWorldMsg( WorldMsg const& msg ) = 0;
	};


	class IObjectQuery 
	{
	public:
		virtual Player* getPlayer( ActorId id ) = 0;
	};

	class Tile
	{
	public:
		MapCoord   pos;
		AreaId      areaId;
		typedef TIntrList< ActorComp , MemberHook< ActorComp , &ActorComp::tileHook > > ActorList;
		ActorList   actors;
	};

	enum ForkedRoadRule
	{
		FRR_RANDOM ,
		FRR_STAY_SELECT ,
		FRR_SELECT ,
	};

	class IWorldMap
	{






	};

	class World : public AreaRegister
	{

		typedef unsigned short TileId;
		static TileId const ERROR_TILE_ID = TileId( -1 );
	public:

		World( IObjectQuery* query );

		void     setupMap(int w, int h);
		void     clearMap();

		void     addMsgListener( IWorldMessageListener& listener );
		void     removeMsgListener( IWorldMessageListener& lister );
		void     dispatchMessage( WorldMsg const& msg );


		void     restAreaMap();


		IRandom& getRandom();

		bool     addTile( MapCoord const& pos , AreaId id );
		bool     removeTile( MapCoord const& pos );

		int      getMovableLinks( MapCoord const& posCur , MapCoord const& posPrev , LinkHandle outLinks[] );

		int      findAreas( MapCoord const& pos , DistType dist , Area* outAreasFound[] );

		AreaId   getAreaId( MapCoord const& pos )
		{
			Tile* tile = getTile( pos );
			if ( !tile )
				return ERROR_AREA_ID;
			return tile->areaId;
		}

		Tile*  getTile( MapCoord const& pos );
		Area*  getArea( MapCoord const& pos )
		{
			AreaId id = getAreaId( pos );
			if ( id == ERROR_AREA_ID )
				return nullptr;
			return mAreaMap[ id ];
		}
		MapCoord getLinkCoord( MapCoord const& pos , LinkHandle linkHandle )
		{
			return pos + dirOffset( linkHandle );
		}

		Area*  getArea( AreaId id ){ return mAreaMap[ id ]; }
		AreaId  registerArea( Area* area , AreaId idReg = ERROR_AREA_ID );
		void   unregisterArea( Area* area );

		void   clearArea( bool beDelete );

		IObjectQuery& getObjectQuery(){ return *mQuery; }

	private:
		
		static int const DirNum = 4;
		MapCoord dirOffset(int dir)
		{
			Vec2i const offset[DirNum] = { Vec2i(1 , 0) , Vec2i(0 , 1) , Vec2i(-1 , 0) , Vec2i(0 , -1) };
			return offset[dir];
		}


		typedef TGrid2D< TileId > MapDataType;
		typedef std::vector< Tile* > TileVec;

		friend class WorldBuilder;
		friend class Scene;
		MapDataType mMapData;
		static int const MaxAreaNum = 255;
		Area*   mAreaMap[ MaxAreaNum ];
		
		TileVec mTileMap;

		int     mIdxLastArea;
		int     mIdxFreeAreaId;

		typedef std::vector< IWorldMessageListener* > EventListerVec;
		EventListerVec mEvtListers;
		IObjectQuery*  mQuery;

	};


}//namespace Rich

#endif // RichWorld_h__
