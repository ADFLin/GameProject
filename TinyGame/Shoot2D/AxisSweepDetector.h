#ifndef AxisSweepDetector_h__
#define AxisSweepDetector_h__

#include "CollisionSystem.h"
#include "Object.h"

#include <vector>
#include <list>
#include <map>
#include <boost/pool/object_pool.hpp>

#define USE_TWO_AXIS 0


namespace Shoot2D
{

	template< class T >
	class Node
	{
		Node(){ prev = next = nullptr; }
		T* prev;
		T* next;
	};

	template< class T , Node< T > T::*Member >
	class List
	{
	public:
		typedef Node< T > NodeType;
		NodeType& getNode( T& v ){ return ( v.*Member ); }
		void pushFront( T& v )
		{
			if ( mFirst )
			{
				getNode( v ).prev = mFirst;
				getNode( v ).next = getNode( *mFirst ).next;
				getNode( *mFirst ).next = &v;
			}
			else
			{
				mFirst = mLast = &v;
			}
		}

		void insertFront( T& pos , T& v )
		{

		}


		T* mFirst;
		T* mLast;
	};

	class AxisSweepDetector : public CollisionSystem
	{
	public:
		AxisSweepDetector( Rect_t const& rect );

		struct ColData;
		struct Node
		{
			Node(ColData* d = nullptr):data(d),val(0){}
			ColData* data;
			float    val;
		};

		typedef std::list<Node>       NodeList;
		typedef NodeList::iterator    NodeIter;
		typedef std::vector<ColData*> ColDataVec;
		typedef std::list<Object*>    ObjList;

		struct ColData
		{
			Object* obj;
#if USE_TWO_AXIS
			NodeIter max[2];
			NodeIter min[2];

			float getMax(int i){ return (*max[i]).val; }
			float getMin(int i){ return (*min[i]).val; }
#else
			NodeIter max;
			NodeIter min;

			float getMax(){ return (*max).val; }
			float getMin(){ return (*min).val; }
#endif
		};

		template <class TFunc>
		void visit(TFunc func);

		static void updatColData(ColData& data);

		size_t getObjNum(){ return NumColObj; }
		void removeColData( Object* obj );
		void update(long time);
		void testCollision();
		void checkCollision( ColData& data);
		void addObj( Object* obj );
		void removeObj( Object* obj );

	protected:

#if USE_TWO_AXIS
		void sortList( int axis , NodeIter from );
		void sortList( int axis );
#else
		void sortList( NodeIter from );
		void sortList( );
#endif


#if USE_TWO_AXIS
		NodeList nodes[2];
		std::vector< bool > ColMap;
#else
		NodeList nodes;
#endif
		size_t     NumColObj;
		ObjList    noCol;
		ColDataVec colVec;
		boost::object_pool<ColData> m_pool;
		Rect_t m_LiveRange;
	};


	template <class TFunc>
	void AxisSweepDetector::visit(TFunc func)
	{
		for(size_t i=0; i < NumColObj ; ++i )
		{
			ColData* data = colVec[i];
			func( data->obj );
		}

		for( ObjList::iterator iter = noCol.begin();
			iter!= noCol.end(); ++iter )
		{
			Object* obj=(*iter);
			func( obj );
		}
	}

}//namespace Shoot2D

#endif // AxisSweepDetector_h__
