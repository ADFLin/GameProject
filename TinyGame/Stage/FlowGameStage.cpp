#include "FlowGameStage.h"

REGISTER_STAGE("Flow Game", Flow::TestStage , EStageGroup::Test);

namespace Flow 
{

	int const CellLength = 50;
	Vec2i ScreenOffset = Vec2i(20, 20);
	int const FlowWidth = 16;
	int const FlowGap = (CellLength - FlowWidth) / 2;
	int const FlowSourceRadius = 20;

	void TestStage::restart()
	{
		bStartFlowOut = false;

		mLevel.setSize(Vec2i(10, 10));
		mLevel.addSource(Vec2i(0, 0), 1);
		mLevel.addSource(Vec2i(8, 8), 1);

		mLevel.addSource(Vec2i(0, 3), 2);
		mLevel.addSource(Vec2i(3, 3), 2);
	}

	Vec2i TestStage::ToScreenPos(Vec2i const& cellPos)
	{
		return ScreenOffset + CellLength * cellPos;
	}

	Vec2i TestStage::ToCellPos(Vec2i const& screenPos)
	{
		return (screenPos - ScreenOffset) / CellLength;
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		Vec2i size = mLevel.getSize();

		RenderUtility::SetPen(g, EColor::White);
		
		for( int i = 0; i <= size.x; ++i )
		{
			Vec2i start = ScreenOffset + Vec2i(i * CellLength, 0);
			g.drawLine(start, start + Vec2i(0, size.y * CellLength));
		}

		for( int j = 0; j <= size.y; ++j )
		{
			Vec2i start = ScreenOffset + Vec2i(0, j * CellLength);
			g.drawLine(start, start + Vec2i(size.x * CellLength, 0));
		}

		int ColorMap[] = 
		{
			EColor::Null , EColor::Red , EColor::Green , EColor::Blue , EColor::Yellow 
		};
		for( int i = 0; i < size.x; ++i )
		{
			for( int j = 0; j < size.y; ++j )
			{
				Cell const& cell = mLevel.getCellChecked(Vec2i(i, j));


				Vec2i posCellLT = ScreenOffset + CellLength * Vec2i(i, j);
				switch( cell.fun )
				{
				case CellFun::Source:
					RenderUtility::SetPen(g, ColorMap[cell.funMeta]);
					RenderUtility::SetBrush(g, ColorMap[cell.funMeta]);
					g.drawCircle( posCellLT + Vec2i(CellLength, CellLength) / 2, FlowSourceRadius );
					break;
				case CellFun::Bridge:
				default:
					break;
				}

				for( int dir = 0; dir < DirCount; ++dir )
				{
					if ( !cell.colors[dir] )
						continue;

					RenderUtility::SetPen(g, ColorMap[cell.colors[dir]]);
					RenderUtility::SetBrush(g, ColorMap[cell.colors[dir]]);

					switch( dir )
					{
					case 0: g.drawRect(posCellLT + Vec2i(FlowGap, FlowGap), Vec2i(CellLength - FlowGap, FlowWidth)); break;
					case 1: g.drawRect(posCellLT + Vec2i(FlowGap, FlowGap), Vec2i(FlowWidth , CellLength - FlowGap)); break;
					case 2: g.drawRect(posCellLT + Vec2i(0 , FlowGap), Vec2i(CellLength - FlowGap, FlowWidth)); break;
					case 3: g.drawRect(posCellLT + Vec2i(FlowGap, 0), Vec2i(FlowWidth, CellLength - FlowGap)); break;
					}
				}
			}
		}
	}

}//namespace Flow
