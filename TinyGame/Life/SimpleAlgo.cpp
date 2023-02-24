#include "SimpleAlgo.h"

namespace Life
{

	void SimpleAlgo::step()
	{
		int indexPrev = mIndex;
		mIndex = 1 - mIndex;

		for (int j = 0; j < mGrid.getSizeY(); ++j)
		{
			for (int i = 0; i < mGrid.getSizeX(); ++i)
			{
				uint32 count = 0;
				for (auto const& offset : NegihborOffsets)
				{
					Vec2i pos = Vec2i(i, j) + offset;
					if (mGrid.checkRange(pos.x, pos.y))
					{
						if (mGrid(pos.x, pos.y).data[indexPrev])
						{
							++count;
						}
					}
				}

				auto data = mGrid(i, j).data;
				data[mIndex] = mRule.getEvoluteValue(count, data[indexPrev]);
			}
		}
	}

	void SimpleAlgo::getPattern(TArray<Vec2i>& outList)
	{
		for (int j = 0; j < mGrid.getSizeY(); ++j)
		{
			for (int i = 0; i < mGrid.getSizeX(); ++i)
			{
				auto data = mGrid(i, j).data[mIndex];
				if (data)
				{
					outList.push_back(Vec2i(i, j));
				}
			}
		}
	}

	void SimpleAlgo::getPattern(BoundBox const& bound, TArray<Vec2i>& outList)
	{
		BoundBox boundClip = bound.intersection(getBound());
		for (int j = boundClip.min.y; j <= boundClip.max.y; ++j)
		{
			for (int i = boundClip.min.x; i <= boundClip.min.x; ++i)
			{
				auto data = mGrid(i, j).data[mIndex];
				if (data)
				{
					outList.push_back(Vec2i(i, j));
				}
			}
		}
	}

}//namespace Life

