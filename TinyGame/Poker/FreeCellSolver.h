#ifndef FreeCellSolver_h__
#define FreeCellSolver_h__

#include "PokerBase.h"
#include "FCSolver/FCSFreecellSolvingAlgorithm.h""
#include "FCSolver/FCCommandLineArguments.h"

namespace FCS
{
	struct MoveInfo
	{
		int from;
		int to;
		int num;
	};
	struct State
	{
		int       step;
		int       numPassibleMove;
		MoveInfo* passibleMove;
		MoveInfo  move;
		uint8     key[52];
	};

	enum SearchResult
	{
		SR_FIND_SOLUTION ,
		SR_NO_SOLUTION ,
		SR_SEARCHING ,
	};


	class ISearchAlgorithm
	{

		struct StateCmp
		{
			bool operator()( State const& s1 , State const& s2 ) const
			{
				for( int i = 0 ; i < 52 ; ++i )
					if ( s1.key[i] < s2.key[i] )
						return true;
				return false;
			}
		};

		std::set< State* , StateCmp > mStateSet;
	};


	class SearchAlgorithm
	{
	public:
		SearchAlgorithm()
		{
			mAlgorithm = NULL;
		}
		bool  init()
		{
			mComArgs.Parse()
			mAlgorithm = FCSFreecellSolvingAlgorithm::Create( &mComArgs );
			if ( !mAlgorithm )
				return false;

			mComArgs.GetInitialState()
		}
		SearchResult runStep()
		{

		}

		SearchResult runSolution()
		{

		}

		FCCommandLineArguments mComArgs;
		FCSFreecellAlgorithm*  mAlgorithm;

	};


}

#endif // FreeCellSolver_h__
