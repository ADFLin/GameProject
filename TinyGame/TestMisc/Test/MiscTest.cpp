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


#include "DataStructure/SkipList.h"


void SkipListTest()
{
	srand(GetTickCount());
	TSkipList<int> myList;

	struct NodeInfo
	{
		int index;
		int prevIndex;
	};

	std::unordered_map< int, NodeInfo > valueMap;
	bool bNeedRefresh = false;
	int  valueOp = INDEX_NONE;

	auto renderScope = FMiscTestUtil::RegisterRender([&](IGraphics2D& g)
	{
		if (bNeedRefresh)
		{
			if (valueOp != INDEX_NONE)
			{
				valueMap.erase(valueOp);
				valueOp = INDEX_NONE;
			}

			bNeedRefresh = false;
			int index = 0;
			for (auto const& value : myList)
			{
				auto iter = valueMap.find(value);
				if (iter == valueMap.end())
				{
					NodeInfo info;
					info.index = index;
					info.prevIndex = INDEX_NONE;
					valueMap.emplace(value, info);
				}
				else
				{
					iter->second.prevIndex = iter->second.index;
					iter->second.index = index;
				}
				++index;
			}
		}

		RenderUtility::SetPen(g, EColor::Black);
		g.setTextColor(Color3f(0.2, 0.2, 0));
		for (int level = 0; level <= myList.getLevel(); ++level)
		{
			auto node = myList.mHeads[level];
			while (node)
			{
				auto info = valueMap[node->value];
				Vector2 size = Vector2(20, 20);
				Vector2 pos = Vector2(10, 10) + Vector2((size.x + 5) * info.index, level * (size.y + 5));

				if (info.prevIndex == INDEX_NONE)
				{
					RenderUtility::SetBrush(g, EColor::Red);
				}
				else
				{
					RenderUtility::SetBrush(g, EColor::Gray, COLOR_LIGHT);
				}
	
				g.drawRect(pos, size);
				g.drawText(pos, size, InlineString<>::Make("%d", node->value));
				node = node->links[level];
			}
		}
		
	}, Vec2i(500, 500), false);


	bool bRunning = true;
	while (bRunning)
	{
		EKeyCode::Type key = FMiscTestUtil::WaitInputKey();
		switch (key)
		{
		case EKeyCode::A:
			myList.insert(rand() % 100);
			bNeedRefresh = true;
			break;
		case EKeyCode::D:
			if (!myList.empty())
			{
				myList.removeIndex(rand() % myList.size(), valueOp);
				bNeedRefresh = true;
			}
			break;
		case EKeyCode::X:
			bRunning = false;
			break;
		}
	}

	for (int i = 1; i <= 20; ++i)
	{
		myList.insert(i);
		bNeedRefresh = true;
		FMiscTestUtil::Pause();
	}
	TEST_CHECK(myList.insert(5) == false);
	myList.remove(5);
	bNeedRefresh = true;
	FMiscTestUtil::Pause();
	TEST_CHECK(myList.remove(5) == false);
}

REGISTER_MISC_TEST_ENTRY("SkipList Test", SkipListTest);