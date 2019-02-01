#include "DataStructure/Grid2D.h"

#include "TestStageHeader.h"

namespace Flow
{
	int const DirCount = 4;
	Vec2i const gDirOffset[] = { Vec2i(1,0) , Vec2i(0,1) , Vec2i(-1,0) , Vec2i(0,-1) };
	using ColorType = uint8;

	int InverseDir(int dir)
	{
		return (dir + 2) % 4;
	}

	enum class CellFun : uint8
	{
		Null = 0,
		Source,
		Bridge,
		Hole ,
	};


	struct Cell
	{
		Cell()
		{
			::memset(this, 0, sizeof(*this));
		}

		int getColor(int dir) const { return colors[dir]; }

		ColorType     colors[DirCount];
		CellFun       fun;
		uint8         funMeta;
		uint8         blockMask;
		mutable uint8 bFilledCached : 1;
		mutable uint8 bFilledCachedDirty : 1;

		void removeFlow(int dir)
		{
			colors[dir] = 0;
		}

		void setSource(ColorType color)
		{
			assert(color);
			fun = CellFun::Source;
			funMeta = color;
		}

		ColorType evalFlowOut(int dir) const 
		{ 
			if( colors[dir] )
				return 0;

			return getFunFlowColor(dir);
		}

		int  getNeedBreakDirs( int dir , int outDirs[DirCount])
		{
			int result = 0;
			switch( fun )
			{
			case CellFun::Null:
				for( int i = 0; i < DirCount; ++i )
				{
					if( colors[i] )
					{
						outDirs[result] = i;
						++result;
					}
				}
				break;
			case CellFun::Source:
				{
					for( int i = 0; i < DirCount; ++i )
					{
						if( colors[i] )
						{
							assert(colors[i] == funMeta);
							outDirs[result] = i;
							++result;
							break;
						}
					}
				}
				break;
			case CellFun::Bridge:
				{

				}
				break;
			default:
				break;
			}
			

			return result;
		}

		bool isFilled() const
		{
			if ( bFilledCachedDirty )
			{
				bFilledCachedDirty = false;
				int count = 0;
				for( int i = 0; i < DirCount; ++i )
				{
					if( colors[i] )
					{
						++count;
					}
				}

				switch( fun )
				{
				case CellFun::Null:
					bFilledCached = count == 2;
				case CellFun::Source:
					bFilledCached =  count == 1;
				case CellFun::Bridge:
					bFilledCached =  count == DirCount;
				default:
					bFilledCached = true;
				}
			}

			return bFilledCached;
		}

		int  getLinkedFlowDir(int dir) const
		{
			ColorType color = colors[dir];
			switch( fun )
			{
			case CellFun::Null:
				{
					int count = 0;
					assert(color);
					for( int i = 0; i < DirCount; ++i )
					{
						if( i == dir || colors[i] == 0 )
							continue;

						assert(colors[i] == color);
						return i;
					}
				}
				break;
			case CellFun::Source:
				assert(color == funMeta);
				return -1;
			case CellFun::Bridge:
				{
					int invDir = InverseDir(dir);
					if( colors[invDir] )
					{
						assert(colors[invDir] == color);
						return invDir;
					}
				}
				break;

			default:
				assert(0);
			}

			return -1;

		}

		ColorType getFunFlowColor( int dir = -1, ColorType color = 0 ) const
		{
			switch( fun )
			{
			case CellFun::Null:
				{
					int count = 0;
					ColorType outColor = color;
					for( int i = 0; i < DirCount; ++i )
					{
						if( colors[i] == 0 )
							continue;

						if( color && colors[i] != color )
							return 0;

						++count;
						if( count >= 2 )
							return 0;

						outColor = colors[i];
					}

					return outColor;
				}
				break;
			case CellFun::Source:
				{
					for( int i = 0; i < DirCount; ++i )
					{
						if( colors[i] )
						{
							assert(colors[i] == funMeta);
							return 0;
						}
					}

					if( color && color != funMeta )
						return 0;

					return funMeta;
				}
				break;
			case CellFun::Bridge:
				{
					if( dir == -1 )
					{
						for( int i = 0; i < DirCount; ++i )
						{
							if( colors[i] )
								return colors[i];
						}
					}
					else
					{
						int outColor = colors[InverseDir(dir)];
						if( color && outColor != color )
							return 0;

						return outColor;
					}
				}
				break;
			default:
				assert(0);
			}

			return 0;
		}

		void flowOut(int dir, ColorType color)
		{
			colors[dir] = color;
			bFilledCachedDirty = true;
		}

		bool flowIn( int dir , ColorType color )
		{
			if( colors[dir] )
				return false;

			if( getFunFlowColor(dir, color) == 0 )
				return false;

			colors[dir] = color;
			bFilledCachedDirty = true;
			return true;
		}
	};

	class Level
	{
	public:

		void setSize(Vec2i const& size)
		{
			mCellMap.resize(size.x, size.y);
		}

		Vec2i getSize() const
		{
			return Vec2i( mCellMap.getSizeX(), mCellMap.getSizeY() );
		}

		bool isValidCellPos(Vec2i const& cellPos) const
		{
			return 0 <= cellPos && cellPos < Vec2i(mCellMap.getSizeX(), mCellMap.getSizeY());
		}

		void addSource(Vec2i const& pos, ColorType color)
		{
			getCellChecked(pos).setSource(color);
		}

		void setCellBlocked(Vec2i const& pos, int dir )
		{
			Vec2i linkPos = getLinkPos(pos, dir);
			getCellChecked(pos).blockMask |= BIT(dir);
			getCellChecked(linkPos).blockMask |= BIT(InverseDir(dir));
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
			HaveBreak ,
			HaveBreakSameColor ,
		};

		int checkFilledCellCount() const
		{
			int count = 0;
			for( int i = 0; i < mCellMap.getRawDataSize(); ++i )
			{
				if( mCellMap.getRawData()[i].isFilled() )
					++count;
			}
			return count;
		}

		BreakResult breakFlow(Vec2i const& pos , int dir , ColorType curColor)
		{
			Cell& cell = getCellChecked( pos );
			BreakResult result = BreakResult::NoBreak;

			int linkDirs[DirCount];
			int numDir = cell.getNeedBreakDirs( dir , linkDirs);

			bool bBreakSameColor = false;
			bool bHaveBreak = false;


			for( int i = 0 ; i < numDir ; ++i )
			{
				int linkDir = linkDirs[i];
				if ( linkDir == -1 )
					continue;
				
				int dist = 0;
				
				ColorType linkColor = cell.colors[linkDir];
				Cell* source = findSourceCell(pos, linkDir, dist);

				auto TryRemoveToSource = [&]() -> bool
				{
					for( int j = i + 1; j < numDir; ++j )
					{
						int otherLinkDir = linkDirs[j];
						if ( otherLinkDir == -1 )
							continue;
						if( cell.colors[linkDirs[j]] == linkColor )
						{
							int distOther = 0;
							Cell* otherSource = findSourceCell(pos, otherLinkDir, distOther);

							if( otherSource )
							{
								int removeDir = (distOther < dist) ? otherLinkDir : linkDir;
								removeFlowToEnd(pos, removeDir);
								linkDirs[j] = -1;
								return true;
							}
						}
					}
					return false;
				};

				if( source )
				{
					if( curColor == 0 )
					{
						if( dist == 0 )
						{
							removeFlowToEnd(pos, linkDirs[i]);
						}
						else
						{
							TryRemoveToSource();
						}				
					}
					else if( linkColor != curColor )
					{
						if( TryRemoveToSource() == false )
						{
							removeFlowLink(pos, linkDir);
						}
					}
				}
				else
				{
					bHaveBreak = true;
					removeFlowToEnd(pos, linkDir);
					if( linkColor == curColor )
						bBreakSameColor = true;
				}
			}

			if( bHaveBreak )
			{
				if( bBreakSameColor )
				{
					result = BreakResult::HaveBreakSameColor;
				}
				else
				{
					result = BreakResult::HaveBreak;
				}
			}

			return result;
		}

		Cell* findSourceCell(Vec2i const& pos, int dir, int& outDist)
		{
			outDist = 0;

			Vec2i curPos = pos;
			int   curDir = dir;

			Cell& cell = getCellChecked(pos);
			if( cell.fun == CellFun::Source )
				return &cell;

			for(;;)
			{
				Vec2i linkPos = getLinkPos(curPos, curDir);
				Cell& linkCell = getCellChecked(linkPos);
				if( linkCell.fun == CellFun::Source )
					return &linkCell;

				int curDirInv = InverseDir(curDir);
				curDir = linkCell.getLinkedFlowDir(curDirInv);
				if( curDir == -1 )
					break;

				curPos = linkPos;
				++outDist;
			}

			return nullptr;
		}

		void removeFlowLink(Vec2i const& pos, int dir)
		{
			Cell& cell = getCellChecked(pos);
			cell.removeFlow(dir);

			Vec2i linkPos = getLinkPos(pos, dir);
			Cell& linkCell = getCellChecked(linkPos);

			int dirInv = InverseDir(dir);
			linkCell.removeFlow(dirInv);
		}

		void removeFlowToEnd(Vec2i const& pos, int dir)
		{
			Vec2i curPos = pos;
			int   curDir = dir;

			Cell* curCell = &getCellChecked(pos);
			for( ;;)
			{
				curCell->removeFlow(curDir);
				Vec2i linkPos = getLinkPos(curPos, curDir);
				Cell& linkCell = getCellChecked(linkPos);

				int curDirInv = InverseDir(curDir);
				curDir = linkCell.getLinkedFlowDir(curDirInv);

				linkCell.removeFlow(curDirInv);

				if ( curDir == -1 )
					break;

				curCell = &linkCell;
				curPos = linkPos;
			}
		}


		ColorType linkFlow( Vec2i const& pos , int dir )
		{
			Cell& cellOut = getCellChecked(pos);

			if( cellOut.blockMask & BIT(dir) )
				return 0;

			ColorType color = cellOut.evalFlowOut(dir);
			if( color == 0 )
				return 0;

			Vec2i linkPos = getLinkPos(pos, dir);
			Cell& cellIn = getCellChecked(linkPos);
			
			int dirIn = InverseDir(dir);
			if ( !cellIn.flowIn( dirIn , color ) )
				return 0;

			cellOut.flowOut(dir, color);
			return color;
		}

		static void WarpPos(int& p, int size)
		{
			if( p < 0 )
				p = size - 1;
			else if( p >= size )
				p = 0;
		}

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
			if( !mCellMap.checkRange(pos.x, pos.y) )
				return nullptr;
			return &mCellMap.getData(pos.x, pos.y);
		}

		TGrid2D< Cell > mCellMap;
	};



	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}

		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		void restart();
		void tick() {}
		void updateFrame(int frame) {}


		Vec2i ToScreenPos(Vec2i const& cellPos);
		Vec2i ToCellPos(Vec2i const& screenPos);

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame);



		Vec2i     flowOutCellPos;
		ColorType flowOutColor;
		bool  bStartFlowOut = false;


		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				Vec2i cPos = ToCellPos(msg.getPos());

				if( mLevel.isValidCellPos(cPos) )
				{
					bStartFlowOut = true;
					flowOutColor   = mLevel.getCellChecked(cPos).getFunFlowColor();
					flowOutCellPos = cPos;
					mLevel.breakFlow(cPos, 0, 0);
				}
			}
			else if( msg.onLeftUp() )
			{
				bStartFlowOut = false;
			}


			if( bStartFlowOut && msg.onMoving() )
			{
				Vec2i cPos = ToCellPos(msg.getPos());

				Vec2i cOffset = cPos - flowOutCellPos;
				Vec2i cOffsetAbs = Abs(cOffset);
				int d = cOffsetAbs.x + cOffsetAbs.y;
				if( d > 0 )
				{
					if( d == 1 )
					{
						int dir;
						if( cOffset.x == 1 )
						{
							dir = 0;
						}
						else if( cOffset.y == 1 )
						{
							dir = 1;
						}
						else if( cOffset.x == -1 )
						{
							dir = 2;
						}
						else
						{
							dir = 3;
						}

						if( mLevel.breakFlow(cPos, dir, flowOutColor) == Level::BreakResult::HaveBreakSameColor )
						{
							flowOutCellPos = cPos;
						}
						else
						{
							ColorType linkColor = mLevel.linkFlow(flowOutCellPos, dir);
							if( linkColor )
							{
								flowOutCellPos = cPos;
								flowOutColor = linkColor;
							}
						}
					}
					else
					{



					}
				}
			}

			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		Level mLevel;
	protected:
	};
}