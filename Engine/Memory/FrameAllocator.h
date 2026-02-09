#ifndef FrameAllocator_h__
#define FrameAllocator_h__

#include "Core/IntegerType.h"
#include <new>

class FrameAllocator
{
public:
	FrameAllocator( size_t size );

	~FrameAllocator();

	void  cleanup();
	void* alloc( size_t size );
	void* alloc( size_t size, size_t alignment );
	void  clearFrame();

	struct Chunk
	{
		int    index;
		size_t size;
		Chunk* link;
#pragma warning(suppress : 4200)
		uint8  storage[0];
	};

	struct StackMarkInfo 
	{
		Chunk*  chunk;
		uint32 offset;
	};

	void markStack(StackMarkInfo& info);
	void freeStack(StackMarkInfo& info);

	Chunk* allocChunk( size_t size );

	void freePageList(Chunk* chunk);

	int     mNextIndex = 0;
	Chunk*  mUsageList;
	Chunk*  mFreeList;
	Chunk*  mCur;
	uint32  mOffset;
};

struct StackMaker
{
	StackMaker(FrameAllocator& allocator)
		:mAllocator(allocator)
	{
		mAllocator.markStack(info);
	}

	~StackMaker()
	{
		mAllocator.freeStack(info);
	}
	FrameAllocator& mAllocator;
	FrameAllocator::StackMarkInfo info;
};

inline void* operator new ( size_t size , FrameAllocator& allocator  )
{
	void* out = allocator.alloc( size , 16 );
	if ( !out )
		throw std::bad_alloc();

	return out;
}

inline void* operator new[] ( size_t size , FrameAllocator& allocator  )
{
	void* out = allocator.alloc( size );
	if ( !out )
		throw std::bad_alloc();

	return out;
}

#endif // FrameAllocator_h__
