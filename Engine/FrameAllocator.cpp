#include "FrameAllocator.h"
#include "MarcoCommon.h"
#include "Core/Memory.h"


FrameAllocator::FrameAllocator(size_t size)
{
	mOffset = 0;

	mCur = allocChunk(size);
	mCur->link = nullptr;
	mUsageList = mCur;
	mFreeList = nullptr;
}

FrameAllocator::~FrameAllocator()
{
	freePageList(mUsageList);
	freePageList(mFreeList);
}

void FrameAllocator::cleanup()
{
	freePageList(mUsageList);
	mUsageList = nullptr;
	freePageList(mFreeList);
	mFreeList = nullptr;
}

void* FrameAllocator::alloc(size_t size)
{
	if (mCur->size < mOffset + size)
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
			{
				return nullptr;
			}
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

void FrameAllocator::clearFrame()
{
	if (mFreeList)
	{
		CHECK(mUsageList);
		freePageList(mUsageList);
		mCur = mFreeList;
		mFreeList = mCur->link;
		mCur->link = nullptr;
	}
	else
	{
		Chunk* cur = mUsageList;
		while (cur != mCur)
		{
			Chunk* next = cur->link;
			::free(cur);
			cur = next;
		}
		mUsageList = mCur;
	}
	mOffset = 0;
}

void FrameAllocator::markStack(StackMarkInfo& info)
{
	info.chunk = mCur;
	info.offset = mOffset;
}

void FrameAllocator::freeStack(StackMarkInfo& info)
{
	CHECK(info.chunk);
	if (info.chunk != mCur)
	{
		mFreeList = info.chunk->link;
		mCur = info.chunk;
		mCur->link = nullptr;
	}

	mOffset = info.offset;
}

FrameAllocator::FrameAllocator::Chunk* FrameAllocator::allocChunk(size_t size)
{
	Chunk* chunk = (Chunk*)FMemory::Alloc(size + sizeof(Chunk));
	chunk->size = size;
	return chunk;
}

void FrameAllocator::freePageList(Chunk* chunk)
{
	while (chunk)
	{
		Chunk* next = chunk->link;
		FMemory::Free(chunk);
		chunk = next;
	}
}
