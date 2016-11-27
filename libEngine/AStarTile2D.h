#ifndef Grid2DAStar_h__
#define Grid2DAStar_h__

#include "Algo/AStar.h"

#include "TGrid2D.h"
#include "TVector2.h"
#include "IntegerType.h"

#include <iterator>
#include <list>

namespace AStar
{
	typedef TVector2< int > Vec2i;

	struct Tile2DNode : NodeBaseT< Tile2DNode , Vec2i , int >
	{

	};

	template< class T >
	class Tile2DMapPolicy 
	{
	public:
		typedef uint32 CountType;
		void resize( int x , int y )
		{
			mMapHash.resize( x , y );
			mCurCount = 0;
			clear();
		}

		template< class T >
		struct Link
		{
			void clear(){ prev = next = NULL; }
			void insert( Link& link )
			{
				assert( prev && next );
				link.next = next;
				next->prev = &link;
				next = &link;
				link.prev = this;
			}

			void remove()
			{
				assert( prev && next );
				prev->next = next;
				next->prev = prev;
			}
			Link* prev;
			Link* next;
		};

		struct Cell : public Link< Cell >
		{
			T         val;
			CountType count;
		};

		class iterator
		{
		public:
			typedef T  value_type;
			typedef T* pointer;
			typedef T& reference;
			typedef std::forward_iterator_tag iterator_category;
			typedef ptrdiff_t difference_type;

			iterator():mLink( NULL ){}
			explicit iterator( Link< Cell >* link ):mLink( link ){}

			reference  operator*()  {  return toCell()->val;  }
			pointer    operator->() {  return &toCell()->val;  }
			iterator&  operator ++()
			{
				mLink = mLink->next;
				return *this;
			}
			iterator   operator ++( int )
			{
				iterator temp( mLink );
				mLink = mLink->next;
				return temp;
			}
			bool operator != ( iterator iter ){  return mLink != iter.mLink;  }
			bool operator == ( iterator iter ){  return mLink == iter.mLink;  }
			Cell*  toCell(){ return static_cast< Cell* >( mLink ); }
		private:
			friend class Tile2DMapPolicy;
			Link< Cell >* mLink;
		};


		void insert( T const& val )
		{	
			Cell& node = mMapHash.getData( val->state.x , val->state.y );
			node.val   = val;
			node.count = mCurCount;
			mHeader.insert( node );
		}

		void clear()
		{
			if ( mCurCount == 0 )
			{
				memset( mMapHash.getRawData() , 0 , sizeof( Cell) * mMapHash.getSizeX() * mMapHash.getSizeY() );
			}
			++mCurCount;
			mHeader.next = &mHeader;
			mHeader.prev = &mHeader;
		}

		inline void erase( iterator iter )
		{
			if ( iter == end() )
				return;

			iter.toCell()->count = mCurCount - 1;
			iter.mLink->remove();
		}

		iterator begin(){ return iterator( mHeader.next ); }
		iterator end()  { return iterator( &mHeader ); }

		template < class State >
		iterator find( State const& state )
		{
			assert( mMapHash.checkRange( state.x , state.y ) );

			Cell& cell = mMapHash.getData( state.x , state.y );
			if ( cell.count == mCurCount ) 
				return  iterator( &cell );

			return end();
		}

		Link< Cell >     mHeader;
		CountType        mCurCount;
		TGrid2D< Cell >  mMapHash;
	};


	template< class T , 
		      template< class > class AllocatePolicy = NewDeletePolicy , 
		      template< class , class > class QueuePolicy = STLHeapQueuePolicy  >
	class AStarTile2DT : public AStarT < T , Tile2DNode , AllocatePolicy , Tile2DMapPolicy , QueuePolicy >
	{
	public:
		typedef AStarTile2DT< T , AllocatePolicy , QueuePolicy > AStarTile2D;

		AStarTile2DT()
		{

		}

		bool isEqual( StateType& state1,StateType& state2 ){  return state1 == state2; }
		typename MapType::iterator findNode( MapType& map , StateType& state )
		{ 
			return map.find( state );
		}

	};

}//namespace AStar


#endif // Grid2DAStar_h__
