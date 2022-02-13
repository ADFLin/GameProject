#ifndef FrameAllocator_h__
#define FrameAllocator_h__

#include "Core/IntegerType.h"
#include <new>

class FrameAllocator
{
public:
	FrameAllocator( size_t size )
	{
		mOffset = 0;

		mCur = allocChunk( size );
		mCur->link = nullptr;
		mUsageList = mCur;
		mFreeList = nullptr;
	}

	~FrameAllocator()
	{
		freePageList(mUsageList);
		freePageList(mFreeList);
	}

	void cleanup()
	{
		freePageList(mUsageList);
		mUsageList = nullptr;
		freePageList(mFreeList);
		mFreeList = nullptr;
	}

	void* alloc( size_t size )
	{
		if ( mCur->size < mOffset + size )
		{
			Chunk* page = nullptr;
			if (mFreeList)
			{
				if (mFreeList->size > size)
				{
					page = mFreeList;
					mFreeList = mFreeList->link;
				}
			}
			if (page == nullptr)
			{
				size_t allocSize = 2 * mCur->size;
				while (allocSize < size) { allocSize *= 2; }
				page = allocChunk(allocSize);
				if (page == nullptr)
					throw std::bad_alloc();
			}

			mCur->link = page;
			page->link = nullptr;
			mCur = page;
			mOffset = 0;
		}

		uint8* out = &mCur->storage[0] + mOffset;
		mOffset += size;
		return out;
	}

	void clearFrame()
	{
		if (mFreeList)
		{
			assert(mUsageList);
			freePageList(mUsageList);
			mCur = mFreeList;
			mFreeList = mCur->link;
			mCur->link = nullptr;
		}
		else
		{
			Chunk* cur = mUsageList;
			while ( cur != mCur )
			{
				Chunk* next = cur->link;
				::free(cur);
				cur = next;
			}
			mUsageList = mCur;
		}
		mOffset = 0;
	}

	struct Chunk
	{
		size_t size;
		Chunk* link;
		uint8  storage[0];
	};


	struct StackMarkInfo 
	{
		Chunk*  chunk;
		uint32 offset;
	};

	void markStack(StackMarkInfo& info)
	{
		info.chunk = mCur;
		info.offset = mOffset;
	}

	void  freeStack(StackMarkInfo& info)
	{
		assert(info.chunk);
		if (info.chunk != mCur)
		{
			mFreeList = info.chunk->link;
			mCur = info.chunk;
			mCur->link = nullptr;
		}

		mOffset = info.offset;
	}

	Chunk* allocChunk( size_t size )
	{
		Chunk* chunk = (Chunk*)::malloc( size + sizeof( Chunk ) );
		chunk->size = size;
		return chunk;
	}

	void freePageList(Chunk* chunk)
	{
		while (chunk)
		{
			Chunk* next = chunk->link;
			::free(chunk);
			chunk = next;
		}
	}

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
	void* out = allocator.alloc( size );
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
