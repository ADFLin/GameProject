#pragma once
#ifndef SimpleAlgo_H_CD401D34_6728_4388_AFE2_D57E90F69FC1
#define SimpleAlgo_H_CD401D34_6728_4388_AFE2_D57E90F69FC1

#include "LifeCore.h"
#include "DataStructure/Grid2D.h"


namespace Life
{

	class SimpleAlgo : public IAlgorithm
	{
	public:
		SimpleAlgo(int x, int y)
		{
			mGrid.resize(x, y);
			mGrid.fillValue({ 0, 0 });
		}
		virtual bool setCell(int x, int y, uint8 value) final
		{
			if (mGrid.checkRange(x, y))
			{
				mGrid(x, y).data[mIndex] = value;
				return true;
			}
			return false;
		}
		virtual uint8 getCell(int x, int y) final
		{
			if (mGrid.checkRange(x, y))
			{
				return mGrid(x, y).data[mIndex];
			}
			return 0;
		}
		virtual void clearCell() final
		{
			mGrid.fillValue({ 0, 0 });
		}
		virtual BoundBox getBound() final
		{
			return getLimitBound();
		}
		virtual BoundBox getLimitBound() final
		{
			BoundBox result;
			result.min = Vec2i::Zero();
			result.max = Vec2i(mGrid.getSizeX() - 1, mGrid.getSizeY() - 1);
			return result;
		}
		virtual void  step() final;

		virtual void  getPattern(std::vector<Vec2i>& outList) final;

		virtual void  getPattern(BoundBox const& bound, std::vector<Vec2i>& outList) final;

		struct DataValue
		{
			uint8 data[2];
		};
		int mIndex = 0;
		TGrid2D< DataValue > mGrid;
		Rule mRule;
	};

}//namespace Life

#endif // SimpleAlgo_H_CD401D34_6728_4388_AFE2_D57E90F69FC1