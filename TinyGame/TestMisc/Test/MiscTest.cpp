#include "StageRegister.h"

#include "Memory/BuddyAllocator.h"

void AllocatorTest()
{
	BuddyAllocatorBase allocator;
	allocator.initialize(256, 16);

	auto renderScope = FMiscTestUtil::RegisterRender([&](IGraphics2D& g)
	{
		Vector2 basePos = Vector2(20, 20);
		float   blockSize = float(760) / (float(allocator.mSize) / allocator.mBlockSize);

		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Yellow);
		{
			g.drawRect(basePos, Vector2(BuddyAllocatorBase::OrderToUnitSize(allocator.mMaxOrder) * blockSize, 10));
		}
		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Red);
		for (uint32 order = 0; order <= allocator.mMaxOrder; ++order)
		{
			auto const& freeList = allocator.mFreeLists[order];
			if (freeList.empty())
				continue;

			uint32 uSize = BuddyAllocatorBase::OrderToUnitSize(order);
			for( uint32 offset : freeList )
			{
				Vector2 pos = basePos + Vector2(float(offset) * blockSize, 0);
				Vector2 size = Vector2(float(uSize) * blockSize, 10);
				g.drawRect(pos, size);
			}
		}
	}, Vec2i(800,60));


	FMiscTestUtil::Pause();

	TArray<BuddyAllocatorBase::Allocation> allocations;

	for (int i = 0; i < 10; ++i)
	{
		BuddyAllocatorBase::Allocation allocation;

		int size = 5 + rand() % 64;
		if (allocator.alloc(size, 1, allocation))
		{
			LogMsg("pos = %u", allocation.pos);
			allocations.push_back(allocation);
		}
		else
		{
			LogMsg("allocate fail");
		}
		FMiscTestUtil::Pause();
	}

	while (!allocations.empty())
	{
		int index = rand() % allocations.size();
		allocator.deallocate(allocations[index]);
		FMiscTestUtil::Pause();
		allocations.removeIndexSwap(index);
	}
}

REGISTER_MISC_TEST_ENTRY("Allocator Test", AllocatorTest);

#if CPP_COMPILER_MSVC
#include <intrin.h>
#endif
int BitScanForward(uint32 n)
{
	unsigned long result = 0;
	_BitScanForward(&result, n);
	return result;
}

int BitScanReverse(uint32 n)
{
	unsigned long result = 0;
	_BitScanReverse(&result, n);
	return result;
}

void BitOpTest()
{
	for (uint32 i = 0; i < 32; ++i)
	{
		uint32 v = 1 << i;
		int a = FBitUtility::CountTrailingZeros(v);
		int b = FBitUtility::CountLeadingZeros(v);
		int c = BitScanForward(v);
		int d = BitScanReverse(v);
		LogMsg("%d %d %d %d", a, b, c, d);
	}

}

REGISTER_MISC_TEST_ENTRY("BitOp Test", BitOpTest);