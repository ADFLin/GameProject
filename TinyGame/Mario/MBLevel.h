#ifndef MBLevel_h__
#define MBLevel_h__

#include "TVector2.h"
#include "Math/Vector2.h"
#include "TGrid2D.h"

namespace Mario
{
	typedef ::Math::Vector2 Vector2;
	typedef TVector2< int > Vec2i;

	int const TileLength = 16;

	float const TICK_TIME = 1.0f / 30.0f;

	enum ActionButton
	{
		ACB_UP    = BIT(0),
		ACB_DOWN  = BIT(1),
		ACB_LEFT  = BIT(2),
		ACB_RIGHT = BIT(3),
		ACB_FUN   = BIT(4),
		ACB_JUMP  = BIT(5),
	};

	enum BlcokType
	{
		BLOCK_NULL ,
		BLOCK_STATIC ,

		BLOCK_BRICK ,
		BLOCK_BOX   ,

		BLOCK_SLOPE_11 ,
		BLOCK_SLOPE_21 ,

	};

	class Block;
	class World;

	struct Tile
	{
		Vec2i pos;
		int   block;
		int   meta;
	};

	

	class Block
	{
	public:

		virtual bool  checkCollision( Tile const& tile , Vector2 const& pos , Vector2 const& size );
		virtual float calcFixPosX(  Tile const& tile , Vector2 const& pos , Vector2 const& size , float offset );
		virtual float calcFixPosY(  Tile const& tile , Vector2 const& pos , Vector2 const& size , float offset );
		
		virtual bool  update( Tile& tile , World& world ){ return false; }

		static void   initMap();
		static Block* get( int type );
	};


	class BlockSlope : public Block
	{
	public:
		virtual bool checkCollision( Tile const& tile , Vector2 const& pos , Vector2 const& size );

		virtual float calcFixPosX(  Tile const& tile , Vector2 const& pos , Vector2 const& size , float offset );
		virtual float calcFixPosY(  Tile const& tile , Vector2 const& pos , Vector2 const& size , float offset );

		static int  getDir( int meta )
		{
			return int( meta & 0xf );
		}
	};

	class BlockBump : public Block
	{
	public:
		virtual bool update( Tile& tile , World& world )
		{	
			return true;
		}
	};

	enum Direction
	{
		DIR_TOP ,
		DIR_DOWN ,
		DIR_LEFT ,
		DIR_RIGHT ,
	};

	class BlockCloud : public Block
	{
	public:
		virtual bool checkCollision( Tile const& tile , Vector2 const& pos , Vector2 const& size );

		virtual float calcFixPosX(  Tile const& tile , Vector2 const& pos , Vector2 const& size , float offset );
		virtual float calcFixPosY(  Tile const& tile , Vector2 const& pos , Vector2 const& size , float offset );



	};


	typedef TGrid2D< Tile > TileMap;


	class World
	{
	public:

		World()
		{


		}

		void init( int x , int y )
		{
			mTileMap.resize( x , y );
			resetTile();
		}

		void tick()
		{

		}

		void resetTile()
		{
			for( int i = 0 ; i < mTileMap.getSizeX() ; ++i )
			for( int j = 0 ; j < mTileMap.getSizeY() ; ++j )
			{
				Tile& tile = mTileMap.getData( i , j );

				tile.pos   = Vec2i( i , j );
				tile.block = BLOCK_NULL;
				tile.meta  = 0;

			}
		}

		Tile*    testTileCollision( Vector2 const& pos , Vector2 const& size );
		TileMap& getTerrain(){ return mTileMap; }

		TileMap mTileMap;
	};


	class Player
	{
	public:

		enum ActorType
		{



		};

		void changeActor( ActorType type )
		{

		}

		void reset();
		enum MoveType
		{
			eRUN      ,
			eJUMP     ,
			eFALLING  ,
			eRIDING   ,
			eGOD_MODE ,
			eLADER    ,
		};

		void   tick();

		World* getWorld(){ return world; }

		int      jumpLoop;
		bool     onGround;
		unsigned button;
		MoveType moveType;
		Vector2    vel;
		Vector2    pos;
		Vector2    size;
		World*   world;
	};


}//namespace Mario


#endif // MBLevel_h__
