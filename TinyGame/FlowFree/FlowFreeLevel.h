#pragma once
#ifndef FlowFreeLevel_H_E6A7EB14_BFE9_4FFE_B10C_2E1493A9CC34
#define FlowFreeLevel_H_E6A7EB14_BFE9_4FFE_B10C_2E1493A9CC34

#include "DataStructure/Grid2D.h"

#include <vector>

namespace FlowFree
{
	int const DirCount = 4;
	Vec2i const gDirOffset[] = { Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1) };
	
	using ColorType = uint8;
	using EdgeMask = uint8;

	namespace EDir
	{
		enum
		{
			Right,
			Bottom,
			Left,
			Top,
		};
	}
	FORCEINLINE int InverseDir(int dir)
	{
		return (dir + (DirCount / 2)) % DirCount;
	}
	FORCEINLINE void WarpPos(int& p, int size)
	{
		if (p < 0)
			p += size;
		else if (p >= size)
			p -= size;
	}

	enum class CellFunc : uint8
	{
		Empty = 0,
		Source,
		Bridge,
		Block,
	};

	struct Cell
	{
		Cell();

		int getColor(int dir) const { return colors[dir]; }

		ColorType     colors[DirCount];
		CellFunc      func;
		uint8         funcMeta;
		EdgeMask      blockMask;
		mutable uint8 bFilledCached : 1;
		mutable uint8 bFilledCachedDirty : 1;

		void removeFlow(int dir)
		{
			colors[dir] = 0;
		}

		void setSource(ColorType color)
		{
			assert(color);
			func = CellFunc::Source;
			funcMeta = color;
		}

		ColorType evalFlowOut(int dir) const
		{
			if (colors[dir])
				return 0;

			return getFunFlowColor(dir);
		}

		int  getNeedBreakDirs(int dir, int outDirs[DirCount]);

		bool isFilled() const;

		int  getLinkedFlowDir(int dir) const;

		ColorType getFunFlowColor(int dir = -1, ColorType inColor = 0) const;

		void flowOut(int dir, ColorType color)
		{
			colors[dir] = color;
			bFilledCachedDirty = true;
		}

		bool flowIn(int dir, ColorType color)
		{
			if (colors[dir])
				return false;

			if (getFunFlowColor(dir, color) == 0)
				return false;

			colors[dir] = color;
			bFilledCachedDirty = true;
			return true;
		}
	};

	class Level
	{
	public:

		void setup(Vec2i const& size);

		void addMapBoundBlock(bool bHEdge = true , bool bVEdge= true);
		Vec2i getSize() const
		{
			return Vec2i(mCellMap.getSizeX(), mCellMap.getSizeY());
		}

		bool isValidCellPos(Vec2i const& cellPos) const
		{
			return Vec2i(0, 0) <= cellPos && cellPos < Vec2i(mCellMap.getSizeX(), mCellMap.getSizeY());
		}

		void addSource(Vec2i const& posA, ColorType color)
		{
			getCellChecked(posA).setSource(color);
			mSourceLocations.push_back(posA);
		}

		void addSource(Vec2i const& posA, Vec2i const& posB, ColorType color)
		{
			getCellChecked(posA).setSource(color);
			mSourceLocations.push_back(posA);
			getCellChecked(posB).setSource(color);
			mSourceLocations.push_back(posB);
		}

		void setCellFunc(Vec2i const& posA, CellFunc func)
		{
			assert(func != CellFunc::Source);
			Cell& cell = getCellChecked(posA);
			cell.func = func;
			cell.funcMeta = 0;
		}

		void setCellBlocked(Vec2i const& pos, int dir)
		{
			Vec2i linkPos = getLinkPos(pos, dir);
			getCellChecked(pos).blockMask |= BIT(dir);
			getCellChecked(linkPos).blockMask |= BIT(InverseDir(dir));
		}

		bool getLinkSource(Vec2i const& inPos, Vec2i& outLinkPos) const
		{
			Cell const& cell = getCellChecked(inPos);

			assert(cell.func == CellFunc::Source);
		}

		Cell const& getCellChecked(Vec2i const& pos) const
		{
			return mCellMap.getData(pos.x, pos.y);
		}

		Cell& getCellChecked(Vec2i const& pos)
		{
			return mCellMap.getData(pos.x, pos.y);
		}

		enum class BreakResult
		{
			NoBreak,
			HaveBreak,
			HaveBreakSameColor,
		};

		int checkFilledCellCount() const
		{
			int count = 0;
			visitAllCells([&count](Cell const& cell)
			{
				if (cell.isFilled())
					++count;
			});
			return count;
		}

		BreakResult breakFlow(Vec2i const& pos, int dir, ColorType curColor);

		Cell* findSourceCell(Vec2i const& pos, int dir, int& outDist);

		void removeFlowLink(Vec2i const& pos, int dir);

		void removeFlowToEnd(Vec2i const& pos, int dir);


		ColorType linkFlow(Vec2i const& pos, int dir);

		Vec2i getLinkPos(Vec2i const& pos, int dir)
		{
			assert(mCellMap.checkRange(pos.x, pos.y));

			Vec2i result = pos + gDirOffset[dir];
			WarpPos(result.x, mCellMap.getSizeX());
			WarpPos(result.y, mCellMap.getSizeY());
			return result;
		}

		Cell* getCellInternal(Vec2i const& pos)
		{
			if (!mCellMap.checkRange(pos.x, pos.y))
				return nullptr;
			return &mCellMap.getData(pos.x, pos.y);
		}


		template< class TFunc >
		void visitAllCellsBreak(TFunc&& func) const
		{
			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				if (func(mCellMap.getRawData()[i]))
					return;
			}
		}
		template< class TFunc >
		void visitAllCells(TFunc&& func) const
		{
			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				func(mCellMap.getRawData()[i]);
			}
		}

		template< class TFunc >
		void visitAllSourcesBreak(TFunc&& func) const
		{
			for (auto const& pos : mSourceLocations)
			{
				if (func(pos, getCellChecked(pos)))
					return;
			}
		}
		template< class TFunc >
		void visitAllSources(TFunc&& func) const
		{
			for (auto const& pos : mSourceLocations)
			{
				func(pos, getCellChecked(pos));
			}
		}

		TGrid2D< Cell > mCellMap;
		std::vector< Vec2i > mSourceLocations;
	};


}//namespace FlowFree

#endif // FlowFreeLevel_H_E6A7EB14_BFE9_4FFE_B10C_2E1493A9CC34
