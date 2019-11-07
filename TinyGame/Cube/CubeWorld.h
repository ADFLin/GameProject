#ifndef CubeWorld_h__
#define CubeWorld_h__

#include "CubeBase.h"
#include "CubeRandom.h"

#include <unordered_map>

namespace Cube
{
	class IWorldEventListener;
	class BlockRenderer;
	class Chunk;

	typedef uint8 MetaType;

	class IBlockAccess
	{
	public:
		virtual BlockId  getBlockId( int bx , int by , int bz ) = 0;
		virtual MetaType getBlockMeta( int bx , int by , int bz ) = 0;
		virtual bool     isOpaqueBlock( int bx , int by , int bz ) = 0;
	};

	unsigned const ChunkBit  = 4;
	unsigned const ChunkSize = 1 << ChunkBit;
	unsigned const ChunkMask = ( 1 << ChunkBit ) - 1;

	unsigned const ChunkBlockMaxHeight = 256;



	struct ChunkPos
	{
		int x;
		int y;
		void   setBlockPos( int bx , int by )
		{
			x = bx >> ChunkBit;
			y = by >> ChunkBit;
		}
		uint64 hash_value() const 
		{ 
			uint32 cx = uint32( x );
			uint32 cy = uint32( y );
			return ( uint64(cx) << 32 ) | uint64( cy ); }
	};



	class Chunk
	{
	public:
		Chunk( ChunkPos const& pos );

		BlockId  getBlockId( int x , int y , int z );
		void     setBlockId( int x , int y , int z , BlockId id );

		MetaType getBlockMeta( int x , int y , int z );
		void     setBlockMeta( int x , int y , int z , MetaType meta );

		ChunkPos const& getPos(){ return mPos; }

		static unsigned const LayerBit  = 5;
		static unsigned const LayerSize = 1 << LayerBit;
		static unsigned const LayerMask = ( 1 << LayerBit ) - 1;

		static unsigned const NumLayer = ChunkBlockMaxHeight >> LayerBit;


		struct LayerData
		{
			LayerData()
			{
				std::fill_n( &blockMap[0][0][0] , ChunkSize * ChunkSize * LayerSize , 0 );
				std::fill_n( &lightMap[0][0][0] , ChunkSize * ChunkSize * LayerSize , 0 );
				std::fill_n( &meta[0][0][0] , ChunkSize * ChunkSize * LayerSize / 2 , 0 );
			}
			BlockId  blockMap[ ChunkSize ][ ChunkSize ][ LayerSize ];
			uint8    meta[ ChunkSize ][ ChunkSize ][ LayerSize / 2 ];
			uint8    lightMap[ ChunkSize ][ ChunkSize ][ LayerSize ];
		};

		LayerData* getLayer( int z )
		{
			return mLayer[ z >> LayerBit ];
		}

		void render( BlockRenderer& renderer );
		LayerData* mLayer[ NumLayer ];
		ChunkPos   mPos;
	};


	class SingleChunkAccess : public IBlockAccess
	{
	public:

		virtual BlockId getBlockId( int x , int y , int z ){ return mChunk.getBlockId( x , y , z ); }
		virtual bool    isOpaqueBlock( int x , int y , int z )
		{ 
			BlockId id = mChunk.getBlockId( x , y , z );
			if ( id == BLOCK_NULL )
				return false;
			return true;
		}
		Chunk& mChunk;
	};


	class TerrainGenerator
	{
	public:
		virtual bool generate( Chunk& chunk , Random& rand ) = 0;

	};


	class FlatPlaneGenerater : public TerrainGenerator
	{

	public:
		virtual bool generate( Chunk& chunk , Random& rand );

		int height;
	};


	class ChunkProvider
	{
	public:
		Chunk* getChunk( int x , int y );

		Chunk* getChunk( ChunkPos const& pos );
		typedef std::unordered_map< uint64 , Chunk* > ChunkMap;
		ChunkMap mMap;
	};


	struct BlockPosInfo
	{
		int x , y , z;
		FaceSide face;
		float    dist;
	};

	class World : public IBlockAccess
	{
	public:

		World();

		BlockId  getBlockId( int bx , int by , int bz ) final;
		MetaType getBlockMeta( int bx , int by , int bz ) final;
		bool     isOpaqueBlock( int x , int y , int z ) final;

		void     setBlock( int bx , int by , int bz , BlockId id );
		void     setBlockNotify( int bx , int by , int bz , BlockId id );

		Chunk*   getChunk( int bx , int by )
		{ 
			ChunkPos cPos; cPos.setBlockPos( bx , by );
			return mChunkProvider->getChunk( cPos ); 
		}
		Chunk*  getChunk( ChunkPos const& pos ){ return mChunkProvider->getChunk( pos ); }


		BlockId rayBlockTest( Vec3f const& pos , Vec3f const& dir , float maxDist ,  BlockPosInfo* info );
		BlockId offsetBBoxBlockTest( Vec3f const& pos , Vec3f const& size , Vec3f const& dir , float dist , BlockPosInfo* info );

		void    addListener( IWorldEventListener& listener );
		void    removeListener( IWorldEventListener& listener );
		void    notifyBlockModified( int bx , int by , int bz );


		typedef std::list< IWorldEventListener* > ListenerList;
		ListenerList   mListeners;
		ChunkProvider* mChunkProvider;
		Random         mRandom;
	};


}//namespace Cube

#endif // CubeWorld_h__
