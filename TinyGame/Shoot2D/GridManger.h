#ifndef GridManger_h__
#define GridManger_h__

#include "CommonFwd.h"
#include <list>
#include <algorithm>

namespace Shoot2D
{

	template< class T , unsigned NUM_X , unsigned NUM_Y >
	class  GridManager
	{
	public:
		typedef std::list<T> Cell;
		static int const NumCellX = NUM_X;
		static int const NumCellY = NUM_Y;

		GridManager( Rect_t const& rect )
			: m_rect( rect )
		{
			dif = rect.Max - rect.Min;
		}

		static bool isInRange(int x,int y)
		{
			return 0 <= x && x < NumCellX &&
				0 <= y && y < NumCellY;
		}

		Cell* getCell( Vec2D const& pos )
		{
			int index[2];
			getCellPos( pos, index );
			return getCell(index[0],index[1]);
		}

		Cell* getCell(int x,int y)
		{
			if ( isInRange(x,y) )
				return &cell[y][x];
			return nullptr;
		}

		void getCellPos( Vec2D const& pos , int* n)
		{
			n[0] = int( ( pos.x - m_rect.Min.x ) / dif.x );
			n[1] = int( ( pos.y - m_rect.Min.y ) / dif.y );
		}

		bool push(T obj , Vec2D const& pos)
		{
			Cell* cell = getCell(pos);
			if ( cell )
			{
				cell->push_back( obj );
				return true;
			}
			return false;
		}

	protected :
		Cell cell[NumCellY][NumCellX];
		Rect_t m_rect;
		Vec2D  dif;
		unsigned m_size[2];

	};

}//namespace Shoot2D

#endif // GridManger_h__