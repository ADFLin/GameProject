#ifndef AStar_h__
#define AStar_h__

#include <algorithm>
class MapPolicy
{
public:
	class ScoreType;
	class StateType;
	ScoreType computeH(StateType& a, StateType& b);
	ScoreType computeDistance(StateType& a, StateType& b);
	bool isEqual(StateType const& node1,StateType const& node2);
	template< class FunType >
	void processNeighborNode( StateType& node, StateType* parent, FunType& fun);
};

template < class T >
class AllocatePolicy
{
public:
	T* alloc();
	void free(T* ptr);
};

template< class T , class CmpFun >
class OpenPolicy
{
public:
	
	void push(T const& val);
	void pop();
	T&   front();
	void clear();
	bool empty();

	class iterator;
	iterator begin();
	iterator end();
	void erase( iterator iter );

	template < class FindNodeFun >
	iterator find_if( FindNodeFun fun );
};

template< class T >
class ClosePolicy
{
public:
	void push(T const& val);
	void clear();

	class iterator;
	iterator begin();
	iterator end();
	void erase( iterator iter );

	template < class FindNodeFun >
	iterator find_if( FindNodeFun fun );
};

template <class  SNode>
class DebugPolicy
{
public:
	void getCloseInfo(SNode* sNode){}
	void getOpenInfo(SNode* sNode){}
	void getCurrectStateInfo( SNode* sNode ){}
};

struct ExtraDataPolicy
{

};

#include "defalutPolicy.h"

template< class MapPolicy , class ExtraDataPolicy >
struct SNode : public ExtraDataPolicy
{
	typedef typename MapPolicy::StateType StateType;
	typedef typename MapPolicy::ScoreType ScoreType;

	StateType state;
	ScoreType g;
	ScoreType f;
	SNode* parent;
	SNode* child;

	struct CmpFun2
	{
		bool operator()(SNode* a , SNode* b)
		{
			if ( a->f > b->f ) 
				return true;
			else if ( a->f == b->f )
				return a->g < b->g;
			return false;
		}
	};

	struct CmpFun
	{
		bool operator()(SNode* a , SNode* b)
		{
			return  a->f > b->f; 
		}
	};
};


template< class SNode >
unsigned hash_value(SNode* sNode)
{
	return hash_value( sNode->state );
}

template< class MapPolicy 
		, template<class > class AllocatePolicy = NewPolicy
		, template<class > class ClosePolicy = StlVectorClosePolicy  
		, template<class ,class > class OpenPolicy = StlHeapOpenPolicy
		, class ExtraDataPolicy = NoExtraData 
        , template<class> class DebugPolicy = NoDebugPolicy >
class AStar : public MapPolicy
			, private AllocatePolicy< SNode<MapPolicy , ExtraDataPolicy > >
			, private ClosePolicy< SNode<MapPolicy , ExtraDataPolicy >* >
			, private OpenPolicy< SNode<MapPolicy , ExtraDataPolicy >* , typename SNode<MapPolicy , ExtraDataPolicy >::CmpFun >
			, public  DebugPolicy< SNode<MapPolicy , ExtraDataPolicy >  >
{
public:
	typedef SNode<MapPolicy , ExtraDataPolicy > SNode;
	typedef typename MapPolicy::ScoreType ScoreType;
	typedef typename MapPolicy::StateType StateType;
	typedef typename ClosePolicy< SNode* > ClosePolicy;
	typedef OpenPolicy< SNode* , typename SNode::CmpFun> OpenPolicy;

	enum AStarState
	{
		STATE_SREACHING,
		STATE_FIND,
		STATE_END,
	};
	AStar()
	{
		m_Step = 0;
	}

	class ProcessNodeFun;
	class CloseInfoFun;
	class OpenInfoFun;

	AStarState sreachStep()
	{
		if ( OpenPolicy::empty() )
			return STATE_END;

		SNode* sNode = OpenPolicy::front();
		OpenPolicy::pop();

		if ( isEqual( sNode->state , m_end ) )
		{
			makePath( sNode );
			m_lastStats = STATE_FIND;
			return STATE_FIND;
		}

		++m_Step;
		StateType* parentState = NULL;
		if ( sNode->parent )
			parentState = &sNode->parent->state;

		processNeighborNode( 
			sNode->state , parentState , 
			ProcessNodeFun(*this , *sNode) );

		ClosePolicy::push( sNode );

		return STATE_SREACHING;
	}

	void initSreach(StateType& start,StateType& end)
	{
		m_start = start;
		m_end   = end; 
		m_Step = 0;

		clear();

		SNode* startNode = alloc();
		startNode->state= m_start;
		startNode->parent = NULL;
		startNode->child = NULL;
		startNode->f = 0;
		startNode->g = 0;

		OpenPolicy::push( startNode );
	}

	size_t getStepNum(){ return m_Step; }
	SNode* getPath(){ return m_Path; }
	int getPathNodeNum() const { return m_pathNodeNum; }

	void clear()
	{
		using std::for_each;
		for_each( OpenPolicy::begin() , OpenPolicy::end() , ClearFun(*this) );
		OpenPolicy::clear();
		for_each( ClosePolicy::begin() , ClosePolicy::end() , ClearFun(*this) );
		ClosePolicy::clear();
	}
	//for debug//

	DebugPolicy& getDebug(){ return *this };
	void debugCloseNode()
	{
		using std::for_each;
		for_each( ClosePolicy::begin() , ClosePolicy::end() ,CloseInfoFun(*this) );
	}

	void debugOpenNode()
	{
		using std::for_each;
		for_each( OpenPolicy::begin() , OpenPolicy::end() ,  OpenInfoFun(*this) );
	}



protected:
	void processNode( StateType& node , SNode& parent )
	{
		ScoreType newG = parent.g + computeDistance( parent.state , node );

		typename OpenPolicy::iterator openIter = 
			OpenPolicy::find_if( FindNodeFun( *this, node ) );
		if ( openIter != OpenPolicy::end() )
		{
			SNode* sn = *openIter;
			if ( sn->g <= newG  )
				return;
			else
			{
				free( sn );
				OpenPolicy::erase( openIter );
			}
		}

		typename ClosePolicy::iterator closeIter =
			ClosePolicy::find_if( FindNodeFun( *this, node ) );
		if ( closeIter != ClosePolicy::end() )
		{
			SNode* sn = *closeIter;
			if ( sn->g <= newG )
				return;
			else
			{
				free( sn );
				ClosePolicy::erase( closeIter );
			}
		}

		SNode* sNode = alloc();
		sNode->state = node;
		sNode->parent = &parent;
		sNode->child = NULL;
		sNode->g = newG;
		sNode->f = newG + computeH( node , m_end );

		OpenPolicy::push( sNode );
	}

	struct FindNodeFun
	{
		typedef StateType StateType; 
		FindNodeFun( AStar& a , StateType& s )
			:aStar ( a )
			,state( s ){	}

		bool operator()( SNode* sNode )
		{	return aStar.isEqual( sNode->state , state );   }
		StateType& getState(){ return state; }

		StateType& state;
		AStar& aStar;
	};
	struct ClearFun
	{
		ClearFun( AStar& a ):aStar ( a ){	}
		void operator()( SNode* sNode )
		{   aStar.free( sNode );  }

		AStar& aStar;
	};

	struct OpenInfoFun
	{
		OpenInfoFun( AStar& a ):aStar ( a ){	}
		void operator()( SNode* sNode )
		{   aStar.getOpenInfo( sNode );  }

		AStar& aStar;
	};

	struct CloseInfoFun
	{
		CloseInfoFun( AStar& a ):aStar ( a ){	}
		void operator()( SNode* sNode )
		{   aStar.getCloseInfo( sNode );  }

		AStar& aStar;
	};

	struct ClearNoUsedFun
	{
		ClearNoUsedFun( AStar& a )
			:aStar ( a ){	}
		void operator()( SNode* sNode )
		{   
			if ( sNode->child == NULL ) 
				aStar.free( sNode );  
		}

		AStar& aStar;
	};
	struct ProcessNodeFun
	{
		ProcessNodeFun( AStar& a , SNode& p )
			:aStar ( a )
			,parent( p ){	}
		void operator()( StateType& node )
		{	aStar.processNode( node , parent );	 }

		SNode& parent;
		AStar& aStar;
	};

	//void clearNoUseNode()
	//{
	//	using std::for_each;
	//	for_each( OpenPolicy::begin() , OpenPolicy::end() , ClearNoUsedFun(*this) );
	//	OpenPolicy::clear();
	//	for_each( ClosePolicy::begin() , ClosePolicy::end() , ClearNoUsedFun(*this) );
	//	ClosePolicy::clear();
	//}

	void makePath( SNode* sNode )
	{
		m_pathNodeNum = 0;
		while ( sNode->parent )
		{
			SNode* parent = sNode->parent;
			parent->child = sNode;
			sNode = parent;
			++m_pathNodeNum;
		}
		m_Path = sNode;
	}

	int      m_pathNodeNum;

	AStarState m_lastStats;
	SNode*   m_Path;
	size_t   m_Step;

	StateType m_start;
	StateType m_end;
};


#endif // AStar_h__