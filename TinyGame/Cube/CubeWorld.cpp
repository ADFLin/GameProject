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
		if ( z < 0 ||z >= ChunkBlockMaxHeight )
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
			for( int i = 0 ; i < ChunkSize ; ++i )
			{
				for ( int j = 0 ; j < ChunkSize ; ++j )
				{
					BlockId* pBlockMap = &layer->blockMap[i][j][0];
					for( int k = 0 ; k < LayerSize ; ++k )
					{
						BlockId id = pBlockMap[k];
						if ( id )
							renderer.draw(  i , j , zOff + k ,  pBlockMap[k] );
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

	Chunk* ChunkProvider::getChunk( ChunkPos const& pos )
	{
		uint64 value = pos.hash_value();

		ChunkMap::iterator iter = mMap.find( value );
		if ( iter == mMap.end() )
		{
			Chunk* chunk = new Chunk( pos );

			FlatPlaneGenerater gen;
			gen.height = 64;
			Random rand;
			rand.setSeed( 0 );
			gen.generate( *chunk , rand );

			mMap.insert( std::make_pair( value , chunk ) );
			return chunk;
		}
		return iter->second;
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


	void World::setBlock( int bx , int by , int bz , BlockId id )
	{
		Chunk* chunk = getChunk( bx , by );
		if ( !chunk )
			return;
		chunk->setBlockId( bx , by , bz , id );
	}

	bool World::isOpaqueBlock( int x , int y , int z )
	{
		if ( z < 0 )
			return true;

		BlockId id = getBlockId( x , y , z );
		if ( id == BLOCK_NULL )
			return false;
		return true;
	}


	BlockId World::rayBlockTest( Vec3f const& pos , Vec3f const& dir , float maxDist , BlockPosInfo* info )
	{
		Vec3f nDir = dir;
		if ( nDir.normalize() < 1e-3 )
			return BLOCK_NULL;

		int curPos[3];
		int nextOffset[3];
		for( int i = 0 ; i < 3 ; ++i )
		{
			if ( Math::abs( nDir[i] ) < 1e-7 )
				nextOffset[i] = 0;
			else if ( nDir[i] > 0 )
				nextOffset[i] = 1;
			else
				nextOffset[i] = -1;
			curPos[i] = Math::floor( pos[i] ) + ( nextOffset[i] == 1  ? 1 : 0 );
		}

		float offset[3];
		for( int i = 0 ; i < 3 ; ++i )
		{
			if ( nextOffset[i] )
				offset[i] = ( float( curPos[i] + nextOffset[i] ) - pos[i] ) / nDir[i];
		}

		int count = 0;
		int const MaxIterCount = 100;
		while( 1 )
		{
			++count;
			if ( count > MaxIterCount )
				break;

			int idxMin = -1;
			for ( int i = 0 ; i < 3 ; ++i )
			{
				if ( nextOffset[i] )
				{
					if ( idxMin == -1 || offset[i] < offset[idxMin] )
						idxMin = i;
				}
			}

			curPos[ idxMin ] += nextOffset[ idxMin ];

			BlockId id = getBlockId( curPos[0] , curPos[1] , curPos[2] );
			if ( id )
			{
				Block* block = Block::get( id );
				if ( block )
				{

					if ( info )
					{
						info->x = curPos[0];
						info->y = curPos[1];
						info->z = curPos[2];
						info->face = getFaceSide( idxMin , nextOffset[idxMin] == 1 );
					}
					return id;
				}
			}

			float dist = Math::abs( offset[idxMin] / nDir[ idxMin ] );
			if ( dist > maxDist )
				break;

		
			offset[ idxMin ] = ( float( curPos[idxMin] + nextOffset[idxMin] ) - pos[idxMin] ) / nDir[idxMin];
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
			int num = 2 + Math::floor( len );
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

	void World::notifyBlockModified( int bx , int by , int bz )
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

	bool FlatPlaneGenerater::generate( Chunk& chunk , Random& rand )
	{
	    int chunkHeight = height + chunk.getPos().x + chunk.getPos().y;
		for( int k = 0 ; k < chunkHeight ; ++k )
		{
			for( int i = 0 ; i < ChunkSize ; ++i )
			{
				for( int j = 0 ; j < ChunkSize ; ++j )
				{
					chunk.setBlockId( i , j , k , BLOCK_DIRT );
				}
			}
		}
		return true;
	}


	class PerlinNoiseFun
	{
	
	};

	class OctaveNoiseFun
	{
	public:
		void generate( float* );









	};

}//namespace Cube