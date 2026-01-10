#ifndef IDAStar_h__594e537d_8986_47af_bf7f_04b8a26c3fb0
#define IDAStar_h__594e537d_8986_47af_bf7f_04b8a26c3fb0

namespace IDA
{
	template< class T >
	struct StateTraits
	{

	};

	template< class T , class ST >
	class AlgoT
	{
		T* _this(){  return static_cast<T*>(this);  }
	public:

		typedef ST StateTraits;
		typedef StateTraits::ScoreType ScoreType;
		typedef StateTraits::StateType StateType;

		ScoreType calcHeuristic( StateType& state ){  NEVER_REACH("Need impl calcHeuristic"); return 0; }
		ScoreType calcDistance( StateType& a, StateType& b){  NEVER_REACH("Need impl calcDistance"); return 0; }
		bool      isEqual(StateType& state1,StateType& state2 ){  NEVER_REACH("Need impl isEqual"); return false; }
		bool      isGoal (StateType& state ){ NEVER_REACH("Need impl isGoal"); return false; }
		//  call addSearchNode for all possible next state
		template < class TFunc >
		void      processNeighborNode( StateType& node , ScoreType const& score , TFunc& func ){  NEVER_REACH("Need impl processNeighborNode"); }

	public:
		void startSearch()
		{

		}

		enum ResultState
		{
			RS_FOUND = 0,
			RS_OVER_THRESHOLD = 1,
			RS_NO_FOUND = 2 ,
		};

		struct SearchResult
		{
			SearchResult( ScoreType inScore , ResultState inState )
				:state(inState),score(inScore){}
			SearchResult(  ResultState inState )
				:state(inState){}

			ResultState state;
			ScoreType   score;
		};

		struct SearchNeighborFunc
		{
			bool operator()( StateType const& newState , ScoreType const& score )
			{
				result = searchInternal( newState , score , *this );
				if ( result.state == RS_FOUND )
					return false;

				if ( result.score < minScore )
				{
					minScore = result.score;
				}
				return true;
			}

			static SearchResult searchInternal( StateType const& state , ScoreType const& score , SearchNeighborFunc& func )
			{
				if ( score > func.threshold )
				{
					return SearchResult( RS_OVER_THRESHOLD , score );
				}
				if ( func.mThis->isGoal( state ) )
				{
					return SearchResult( RS_FOUND );
				}

				func.mThis->processNeighborNode( state , score , func);
				if (func.result.state == RS_FOUND )
					return SearchResult( RS_FOUND );

				return SearchResult( RS_NO_FOUND );
			}

	
			T* mThis;
			SearchResult result;
			ScoreType    threshold;
			ScoreType    minScore;
		};

		bool search( StateType const& start )
		{
			startSearch( start );

			SearchNeighborFunc func;
			func.mThis = _this();
			ScoreType threshold = calcHeuristic( start );
			for(;;)
			{
				func.minScore = std::numeric_limits< ScoreType >::max();
				SearchResult result = SearchNeighborFunc::searchInternal( start , ScoreType(0) , func);

				if ( result.state == RS_FOUND )
				{
					return true;
				}
				else
				{
					func.threshold = func.minScore;
				}
			}

			return false;
		}
	};

}//namespace AStar

#endif // IDAStar_h__594e537d_8986_47af_bf7f_04b8a26c3fb0
