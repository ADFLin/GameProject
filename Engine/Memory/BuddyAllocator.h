#pragma once
#ifndef BuddyAllocator_H_58E49EC4_DA48_4FE0_A9EB_B6F69A0FE500
#define BuddyAllocator_H_58E49EC4_DA48_4FE0_A9EB_B6F69A0FE500

#include "BitUtility.h"
#include "MarcoCommon.h"

#include <vector>
#include <unordered_set>


class BuddyAllocatorBase
{
public:

	uint32 SizeToUnitSize(uint32 size)
	{
		return (size + mBlockSize - 1) / mBlockSize;
	}

	static uint32 UnitSizeToOrder(uint32 uSize)
	{
		return (8 * sizeof(uint32) - FBitUtility::CountLeadingZeros(uSize + uSize - 1) - 1);
	}

	static uint32 OrderToUnitSize(uint32 order) { return 1 << order; }
	static uint32 GetBuddy(uint32 offset, uint32 size)
	{
		return offset ^ size;
	}

	void initialize(uint32 size, uint32 blockSize)
	{
		mSize = size;
		mBlockSize = blockSize;
		CHECK(FBitUtility::IsOneBitSet(size));
		mMaxOrder = UnitSizeToOrder(SizeToUnitSize(size));
		mFreeLists.resize(mMaxOrder + 1);
		mFreeLists[mMaxOrder].emplace(0);
		mSizeUsed = 0;
	}

	struct AllocationBlock
	{
		uint32 offset;
		uint32 order;

	};

	struct Allocation : AllocationBlock
	{
		uint32 pos;
	};

	bool canAllocate(uint32 order);
	bool canAllocate(uint32 size, uint32 alignment);

	bool alloc(uint32 size, uint32 alignment, Allocation& allocation);

	void deallcateAll()
	{
		mSizeUsed = 0;
		for (auto& list : mFreeLists)
		{
			list.clear();
		}
		mFreeLists[mMaxOrder].emplace(0);
	}

	void deallocate(AllocationBlock const& block);

	uint32 allocateBlock(uint32 order);

	void deallocateBlock(uint32 offset, uint32 order);

	uint32 mSize = 0;
	uint32 mBlockSize;
	uint32 mMaxOrder;
	uint32 mSizeUsed = 0;
	std::vector< std::unordered_set< uint32 > > mFreeLists;
};

#endif // BuddyAllocator_H_58E49EC4_DA48_4FE0_A9EB_B6F69A0FE500
