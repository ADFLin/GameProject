#include "ABPCH.h"
#include "ABPlayerBoard.h"
#include "RenderUtility.h"
#include "ABDefine.h"

#include "GameGraphics2D.h"

namespace AutoBattler
{
	Vec2i const PlayerBoard::MapSize(AB_MAP_SIZE_X, AB_MAP_SIZE_Y * 2);
	Vec2i const PlayerBoard::CellSize(AB_CELL_SIZE, AB_CELL_SIZE);

	PlayerBoard::PlayerBoard()
	{

	}

	void PlayerBoard::render(IGraphics2D& g)
	{
		RenderUtility::SetPen(g, EColor::Gray);
		RenderUtility::SetBrush(g, EColor::Null);

		for( int y = 0; y < MapSize.y; ++y )
		{
			for( int x = 0; x < MapSize.x; ++x )
			{
				Vector2 pos = getWorldPos(x, y);

				// Draw Hexagon or Rect? using Rect for Staggered Hex debug visualization for now
				// Staggered offset handled in getWorldPos logic usually, but here just drawing simple rects/circles to verify
				g.drawCircle(pos, CellSize.x / 2 - 2);
				
				// Draw Coords
				// RenderUtility::SetFont(g, FONT_S8);
				// g.drawText(pos - Vector2(10, 10), InlineString<32>::Make("%d,%d", x, y));
			}
		}

		// Draw separation line
		RenderUtility::SetPen(g, EColor::Red);
		Vector2 p1 = getWorldPos(0, MapSize.y / 2) - Vector2(CellSize.x / 2, CellSize.y / 2);
		Vector2 p2 = getWorldPos(MapSize.x, MapSize.y / 2) - Vector2(CellSize.x / 2, CellSize.y / 2);
		// g.drawLine(p1, p2); 
	}

	Vector2 PlayerBoard::getWorldPos(int x, int y) const
	{
		// Staggered Hex Grid (Pointy topped)
		// Offset every other row
		float xPos = x * CellSize.x;
		if (y % 2 != 0)
		{
			xPos += CellSize.x * 0.5f;
		}
		float yPos = y * CellSize.y * 0.75f; // Hex vertical spacing

		return Vector2(xPos, yPos) + mOffset;
	}

	Vec2i PlayerBoard::getCoord(Vector2 const& pos) const
	{
		// Approximate picking for Pointy-topped Staggered Grid
		// Inverse of:
		// xPos = x * CellSize.x + (y % 2 != 0 ? 0.5f * CellSize.x : 0)
		// yPos = y * CellSize.y * 0.75f

		Vector2 localPos = pos - mOffset;
		
		int y = Math::FloorToInt(localPos.y / (CellSize.y * 0.75f) + 0.5f);
		
		float xOffset = (y % 2 != 0) ? 0.5f * CellSize.x : 0.0f;
		int x = Math::FloorToInt((localPos.x - xOffset) / CellSize.x + 0.5f);

		// Accurate Hex picking is complex, this rect-based approx is okay for now
		// as long as we select center of cells.

		return Vec2i(x, y);
	}

	bool PlayerBoard::isValid(Vec2i const& coord) const
	{
		return coord.x >= 0 && coord.x < MapSize.x &&
			   coord.y >= 0 && coord.y < MapSize.y;
	}

	void PlayerBoard::setup()
	{
		mCells.resize(MapSize.x * MapSize.y);
		for (auto& cell : mCells)
			cell.unit = nullptr;
	}

	Unit* PlayerBoard::getUnit(int x, int y) const
	{
		if (!isValid(Vec2i(x,y))) return nullptr;
		return mCells[getIndex(x,y)].unit;
	}

	void PlayerBoard::setUnit(int x, int y, Unit* unit)
	{
		if (isValid(Vec2i(x,y)))
		{
			mCells[getIndex(x,y)].unit = unit;
		}
	}

	bool PlayerBoard::isOccupied(int x, int y) const
	{
		return getUnit(x, y) != nullptr;
	}

}//namespace AutoBattler
