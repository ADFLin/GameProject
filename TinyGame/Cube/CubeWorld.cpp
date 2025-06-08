#include "CubePCH.h"
#include "CubeWorld.h"

#include "CubeBlock.h"
#include "CubeRandom.h"
#include "CubeBlockType.h"

#include "CubeBlockRenderer.h"
#include "IWorldEventListener.h"

#include <algorithm>


namespace Cube
{
	Chunk::Chunk( ChunkPos const& pos )
		:mPos( pos )
	{
		std::fill_n( mLayer , NumLayer , static_cast< LayerData*>(0) );
	}

	BlockId Chunk::getBlockId( int x , int y , int z )
	{
		if (z < 0)
			return BLOCK_BASE;
		if (z >= ChunkBlockMaxHeight)
			return BLOCK_NULL;

		LayerData* layer = getLayer( z );
		if ( !layer )
			return BLOCK_NULL;
		int mx = x & ChunkMask;
		int my = y & ChunkMask;
		int mz = z & LayerMask;

		return layer->blockMap[ x & ChunkMask ][ y & ChunkMask ][ z & LayerMask ];
	}

	void Chunk::setBlockId( int x , int y , int z , BlockId id )
	{
		if ( z < 0 || z >= ChunkBlockMaxHeight )
			return;

		LayerData* layer = getLayer( z );
		if ( !layer )
		{
			if ( id == BLOCK_NULL )
				return;
			layer = new LayerData;
			mLayer[ z >> LayerBit ] = layer;
		}

		layer->blockMap[ x & ChunkMask ][ y & ChunkMask ][ z & LayerMask ] = id;
	}

	MetaType Chunk::getBlockMeta( int x , int y , int z )
	{
		if ( z < 0 ||z >= ChunkBlockMaxHeight )
			return 0;
		LayerData* layer = getLayer( z );
		if ( !layer )
			return 0;

		uint32 meta = layer->meta[ x & ChunkMask ][ y & ChunkMask ][ ( z & LayerMask ) / 2 ];
		return ( z & 0x1 ) ? ( meta & 0xf ) : ( meta >> 4 );
	}
	void     Chunk::setBlockMeta( int x , int y , int z , MetaType meta )
	{
		assert( meta < 16 );

		if ( z < 0 || z >= ChunkBlockMaxHeight )
			return;

		LayerData* layer = getLayer( z );
		if ( !layer )
		{
			layer = new LayerData;
			mLayer[ z >> LayerBit ] = layer;
		}

		uint8& holdMeta = layer->meta[ x & ChunkMask ][ y & ChunkMask ][ z & LayerMask ];

		if ( z & 1 )
			holdMeta = ( holdMeta & 0xf0 ) | meta;
		else
			holdMeta = ( meta << 4 ) | ( holdMeta & 0xf);
	}

	void Chunk::render( BlockRenderer& renderer )
	{
		renderer.setBasePos( Vec3i( mPos.x * ChunkSize , mPos.y * ChunkSize , 0 ) );

		for( int n = 0 ; n < NumLayer ; ++n )
		{
			LayerData* layer = mLayer[ n ];
			if ( !layer )
				continue;

			int zOff = n * LayerSize;

			Vec3i offset;
			for(offset.x = 0 ; offset.x < ChunkSize ; ++offset.x )
			{
				for (offset.y = 0 ; offset.y < ChunkSize ; ++offset.y )
				{
					BlockId* pBlockMap = &layer->blockMap[offset.x][offset.y][0];
					for( int k = 0 ; k < LayerSize ; ++k )
					{
						BlockId id = pBlockMap[k];
						if (id)
						{
							offset.z = zOff + k;
							renderer.draw(offset, id);
						}

					}
				}
			}
		}
	}


	Chunk* ChunkProvider::getChunk( int x , int y )
	{
		ChunkPos cPos;
		cPos.setBlockPos( x , y );
		return getChunk( cPos );
	}


	class ChunkGenerateWork : public IQueuedWork
	{
	public:

		virtual void executeWork()
		{
			chunk->state = EChunkLoadState::Generating;
			LandGenerater gen;
			gen.height = 64;
			Random rand;
			rand.setSeed(0);
			gen.generate(*chunk, rand);
			chunk->state = EChunkLoadState::Ok;

		}
		virtual void abandon()
		{
			chunk->state = EChunkLoadState::Unintialized;
		}

		virtual void release()
		{
			delete this;
		}

		ChunkProvider* provider;
		Chunk* chunk;
	};


	Chunk* ChunkProvider::getChunk(ChunkPos const& pos, bool bGneRequest)
	{
		uint64 value = pos.hash_value();

		ChunkMap::iterator iter = mMap.find( value );
		if ( iter == mMap.end() )
		{
			if (bGneRequest)
			{
				{
					Mutex::Locker locker(mMutexPendingAdd);
					for (auto chunk : mPendingAddChunks)
					{
						if (chunk->getPos() == pos)
							return nullptr;
					}
				}
				Chunk* chunk = new Chunk(pos);
				chunk->state = EChunkLoadState::Unintialized;
				ChunkGenerateWork* work = new ChunkGenerateWork;
				work->chunk = chunk;
				work->provider = this;
				mGeneratePool->addWork(work);

				Mutex::Locker locker(mMutexPendingAdd);
				mPendingAddChunks.push_back(chunk);

			}

			return nullptr;
		}

		if (iter->second->state != EChunkLoadState::Ok)
			return nullptr;

		return iter->second;
	}


	void ChunkProvider::update(float deltaTime)
	{
		{
			Mutex::Locker locker(mMutexPendingAdd);
			for (int index = 0; index < mPendingAddChunks.size(); ++index)
			{
				Chunk* chunk = mPendingAddChunks[index];

				if (chunk->state == EChunkLoadState::Ok)
				{
					uint64 value = chunk->getPos().hash_value();
					mMap.insert(std::make_pair(value, chunk));
					mListener->onChunkAdded(chunk);
					mPendingAddChunks.removeIndexSwap(index);
					--index;
				}
			}
		}
	}

	World::World()
	{
		mChunkProvider = new ChunkProvider;
	}

	Cube::BlockId World::getBlockId( int bx , int by , int bz )
	{
		Chunk* chunk = mChunkProvider->getChunk( bx , by );
		if ( !chunk )
			return BLOCK_NULL;
		return chunk->getBlockId( bx , by , bz );
	}

	Cube::MetaType World::getBlockMeta( int bx , int by , int bz )
	{
		Chunk* chunk = getChunk( bx , by );
		if ( !chunk )
			return 0;
		return chunk->getBlockMeta( bx , by , bz );
	}

	void World::getNeighborBlockIds(Vec3i const& blockPos, BlockId outIds[])
	{
		for (int i = 0; i < FaceSide::COUNT; ++i)
		{
			Vec3i pos = blockPos + GetFaceOffset(FaceSide(i));

			Chunk* chunk = mChunkProvider->getChunk(pos.x, pos.y);
			if (chunk)
			{
				outIds[i] = chunk->getBlockId(pos.x, pos.y, pos.z);
			}
			else
			{
				outIds[i] = BLOCK_NULL;
			}
		}
	}

	void World::setBlock( int bx , int by , int bz , BlockId id )
	{
		Chunk* chunk = getChunk( bx , by );
		if ( !chunk )
			return;
		chunk->setBlockId( bx , by , bz , id );
	}


	BlockId World::rayBlockTest( Vec3f const& pos , Vec3f const& dir , float maxDist , BlockPosInfo* info )
	{
		Vec3f nDir = dir;
		if ( nDir.normalize() < 1e-4 )
			return BLOCK_NULL;

		int curPos[3];
		float distStep[3];
		float nextDist[3];
		int posOffset[3];

		for( int i = 0 ; i < 3 ; ++i )
		{
			curPos[i] = Math::FloorToInt(pos[i]);
			if ( Math::Abs( nDir[i] ) < 1e-7 )
			{
				posOffset[i] = 0;
				distStep[i] = 0.0;
				nextDist[i] = 0;
			}
			else if ( nDir[i] > 0 )
			{ 
				posOffset[i] = 1;
				distStep[i] = 1.0 / nDir[i];
				nextDist[i] = (1.0 - (pos[i] - curPos[i])) / nDir[i];
			}
			else
			{ 
				posOffset[i] = -1;
				distStep[i] = 1.0 / -nDir[i];
				nextDist[i] = -(pos[i] - curPos[i]) / nDir[i];
			}
		}

		float dist = 0.0;

		int const MaxIterCount = 100;
		for( int iter = 0; iter < MaxIterCount ; ++iter)
		{
			int idxMin = -1;
			for ( int i = 0 ; i < 3 ; ++i )
			{
				if ( posOffset[i] )
				{
					if ( idxMin == -1 || nextDist[i] < nextDist[idxMin] )
						idxMin = i;
				}
			}

			curPos[ idxMin ] += posOffset[ idxMin ];

			BlockId id = getBlockId( curPos[0] , curPos[1] , curPos[2] );
			if ( id )
			{
				Block* block = Block::Get( id );
				if ( block )
				{
					if ( info )
					{
						info->x = curPos[0];
						info->y = curPos[1];
						info->z = curPos[2];
						info->face = getFaceSide( idxMin , posOffset[idxMin] == 1 );
					}
					return id;
				}
			}

			float deltaDist = nextDist[idxMin];
			dist += deltaDist;
			if ( dist > maxDist )
				break;

			for (int i = 0; i < 3; ++i)
			{
				if (posOffset[i])
				{
					nextDist[i] -= deltaDist;
				}
			}

			nextDist[idxMin] = distStep[idxMin];
		}

		return BLOCK_NULL;
	}


	unsigned calcDirFaceBits( Vec3f const& dir )
	{
		float const zeroError = 1e-4;
		unsigned result = 0;
		if (  dir.x > zeroError )
			result |= BIT( FACE_X );
		else if (  dir.x < -zeroError )
			result |= BIT( FACE_NX );

		if (  dir.y > zeroError )
			result |= BIT( FACE_Y );
		else if (  dir.y < -zeroError )
			result |= BIT( FACE_NY );

		if (  dir.z > zeroError )
			result |= BIT( FACE_Z );
		else if (  dir.z < -zeroError )
			result |= BIT( FACE_NZ );

		return result;
	}

	BlockId World::offsetBBoxBlockTest( Vec3f const& min , Vec3f const& max , Vec3f const& dir , float dist , BlockPosInfo* info )
	{
		unsigned faceBits = calcDirFaceBits( dir );

		for ( int axis = 0 ; axis < 3 ; ++axis )
		{
			FaceSide curFace;

			float len = max[ axis ] - min[ axis ];
			int num = 2 + Math::FloorToInt( len );
			float offset = len / ( num - 1 );

			Vec3f const& pos = min;
			for ( int i = 0 ; i < num ; ++i )
			{
				Vec3f const& pos = min;
			}

			if ( faceBits & getFaceSide( axis , false ) )
			{



			}
			else if ( faceBits & getFaceSide( axis , true ) )
			{



			}
		}


		return BLOCK_NULL;
	}

	void World::setBlockNotify( int bx , int by , int bz , BlockId id )
	{
		setBlock( bx , by , bz , id );
		notifyBlockModified( bx , by , bz );
	}

	void World::notifyBlockModified(int bx, int by, int bz)
	{
		for( ListenerList::iterator iter = mListeners.begin();
			iter != mListeners.end() ; ++iter )
		{
			(*iter)->onModifyBlock( bx , by , bz );
		}
	}

	void World::removeListener( IWorldEventListener& listener )
	{
		ListenerList::iterator iter = std::find( mListeners.begin() , mListeners.end() , &listener );
		if ( iter != mListeners.end() )
			mListeners.erase( iter );
	}

	void World::addListener( IWorldEventListener& listener )
	{
		assert( std::find( mListeners.begin() , mListeners.end() , &listener ) == mListeners.end() );
		mListeners.push_back( &listener );
	}

	bool LandGenerater::generate( Chunk& chunk , Random& rand )
	{
	    //int chunkHeight = height + chunk.getPos().x + chunk.getPos().y;
		Vec2i offset = ChunkSize * chunk.getPos();

		for( int i = 0 ; i < ChunkSize ; ++i )
		{
			for( int j = 0 ; j < ChunkSize ; ++j )
			{
				float scale = 24.0f / (256.0f * 10);
				float nH = mNoise.getValue(float(offset.x + i) * scale, float(offset.y + j) * scale);
				int chunkHeight = Math::FloorToInt(height + 50 * nH);
				for (int k = 0; k < chunkHeight; ++k)
				{
					chunk.setBlockId(i, j, k, BLOCK_DIRT);
				}
			}
		}
		return true;
	}


	class PerlinNoiseFunc
	{
	
	};

	class OctaveNoiseFunc
	{
	public:
		void generate( float* );









	};

}//namespace Cube