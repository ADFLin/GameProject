#pragma once
#ifndef FlowFreeLevel_H_E6A7EB14_BFE9_4FFE_B10C_2E1493A9CC34
#define FlowFreeLevel_H_E6A7EB14_BFE9_4FFE_B10C_2E1493A9CC34

#include "DataStructure/Grid2D.h"
#include "Math/TVector2.h"
#include "DataStructure/Array.h"

namespace FlowFree
{
	using Vec2i = TVector2<int>;

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
	FORCEINLINE bool WarpPos(int& p, int size)
	{
		if (p < 0)
		{
			p += size;
			return true;
		}
		else if (p >= size)
		{
			p -= size;
			return true;
		}
		return false;
	}

	enum class CellFunc : uint8
	{
		Empty = 0,
		Source,
		Bridge,
		Tunnel,
		Block,
	};

	struct Cell
	{
		Cell();

		int getColor(int dir) const { return colors[dir]; }

		union
		{
			ColorType  colors[DirCount];
			uint32     colorValue;
		};

		CellFunc      func;
		uint8         funcMeta;
		EdgeMask      blockMask;

		union
		{
			struct
			{
				mutable uint8 bFilledCached : 1;
				mutable uint8 bFilledCachedDirty : 1;
				uint8         bCompleted : 1;
				uint8         bStartSource : 1;
				uint8         paddingFlags : 4;
			};
			uint8 flags;
		};

		void removeFlow(int dir)
		{
			colors[dir] = 0;
			flags = 0;
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

			return getFuncFlowColor(dir);
		}

		int  getNeedBreakDirs(int dir, int outDirs[DirCount]);

		bool isFilled() const;

		int  getLinkedFlowDir(int dir) const;

		ColorType getFuncFlowColor(int dir = -1, ColorType inColor = 0) const;

		void flowClear()
		{
			colorValue = 0;
			flags = 0;
		}
		void flowOut(int dir, ColorType color)
		{
			colors[dir] = color;
			bFilledCachedDirty = true;
		}

		bool flowIn(int dir, ColorType color)
		{
			if (colors[dir])
				return false;

			if (getFuncFlowColor(dir, color) == 0)
				return false;

			colors[dir] = color;
			bFilledCachedDirty = true;
			return true;
		}

		void restore(uint32 value)
		{
			colorValue = value;
			bFilledCachedDirty = true;
		}
	};

	struct LevelState
	{
		TGrid2D< uint32 > colors;
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

		void saveState(LevelState& outState) const
		{
			if (outState.colors.getSize() != mCellMap.getSize())
			{
				outState.colors.resize(mCellMap.getSizeX(), mCellMap.getSizeY());
			}

			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				Cell const& cell = mCellMap[i];
				outState.colors[i] = cell.colorValue;
			}
		}

		void restoreState(LevelState const& state)
		{
			CHECK(state.colors.getSize() == mCellMap.getSize());

			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				Cell& cell = mCellMap[i];
				cell.restore(state.colors[i]);
			}
		}

		bool addSource(Vec2i const& pos, ColorType color)
		{
			getCellChecked(pos).setSource(color);
			for (int i = 0; i < mSources.size(); ++i)
			{
				SourceInfo& source = mSources[i];
				Cell& cell = getCellChecked(source.posPair[0]);
				if (cell.funcMeta == color)
				{
					if (source.posPair[1] != Vec2i(INDEX_NONE, INDEX_NONE))
					{
						return false;
					}
					source.posPair[1] = pos;
					return true;
				}
			}

			SourceInfo source;
			source.posPair[0] = pos;
			source.posPair[1] = Vec2i(INDEX_NONE, INDEX_NONE);
			mSources.push_back(source);
			return true;
		}

		void addSource(Vec2i const& posA, Vec2i const& posB, ColorType color)
		{
			getCellChecked(posA).setSource(color);
			getCellChecked(posB).setSource(color);
			SourceInfo info;
			info.posPair[0] = posA;
			info.posPair[1] = posB;
			mSources.push_back(info);
		}

		int getSourceCount() const
		{
			return mSources.size();
		}

		Vec2i getSourcePos(ColorType color);

		void updateGobalStatus();

		void setCellFunc(Vec2i const& pos, CellFunc func)
		{
			assert(func != CellFunc::Source);
			Cell& cell = getCellChecked(pos);
			cell.func = func;
			cell.funcMeta = 0;

			if (func == CellFunc::Block)
			{
				for (int dir = 0; dir < DirCount; ++dir)
				{
					setBlockLinklBlocked(pos, dir);
				}
			}
		}

		void setBlockLinklBlocked(Vec2i const& pos, int dir)
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
			visitCells([&count](Cell const& cell)
			{
				if (cell.isFilled())
					++count;
			});
			return count;
		}

		BreakResult breakFlow(Vec2i const& pos, int dir, ColorType curColor);

		void clearAllFlows();

		Cell* findSourceCell(Vec2i const& pos, int dir, int& outDist);

		void removeFlowLink(Vec2i const& pos, int dir);

		void removeFlowToEnd(Vec2i const& pos, int dir);


		ColorType linkFlow(Vec2i const& pos, int dir);

		Vec2i getLinkPos(Vec2i const& pos, int dir)
		{
			bool bWarpPos;
			return getLinkPos(pos, dir, bWarpPos);
		}

		Vec2i getLinkPos(Vec2i const& pos, int dir, bool& bWarpPos)
		{
			assert(mCellMap.checkRange(pos.x, pos.y));

			Vec2i result = pos + gDirOffset[dir];
			bWarpPos |= WarpPos(result.x, mCellMap.getSizeX());
			bWarpPos |= WarpPos(result.y, mCellMap.getSizeY());
			return result;
		}

		Cell* getCellInternal(Vec2i const& pos)
		{
			if (!mCellMap.checkRange(pos.x, pos.y))
				return nullptr;
			return &mCellMap.getData(pos.x, pos.y);
		}

		template< typename TFunc >
		bool visitCells(TFunc&& func) const
		{
			constexpr bool bCanBreak = !Meta::IsSameType< decltype(func(Cell())), void >::Value;
			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				if constexpr (bCanBreak)
				{
					if (func(mCellMap.getRawData()[i]))
						return true;
				}
				else
				{
					func(mCellMap.getRawData()[i]);
				}
			}
			return false;
		}

		template< typename TFunc >
		bool visitCells(TFunc&& func)
		{
			constexpr bool bCanBreak = !Meta::IsSameType< decltype(func(Cell())), void >::Value;

			for (int i = 0; i < mCellMap.getRawDataSize(); ++i)
			{
				if constexpr (bCanBreak)
				{
					if (func(mCellMap.getRawData()[i]))
						return true;
				}
				else
				{
					func(mCellMap.getRawData()[i]);
				}
			}
			return false;
		}


		template< typename TFunc >
		bool visitSources(TFunc&& func)
		{
			constexpr bool bCanBreak = !Meta::IsSameType< decltype(func((Vec2i*)(nullptr), Cell(), Cell())), void >::Value;

			for (auto const& source : mSources)
			{
				if constexpr (bCanBreak)
				{
					if (func(source.posPair, getCellChecked(source.posPair[0]), getCellChecked(source.posPair[1])))
						return true;
				}
				else
				{
					func(source.posPair, getCellChecked(source.posPair[0]), getCellChecked(source.posPair[1]));
				}
			}

			return false;
		}


		template< typename TFunc >
		bool visitSourcePosList(TFunc&& func)
		{
			constexpr bool bCanBreak = !Meta::IsSameType< decltype(func((Vec2i*)(nullptr))), void >::Value;

			for (auto const& source : mSources)
			{
				if constexpr (bCanBreak)
				{
					if (func(source.posPair))
						return true;
				}
				else
				{
					func(source.posPair);
				}
			}

			return false;
		}


		template< typename TFunc >
		bool visitFlow(Vec2i const& sourcePos, int startDir, bool bVisitStart, TFunc&& func)
		{
			constexpr bool bCanBreak = !Meta::IsSameType< decltype(func(Vec2i(), Cell())), void >::Value;

			Vec2i pos = sourcePos;
			Cell* cell = &getCellChecked(pos);

			if (bVisitStart)
			{
				if constexpr (bCanBreak)
				{
					if (func(pos, *cell))
						return true;
				}
				else
				{
					func(pos, *cell);
				}
			}
			int dir = startDir;
			for (;;)
			{
				if (dir == INDEX_NONE)
					break;

				pos = getLinkPos(pos, dir);
				cell = &getCellChecked(pos);

				if constexpr (bCanBreak)
				{
					if (func(pos, *cell))
						return true;
				}
				else
				{
					func(pos, *cell);
				}
				dir = cell->getLinkedFlowDir(InverseDir(dir));
			}
			return false;
		}

		

		TGrid2D< Cell > mCellMap;

		struct SourceInfo
		{
			Vec2i posPair[2];
		};
		TArray< SourceInfo > mSources;
	};


}//namespace FlowFree

#endif // FlowFreeLevel_H_E6A7EB14_BFE9_4FFE_B10C_2E1493A9CC34
