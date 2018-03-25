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
	};


	struct Cell
	{
		Cell()
		{
			::memset(this, 0, sizeof(*this));
		}
		ColorType    colors[4];
		CellFun      fun;
		uint8        funMeta;
		uint8        blockMask;
		uint8        padding;


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

			if( blockMask & BIT(dir) )
				return 0;

			return getFunFlowColor(dir);
		}

		int  getLinkedDirs(int outDirs[DirCount])
		{
			int result = 0;
			for( int i = 0; i < DirCount; ++i )
			{
				if( colors[i] )
				{
					outDirs[result] = i;
					++result;
				}
			}
			return result;

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
			case CellFun::Source:
				assert(color == funMeta);
				return -1;
			default:
				assert(0);
			}

			return -1;

		}
		ColorType getFunFlowColor( int dir , ColorType color = 0 ) const
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
			case CellFun::Source:
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
			default:
				assert(0);
			}

			return 0;
		}

		void flowOut(int dir, ColorType color)
		{
			colors[dir] = color;
		}

		bool flowIn( int dir , ColorType color )
		{
			if( colors[dir] )
				return false;

			if( getFunFlowColor(dir, color) == 0 )
				return false;

			colors[dir] = color;
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

		Cell const& getCellChecked(Vec2i const& pos) const
		{
			return mCellMap.getData(pos.x, pos.y);
		}

		Cell& getCellChecked(Vec2i const& pos)
		{
			return mCellMap.getData(pos.x, pos.y);
		}

		bool canFlowOut(Vec2i const& pos, int dir)
		{
			ColorType color;
			return !!evalFlowOut(pos, dir , color);
		}

		int breakFlow(Vec2i const& pos)
		{
			Cell& cell = getCellChecked( pos );

			int linkDirs[DirCount];
			int numDir = cell.getLinkedDirs(linkDirs);

			for( int i = 0 ; i < numDir ; ++i )
			{
				if( findSourceCell(pos, linkDirs[i]) == nullptr )
				{
					removeFlowToEnd(pos, linkDirs[i]);
				}
				else
				{
					removeFlowLink(pos, linkDirs[i]);
				}
			}

			return numDir;
		}

		Cell* findSourceCell(Vec2i const& pos, int dir)
		{
			Vec2i curPos = getLinkPos(pos , dir);
			for( ;; )
			{
				Cell& cell = getCellChecked(curPos);
				if( cell.fun == CellFun::Source )
					return &cell;

				int linkDir = cell.getLinkedFlowDir(dir);
				if( linkDir == -1 )
					break;

				curPos = getLinkPos(curPos, linkDir);
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
			do
			{
				Cell& cell = getCellChecked(curPos);
				cell.removeFlow(curDir);

				Vec2i linkPos = getLinkPos(curPos, curDir);
				Cell& linkCell = getCellChecked(linkPos);

				int curDirInv = InverseDir(curDir);
				linkCell.removeFlow(curDirInv);

				curDir = linkCell.getLinkedFlowDir(curDirInv);

			} 
			while( curDir != -1 );
		}

		Cell* evalFlowOut(Vec2i const& pos, int dir , ColorType& outColor )
		{
			Cell* cell = getCellInternal(pos);
			if( cell == nullptr )
				return nullptr;

			outColor = cell->evalFlowOut(dir);
			if( outColor == 0 )
				return nullptr;

			return cell;
		}

		bool linkFlow( Vec2i const& pos , int dir )
		{
			ColorType color;
			Cell* cellOut = evalFlowOut(pos, dir, color);
			if( cellOut == nullptr )
				return false;

			Vec2i linkPos = getLinkPos(pos, dir);

			Cell& tileIn = getCellChecked(linkPos);
			
			int dirIn = InverseDir(dir);
			if ( !tileIn.flowIn( dirIn , color ) )
				return false;

			cellOut->flowOut(dir, color);
			return true;
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



		Vec2i flowOutCellPos;
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
					flowOutCellPos = cPos;
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

						mLevel.breakFlow(cPos);

						if( mLevel.linkFlow(flowOutCellPos, dir) )
						{
							flowOutCellPos = cPos;
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