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
		Chunk* newPage = nullptr;
		if (mFreeList)
		{
			if (mFreeList->size > size)
			{
				newPage = mFreeList;
				mFreeList = mFreeList->link;
			}
		}
		if (newPage == nullptr)
		{
			size_t allocSize = 2 * mCur->size;
			while (allocSize < size) { allocSize *= 2; }
			newPage = allocChunk(allocSize);
			CHECK(newPage != nullptr);
		}

		mCur->link = newPage;
		newPage->link = nullptr;
		mCur = newPage;
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
		mUsageList = mFreeList;
		mFreeList = mFreeList->link;
		mUsageList->link = nullptr;
		mCur = mUsageList;
	}
	else
	{
		Chunk* cur = mUsageList;
		while (cur != mCur)
		{
			Chunk* next = cur->link;
			FMemory::Free(cur);
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
		if (mFreeList)
		{
			Chunk* chunk = info.chunk->link;
			while (chunk)
			{
				if (chunk->link == nullptr)
				{
					chunk->link = mFreeList;
					break;
				}
				chunk = chunk->link;
			}
		}

		mFreeList = info.chunk->link;
		mCur = info.chunk;
		mCur->link = nullptr;
	}

	mOffset = info.offset;
}

FrameAllocator::FrameAllocator::Chunk* FrameAllocator::allocChunk(size_t size)
{
	Chunk* chunk = (Chunk*)FMemory::Alloc(size + sizeof(Chunk));
	chunk->index = mNextIndex++;
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
