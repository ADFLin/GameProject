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
		//  call addSreachNode for all possible next state
		template < class Fun >
		void      processNeighborNode( StateType& node , ScoreType const& score , Fun& fun ){  NEVER_REACH("Need impl processNeighborNode"); }

	public:
		void startSreach()
		{

		}

		enum ResultState
		{
			RS_FOUND = 0,
			RS_OVER_THRESHOLD = 1,
			RS_NO_FOUND = 2 ,
		};
		struct SreachResult
		{
			SreachResult( ScoreType inScore , ResultState inState )
				:state(inState),score(inScore){}
			SreachResult(  ResultState inState )
				:state(inState){}

			ResultState state;
			ScoreType   score;
		};
		struct SreachNeighborFun
		{
			bool operator()( StateType const& newState , ScoreType const& score )
			{
				result = sreachInternal( newState , score , *this );
				if ( result.state == RS_FOUND )
					return false;

				if ( result.score < minScore )
				{
					minScore = result.score;
				}
				return true;
			}

			static SreachResult sreachInternal( StateType const& state , ScoreType const& score , SreachNeighborFun& fun )
			{
				if ( score > fun.threshold )
				{
					return SreachResult( RS_OVER_THRESHOLD , score );
				}
				if ( fun.mThis->isGoal( state ) )
				{
					return SreachResult( RS_FOUND );
				}

				fun.mThis->processNeighborNode( state , score , fun );
				if ( fun.result.state == RS_FOUND )
					return SreachResult( RS_FOUND );

				return SreachResult( RS_NO_FOUND );
			}

	
			T* mThis;
			SreachResult result;
			ScoreType    threshold;
			ScoreType    minScore;
		};

		bool sreach( StateType const& start )
		{
			startSreach( start );

			SreachNeighborFun fun;
			fun.mThis = _this();
			ScoreType threshold = calcHeuristic( start );
			for(;;)
			{
				fun.minScore = std::numeric_limits< ScoreType >::max();
				SreachResult result = SreachNeighborFun::sreachInternal( start , ScoreType(0) , fun );

				if ( result.state == RS_FOUND )
				{
					return true;
				}
				else
				{
					fun.threshold = fun.minScore;
				}
			}

			return false;
		}
	};

}//namespace AStar

#endif // IDAStar_h__594e537d_8986_47af_bf7f_04b8a26c3fb0
