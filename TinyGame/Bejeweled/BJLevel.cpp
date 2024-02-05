#include "TinyGamePCH.h"
#include "BJLevel.h"


#include "GameGlobal.h"

#define DBG_MSG(...)

namespace Bejeweled
{

	Level::Level( Listener* listener )
	{
		mNumGemType = 7; 
		mListener = listener;
	}

	bool Level::swapSequence( ActionContext& context, bool bRequestFill )
	{
		stepSwap(context);
		stepCheckSwap(context);
		if ( context.actionCount == 0)
		{
			stepSwap(context);
			return false;
		}
		
		int combo = 0;

		for(;;)
		{
			stepAction(context);
			if ( bRequestFill )
			{
				stepFill(context);
				stepCheckFill(context);
			}
			else
			{
				context.clearMark();
			}

			if ( context.actionCount == 0)
				break;

			++combo;
			DBG_MSG( "Combo = %d" , combo );
		}
		return true;
	}

	void Level::stepSwap(ActionContext& context)
	{
		int idx1 = context.swapIndices[0];
		int idx2 = context.swapIndices[1];
		swapGeom(idx1, idx2);
	}

	size_t Level::stepCheckSwap(ActionContext& context)
	{
		context.clearMark();

		mListener->prevCheckGems();
		auto DoCheck = [this, &context](int index)
		{
			if (mBoard[index].type == GemData::eHypercube)
			{
				RemoveDesc desc;
				desc.method = RemoveDesc::eTheSameId;
				desc.index = context.swapIndices[0] == index ? context.swapIndices[1] : context.swapIndices[0];
				markRemove(context, desc);

				desc.method = RemoveDesc::eNormal;
				desc.num = 1;
				desc.index = index;
				desc.bVertical = false;

				markRemove(context, desc);
				return;
			}

			int countA = checkGemVertical(context, index);
			int countB = checkGemHorizontal(context, index);

			if (countA >= 5 || countB >= 5)
			{
				context.actionMap[index] = EAction::BecomeHypercube;
			}
			else if (countA == 4 || countB == 4 || countA + countB == 6)
			{
				context.actionMap[index] = EAction::BecomePower;
			}

		};

		DoCheck(context.swapIndices[0]);
		DoCheck(context.swapIndices[1]);
		mListener->postCheckGems();

		return context.actionCount;
	}

	bool Level::testRemoveGeom( int idx1 )
	{
		int idxStart;
		int num;
		if (mBoard[idx1].type == GemData::eHypercube)
			return true;

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
		swapGeom(idx1, idx2);
		bool result = testRemoveGeom( idx1 ) || testRemoveGeom( idx2 );
		swapGeom(idx1, idx2);
		return result;
	}

	void Level::swapGeom(int idx1, int idx2)
	{
		assert(0 <= idx1 && idx1 < BoardStorageSize);
		assert(0 <= idx2 && idx2 < BoardStorageSize);
		assert(isNeighboring(idx1, idx2));

		std::swap(mBoard[idx1], mBoard[idx2]);
	}

	int Level::checkGemVertical(ActionContext& context, int idx)
	{
		RemoveDesc desc;
		desc.num = countGemsVertical( idx , desc.index );
		if ( desc.num >= MinRmoveCount )
		{
			desc.bVertical = true;
			desc.method = RemoveDesc::eNormal;

			DBG_MSG( "add remove line %d %d %s" , desc.index , desc.num , desc.bVertical ? "Y" : "N" );
			markRemove(context, desc);
			return desc.num;
		}
		return 0;
	}

	int Level::checkGemHorizontal(ActionContext& context, int idx )
	{
		RemoveDesc desc;
		desc.num = countGemsHorizontal( idx , desc.index );
		if ( desc.num >= MinRmoveCount )
		{
			desc.bVertical = false;
			desc.method = RemoveDesc::eNormal;

			DBG_MSG( "add remove line %d %d %s" , desc.index , desc.num , desc.bVertical ? "Y" : "N" );
			markRemove(context, desc);
			return desc.num;
		}

		return 0;
	}

	void Level::stepFill(ActionContext& context)
	{
		for( int i = 0 ; i < BoardSize ; ++i )
		{
			if (context.removeIndexMax[i] == INDEX_NONE )
				continue;
			fillGem( i , context.removeIndexMax[i] );
		}
	}

	size_t Level::stepCheckFill(ActionContext& context)
	{
		context.clearMark();
		mListener->prevCheckGems();

		for( int i = 0 ; i < BoardSize ; ++ i )
		{
			if (context.removeIndexMax[i] == INDEX_NONE )
				continue;

			checkFillGem(context, i , context.removeIndexMax[i] );
		}

		mListener->postCheckGems();
		return context.actionCount;
	}

	void Level::markRemoveGem(ActionContext& context, int index)
	{
		if (context.actionMap[index] != EAction::None)
			return;

		context.actionMap[index] = EAction::Destroy;
		context.actionCount += 1;

		switch (mBoard[index].type)
		{
		case GemData::ePower:
			{
				RemoveDesc desc;
				desc.method = RemoveDesc::eBomb;
				desc.num = 1;
				desc.bVertical = false;
				desc.index = index;
				markRemove(context, desc);
			}
			break;
		}
	}

	void Level::markRemove(ActionContext& context, RemoveDesc const& desc)
	{
		switch (desc.method)
		{
		case RemoveDesc::eNormal:
			{
				int indexOffset = desc.bVertical ? BoardSize : 1;
				int index = desc.index;
				for (int i = 0; i < desc.num; ++i)
				{
					markRemoveGem(context, index);
					index += indexOffset;
				}
			}
			break;
		case RemoveDesc::eLine:
			break;
		case RemoveDesc::eBomb:
			{
				int x, y;
				ToCell(desc.index, x, y);

				for( int i = -desc.num; i <= desc.num; ++i )
				{
					for (int j = -desc.num; j <= desc.num; ++j)
					{
						int cx = x + i;
						int cy = y + j;
						if ( !IsVaildRange( cx , cy ) )
							continue;

						int index = ToIndex(cx , cy);
						markRemoveGem(context, index);
					}
				}
			}
			break;
		case RemoveDesc::eTheSameId:
			{
				GemData gem = mBoard[desc.index];

				for (int index = 0; index < BoardStorageSize; ++index)
				{
					if (mBoard[index].isSameType(gem))
					{
						markRemoveGem(context, index);
					}
				}
			}
			break;
		}
	}

	void Level::destroyGem(int index)
	{
		GemData type = mBoard[index];
		mBoard[index] = GemData::None();
		mListener->onDestroyGem(index, type);

	}

	void Level::stepAction(ActionContext& context)
	{
		std::fill_n(context.removeIndexMax , BoardSize , INDEX_NONE );

		for (int x = 0; x < BoardSize; ++x )
		{
			for (int y = 0; y < BoardSize; ++y)
			{
				int idx = ToIndex(x, y);
				if ( !context.actionMap[idx] )
					continue;

				switch (context.actionMap[idx])
				{
				case EAction::Destroy:
					destroyGem(idx);
					context.removeIndexMax[x] = idx;
					break;

				case EAction::BecomeHypercube:
					{
						mBoard[idx] = GemData::Hypercube();
					}
					break;
				case EAction::BecomePower:
					{
						mBoard[idx] = GemData(GemData::ePower, mBoard[idx].meta);
					}
					break;
				}
			}
		}

	}

	void Level::checkFillGem(ActionContext& context, int x , int maxIdx )
	{
		for( int idx = maxIdx ; idx >= 0 ; idx -= BoardSize )
		{
			RemoveDesc desc;
			desc.num = countGemsVertical( idx , desc.index );
			if (desc.num < MinRmoveCount )
				continue;

			desc.bVertical = true;
			desc.method = RemoveDesc::eNormal;

			markRemove(context, desc);
			DBG_MSG( "add remove line %d %d %s" , range.idxStart , range.num , range.beVertical ? "Y" : "N" );

			idx = desc.index;
		}

		for( int idx = maxIdx ; idx >= 0 ; idx -= BoardSize )
		{
			RemoveDesc desc;
			desc.num = countGemsHorizontal( idx , desc.index );
			if ( desc.num < MinRmoveCount )
				continue;

			desc.bVertical = false;
			desc.method = RemoveDesc::eNormal;
			markRemove(context, desc);
			DBG_MSG( "add remove line %d %d %s" , desc.index , desc.num , desc.bVertical ? "Y" : "N" );
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
				if ( mBoard[ idxFind ] != GemData::None() )
					break;
			}

			if ( idxFind < 0 )
				break;

			mBoard[ idx ] = mBoard[ idxFind ];
			mBoard[ idxFind ] = GemData::None();
			mListener->onMoveDownGem( idxFind  , idx );

		}

		for( ; idx >= 0 ; idx -= BoardSize )
		{
			CHECK( mBoard[idx] == GemData::None() );
			GemData type = produceGem( idx );
			mListener->onFillGem( idx , type );
		}
	}

	GemData Level::produceGem( int idx )
	{
		GemData test[ MaxGemTypeNum ];
		for (int i = 0; i < MaxGemTypeNum; ++i)
			test[i] = GemData::Normal(i);

		int numUnTested = MaxGemTypeNum;
		while( numUnTested )
		{
			int idxTest = Global::RandomNet() % numUnTested;
			GemData type = test[ idxTest ];

			mBoard[ idx ] = type;
			int idxStart;

			if ( countGemsHorizontal( idx , idxStart ) < MinRmoveCount &&
				 countGemsVertical( idx , idxStart ) < MinRmoveCount )
			{
				return type;
			}
			--numUnTested;
			if (idxTest != numUnTested)
			{
				test[idxTest] = test[numUnTested];
			}
		}

		mBoard[ idx ] = GemData::None();
		return mBoard[idx];
	}

	bool CanCountGem(GemData const& gem)
	{
		return gem.type == GemData::eNormal || gem.type == GemData::ePower;
	}
	bool CanCountGem(GemData const& gem, GemData const& testGem)
	{
		CHECK(CanCountGem(gem));

		if (!CanCountGem(testGem))
			return false;
		return testGem.meta == gem.meta;
	}

	int Level::countGemsVertical( int idx , int& idxStart )
	{
		GemData gem = mBoard[idx];
		if (!CanCountGem(gem))
			return 0;

		int const offset = BoardSize;
		int count = 1;
		int idxTest = idx - offset;
		for(  ; idxTest >= 0 ; idxTest -= offset )
		{
			if (!CanCountGem(gem , mBoard[idxTest]))
				break;
			++count;
		}
		idxStart = idxTest + offset;
		idxTest = idx + offset ; 
		for( ; idxTest < BoardSize * BoardSize ; idxTest += offset )
		{
			if (!CanCountGem(gem, mBoard[idxTest]))
				break;
			++count;
		}
		return count;
	}

	int Level::countGemsHorizontal( int idx , int& idxStart )
	{
		GemData gem = mBoard[ idx ];
		if (!CanCountGem(gem))
			return 0;

		int const offset = 1;
		int count = 1;

		int y = idx / BoardSize;
		int endIdx = y * BoardSize;

		int idxTest = idx - offset;
		for(  ; idxTest >= endIdx ; idxTest -= offset )
		{
			if (!CanCountGem(gem, mBoard[idxTest]))
				break;
			++count;
		}

		idxStart = idxTest + offset;
		endIdx += BoardSize;
		idxTest = idx + offset ; 
		for( ; idxTest < endIdx ; idxTest += offset )
		{
			if (!CanCountGem(gem, mBoard[idxTest]))
				break;
			++count;
		}
		return count;
	}

	void Level::generateRandGems()
	{
		std::fill_n( mBoard , BoardStorageSize , GemData::None() );
		for( int i = 0 ; i < BoardStorageSize ; ++i )
		{
			produceGem( i );
		}
	}

}//namespace Bejeweled
