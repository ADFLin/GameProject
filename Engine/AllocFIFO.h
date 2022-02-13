#ifndef AllocFIFO_h__
#define AllocFIFO_h__

#define  ALLOC_FREE_CHECK

void* SystemAlloc( size_t size )
{

	::VirtualAlloc( )

}

//                     chunk
//|--------------------------------------------------|
//|-------------------|
class AllocatorFIFO
{
public:
	void* alloc( size_t size )
	{
		Chunk* chunk = mChunk;
		void* ptr = allocFromChunk( chunk , size );
		if ( ptr )
			return ptr;
		chunk = chunk->next;
		if ( chunk == NULL )
		{
			chunk = createChunk( mChunkSize );
			return allocFromChunk( chunk , size );
		}
	}
	void  free( void* ptr )
	{
		Block* block = reinterpret_cast< Block* >( (char*)ptr - sizeof( Block ) );
		Chunk* chunk;
		Block* prev;
	}

	void* allocFromChunk( Chunk* chunk , size_t size )
	{

	}

	Chunk* createChunk( size_t size )
	{
		Chunk* chunk = ( Chunk* )::malloc( size + sizeof( Chunk ));
		chunk->next  = mChunk;
		mChunk = chunk;
		chunk->size   = size;
		chunk->unused = sizeof( Chunk );

		Block* block = (Block*)( (char*)( chunk ) + chunk->unused );
		block->size = size;
		block->next = 0;
		return chunk;
	}

	typedef unsigned short DiffType;
	struct Block
	{
		DiffType offset;
		DiffType size;
		DiffType next;
	};
	struct Chunk
	{
		size_t    size;
		DiffType  maxSize;
		DiffType  unused;
		Chunk*    next;
	};

	size_t mChunkSize;
	Chunk* mChunk;
};

class AllocDoubleBuffer
{





};

#endif // AllocFIFO_h__
