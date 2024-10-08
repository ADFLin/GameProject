#ifndef ObjectManger_H
#define ObjectManger_H

#include "CommonFwd.h"

#include <list>

#include <vector>
#include "Common.h"
#include "Object.h"
#include "ObjModel.h"
#include "GridManger.h"
#include "CollisionSystem.h"

namespace Shoot2D
{

	typedef GridManager<Object*,2,3> ObjectGrid;
	class ObjectManger : public ObjectGrid
		               , public CollisionSystem
	{
	public:
		ObjectManger( Rect_t const& rect )
			:ObjectGrid( rect )
			,NumObj(0)
		{
			m_LiveRange.Min = rect.Min - Vec2D( 40 , 40 );
			m_LiveRange.Max = rect.Max + Vec2D( 40 , 40 );
		}

		void update(long time);
		void addObj(Object* obj);
		void removeObj( Object* obj );

		template <class TFunc>
		void visit(TFunc func);

		bool updateObjCell( Object& obj , Cell* cell);
		void testCollision( );
		void testCollision( int i, int j );
		unsigned getObjNum(){ return NumObj; }

		typedef std::vector<Object*> ObjList;
		ObjList  testObj;
		ObjList  negObj;

		unsigned NumObj;
		Rect_t m_LiveRange;
	};


	template <class TFunc>
	void ObjectManger::visit( TFunc func )
	{
		for ( int i = 0 ; i < NumCellX ; ++i )
		{
			for ( int j = 0 ; j < NumCellY ; ++j )
			{
				Cell* cell = getCell(i,j);

				if ( cell == nullptr )
					continue;

				for( Cell::iterator iter = cell->begin();
					iter!= cell->end();++iter )
				{
					func( *iter );
				}
			}
		}
	}

}//namespace Shoot2D

#endif //ObjectManger_H