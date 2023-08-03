#ifndef RichWorld_h__
#define RichWorld_h__

#include "RichBase.h"

#include "ParamCollection.h"
#include "DataStructure/Grid2D.h"
#include "DataStructure/Array.h"

namespace Rich
{
	typedef TVector2< int > Vec2i;

	AreaId const ERROR_AREA_ID = AreaId( -1 );
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
		MSG_DISPOSE_ASSET,
		MSG_BUY_STATION,
		MSG_UPGRADE_LAND ,
		MSG_PAY_PASS_TOLL ,
		MSG_ENTER_STORE ,
		MSG_MOVE_STEP ,

		MSG_PLAYER_MONEY_MODIFIED,
	};


	struct WorldMsg : ParamCollection< 4 >
	{
		MsgId id;
	};

	class IWorldMessageListener
	{
	public:
		virtual ~IWorldMessageListener() = default;
		virtual void onWorldMsg( WorldMsg const& msg ) = 0;
	};


	class IActorFactory
	{
	public:
		virtual ~IActorFactory() = default;
		virtual Player* createPlayer() = 0;
	};

	struct GameOptions
	{
		int initialCash = 1500;
		int passingGoAmount = 200;
	};


	class IObjectQuery
	{
	public:
		virtual ~IObjectQuery() = default;
		virtual TArray<Player*> const& getPlayerList() = 0;
		virtual GameOptions const& getGameOptions() = 0;
	};


	class Tile
	{
	public:
		MapCoord   pos;
		AreaId     areaId;

		void reset()
		{
			actors.clear();
		}
		
		using ActorList = TIntrList< ActorComp , MemberHook< ActorComp , &ActorComp::tileHook > >;
		ActorList   actors;
	};

	struct AreaGroupInfo
	{
		TArray<Area*> areas;
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


	class World
	{

		typedef unsigned short TileId;
		static TileId const ERROR_TILE_ID = TileId( -1 );
	public:

		World(IObjectQuery* objectQuery, IActorFactory* actorFactory);

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
		int      getLinks(MapCoord const& posCur, LinkHandle outLinks[]);
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

		Area*   getArea( AreaId id ){ return mAreaMap[ id ]; }
		AreaId  registerArea( Area* area , AreaId idReg = ERROR_AREA_ID );
		void    unregisterArea( Area* area );

		void   clearArea( bool beDelete );


		void registerAreaGroup(Area* area, int group)
		{
			if (group >= mAreaGroupMap.size())
			{
				mAreaGroupMap.resize(group + 1);
			}
			mAreaGroupMap[group].areas.push_back(area);
		}
		AreaGroupInfo const& getAreaGroup(int group)
		{
			return mAreaGroupMap[group];
		}

		TArray< AreaGroupInfo > mAreaGroupMap;

		void  registerAreaTag(EAreaTag tag, Area* area)
		{
			mAreaTagMap[tag] = area;
		}

		Area* getAreaFromTag(EAreaTag tag)
		{
			auto iter = mAreaTagMap.find(tag);
			if (iter == mAreaTagMap.end())
				return nullptr;

			return iter->second;
		}


		std::unordered_map< EAreaTag, Area* > mAreaTagMap;

		struct TileIterator
		{
			TileIterator(World& inWorld)
				:world(inWorld), pos(0, 0), index(INDEX_NONE)
			{
				CHECK(haveMore());
				advance();
			}

			bool haveMore() const { return index < world.mMapData.getRawDataSize(); }
			void advance()
			{
				do
				{
					++index;
					world.mMapData.toCoord(index, pos.x, pos.y);
					if (world.getTile(pos))
						break;
				}
				while (haveMore());
			}

			Tile* getTile()
			{
				return world.getTile(pos);
			}

			MapCoord const& getPos() { return pos; }
			int index = INDEX_NONE;
			World& world;
			MapCoord pos;
		};

		TileIterator createTileIterator()
		{
			return TileIterator(*this);
		}
		static int const MaxAreaNum = 255;
		struct AreaIterator
		{
			AreaIterator(World& inWorld)
				:world(inWorld), index(INDEX_NONE)
			{
				CHECK(haveMore());
				advance();
			}

			bool haveMore() const { return index < MaxAreaNum; }
			void advance()
			{
				do
				{
					++index;
					if (world.mAreaMap[index])
						break;
				}
				while (haveMore());
			}

			Area* getArea()
			{
				return world.mAreaMap[index];
			}


			int index = INDEX_NONE;
			World& world;
		};
			
		AreaIterator createAreaIterator()
		{
			return AreaIterator(*this);
		}

		IActorFactory*  mActorFactory;

		IObjectQuery* mObjectQuery;

	private:
		
		static int const DirNum = 4;
		MapCoord dirOffset(int dir)
		{
			Vec2i const offset[DirNum] = { Vec2i(1 , 0) , Vec2i(0 , 1) , Vec2i(-1 , 0) , Vec2i(0 , -1) };
			return offset[dir];
		}


		typedef TGrid2D< TileId > MapDataType;
		typedef TArray< Tile* > TileVec;

		friend class WorldBuilder;
		friend class Scene;
		MapDataType mMapData;

		Area*   mAreaMap[ MaxAreaNum ];
		
		TileVec mTileMap;

		int     mIdxLastArea;
		int     mIdxFreeAreaId;

		typedef TArray< IWorldMessageListener* > EventListerVec;
		EventListerVec mEvtListers;
	};


}//namespace Rich

#endif // RichWorld_h__
