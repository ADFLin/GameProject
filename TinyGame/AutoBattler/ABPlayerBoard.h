#ifndef ABPlayerBoard_h__
#define ABPlayerBoard_h__

#include "Math/Vector2.h"
#include "Math/TVector2.h"
#include "DataStructure/Array.h"

namespace AutoBattler
{
	using Vec2i = TVector2<int>;
	class Unit;

	class PlayerBoard
	{
	public:
		PlayerBoard();

		static Vec2i Mirror(Vec2i const pos)
		{
			return MapSize - pos - Vec2i(1, 1);
		}
		
		void setup();
		void render(IGraphics2D& g);

		static Vec2i const MapSize;
		static Vec2i const CellSize;

		// Convert grid coordinate to world position
		Vector2 getWorldPos(int x, int y) const;

		Vec2i   getCoord(Vector2 const& pos) const;
		bool    isValid(Vec2i const& coord) const;

		Unit*   getUnit(int x, int y) const;
		void    setUnit(int x, int y, Unit* unit);

		bool    isOccupied(int x, int y) const;

	private:
		struct Cell
		{
			Unit* unit = nullptr;
		};
		TArray<Cell> mCells;
		int   getIndex(int x, int y) const { return y * MapSize.x + x; }

		class Player* mOwner = nullptr;
	public:
		void setOwner(Player* player) { mOwner = player; }
		Player* getOwner() { return mOwner; }

		void setOffset(Vector2 const& offset) { mOffset = offset; }
		Vector2 getOffset() const { return mOffset; }
	
	private:
		Vector2 mOffset = Vector2(0, 0);
	};

}//namespace AutoBattler

#endif // ABPlayerBoard_h__
