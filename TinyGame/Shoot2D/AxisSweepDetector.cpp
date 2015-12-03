#include "AxisSweepDetector.h"

#include "Common.h"
#include "ObjectFactory.h"
#include "ObjModel.h"

namespace Shoot2D
{

	AxisSweepDetector::AxisSweepDetector( Rect_t const& rect ) 
		:NumColObj(0)
	{
		m_LiveRange.Min = rect.Min - Vec2D( 40 , 40 );
		m_LiveRange.Max = rect.Max + Vec2D( 40 , 40 );
	}

#if USE_TWO_AXIS
	void AxisSweepDetector::sortList( int axis , NodeIter from )
	{
		NodeIter next = from;
		++next;
		while( next != nodes[axis].end() )
		{
			if ( (*from).val <= (*next).val )
				break;
			++next;
		}
		nodes[axis].splice( next , nodes[axis] , from );
	}

	void AxisSweepDetector::sortList(int axis)
	{
		NodeIter iter = nodes[axis].end();
		--iter;
		for(; iter != nodes[axis].begin(); --iter )
		{
			sortList( axis , iter );
		}
		sortList(  axis , nodes[axis].begin() );
	}

#else

	void AxisSweepDetector::sortList( NodeIter from )
	{
		NodeIter next = from;
		++next;
		while( next != nodes.end() )
		{
			if ( (*from).val <= (*next).val )
				break;
			++next;
		}
		nodes.splice( next , nodes , from );
	}

	void AxisSweepDetector::sortList( )
	{
		NodeIter iter = nodes.end();
		--iter;
		for(; iter != nodes.begin(); --iter )
		{
			sortList( iter );
		}
		sortList(  nodes.begin() );
	}

#endif

	void AxisSweepDetector::checkCollision( ColData& data )
	{
		Object& obj1 = *data.obj;

#if USE_TWO_AXIS
		ColMap.clear();
		ColMap.resize( NumColObj );

		NodeList::iterator iter;

		iter = data.min[0];
		++iter;
		for(; iter != nodes[0].end() ; ++iter )
		{
			if ( iter->val > data.getMax(0) )
				break;

			ColData& data2 = *( (*iter).data );
			if ( iter == data2.max[0] ||
				data.getMax(0) <= data2.getMax(0) )
			{
				Object& obj2 = *data2.obj;
				ColMap[ obj2.getColID()] = true;
			}
		}

		iter = data.min[1];
		++iter;
		for(; iter != nodes[1].end() ; ++iter )
		{
			if ( iter->val > data.getMax(1) )
				break;

			ColData& data2 = *( (*iter).data );
			if ( iter == data2.max[1] ||
				data.getMax(1) <= data2.getMax(1) )
			{
				Object& obj2 = *data2.obj;
				if ( ColMap[ obj2.getColID()] )
					processCollision(obj1,obj2);
			}
		}

#else

		NodeList::iterator iter = data.min;
		++iter;
		for(; iter != nodes.end() ; ++iter )
		{
			if ( iter->val > data.getMax() )
				break;

			ColData& data2 = *( (*iter).data );
			if ( iter == data2.max ||
				data.getMax() <= data2.getMax() )
			{
				Object& obj2 = *data2.obj;
				processCollision(obj1,obj2);
			}
		}
#endif
	}

	void AxisSweepDetector::testCollision()
	{
		for(size_t i=0; i < NumColObj ; ++i )
		{
			ColData* data = colVec[i];
			checkCollision( *data ); 
		}
	}

	void AxisSweepDetector::removeObj( Object* obj )
	{
		ObjectCreator::destroy( obj );
	}

	void AxisSweepDetector::addObj( Object* obj )
	{
		if ( obj->getColFlag() == COL_NO_NEED )
		{
			noCol.push_back( obj );
			return;
		}

		ColData* data = m_pool.construct();
		data->obj = obj;

#if USE_TWO_AXIS
		nodes[0].push_front(Node());
		data->max[0] = nodes[0].begin();
		nodes[0].push_front(Node());
		data->min[0] = nodes[0].begin();

		(*data->max[0]).data = data;
		(*data->min[0]).data = data;

		nodes[1].push_front(Node());
		data->max[1] = nodes[1].begin();
		nodes[1].push_front(Node());
		data->min[1] = nodes[1].begin();

		(*data->max[1]).data = data;
		(*data->min[1]).data = data;
#else
		nodes.push_front(Node());
		data->max = nodes.begin();
		nodes.push_front(Node());
		data->min = nodes.begin();

		(*data->max).data = data;
		(*data->min).data = data;

#endif

		updatColData( *data );

#if USE_TWO_AXIS
		sortList( 0 ,++nodes[0].begin() );
		sortList( 0 ,  nodes[0].begin() );
		sortList( 1 ,++nodes[1].begin() );
		sortList( 1 ,  nodes[1].begin() );
#else
		sortList(  ++nodes.begin() );
		sortList(    nodes.begin() );
#endif

		if (  NumColObj < colVec.size() )
			colVec[NumColObj] = data;
		else
			colVec.push_back(data);

		obj->setColID( NumColObj );
		++NumColObj;
	}

	void AxisSweepDetector::updatColData( ColData& data )
	{
		Object& obj = *data.obj;
		ObjModel const& model = getModel(obj);

		if ( model.geomType == GEOM_RECT )
		{
#if USE_TWO_AXIS
			(*data.min[0]).val = obj.getPos().x;
			(*data.max[0]).val = obj.getPos().x + model.x;
			(*data.min[1]).val = obj.getPos().y;
			(*data.max[1]).val = obj.getPos().y + model.y;
#else
			(*data.min).val = obj.getPos().y;
			(*data.max).val = obj.getPos().y + model.y;
#endif
		}
		else if ( model.geomType == GEOM_CIRCLE )
		{
#if USE_TWO_AXIS
			(*data.min[0]).val = obj.getPos().x - model.r;
			(*data.max[0]).val = obj.getPos().x + model.r;
			(*data.min[1]).val = obj.getPos().y - model.r;
			(*data.max[1]).val = obj.getPos().y + model.r;
#else
			(*data.min).val = obj.getPos().y - model.r;
			(*data.max).val = obj.getPos().y + model.r;
#endif

		}
	}

	void AxisSweepDetector::update(long time)
	{
		for(size_t i=0; i < NumColObj ;)
		{
			ColData* data = colVec[i];
			Object& obj = *data->obj;

			//if ( obj.getStats() == STATS_CLEAR )
			//{
			//	removeObj(&obj);
			//}

			if ( !m_LiveRange.isInRange( obj.getPos() ) ||
				obj.getStats() == STATS_DEAD )
			{
				obj.setStats( STATS_CLEAR );
				removeColData( &obj );
				removeObj(&obj);
				continue;
			}

			obj.update(time);
			updatColData( *data );
			++i;
		}
#if USE_TWO_AXIS
		sortList(0);
		sortList(1);
#else
		sortList();
#endif

		for( ObjList::iterator iter = noCol.begin();
			iter!= noCol.end(); )
		{
			Object* obj=(*iter);

			if ( !m_LiveRange.isInRange( obj->getPos() ) ||
				obj->getStats() == STATS_DEAD )
			{
				removeObj( obj );
				iter = noCol.erase(iter);
				continue;
			}
			obj->update(time);

			++iter;
		}
	}

	void AxisSweepDetector::removeColData( Object* obj )
	{
		if ( obj->getColFlag() == COL_NO_NEED )
			return;

		size_t id = obj->getColID();
#if USE_TWO_AXIS
		nodes[0].erase( colVec[id]->max[0] );
		nodes[0].erase( colVec[id]->min[0] );

		nodes[1].erase( colVec[id]->max[1] );
		nodes[1].erase( colVec[id]->min[1] );
#else
		nodes.erase( colVec[id]->max );
		nodes.erase( colVec[id]->min );
#endif
		m_pool.destroy( colVec[id] );

		if ( id != NumColObj-1 )
		{
			colVec[id] = colVec[NumColObj-1];
			colVec[id]->obj->setColID(id);
		}
		--NumColObj;
	}

}//namespace Shoot2D