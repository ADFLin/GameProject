#include "BuddyAllocator.h"

#include "Math/Base.h"

bool BuddyAllocatorBase::canAllocate(uint32 order)
{
	for (uint32 i = order; i <= mMaxOrder; ++i)
	{
		if (!mFreeLists[i].empty())
			return true;
	}
	return false;
}

bool BuddyAllocatorBase::canAllocate(uint32 size, uint32 alignment)
{
	uint32 allocateSize = size;
	if (alignment != 0 && mBlockSize % alignment != 0)
	{
		allocateSize = size + alignment;
	}

	uint32 uSize = SizeToUnitSize(allocateSize);
	uint32 order = UnitSizeToOrder(uSize);
	return canAllocate(order);
}

bool BuddyAllocatorBase::alloc(uint32 size, uint32 alignment, Allocation& allocation)
{
	uint32 allocateSize = size;
	if (alignment != 0 && mBlockSize % alignment != 0)
	{
		allocateSize = size + alignment;
	}

	uint32 uSize = SizeToUnitSize(allocateSize);
	uint32 order = UnitSizeToOrder(uSize);

	if (!canAllocate(order))
		return false;

	uint32 offset = allocateBlock(order);
	uint32 pos = offset * mBlockSize;
	if (alignment != 0 && mBlockSize % alignment != 0)
	{
		pos = AlignArbitrary(pos, alignment);
	}

	uint32 sizeAllocated = mBlockSize * uSize;
	mSizeUsed += sizeAllocated;

	allocation.pos = pos;
	allocation.order = order;
	allocation.offset = offset;
	allocation.maxSize = sizeAllocated;

	return true;
}

void BuddyAllocatorBase::deallocate(AllocationBlock const& block)
{
	deallocateBlock(block.offset, block.order);
	mSizeUsed -= mBlockSize * OrderToUnitSize(block.order);
}

uint32 BuddyAllocatorBase::allocateBlock(uint32 order)
{
	CHECK(order <= mMaxOrder);
	int offset;

	if (mFreeLists[order].empty())
	{
		uint32 left = allocateBlock(order + 1);
		uint32 uSize = OrderToUnitSize(order);
		uint32 right = left + uSize;
		mFreeLists[order].emplace(right);
		offset = left;
	}
	else
	{
		auto iter = mFreeLists[order].begin();
		offset = *iter;
		mFreeLists[order].erase(iter);
	}

	return offset;
}

void BuddyAllocatorBase::deallocateBlock(uint32 offset, uint32 order)
{
	uint32 uSize = OrderToUnitSize(order);
	uint32 offsetBuddy = GetBuddy(offset, uSize);

	auto iter = mFreeLists[order].find(offsetBuddy);
	if (iter != mFreeLists[order].end())
	{
		mFreeLists[order].erase(iter);
		deallocateBlock(Math::Min(offset, offsetBuddy), order + 1);
	}
	else
	{
		mFreeLists[order].emplace_hint(iter, offset);
	}
}
