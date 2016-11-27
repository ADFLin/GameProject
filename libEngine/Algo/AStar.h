#ifndef AStar_h__
#define AStar_h__

#include <cassert>
#include "AStarDefultPolicy.h"
#include "IntegerType.h"

#if 1
#	include "ProfileSystem.h"
#	define  ASTAR_PROFILE( name )  PROFILE_ENTRY( name )
#else
#   define  ASTAR_PROFILE( name )
#endif

#define NEVER_REACH( STR ) assert( !STR )

#include "AStarDefultPolicy.h"


enum
{
	ASTAR_SREACHING ,
	ASTAR_SREACH_SUCCESS ,
	ASTAR_SREACH_FAIL ,
};

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

namespace AStar
{
	template < class T >
	class AllocatePolicy
	{
	public:
		T*   alloc();
		void free(T* ptr);
	};

	template< class T , class CmpFun >
	class QueuePolicy
	{
	public:
		bool empty();
		void insert(T const& val);
		void pop();
		T&   front();
		void clear();
	};

	template< class T >
	class MapPolicy
	{
	public:
		class iterator;
		void insert(T const& val);
		void clear();
		void erase( iterator iter );
		iterator begin();
		iterator end();
	};

	template< class T , class STATE , class SCORE >
	struct NodeBaseT
	{
		typedef NodeBaseT< T , STATE , SCORE > NodeBase;
		typedef typename STATE StateType;
		typedef typename SCORE ScoreType;

		enum NodeFlag
		{
			eCLOSE  = BIT(0),
			ePATH   = BIT(1),
			eREMOVE = BIT(2),
		};
		StateType state;
		ScoreType g;
		ScoreType f;
		T*        parent;
		T*        child;
		uint8     flag;

		bool isClose(){ return ( flag & eCLOSE ) != 0; }
		bool isPath() { return ( flag & ePATH ) != 0; }

		struct CmpFun
		{
			template< class T >
			bool operator()(T* a , T* b)
			{
				if ( a->f > b->f ) 
					return true;
#if 1
				else if ( a->f == b->f )
					return a->g < b->g;
#endif
				return false;
			}
		};
	};


	template< class T , class NodeTraits >
	class AStarCoreT
	{
	public:
		typedef AStarCoreT< T , NodeTraits > AStarCore;
		typedef typename NodeTraits::ScoreType ScoreType;
		typedef typename NodeTraits::StateType StateType;
	};



	template< class T , class Node , 
		      template< class > class AllocatePolicy     = NewDeletePolicy , 
		      template< class > class MapPolicy          = STLVecotorMapPolicy  , 
		      template< class ,class > class QueuePolicy = STLHeapQueuePolicy >
	class AStarT : private AllocatePolicy< Node >
	{
		T* _this(){ return static_cast< T* >( this ); }

	public:
		typedef AStarT< T , Node , AllocatePolicy , MapPolicy , QueuePolicy >  AStar;
		typedef Node                             NodeType;
		typedef AllocatePolicy< NodeType >       Allocator;
		typedef MapPolicy< NodeType* >           MapType;
		typedef QueuePolicy< NodeType* , typename NodeType::CmpFun > QueueType;
		typedef typename NodeType::ScoreType     ScoreType;
		typedef typename NodeType::StateType     StateType;

		///////////////////////////////////////////////////////
		//=== override function ================//
	protected:
		ScoreType calcHeuristic( StateType& state ){  NEVER_REACH("Need impl calcHeuristic"); return 0; }
		ScoreType calcDistance( StateType& a, StateType& b){  NEVER_REACH("Need impl calcDistance"); return 0; }
		bool      isEqual(StateType& state1,StateType& state2 ){  NEVER_REACH("Need impl isEqual"); return false; }
		bool      isGoal (StateType& state ){ NEVER_REACH("Need impl isGoal"); return false; }
		//  call addSreachNode for all possible next state
		void      processNeighborNode( NodeType& node ){  NEVER_REACH("Need impl processNeighborNode"); }


		//===optional===//
		void      prevProcNeighborNode( NodeType& node ){}
		void      postProcNeighborNode( NodeType& node ){}
		void      onFindGoal( NodeType& node ){}

		typename MapType::iterator findNode( MapType& map , StateType& state )
		{  
			ASTAR_PROFILE( "findNode" );
			return std::find_if( map.begin() , map.end() , FindNodeFun( *this, state ) ); 
		}

		template< class Fun >
		void cleanupNode( MapType& map , Fun funClear)
		{
			ASTAR_PROFILE( "clearNode" );
			using std::for_each;
			for_each( map.begin() , map.end() , funClear );
			map.clear();
		}
		//////////////////////////////////////////////////////////////////
	public:
		AStarT()
		{
			mPath = NULL;
			mCache = NULL;
			mNumPathNode = 0;
		}

		~AStarT(){  cleanup( false );  }

		QueueType&  getQueue(){ return mQueue; }
		MapType&    getMap(){ return mMap; }

		void startSreach( StateType const& start );
		int  sreachStep();

		bool sreach( StateType const& start )
		{
			startSreach( start );

			int result;
			do 
			{
				result = sreachStep();
			}
			while ( result == ASTAR_SREACHING );

			return result == ASTAR_SREACH_SUCCESS;
		}

		NodeType* getPath(){ return mPath; }
		int       getPathNodeNum() const { return mNumPathNode; }

	protected:

		void cleanup( bool beCache )
		{
			mQueue.clear();
			if ( beCache )
			{
				_this()->cleanupNode( getMap() , CacheFun(*this) );
			}
			else
			{
				_this()->cleanupNode( getMap() , FreeFun(*this) );
				cleanupCache();
			}
		}
		bool addSreachNode( StateType& nextState , NodeType& node )
		{
			return addSreachNode( nextState , node , _this()->calcDistance( node.state , nextState ) );
		}
		bool addSreachNode( StateType& nextState , NodeType& node , ScoreType dist );

		struct FindNodeFun
		{
			typedef StateType StateType; 
			FindNodeFun( AStarT& a , StateType& s ):aStar ( a ),state( s ){}
			bool operator()( NodeType* node ){  return aStar._this()->isEqual( node->state , state );  }
			StateType& getState(){ return state; }
			StateType& state;
			AStarT&    aStar;
		};

		struct CacheFun
		{
			CacheFun( AStarT& a ):aStar( a ){}
			void operator()( NodeType* node )
			{   
				aStar.addCache( node );
			}
			NodeType* cache;
			AStarT&   aStar;
		};

		struct CacheNonpathFun
		{
			CacheNonpathFun( AStarT& a ):aStar( a ){}
			void operator()( NodeType* node )
			{   
				if ( node->flag & NodeType::ePATH )
					return;
				aStar.addCache( node );
			}
			AStarT&   aStar;
		};

		struct FreeFun
		{
			FreeFun( AStarT& a ):aStar( a ){}
			void operator()( NodeType* node )
			{   
				aStar.free( node );
			}
			AStarT&   aStar;
		};

		struct FreeNonpathFun
		{
			FreeNonpathFun( AStarT& a ):aStar( a ){}
			void operator()( NodeType* node )
			{   
				if ( node->flag & NodeType::ePATH )
					return;
				aStar.free( node );
			}
			AStarT&   aStar;
		};

		void cleanupNonpathNode( bool beCache )
		{
			using std::for_each;

			if ( beCache )
			{
				for_each( mMap.begin() , mMap.end() , CacheNonpathFun(*this) );
			}
			else
			{
				for_each( mMap.begin() , mMap.end() , FreeNonpathFun(*this) );
				cleanupCache();
			}
			mMap.clear();
			mQueue.clear();
		}

	private:
		NodeType* fetchNode()
		{
			if ( mCache )
			{
				NodeType* result = mCache;
				mCache = mCache->child;
				return result;
			}
			return Allocator::alloc();
		}
		void addCache( NodeType* node )
		{
			node->child = mCache;
			mCache = node;
		}

		void cleanupCache()
		{
			NodeType* cur = mCache;
			while( cur )
			{
				NodeType* temp = cur;
				cur = cur->child;
				Allocator::free( temp );
			}
			mCache = NULL;
		}

		void constructPath( NodeType* node )
		{
			mNumPathNode = 1;
			while ( node->parent )
			{
				NodeType* parent = node->parent;
				parent->child = node;
				node = parent;
				node->flag |= NodeType::ePATH;
				++mNumPathNode;
			}
			mPath = node;
		}

		int          mNumPathNode;
		StateType    mStartState;
		NodeType*    mPath;
		NodeType*    mCache;
		QueueType    mQueue;
		MapType      mMap;
	};

#define TEMPLATE_PARAM \
	class T , class MT , template<class > class AP , template<class > class MP , template< class ,class > class QP

#define CLASS_PARAM T,MT,AP,MP,QP

	template< TEMPLATE_PARAM >
	void AStarT< CLASS_PARAM >::startSreach( StateType const& start )
	{
		ASTAR_PROFILE( "start" );

		cleanup( true );
		mStartState = start;

		NodeType* node = fetchNode();
		node->state  = mStartState;
		node->parent = NULL;
		node->child  = NULL;
		node->f = 0;
		node->g = 0;
		node->flag = 0;
		mQueue.insert( node );
	}

	template< TEMPLATE_PARAM >
	int  AStarT< CLASS_PARAM >::sreachStep()
	{
		NodeType* node;
		for(;;)
		{
			if ( mQueue.empty() )
				return ASTAR_SREACH_FAIL;

			node = mQueue.front();

			mQueue.pop();

			if ( node->flag & NodeType::eREMOVE )
			{
				addCache( node );
			}
			else
			{
				break;
			}
		}

		if ( _this()->isGoal( node->state ) )
		{
			_this()->onFindGoal( *node );
			constructPath( node );
			return ASTAR_SREACH_SUCCESS;
		}

		_this()->prevProcNeighborNode( *node );
		{
			ASTAR_PROFILE( "processNeighborNode" );
			_this()->processNeighborNode( *node );
		}
		_this()->postProcNeighborNode( *node );

		node->flag |= NodeType::eCLOSE;
		return ASTAR_SREACHING;
	}

	template< TEMPLATE_PARAM >
	bool AStarT< CLASS_PARAM >::addSreachNode( StateType& nextState , NodeType& nodeLink , ScoreType dist )
	{
		//ASTAR_PROFILE( "addSreachNode" );

		ScoreType newG = nodeLink.g + dist;

		NodeType* nodeNew = NULL;

		{
			ASTAR_PROFILE( "FindNode" );

			typename MapType::iterator iter = _this()->findNode( mMap , nextState );

			if ( iter != mMap.end() )
			{
				NodeType* node = *iter;
				if ( node->g <= newG  )
					return false;

				if ( node->isClose() )
				{
					nodeNew = node;
					nodeNew->flag &= ~NodeType::eCLOSE;
				}
				else
				{
					mMap.erase( iter );
					node->flag |= NodeType::eREMOVE;
				}
			}
		}

		{
			ASTAR_PROFILE( "AddNewNode" );

			if ( !nodeNew )
			{
				nodeNew = fetchNode();
				nodeNew->state  = nextState;
				nodeNew->flag = 0;
				nodeNew->child  = NULL;
				mMap.insert( nodeNew );
			}

			assert( _this()->isEqual( nodeNew->state , nextState ) );

			nodeNew->parent = &nodeLink;		
			nodeNew->g = newG;
			nodeNew->f = newG + _this()->calcHeuristic( nextState );

			mQueue.insert( nodeNew );
		}

		return true;
	}
#undef TEMPLATE_PARAM
#undef CLASS_PARAM

}// namespace AStar


#endif // AStar_h__
