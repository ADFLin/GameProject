#ifndef RichWorld_h__
#define RichWorld_h__

#include "RichBase.h"

#include "TGrid2D.h"

#include <vector>

namespace Rich
{
	typedef TVector2< int > Vec2i;

	CellId const ERROR_CELL_ID = CellId( 0xff );
	CellId const EMPTY_CELL_ID = CellId( 0 );

	typedef int DirType;
	DirType const NoDir = DirType( -1 );
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

	struct WorldMsg
	{
		MsgId id;
		union
		{
			int         intVal;
			Player*     player;
			LandCell*   land;
			PlayerTurn* turn;

			struct  
			{
				int  numDice;
				int* diceValue;
			};
		};
	};

	class IMsgListener
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
		CellId      cell;
		typedef IntrList< Actor , MemberHook< Actor , &Actor::tileHook > > ActorList;
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

	class World : public CellRegister
	{

		typedef unsigned short TileId;
		static TileId const ERROR_TILE_ID = TileId( -1 );
	public:

		World( IObjectQuery* query );

		void addMsgListener( IMsgListener& listener );
		void removeMsgListener( IMsgListener& lister );
		void sendMsg( WorldMsg const& event );


		void restCellMap();

		static int const DirNum = 4;
		MapCoord dirOffset( int dir )
		{
			Vec2i const offset[ DirNum ] = { Vec2i( 1 , 0 ) , Vec2i( 0 , 1 ) , Vec2i( -1 , 0 ) , Vec2i( 0 , -1 ) };
			return offset[ dir ];
		}

		IRandom& getRandom();
		void     setupMap( int w , int h );
		void     clearMap();
		bool     addTile( MapCoord const& pos , CellId id );
		bool     removeTile( MapCoord const& pos );

		int      getMoveDir( MapCoord const& posCur , MapCoord const& posPrev , DirType dir[] );

		int      findCell( MapCoord const& pos , DistType dist , Cell* cellFound[] );

		CellId   getCellId( MapCoord const& pos )
		{
			Tile* tile = getTile( pos );
			if ( !tile )
				return ERROR_CELL_ID;
			return tile->cell;
		}

		Tile*  getTile( MapCoord const& pos );
		Cell*  getCell( MapCoord const& pos )
		{
			CellId id = getCellId( pos );
			if ( id == ERROR_CELL_ID )
				return nullptr;
			return mCellMap[ id ];
		}
		Vec2i  getConPos( Vec2i const& pos , int dir )
		{
			return pos + dirOffset( dir );
		}

		Cell*  getCell( CellId id ){ return mCellMap[ id ]; }
		CellId registerCell( Cell* cell , CellId idReg = ERROR_CELL_ID );
		void   unregisterCell( Cell* cell );

		void   clearCell( bool beDelete );

		IObjectQuery& getQuery(){ return *mQuery; }
		
		typedef TGrid2D< TileId > MapDataType;
		typedef std::vector< Tile* > TileVec;

		friend class WorldBuilder;
		friend class Scene;
		MapDataType mMapData;
		static int const MaxCellNum = 255;
		Cell*   mCellMap[ MaxCellNum ];
		
		TileVec mTileMap;

		int     mIdxLastCell;
		int     mIdxFreeCellId;

		typedef std::vector< IMsgListener* > EventListerVec;
		EventListerVec mEvtListers;
		IObjectQuery*  mQuery;

	};


}//namespace Rich

#endif // RichWorld_h__
