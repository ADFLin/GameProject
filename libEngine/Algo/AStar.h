#ifndef AStar_h__
#define AStar_h__

#include <cassert>
#include "AStarDefultPolicy.h"
#include "Core/IntegerType.h"

#if 1
#	include "ProfileSystem.h"
#	define  ASTAR_PROFILE( name )  PROFILE_ENTRY( name )
#else
#   define  ASTAR_PROFILE( name )
#endif

#include "AStarDefultPolicy.h"


#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

namespace AStar
{
	enum ESrachStatus
	{
		eSREACHING,
		eSREACH_SUCCESS,
		eSREACH_FAIL,
	};


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
		typedef STATE StateType;
		typedef SCORE ScoreType;

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
		typedef AStarT< T , Node , AllocatePolicy , MapPolicy , QueuePolicy >  AStarType;
		
		
#if 0
		typedef Node                             NodeType;
		typedef AllocatePolicy< NodeType >       Allocator;
		typedef MapPolicy< NodeType* >           MapType;
		typedef QueuePolicy< NodeType* , typename NodeType::CmpFun > QueueType;
		typedef typename NodeType::ScoreType     ScoreType;
		typedef typename NodeType::StateType     StateType;
#else
		using NodeType = Node;
		using Allocator = AllocatePolicy< NodeType >;
		using MapType = MapPolicy< NodeType* >;
		using QueueType = QueuePolicy< NodeType*, typename NodeType::CmpFun >;
		using ScoreType = typename NodeType::ScoreType;
		using StateType = typename NodeType::StateType;

#endif
		///////////////////////////////////////////////////////
		//=== override function ================//
	protected:
		ScoreType calcHeuristic( StateType& state ){  NEVER_REACH("Need impl calcHeuristic"); return 0; }
		ScoreType calcDistance( StateType& a, StateType& b){  NEVER_REACH("Need impl calcDistance"); return 0; }
		bool      isEqual(StateType& state1,StateType& state2 ){  NEVER_REACH("Need impl isEqual"); return false; }
		bool      isGoal (StateType& state ){ NEVER_REACH("Need impl isGoal"); return false; }
		//  call addSreachNode for all possible next state
		void      processNeighborNode( NodeType& node ){  NEVER_REACH("Need impl processNeighborNode"); }


		typename MapType::iterator findNode( MapType& map , StateType& state )
		{  
			ASTAR_PROFILE( "findNode" );
			return std::find_if( map.begin() , map.end() , 
				[this, &state](NodeType* node)
				{
					return _this()->isEqual(node->state, state);
				}
			); 
		}

		template< class TFunc >
		void cleanupNode( MapType& map , TFunc&& funcCleaup )
		{
			ASTAR_PROFILE( "clearNode" );
			using std::for_each;
			for_each( map.begin() , map.end() , std::forward<TFunc>(funcCleaup) );
			map.clear();
		}
		//////////////////////////////////////////////////////////////////
	public:
		AStarT()
		{
			mCache = nullptr;
		}

		~AStarT(){  cleanup( false );  }

		QueueType&  getQueue(){ return mQueue; }
		MapType&    getMap(){ return mMap; }


		struct SreachResult
		{
			NodeType*   startNode;
			NodeType*   globalNode;
		};


		void startSreach( StateType const& start , SreachResult& sreachResult);
		ESrachStatus  sreachStep(SreachResult& sreachResult);

		bool sreach( StateType const& start, SreachResult& sreachResult)
		{
			startSreach( start , sreachResult);

			int result;
			do 
			{
				result = sreachStep(sreachResult);
			}
			while ( result == eSREACHING );

			return result == eSREACH_SUCCESS;
		}

		template< class TFunc >
		int constructPath(NodeType* globalNode, TFunc&& func)
		{
			NodeType* node = globalNode;
			int nodeCount = 0;
			do
			{
				node->flag |= NodeType::ePATH;
				NodeType* parent = node->parent;
				func(node);
				node = parent;
				++nodeCount;
			}
			while( node != nullptr );
			return nodeCount;
		}
	protected:

		void cleanup( bool bAddToCache )
		{
			mQueue.clear();
			if ( bAddToCache )
			{
				_this()->cleanupNode(getMap(), 
					[this](NodeType* node)
					{
						addToCache(node);
					}
				);
			}
			else
			{
				_this()->cleanupNode( getMap() , 
					[this](NodeType* node)
					{
						deleteNode(node);
					}
				);
				cleanupCache();
			}
		}
		bool addSreachNode( StateType& nextState , NodeType& node )
		{
			return addSreachNode( nextState , node , _this()->calcDistance( node.state , nextState ) );
		}
		bool addSreachNode( StateType& nextState , NodeType& node , ScoreType dist );

		void cleanupNonpathNode( bool bAddToCache )
		{
			using std::for_each;

			if ( bAddToCache )
			{
				for_each( mMap.begin() , mMap.end() , 
					[this](NodeType* node)
					{
						if( node->flag & NodeType::ePATH )
							return;
						addToCache(node);
					}
				);
			}
			else
			{
				for_each( mMap.begin() , mMap.end() , 
					[this](NodeType* node)
					{
						if( node->flag & NodeType::ePATH )
							return;
						
						deleteNode(node);
					}
				);
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
				mCache = mCache->parent;
				return result;
			}
			return Allocator::alloc();
		}


		void deleteNode(NodeType* node)
		{
			Allocator::free(node);
		}

		void addToCache( NodeType* node )
		{
			node->parent = mCache;
			mCache = node;
		}

		void cleanupCache()
		{
			NodeType* cur = mCache;
			while( cur )
			{
				NodeType* temp = cur;
				cur = cur->parent;
				deleteNode( temp );
			}
			mCache = NULL;
		}

		NodeType*    mCache;
		QueueType    mQueue;
		MapType      mMap;
	};

#define TEMPLATE_PARAM \
	class T , class MT , template<class > class AP , template<class > class MP , template< class ,class > class QP

#define CLASS_PARAM T,MT,AP,MP,QP

	template< TEMPLATE_PARAM >
	void AStarT< CLASS_PARAM >::startSreach( StateType const& start , SreachResult& sreachResult)
	{
		ASTAR_PROFILE( "start" );

		cleanup( true );

		NodeType* node = fetchNode();
		node->state  = start;
		node->parent = nullptr;
		node->f = 0;
		node->g = 0;
		node->flag = 0;

		sreachResult.startNode = node;
		mQueue.insert( node );
	}

	template< TEMPLATE_PARAM >
	ESrachStatus  AStarT< CLASS_PARAM >::sreachStep( SreachResult& sreachResult)
	{
		NodeType* node;
		for(;;)
		{
			if ( mQueue.empty() )
				return eSREACH_FAIL;

			node = mQueue.front();

			mQueue.pop();

			if ( node->flag & NodeType::eREMOVE )
			{
				addToCache( node );
			}
			else
			{
				break;
			}
		}

		node->flag |= NodeType::eCLOSE;

		if ( _this()->isGoal( node->state ) )
		{
			sreachResult.globalNode = node;
			return eSREACH_SUCCESS;
		}

		{
			ASTAR_PROFILE( "processNeighborNode" );
			_this()->processNeighborNode( *node );
		}

		return eSREACHING;
	}

	template< TEMPLATE_PARAM >
	bool AStarT< CLASS_PARAM >::addSreachNode( StateType& nextState , NodeType& nodeLink , ScoreType dist )
	{
		//ASTAR_PROFILE( "addSreachNode" );

		ScoreType newG = nodeLink.g + dist;

		NodeType* nodeNew = nullptr;

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
