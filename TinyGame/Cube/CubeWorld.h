#ifndef CubeWorld_h__
#define CubeWorld_h__

#include "CubeBase.h"
#include "CubeRandom.h"

#include "Math/PerlinNoise.h"
#include "Math/TVector2.h"

#include "Async/AsyncWork.h"

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
		virtual void     getNeighborBlockIds(Vec3i const& blockPos, BlockId outIds[]) = 0;
	};

	int const ChunkBitCount  = 4;
	int const ChunkSize = 1 << ChunkBitCount;
	int const ChunkMask = ( 1 << ChunkBitCount ) - 1;

	int const ChunkBlockMaxHeight = 2048;

	struct ChunkPos : public TVector2<int>
	{
		using TVector2::TVector2;

		void   setBlockPos( int bx , int by )
		{
			x = bx >> ChunkBitCount;
			y = by >> ChunkBitCount;
		}
		uint64 hash_value() const 
		{ 
			uint32 cx = uint32( x );
			uint32 cy = uint32( y );
			return ( uint64(cx) << 32 ) | uint64( cy ); }
	};

	enum class EChunkLoadState : uint8
	{
		Unintialized,
		Generating,
		Ok,
	};

	class Chunk
	{
	public:
		EChunkLoadState state;


		Chunk( ChunkPos const& pos );

		BlockId  getBlockId( int x , int y , int z );
		void     setBlockId( int x , int y , int z , BlockId id );

		MetaType getBlockMeta( int x , int y , int z );
		void     setBlockMeta( int x , int y , int z , MetaType meta );

		ChunkPos const& getPos(){ return mPos; }

		static unsigned const LayerBitCount  = 6;
		static unsigned const LayerSize = 1 << LayerBitCount;
		static unsigned const LayerMask = ( 1 << LayerBitCount ) - 1;

		static unsigned const NumLayer = ChunkBlockMaxHeight >> LayerBitCount;


		struct LayerData
		{
			LayerData()
			{
				FMemory::Zero(this, sizeof(*this));
			}
			BlockId  blockMap[ ChunkSize ][ ChunkSize ][ LayerSize ];
			uint8    meta[ ChunkSize ][ ChunkSize ][ LayerSize / 2 ];
			uint8    lightMap[ ChunkSize ][ ChunkSize ][ LayerSize ];
		};

		LayerData* getLayer( int z )
		{
			return mLayer[ z >> LayerBitCount ];
		}

		void render( BlockRenderer& renderer );
		LayerData* mLayer[ NumLayer ];
		ChunkPos   mPos;
	};



	class TerrainGenerator
	{
	public:
		virtual bool generate( Chunk& chunk , Random& rand ) = 0;

	};


	class LandGenerater : public TerrainGenerator
	{

	public:
		virtual bool generate( Chunk& chunk , Random& rand );

		int height;
		TPerlinNoise<false> mNoise;
	};


	class IChunkEventListener
	{
	public:
		virtual void onChunkAdded(Chunk* chunk) = 0;
		virtual void onPrevRemovChunk(Chunk* chunk) = 0;
	};


	class ChunkProvider
	{
	public:
		ChunkProvider()
		{
			mGeneratePool = new QueueThreadPool();
			mGeneratePool->init(8);
		}

		~ChunkProvider()
		{
			delete mGeneratePool;
		}
		Chunk* getChunk( int x , int y );

		Chunk* getChunk( ChunkPos const& pos, bool bGneRequest = false);

		void update(float deltaTime);

		typedef std::unordered_map< uint64 , Chunk* > ChunkMap;
		ChunkMap mMap;

		Mutex mMutexPendingAdd;
		TArray<Chunk*> mPendingAddChunks;

		IChunkEventListener* mListener = nullptr;
		QueueThreadPool* mGeneratePool;
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
		void     getNeighborBlockIds(Vec3i const& blockPos, BlockId outIds[]);

		void     setBlock( int bx , int by , int bz , BlockId id );
		void     setBlockNotify( int bx , int by , int bz , BlockId id );

		Chunk*   getChunk( int bx , int by )
		{ 
			ChunkPos cPos; cPos.setBlockPos( bx , by );
			return mChunkProvider->getChunk( cPos ); 
		}
		Chunk*  getChunk( ChunkPos const& pos, bool bGneRequest = false){ return mChunkProvider->getChunk( pos , bGneRequest); }


		BlockId rayBlockTest( Vec3f const& pos , Vec3f const& dir , float maxDist ,  BlockPosInfo* info );
		BlockId offsetBBoxBlockTest( Vec3f const& pos , Vec3f const& size , Vec3f const& dir , float dist , BlockPosInfo* info );

		void    addListener( IWorldEventListener& listener );
		void    removeListener( IWorldEventListener& listener );
		void    notifyBlockModified( int bx , int by , int bz );

		void update(float deltaTime)
		{

			mChunkProvider->update(deltaTime);
		}


		typedef std::list< IWorldEventListener* > ListenerList;
		ListenerList   mListeners;
		ChunkProvider* mChunkProvider;
		Random         mRandom;
	};


}//namespace Cube

#endif // CubeWorld_h__
