#include "ObjectManger.h"

#include "Common.h"
#include "ObjectFactory.h"
#include "Object.h"

namespace Shoot2D
{

	void ObjectManger::update(long time)
	{
		for ( int i = 0 ; i < NumCellX ; ++i )
		for ( int j = 0 ; j < NumCellY ; ++j )
		{
			Cell* cell = getCell(i,j);

			for( Cell::iterator iter = cell->begin();
				iter!= cell->end(); )
			{
				Object* obj=(*iter);

				if ( !m_LiveRange.isInRange( obj->getPos() ) ||
					obj->getStats() == STATS_DEAD )
				{
					removeObj( obj );
					iter = cell->erase(iter);
					continue;
				}
				obj->update(time);
				if ( updateObjCell( *obj , cell ) )
					iter = cell->erase(iter);
				else
					++iter;
			}
		}
	}


	bool ObjectManger::updateObjCell( Object& obj , Cell* cell )
	{
		Cell* newCell = getCell( obj.getPos() );

		if ( newCell == cell )
			return false;

		if ( newCell )
			newCell->push_back( &obj );
		else
			removeObj( &obj );
		return true;
	}
	void ObjectManger::addObj( Object* obj )
	{
		push(obj , obj->getPos() );
		++NumObj;
	}

	void ObjectManger::removeObj( Object* obj )
	{
		ObjectCreator::destroy( obj );
		--NumObj;
	}

	void ObjectManger::testCollision()
	{
		for ( int i = 0 ; i < NumCellX ; ++i )
			for ( int j = 0 ; j < NumCellY ; ++j )
			{
				testCollision(i, j);
			}
	}

	void ObjectManger::testCollision( int x, int y )
	{
		Cell* cell = getCell(x,y);

		for (Cell::iterator iter1 = cell->begin();
			iter1 != cell->end(); ++iter1 )
		{
			Object* obj1 = *iter1;

			for (int i= x ; i< x+1 ; ++i )
			for (int j= y-1 ; j< y+1 ; ++j )
			{
				Cell* cell2 = getCell(i,j);
				if ( cell2 == NULL ) 
					continue;

				for (Cell::iterator iter2 = cell2->begin();
					iter2 != cell2->end(); ++iter2 )
				{
					Object* obj2 = *iter2;

					if ( isNeedTestCollision(*obj1,*obj2) )
					{
						if ( CollisionSystem::testCollision(*obj1,*obj2) )
						{
							obj1->processCollision(*obj2);
							obj2->processCollision(*obj1);
						}
					}
				}
			}
		}
	}

}//namespace Shoot2D
