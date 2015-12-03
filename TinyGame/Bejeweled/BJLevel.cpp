#include "TinyGamePCH.h"
#include "BJLevel.h"


#include "GameGlobal.h"

#define DBG_MSG(...)

namespace Bejeweled
{

	Level::Level( Listener* listener )
	{
		mDestroyRangeVec.reserve( 8 );
		mNumGemType = 7; 
		mListener = listener;
	}

	bool Level::swapSequence( int idx1 , int idx2 )
	{
		stepSwap( idx1 , idx2 );
		stepCheckSwap( idx1 , idx2 );
		if ( mDestroyRangeVec.empty() )
		{
			stepSwap( idx1 , idx2 );
			return false;
		}
		
		int combo = 0;

		for(;;)
		{
			stepDestroy();
			stepFill();
			setepCheckFill();

			if ( mDestroyRangeVec.empty() )
				break;

			++combo;
			DBG_MSG( "Combo = %d" , combo );
		}
		return true;
	}

	void Level::stepSwap( int idx1 , int idx2 )
	{
		assert( 0 <= idx1 && idx1 < BoardStorageSize );
		assert( 0 <= idx2 && idx2 < BoardStorageSize );
		assert( isNeighboring( idx1 , idx2 ) );

		std::swap( mBoard[idx1] , mBoard[idx2] );
	}

	size_t Level::stepCheckSwap( int idx1 , int idx2 )
	{
		mListener->prevCheckGems();

		checkGemVertical( idx1 );
		checkGemHorizontal( idx1 );
		checkGemVertical( idx2 );
		checkGemHorizontal( idx2 );

		mListener->postCheckGems();

		return mDestroyRangeVec.size();
	}

	bool Level::testRemoveGeom( int idx1 )
	{
		int idxStart;
		int num;
		num = countGemsHorizontal( idx1 , idxStart );
		if ( num >= MinRmoveCount )
			return true;
		num = countGemsVertical( idx1 , idxStart );
		if ( num >= MinRmoveCount )
			return true;

		return false;
	}

	bool Level::testSwapPairHaveRemove(int idx1 , int idx2)
	{
		stepSwap( idx1 , idx2 );
		bool result = testRemoveGeom( idx1 ) || testRemoveGeom( idx2 );
		stepSwap( idx1 , idx2 );
		return result;
	}

	void Level::checkGemVertical( int idx )
	{
		LineRange range;
		range.num = countGemsVertical( idx , range.idxStart );
		if ( range.num >= MinRmoveCount )
		{
			range.beVertical = true;
			range.method     = mListener->onFindSameGems( range.idxStart , range.num , range.beVertical );

			DBG_MSG( "add remove line %d %d %s" , range.idxStart , range.num , range.beVertical ? "Y" : "N" );
			mDestroyRangeVec.push_back( range );
		}
	}

	void Level::checkGemHorizontal( int idx )
	{
		LineRange range;
		range.num = countGemsHorizontal( idx , range.idxStart );
		if ( range.num >= MinRmoveCount )
		{
			range.beVertical = false;
			range.method     = mListener->onFindSameGems( range.idxStart , range.num , range.beVertical );

			DBG_MSG( "add remove line %d %d %s" , range.idxStart , range.num , range.beVertical ? "Y" : "N" );
			mDestroyRangeVec.push_back( range );
		}
	}

	void Level::stepFill()
	{
		for( int i = 0 ; i < BoardSize ; ++i )
		{
			if ( mIndexMax[i] == -1 )
				continue;
			fillGem( i , mIndexMax[i] );
		}
	}

	size_t Level::setepCheckFill()
	{
		mListener->prevCheckGems();

		for( int i = 0 ; i < BoardSize ; ++ i )
		{
			if ( mIndexMax[i] == -1 )
				continue;

			checkFillGem( i , mIndexMax[i] );
		}

		mListener->postCheckGems();
		return mDestroyRangeVec.size();
	}

	void Level::stepDestroy()
	{
		std::fill_n( mIndexMax , BoardSize , -1 );

		for ( LineRangeVec::iterator iter = mDestroyRangeVec.begin();
			iter != mDestroyRangeVec.end() ; ++iter )
		{
			LineRange& range = *iter;

			int idx = range.idxStart;
			if ( range.beVertical )
			{
				int const offset = BoardSize;

				for( int i = 0 ; i < range.num ; ++i  )
				{
					if ( mBoard[idx] )
					{
						int type = mBoard[idx];
						mBoard[idx] = 0;
						mListener->onDestroyGem( idx , type );
					}
					idx += offset;
				}

				idx -= offset;
				int x = idx % BoardSize;
				if ( idx > mIndexMax[x] )
					mIndexMax[ x ] = idx;
			}
			else
			{
				int const offset = 1;

				for( int i = 0 ; i < range.num ; ++i  )
				{
					if ( mBoard[idx] )
					{
						int type = mBoard[idx];
						mBoard[idx] = 0;
						mListener->onDestroyGem( idx , type );

						int x = idx % BoardSize;
						if ( idx > mIndexMax[x] )
							mIndexMax[ x ] = idx;
					}
					idx += offset;
				}
			}

		}
		mDestroyRangeVec.clear();
	}

	void Level::checkFillGem( int x , int maxIdx )
	{
		for( int idx = maxIdx ; idx >= 0 ; idx -= BoardSize )
		{
			LineRange range;
			range.num = countGemsVertical( idx , range.idxStart );
			if ( range.num < MinRmoveCount )
				continue;

			range.beVertical = true;
			range.method     = mListener->onFindSameGems( range.idxStart , range.num , range.beVertical );
			mDestroyRangeVec.push_back( range );
			DBG_MSG( "add remove line %d %d %s" , range.idxStart , range.num , range.beVertical ? "Y" : "N" );

			idx = range.idxStart;
		}

		for( int idx = maxIdx ; idx >= 0 ; idx -= BoardSize )
		{
			LineRange range;
			range.num = countGemsHorizontal( idx , range.idxStart );
			if ( range.num < MinRmoveCount )
				continue;

			bool beFound = false;
			for( LineRangeVec::iterator iter = mDestroyRangeVec.begin();
				iter != mDestroyRangeVec.end() ; ++iter )
			{
				if ( iter->beVertical )
					continue;
				if ( iter->idxStart == range.idxStart )
				{
					assert( iter->num == range.num );
					beFound = true;
					break;
				}
			}

			if ( beFound )
				continue;

			range.beVertical = false;
			range.method     = mListener->onFindSameGems( range.idxStart , range.num , range.beVertical );
			mDestroyRangeVec.push_back( range );
			DBG_MSG( "add remove line %d %d %s" , range.idxStart , range.num , range.beVertical ? "Y" : "N" );
		}
	}

	void Level::fillGem( int x , int idxMax )
	{
		int idx = idxMax;
		int idxFind = idxMax;

		for( ; idx >= 0 ; idx -= BoardSize )
		{
			for( idxFind -= BoardSize ; idxFind >= 0 ; idxFind -= BoardSize )
			{
				if ( mBoard[ idxFind ] )
					break;
			}

			if ( idxFind < 0 )
				break;

			mBoard[ idx ] = mBoard[ idxFind ];
			mBoard[ idxFind ] = 0;
			mListener->onMoveDownGem( idxFind  , idx );

		}

		for( ; idx >= 0 ; idx -= BoardSize )
		{
			assert( mBoard[idx] == 0 );
			int type = produceGem( idx );
			mListener->onFillGem( idx , type );
		}
	}

	int Level::produceGem( int idx )
	{
		GemType test[ MaxGemTypeNum ];
		std::fill_n( test , MaxGemTypeNum , 0 );
		for( int i = 0 ; i < MaxGemTypeNum ; ++i )
			test[i] = i + 1;

		int numUnTested = MaxGemTypeNum;
		while( numUnTested )
		{
			int idxTest = Global::RandomNet() % numUnTested;
			GemType type = test[ idxTest ] ? test[ idxTest ] : ( idxTest + 1 );

			mBoard[ idx ] = type;
			int idxStart;

			if ( countGemsHorizontal( idx , idxStart ) < MinRmoveCount &&
				 countGemsVertical( idx , idxStart ) < MinRmoveCount )
			{
				return type;
			}

			test[ idxTest ] = numUnTested;
			--numUnTested;
		}

		mBoard[ idx ] = 0;
		return 0;
	}

	int Level::countGemsVertical( int idx , int& idxStart )
	{
		int const offset = BoardSize;

		int count = 1;
		int idxTest = idx - offset;

		GemType gem = mBoard[ idx ];
		for(  ; idxTest >= 0 ; idxTest -= offset )
		{
			if ( mBoard[ idxTest ] != gem )
				break;
			++count;
		}
		idxStart = idxTest + offset;
		idxTest = idx + offset ; 
		for( ; idxTest < BoardSize * BoardSize ; idxTest += offset )
		{
			if ( mBoard[ idxTest ] != gem )
				break;
			++count;
		}
		return count;
	}

	int Level::countGemsHorizontal( int idx , int& idxStart )
	{
		int const offset = 1;
		int count = 1;

		int y = idx / BoardSize;
		int endIdx = y * BoardSize;

		GemType gem = mBoard[ idx ];

		int idxTest = idx - offset;
		for(  ; idxTest >= endIdx ; idxTest -= offset )
		{
			if ( mBoard[ idxTest ] != gem )
				break;
			++count;
		}

		idxStart = idxTest + offset;
		endIdx += BoardSize;
		idxTest = idx + offset ; 
		for( ; idxTest < endIdx ; idxTest += offset )
		{
			if ( mBoard[ idxTest ] != gem )
				break;
			++count;
		}
		return count;
	}

	void Level::generateRandGems()
	{
		std::fill_n( mBoard , BoardStorageSize , 0 );
		for( int i = 0 ; i < BoardStorageSize ; ++i )
		{
			produceGem( i );
		}
	}

}//namespace Bejeweled
