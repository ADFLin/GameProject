#include "TinyGamePCH.h"
#include "HoldemLevel.h"


#include <algorithm>
#include <functional>

namespace Holdem {

	int const gFaceRankPower[13] =
	{
		12 , 0 , 1 , 2 , 3 ,4 , 5 ,6 ,7 ,8 ,9 ,10 ,11,
	};



	struct CardSortCmp
	{
		bool operator()( Card const& c1 , Card const& c2 )
		{
			return gFaceRankPower[ c1.getFaceRank() ] < gFaceRankPower[ c2.getFaceRank() ];
		}
	};

	class CardTrickHelper
	{
	public:
		static CardGroup getGroup( int power )
		{
			return CardGroup( power >> 20 );
		}
		int calcPower( Card sortedCards[] , int numCard , int idxCardTake[] );
	private:
		static int const MaxCardNum = CommunityCardNum + PocketCardNum;
		struct FaceGroup
		{
			int idx;
			int power;
			int rank;
			int num;
		};
		struct SuitGroup
		{
			int num;
			int index[ MaxCardNum ];
		};
		static int MakePower( CardGroup group , int c0 = 0 , int c1 = 0 , int c2 = 0, int c3 = 0 , int c4 = 0 )
		{
			return ( group << 20 ) | ( c0 << 16 ) | ( c1 << 12 ) | ( c2 << 8 ) | ( c3 << 4 ) | c4;
		}

		void initGroup( Card sortedCards[] , int numCard );
		int calcStraight( int& idxStraight );
		int makeTwoParis( int idxCardTake[] , int idxSubPair );
		int makeFullHouse( int idxCardTake[] , int idxSubPair );
		int makePair( int idxCardTake[] );
		int makeThreeOfKind( int idxCardTake[] );
		int makeFourOfAKind( int idxCardTake[] );
		int makeHighHand(int idxCardTake[] );
		int makeFlash( int idxCardTake[] , int idxFlushSuit , Card sortedCards[] );
		int makeStraightFlush(int idxCardTake[] , int idxStraight , int idxFlushSuit , Card sortedCards[] );
		int makeStraight(int idxCardTake[] , int idxStraight );

		FaceGroup faceGroup[ MaxCardNum ];
		SuitGroup suitGroup[ 4 ];

		int idxLastGroup;
		int idxMaxNumGroup;
	};

	int CardTrickHelper::calcPower( Card sortedCards[] , int numCard , int idxCardTake[] )
	{
		assert( PocketCardNum == 2 );
		assert( numCard >= 5 && numCard <= MaxCardNum );

#if _DEBUG
		for( int i = 0 ; i < numCard - 1; ++i )
			assert( gFaceRankPower[ sortedCards[ i ].getFaceRank() ] <= gFaceRankPower[ sortedCards[ i + 1 ].getFaceRank() ]);
#endif
		initGroup(sortedCards, numCard);

		FaceGroup& maxNumGroup = faceGroup[ idxMaxNumGroup ];

		int idxSubPair = INDEX_NONE;
		if ( maxNumGroup.num == 4 )
		{
		   return makeFourOfAKind( idxCardTake );
		}
		else if ( maxNumGroup.num >= 2 )
		{
			idxSubPair = idxLastGroup;
			for(  ; idxSubPair >= 0 ; --idxSubPair )
			{
				if ( idxSubPair == idxMaxNumGroup )
					continue;
				if ( faceGroup[ idxSubPair ].num >= 2 )
				{
					if ( maxNumGroup.num == 3 )
					{
						return makeFullHouse( idxCardTake , idxSubPair);
					}
					break;
				}
			}
		}

		int idxFlushSuit = INDEX_NONE;
		for( int i = 0 ; i < 4 ; ++i )
		{
			if ( suitGroup[i].num >= CardTrickNum )
			{
				idxFlushSuit = i;
				break;
			}
		}

		int idxStraight;
		int countSeq = calcStraight( idxStraight );

		if ( idxFlushSuit != INDEX_NONE)
		{
			if ( countSeq == CardTrickNum )
			{
				return makeStraightFlush(idxCardTake , idxStraight, idxFlushSuit , sortedCards );
			}
			else
			{
				return makeFlash( idxCardTake , idxFlushSuit, sortedCards );
			}
		}

		if ( countSeq == CardTrickNum )
		{
			return makeStraight( idxCardTake , idxStraight );
		}

		switch ( maxNumGroup.num )
		{
		case 3: return makeThreeOfKind(idxCardTake);
		case 2:
			if ( idxSubPair >= 0 )
			{
				return makeTwoParis( idxCardTake , idxSubPair );
			}
			else
			{
				return makePair( idxCardTake );
			}
			break;
		}
		return makeHighHand(idxCardTake);

	}

	void CardTrickHelper::initGroup(Card sortedCards[] , int numCard)
	{
		for( int i = 0 ; i < 4 ; ++i )
			suitGroup[i].num = 0;

		faceGroup[0].idx   = 0;
		faceGroup[0].power = gFaceRankPower[ sortedCards[0].getFaceRank() ];
		faceGroup[0].rank  = sortedCards[0].getFaceRank();
		faceGroup[0].num   = 1;

		idxLastGroup = 0;
		idxMaxNumGroup = 0;

		for( int i = 1 ; i < numCard ; ++i )
		{
			Card& card = sortedCards[i];
			int rank = card.getFaceRank();
			int suit = card.getSuit();

			SuitGroup& sg = suitGroup[ suit ];
			sg.index[ sg.num ] = i;
			sg.num += 1;

			if ( faceGroup[ idxLastGroup ].rank == rank )
			{
				++faceGroup[ idxLastGroup ].num;
			}
			else
			{
				if ( faceGroup[ idxLastGroup ].num >= faceGroup[ idxMaxNumGroup ].num )
					idxMaxNumGroup = idxLastGroup;

				++idxLastGroup;
				FaceGroup& newGroup = faceGroup[ idxLastGroup ];
				newGroup.idx   = i;
				newGroup.power = gFaceRankPower[ rank ];
				newGroup.rank  = rank;
				newGroup.num   = 1;
			}
		}

		if ( faceGroup[ idxLastGroup ].num >= faceGroup[ idxMaxNumGroup ].num )
			idxMaxNumGroup = idxLastGroup;
	}

	int CardTrickHelper::calcStraight(int& idxStraight)
	{
		idxStraight = idxLastGroup;

		int countSeq = 1;
		int idxCur = idxLastGroup;
		int rankSeq = faceGroup[ idxCur ].rank;
		// check 10 J Q K A
		if ( rankSeq == Card::ToRank( Card::eACE ) && 
			faceGroup[ idxCur - 1 ].rank == Card::ToRank( Card::eKING ) )
		{
			++countSeq;
			--idxCur;
			rankSeq = Card::ToRank( Card::eKING );
		}

		for(  ; idxCur >= 0 ; --idxCur )
		{
			if ( faceGroup[ idxCur ].rank + 1 == rankSeq )
			{
				rankSeq -= 1;
				++countSeq;
				if ( countSeq == CardTrickNum )
					break;
			}
			else
			{
				rankSeq = faceGroup[ idxCur ].rank;
				countSeq = 1;
				idxStraight = idxCur;
			}
		}

		//check A 2 3 4 5
		if ( countSeq == CardTrickNum - 1 && 
			rankSeq == Card::ToRank( Card::eN2 ) &&
			faceGroup[ idxLastGroup ].rank == Card::ToRank( Card::eACE ) )
		{
			++countSeq;
		}

		return countSeq;
	}

	int CardTrickHelper::makeFourOfAKind(int idxCardTake[])
	{
		FaceGroup& maxNumGroup = faceGroup[ idxMaxNumGroup ];
		for( int i = 0 ; i < 4 ; ++i )
			idxCardTake[i] = maxNumGroup.idx + i;
		int idxSub = idxLastGroup;
		if ( idxSub == idxMaxNumGroup )
			--idxSub;
		idxCardTake[ 4 ] = faceGroup[ idxSub ].idx;
		return MakePower( CG_FOUR_OF_A_KIND , 
			maxNumGroup.power , faceGroup[ idxSub ].power );
	}

	int CardTrickHelper::makeThreeOfKind(int idxCardTake[])
	{
		FaceGroup &maxNumGroup = faceGroup[ idxMaxNumGroup ];
		int idxSub[2];
		idxSub[0] = idxLastGroup;
		if ( idxSub[0] == idxMaxNumGroup )
			--idxSub[0];
		idxSub[1] = idxSub[0] - 1;
		if ( idxSub[1] == idxMaxNumGroup )
			--idxSub[1];
		for( int i = 0 ; i < 3 ; ++i )
			idxCardTake[i] = maxNumGroup.idx + i;
		for( int i = 0 ; i < 2 ; ++i )
			idxCardTake[3+i] = faceGroup[ idxSub[i] ].idx;
		return MakePower( CG_THREE_OF_A_KIND , 
			maxNumGroup.power , 
			faceGroup[ idxSub[0] ].power , 
			faceGroup[ idxSub[1] ].power );
	}

	int CardTrickHelper::makeTwoParis(int idxCardTake[] , int idxSubPair)
	{
		FaceGroup &maxNumGroup = faceGroup[ idxMaxNumGroup ];
		int idxSub2 = idxLastGroup;
		if ( idxSub2 == idxMaxNumGroup )
		{
			--idxSub2;
			if ( idxSub2 == idxSubPair )
				--idxSub2;
		}

		for( int i = 0 ; i < 2 ; ++i )
			idxCardTake[i] = maxNumGroup.idx + i;
		for( int i = 0 ; i < 2 ; ++i )
			idxCardTake[2+i] = faceGroup[ idxSubPair ].idx + i;
		idxCardTake[4] = faceGroup[ idxSub2 ].idx;
		return MakePower( CG_TWO_PAIRS ,
			maxNumGroup.power , 
			faceGroup[ idxSubPair ].power , 
			faceGroup[ idxSub2 ].power );
	}

	int CardTrickHelper::makePair(int idxCardTake[])
	{
		FaceGroup& maxNumGroup = faceGroup[ idxMaxNumGroup ];
		int idxSub[3];
		idxSub[0] = idxLastGroup;
		if ( idxSub[0] == idxMaxNumGroup )
			--idxSub[0];
		idxSub[1] = idxSub[0] - 1;
		if ( idxSub[1] == idxMaxNumGroup )
			--idxSub[1];
		idxSub[2] = idxSub[1] - 1;
		if ( idxSub[2] == idxMaxNumGroup )
			--idxSub[2];

		for( int i = 0 ; i < 2 ; ++i )
			idxCardTake[i] = maxNumGroup.idx + i;
		for( int i = 0 ; i < 3 ; ++i )
			idxCardTake[2+i] = faceGroup[ idxSub[i] ].idx;

		return MakePower( CG_PAIR , 
			maxNumGroup.power , 
			faceGroup[ idxSub[0] ].power , 
			faceGroup[ idxSub[1] ].power , 
			faceGroup[ idxSub[2] ].power );
	}

	int CardTrickHelper::makeFullHouse(int idxCardTake[] , int idxSubPair)
	{
		FaceGroup& maxNumGroup = faceGroup[ idxMaxNumGroup ];
		for( int i = 0 ; i < 3 ; ++i )
			idxCardTake[i] = maxNumGroup.idx + i;
		for( int i = 0 ; i < 2 ; ++i )
			idxCardTake[3+i] = faceGroup[ idxSubPair ].idx + i;
		return MakePower( CG_FULL_HOUSE , 
			maxNumGroup.power , faceGroup[ idxSubPair ].power );
	}

	int CardTrickHelper::makeStraight(int idxCardTake[] , int idxStraight)
	{
		for( int i = 0 ; i < CardTrickNum ; ++i )
		{
			int idxGroup = idxStraight - i;
			if ( idxGroup < 0 )
				idxGroup += 13;
			idxCardTake[i] = faceGroup[ idxGroup ].idx;
		}
		return MakePower( CG_STRAIGHT , faceGroup[ idxStraight ].power );
	}

	int CardTrickHelper::makeStraightFlush(int idxCardTake[] , int idxStraight , int idxFlushSuit , Card sortedCards[])
	{
		for( int i = 0 ; i < CardTrickNum ; ++i )
		{
			int idxGroup = idxStraight - i;
			if ( idxGroup < 0 )
				idxGroup += 13;
			FaceGroup& group = faceGroup[ idxGroup ];
			for ( int n = 0 ; n < group.num ; ++n )
			{
				int idx = group.idx + n;
				if ( sortedCards[ idx ].getSuit() == idxFlushSuit )
				{
					idxCardTake[i] = idx;
					break;
				}
			}
		}

		if ( faceGroup[ idxStraight ].rank == Card::ToRank( Card::eACE ) )
		{
			return MakePower( CG_ROYAL_FLUSH );
		}
		else
		{
			return MakePower( CG_STRAIGHT_FLUSH , faceGroup[ idxStraight ].power );
		}
	}

	int CardTrickHelper::makeFlash(int idxCardTake[] , int idxFlushSuit , Card sortedCards[])
	{
		SuitGroup& sg = suitGroup[ idxFlushSuit ];
		int power[ CardTrickNum ];
		for( int i = 0 ; i < CardTrickNum ; ++i )
		{
			int idx = sg.index[ sg.num - 1 - i ];
			idxCardTake[i] = idx;
			power[i] = gFaceRankPower[ sortedCards[ idx ].getFaceRank() ];
		}
		return MakePower( CG_FLUSH , power[0] , power[1] , power[2] , power[3] , power[4] );
	}

	int CardTrickHelper::makeHighHand(int idxCardTake[])
	{
		int power[ CardTrickNum ];
		for( int i = 0 ; i < CardTrickNum ; ++i )
		{
			FaceGroup& group = faceGroup[ idxLastGroup - i ];
			power[i] = group.power;
			idxCardTake[i] = group.idx;
		}
		return MakePower( CG_HIGH_HAND , power[0] , power[1] , power[2] , power[3] , power[4] );
	}

	static int NextPos( int pos )
	{
		return ( pos == MaxPlayerNum - 1 ) ? 0 : pos + 1;
	}
	static int PrevPos( int pos )
	{
		return ( pos == 0 ) ? MaxPlayerNum - 1 : pos - 1;
	}

	struct SDBetCall
	{
		char pos;
	};
	struct SCDBetInfo
	{
		char  pos;
		char  step;
		uint8 type;
		int   money;
	};

	struct SDSlotInfo
	{
		char pos;
		int  playerId;
		int  money;
	};

	struct SDGameInfo
	{
		Rule       rule;
		int        numPlayer;
		SDSlotInfo info[ MaxPlayerNum ];

		int getSendSize() const
		{
			return sizeof(*this) + ( numPlayer - MaxPlayerNum ) * sizeof(SDSlotInfo);
		}
	};

	struct SDShowPocketCard
	{
		int  numPlayer;
		struct
		{
			char pos;
			char cards[2];
		} pocketCardInfos[ MaxPlayerNum ];

		int getSendSize() const
		{
			return sizeof(*this) + ( numPlayer - MaxPlayerNum ) * sizeof(SDShowPocketCard::pocketCardInfos[0]);
		}
	};

	struct SDRoundInit
	{
		int  startBetMoney[ MaxPlayerNum ];
		int  ownMoney[ MaxPlayerNum ];
		char posButton;
		char posBet;
		char pocketCard[ PocketCardNum ];
	};

	struct SDNextStep
	{
		char step;
		char card[ 3 ];
	};

	struct SDTrickResult
	{
		char numSlotTrick;
		SlotTrickInfo info[ MaxPlayerNum ];

		int getSendSize() const
		{
			return sizeof(*this) + ( numSlotTrick - MaxPlayerNum ) * sizeof(SlotTrickInfo);
		}
	};

	struct SDWinnerInfo
	{
		int  pos   : 4;
		int  money : 28;	
	};

	struct SDWinnerResult
	{
		static int const BaseSize;
		char         numPot;
		char         numWinner[ MaxPlayerNum ];
		SDWinnerInfo info[ MaxPlayerNum * MaxPlayerNum ];
	};

	int const SDWinnerResult::BaseSize = sizeof( SDWinnerResult ) - sizeof( SDWinnerInfo ) * MaxPlayerNum * MaxPlayerNum;

#define DATA_LIST( op )\
		op(SDGameInfo)\
	    op(SDRoundInit)\
		op(SDBetCall)\
		op(SCDBetInfo)\
		op(SDNextStep)\
		op(SDTrickResult)\
		op(SDWinnerResult)\
		op(SDShowPocketCard)

#define COMMON_LIST(op)

	DEFINE_DATA2ID( DATA_LIST , COMMON_LIST)

#undef DATA_LIST

	LevelBase::LevelBase()
	{
		for( int i = 0 ; i < MaxPlayerNum ; ++i )
			mSlotInfoVec[i].pos = i;
		mBetStep = STEP_NO_BET;
		mMaxBetMoney = 0;
	}

	void LevelBase::doGameInit( Rule const& rule )
	{
		mRule    = rule;
		mBetStep = STEP_NO_BET;
	}

	void LevelBase::doInitNewRound()
	{
		std::fill_n(mPotPool, MaxPlayerNum, 0);
		mIdxPotLast = 0;
		mPotBetMoney = 0;

		for( int i = 0; i < MaxPlayerNum; ++i )
		{
			SlotInfo& info = getSlotInfo(i);
			std::fill_n(info.betMoney, (int)NUM_BET_STEP, 0);

			info.betMoneyOrder = -1;
			for( int n = 0; n < PocketCardNum; ++n )
				info.pocketCards[n] = -1;
		}
	}

	void LevelBase::doNewRound(int posButton, int posBet)
	{
		mPosButton   = posButton;
		mPosCurBet   = posBet;
		mMaxBetMoney = getRule().bigBlind;

		for( int i = 0; i < MaxPlayerNum; ++i )
		{
			SlotInfo& info = getSlotInfo(i);
			info.totalBetMoney = info.betMoney[STEP_HOLE_CARDS];
		}

		doNextStep( STEP_HOLE_CARDS , NULL );
	}

	void LevelBase::doBet( int pos , EBetAction type , int money )
	{
		assert(isPlaying());

		if ( pos != mPosCurBet )
			return;

		SlotInfo& info = getSlotInfo( pos );

		info.lastAction = type;

		if ( type != EBetAction::Fold )
		{
			int delta = money - info.betMoney[ mBetStep ];
			info.ownMoney -= delta;
			info.totalBetMoney += delta;
			info.betMoney[ mBetStep ] = money;
			if ( money > mMaxBetMoney )
				mMaxBetMoney = money;
		}
	}

	void LevelBase::doNextStep( BetStep step , char cards[] )
	{

		if ( step != STEP_HOLE_CARDS )
		{
			updatePotPool( false );
		}

		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			SlotInfo& info = getSlotInfo( i );
			if ( info.status != ESlotStatus::Playing )
				continue;

			if ( info.lastAction != EBetAction::Fold )
				info.lastAction = EBetAction::None;
		}

		mBetStep     = step;
		mMaxBetMoney = 0;

		switch( step )
		{
		case STEP_FLOP_CARDS:
			mCommunityCards[0] = Card( cards[0] );
			mCommunityCards[1] = Card( cards[1] );
			mCommunityCards[2] = Card( cards[2] );
			break;
		case STEP_TURN_CARD:
			mCommunityCards[3] = Card( cards[0] );
			break;
		case STEP_RIVER_CARD:
			mCommunityCards[4] = Card( cards[0] );
			break;
		}	
	}

	void LevelBase::doRoundEnd()
	{
		mBetStep = STEP_SHOW_DOWN;
		mPosCurBet = -1;
	}

	void LevelBase::doWinMoney( int pos , int money )
	{
		SlotInfo& info = getSlotInfo( pos );
		info.ownMoney += money;
	}

	int LevelBase::nextPlayingPos( int pos )
	{
		do 
		{ 
			pos = NextPos( pos );
		}
		while ( getSlotInfo( pos ).status != ESlotStatus::Playing );
		return pos;
	}

	int LevelBase::prevPlayingPos( int pos )
	{
		do 
		{ 
			pos = PrevPos( pos );
		}
		while ( getSlotInfo( pos ).status != ESlotStatus::Playing );
		return pos;
	}

	int LevelBase::getCommunityCardNum()
	{
		int const cardNum[] = { 5 , 0 , 0 , 3 , 4 , 5 };
		return cardNum[ mBetStep + 2 ];
	}

	void LevelBase::setupTransfer( IDataTransfer* transfer )
	{
		assert( transfer );
		mTransfer = transfer;
		mTransfer->setRecvFunc( RecvFunc( this , &LevelBase::procRecvData ) );
	}

	void LevelBase::updatePotPool( bool beEnd )
	{
		int posBetSorted[ MaxPlayerNum ];
		int numBetEnd = 0;
		int numPlayer = 0;
		int betMoney = 0;
		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			SlotInfo& info = getSlotInfo( i );
			if ( info.status != ESlotStatus::Playing )
				continue;

			if ( info.betMoneyOrder != -1 )
				continue;

			betMoney += info.betMoney[ getBetStep() ];
			++numPlayer;

			if ( beEnd || info.betMoney[ getBetStep() ] < getMaxBetMoney() )
			{
				posBetSorted[ numBetEnd ] = i;
				++numBetEnd;
			}
		}

		struct CmpFun
		{
			CmpFun( SlotInfo* info ):info(info){} 
			bool operator()( int p1 , int p2 )
			{
				return info[p1].totalBetMoney < info[p2].totalBetMoney;
			}
			SlotInfo* info;
		};

		if ( numBetEnd )
		{
			if ( numBetEnd > 1 )
			{
				std::sort( posBetSorted , posBetSorted + numBetEnd , CmpFun( mSlotInfoVec ) );
			}

			int betOrderLast = mIdxPotLast;
			int prevMoney    = mPotBetMoney;
			int money = getSlotInfo( posBetSorted[0] ).totalBetMoney;
			getSlotInfo( posBetSorted[0] ).betMoneyOrder = betOrderLast;

			int numBetPlayer = numPlayer;
			int numThisBet = 1;
			for( int i = 1 ; i < numBetEnd ; ++i )
			{
				SlotInfo& info = getSlotInfo( posBetSorted[ i ] );

				if ( info.totalBetMoney != money )
				{
					mPotPool[ betOrderLast ] = ( money - prevMoney ) * numBetPlayer;

					numBetPlayer -= numThisBet;

					++betOrderLast;
					prevMoney = money;
					money     = info.totalBetMoney;
					numThisBet = 1;
				}
				else
				{
					++numThisBet;
				}

				info.betMoneyOrder = betOrderLast;
			}

			mPotPool[ betOrderLast ] = ( money - prevMoney ) * numBetPlayer;
			mIdxPotLast  = betOrderLast;
			mPotBetMoney = prevMoney;
		}
		else
		{
			mPotPool[ mIdxPotLast ] += betMoney;
		}
	}

	void ServerLevel::showHandCard(uint32 betTypeMask)
	{
		SDShowPocketCard data;
		data.numPlayer = 0;
		for( int i = 0; i < MaxPlayerNum; ++i )
		{
			SlotInfo& slot = getSlotInfo(i);

			if( slot.status != ESlotStatus::Playing )
				continue;
			if( !(BIT(uint32(slot.lastAction)) & betTypeMask) )
				continue;
			int idx = data.numPlayer;
			data.pocketCardInfos[idx].pos = slot.pos;
			for( int n = 0; n < PocketCardNum; ++n )
				data.pocketCardInfos[idx].cards[n] = mSlotPocketCards[slot.pos][n].getIndex();
			++data.numPlayer;
		}

		getTransfer().sendData( SLOT_SERVER , data );

	}

	void ServerLevel::shuffle(IRandom& random)
	{
		for( int i = 0 ; i < 2 ; ++i )
			random.getInt();

		mDecks.clear();

		int card[52];
		for (int i = 0 ; i < 52 ; ++i )
			card[i] = i;
		for( int i = 0 ; i < 52 ; ++i )
			std::swap( card[i] , card[ random.getInt() % 52 ] );

		for (int i = 0 ; i < 52; ++i )
		{
			mDecks.push_back( card[i] );
		}
	}

	void ServerLevel::procRecvData( int recvId , int dataId , void* data , int dataSize )
	{
		switch( dataId )
		{
		case DATA2ID( SCDBetInfo ):
			{
				SCDBetInfo* myData = static_cast< SCDBetInfo* >( data );
				if ( mBetStep == myData->step )
					procBetRequest( myData->pos , EBetAction( myData->type ) , myData->money );
			}
			break;
		}
	}

	void ServerLevel::setupGame( Rule const& rule )
	{
		doGameInit( rule );
		SDGameInfo data;
		data.rule = rule;

		mPosButton = MaxPlayerNum - 1;
		int num = 0;
		for ( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			SlotInfo const& info = getSlotInfo( i );
			switch( info.status )
			{
			case ESlotStatus::Playing:
			case ESlotStatus::WaitNext:
				if ( info.status == ESlotStatus::WaitNext )
					data.info[ num ].playerId = -( int(info.playerId) + 1 );
				else
					data.info[ num ].playerId = ( int(info.playerId) + 1 );
				data.info[ num ].pos   = i;
				data.info[ num ].money = info.ownMoney;
				++num;
				break;
			}
		}
		data.numPlayer = num;
		getTransfer().sendData( SLOT_SERVER , data );
	}

	ServerLevel::ServerLevel()
	{
		mListener = nullptr;
	}

	void ServerLevel::startNewRound(IRandom& random)
	{
		//FIXME
		if ( isPlaying() )
		{
			for( int i = 0 ; i < MaxPlayerNum; ++i )
			{
				SlotInfo& info = getSlotInfo( i );
				if ( info.status != ESlotStatus::Playing )
					continue;
				info.ownMoney += info.totalBetMoney;
				info.totalBetMoney = 0;
			}
		}

		doInitNewRound();

		int numPlaying = 0;
		for( int i = 0 ; i < MaxPlayerNum; ++i )
		{
			SlotInfo& info = getSlotInfo( i );
			if ( info.status == ESlotStatus::Empty )
				continue;

			if ( info.ownMoney < getRule().bigBlind )
			{
				if( mListener )
					mListener->onPlayerLessBetMoney(info.pos);

				if( info.ownMoney < getRule().bigBlind )
				{
					info.status = ESlotStatus::WaitNext;
					//FIXME
					continue;
				}
			}

			++numPlaying;
			info.lastAction = EBetAction::None;
			if ( info.status == ESlotStatus::WaitNext )
			{
				info.status = ESlotStatus::Playing;
				info.betMoney[ STEP_HOLE_CARDS ] = getRule().bigBlind;
			}
			else
			{
				info.betMoney[ STEP_HOLE_CARDS ] = 0;
			}
		}
		if ( numPlaying < 2 )
			return;

		int posButton = mPosButton;

		for ( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			posButton = NextPos( posButton );
			SlotInfo& info = getSlotInfo( posButton );

			if ( getSlotInfo( posButton ).status == ESlotStatus::Playing )
				break;
		}

		int posShuffle;
		int posBet;
		if ( numPlaying == 2 )
		{
			int pos = posButton;
			posShuffle = pos;
			posBet     = pos;
			if ( getSlotInfo( pos ).betMoney[ STEP_HOLE_CARDS ] == 0 )
				getSlotInfo( pos ).betMoney[ STEP_HOLE_CARDS ] = getRule().smallBlind;
			pos = nextPlayingPos( pos );
			getSlotInfo( pos ).betMoney[ STEP_HOLE_CARDS ] = getRule().bigBlind;
		}
		else
		{
			int pos = nextPlayingPos( posButton );
			posShuffle = pos;
			if ( getSlotInfo( pos ).betMoney[ STEP_HOLE_CARDS ] == 0 )
				getSlotInfo( pos ).betMoney[ STEP_HOLE_CARDS ] = getRule().smallBlind;
			
			pos = nextPlayingPos( pos );
			getSlotInfo( pos ).betMoney[ STEP_HOLE_CARDS ] = getRule().bigBlind;

			posBet = nextPlayingPos( pos );
		}

		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			SlotInfo& info = getSlotInfo( i );
			info.ownMoney -= info.betMoney[ STEP_HOLE_CARDS ];
		}

		mNumBet = numPlaying;
		
		mNumAllIn = 0;
		mNumFinishBet = mNumAllIn;
		mNumCall = 0;
		mPosLastBet = -1;
		doNewRound( posButton , posBet );
		shuffle( random );

		for( int n = 0 ; n < PocketCardNum ; ++n )
		{
			int pos = posShuffle;
			for( int i = 0 ; i < numPlaying ; ++i )
			{
				SlotInfo& info = getSlotInfo( pos );
				if ( info.status == ESlotStatus::Playing )
				{
					mSlotPocketCards[ pos ][ n ] = Card( mDecks.back() );
					mDecks.pop_back();
				}
				pos = nextPlayingPos( pos );
			}
		}

		{
			SDRoundInit data;
			data.posButton = mPosButton;

			for( int i = 0 ; i < MaxPlayerNum ; ++i )
			{
				SlotInfo& info = getSlotInfo( i );
				switch( info.status )
				{
				case ESlotStatus::Playing: 
					data.startBetMoney[i] = info.betMoney[ STEP_HOLE_CARDS ];
					data.ownMoney[i] = info.ownMoney;
					break;
				case ESlotStatus::WaitNext:
					data.startBetMoney[i] = -2;
					data.ownMoney[i] = info.ownMoney;
					break;
				case ESlotStatus::Empty:
					data.startBetMoney[i] = -1;
					data.ownMoney[i] = 0;
					break;	
				}
			}

			for( int i = 0 ; i < MaxPlayerNum ; ++i )
			{
				SlotInfo& slot = getSlotInfo( i );

				if ( slot.status == ESlotStatus::Empty )
					continue;
				
				if( slot.status == ESlotStatus::Playing )
				{
					for( int n = 0; n < PocketCardNum; ++n )
					{
						data.pocketCard[n] = mSlotPocketCards[i][n].getIndex();
					}
				}
				else
				{
					for( int n = 0; n < PocketCardNum; ++n )
					{
						data.pocketCard[n] = -1;
					}
				}
				getTransfer().sendData( i, data);
			}
		}

		{
			SDBetCall data;
			data.pos = mPosCurBet;
			getTransfer().sendData( SLOT_SERVER , data );
		}
	}

	void ServerLevel::procBetRequest( int pos , EBetAction type , int money )
	{
		if( !isPlaying() )
			return;

		if ( pos != mPosCurBet )
			return;

		SlotInfo& info = getSlotInfo( pos );

		int betMoney = 0;
		bool needRecount = false;
		int slotMoney = info.ownMoney + info.betMoney[ mBetStep ];
		switch( type )
		{
		case EBetAction::Fold:
			--mNumBet;
			break;
		case EBetAction::Call:
			if ( slotMoney <= getMaxBetMoney() )
				return;
			betMoney = getMaxBetMoney();
			mPosLastBet = pos;
			++mNumCall;
			break;

		case EBetAction::Raise:
			if ( info.ownMoney < money )
				return;
			if ( money < getRule().bigBlind || money % getRule().smallBlind != 0 )
				return;

			betMoney    = info.betMoney[ mBetStep ] + money;
			mPosLastBet = pos;
			needRecount = true;
			break;

		case EBetAction::AllIn:
			betMoney    = slotMoney;
			mPosLastBet = pos;
			++mNumAllIn;

			if ( slotMoney > getMaxBetMoney() && mNumCall )
			{
				needRecount = true;
			}
			break;
		}

		if ( needRecount )
		{
			mNumFinishBet = mNumAllIn;
			mNumCall = 0;
		}
		if ( type != EBetAction::Fold )
			++mNumFinishBet;

		doBet( pos , type , betMoney );

		{
			SCDBetInfo data;
			
			data.pos   = pos;
			data.type  = uint8(type);
			data.step  = mBetStep;
			data.money = betMoney;
			getTransfer().sendData( SLOT_SERVER, data);
		}
		
		if ( mNumBet == 1 )
		{
			int posWin = mPosLastBet;

			if ( posWin == -1 )
			{
				posWin = pos;
				for(;;)
				{
					posWin = nextPlayingPos( posWin );
					if ( getSlotInfo( posWin ).lastAction == EBetAction::None )
						break;
				}
			}

			int totalMoney = 0;
			for( int i = 0 ; i < MaxPlayerNum ; ++ i )
			{
				SlotInfo& info = getSlotInfo( i );
				if ( info.status != ESlotStatus::Playing )
					continue;

				totalMoney += info.totalBetMoney;
			}

			SDWinnerResult data;
			data.numPot = 1;
			data.numWinner[0] = 1;
			for( int i = 1;i < MaxPlayerNum; ++i )
				data.numWinner[i] = 0;
			data.info[0].pos   = pos;
			data.info[0].money = totalMoney;

			doWinMoney( posWin , totalMoney );
			getTransfer().sendData( SLOT_SERVER , data , SDWinnerResult::BaseSize + 1 * sizeof( SDWinnerInfo ) );

			doRoundEnd();
			if( mListener )
				mListener->onRoundEnd();
		}
		else if ( mNumFinishBet == mNumBet )
		{
			if( mNumAllIn == mNumBet )
			{
				showHandCard(BIT(uint32(EBetAction::AllIn)));
			}
			//
			do 
			{
				nextStep();
				if ( getBetStep() == STEP_SHOW_DOWN )
					break;
			}
			while( mNumBet - mNumFinishBet <= 1 );	
		}
		else
		{
			int pos = mPosCurBet;
			for(;;)
			{
				pos = nextPlayingPos( pos );
				if ( getSlotInfo( pos ).lastAction != EBetAction::Fold && 
					 getSlotInfo( pos ).lastAction != EBetAction::AllIn )
					break;
			}

			mPosCurBet = pos;
			SDBetCall data;
			data.pos = pos;
			getTransfer().sendData( SLOT_SERVER , data );
		}
	}

	void ServerLevel::nextStep()
	{
		if ( mBetStep == STEP_RIVER_CARD )
		{
			updatePotPool( true );
			calcRoundResult();
			doRoundEnd();
			if( mListener )
				mListener->onRoundEnd();

			return;
		}

		SDNextStep data;
		BetStep step = BetStep( mBetStep + 1 );
		data.step = step;

		mDecks.pop_back();
		switch( step )
		{
		case STEP_FLOP_CARDS:
			data.card[0] = mDecks.back(); mDecks.pop_back();
			data.card[1] = mDecks.back(); mDecks.pop_back();
			data.card[2] = mDecks.back(); mDecks.pop_back();
			break;
		case STEP_TURN_CARD:
		case STEP_RIVER_CARD:
			data.card[0] = mDecks.back(); mDecks.pop_back();
			break;
		}

		mNumFinishBet = mNumAllIn;
		mNumCall      = 0;
		mPosLastBet   = -1;
		doNextStep( step , data.card );
		getTransfer().sendData( SLOT_SERVER , data );

		if ( mNumBet - mNumFinishBet > 1 )
		{
			mPosCurBet = mPosButton;
			for(;;)
			{
				mPosCurBet = nextPlayingPos( mPosCurBet );
				if ( getSlotInfo( mPosCurBet ).lastAction != EBetAction::Fold && 
					 getSlotInfo( mPosCurBet ).lastAction != EBetAction::AllIn )
					break;
			}

			SDBetCall data;
			data.pos = mPosCurBet;
			getTransfer().sendData( SLOT_SERVER , data );
		}
	}

	struct CardTrickInfo 
	{
		int  pos;
		int  betMoney;
		int  power;
		int  powerOrder;
		int  betOrder;
		int  index[5];
	};

	void ServerLevel::calcRoundResult()
	{

		CardTrickInfo  storage[ MaxPlayerNum ];
		CardTrickInfo* powerSorted[ MaxPlayerNum ];
		int numPlayer = 0;
		
		SDTrickResult trickResult;
		int numTrick = 0;

		int const HandCardNum = CommunityCardNum + PocketCardNum;

		Card cards[ HandCardNum ];
		for ( int i = 0 ; i < CommunityCardNum ; ++i )
			cards[i] = mCommunityCards[i];

		int cmpMoney = -1;
		int cmpMoneyCount = 0;
		for( int pos = 0 ; pos < MaxPlayerNum ; ++pos )
		{
			SlotInfo& info = getSlotInfo( pos );

			if ( info.status != ESlotStatus::Playing )
				continue;

			CardTrickInfo& trickInfo = storage[ numPlayer ];

			powerSorted[numPlayer] = &trickInfo;
			++numPlayer;
			
			trickInfo.pos = pos;
			trickInfo.betMoney = info.totalBetMoney;

			if ( info.lastAction != EBetAction::Fold )
			{
				if ( info.totalBetMoney != cmpMoney )
				{
					cmpMoney = info.totalBetMoney;
					++cmpMoneyCount;
				}
				SlotTrickInfo& slotTrickInfo = trickResult.info[ numTrick ];
				++numTrick;

				for ( int i = 0 ; i < PocketCardNum ; ++i )
				{
					int idx = CommunityCardNum + i;
					cards[ idx ] = mSlotPocketCards[ pos ][ i ];
					slotTrickInfo.card[i] = mSlotPocketCards[ pos ][ i ].getIndex();
				}

				int idxOldMap[ HandCardNum ];
				for( int i = 0 ; i < HandCardNum ; ++i )
					idxOldMap[i] = i;

				std::sort(idxOldMap, idxOldMap + HandCardNum, 
					[&cards](int idx1, int idx2)
					{
						return gFaceRankPower[cards[idx1].getFaceRank()] <
							   gFaceRankPower[cards[idx2].getFaceRank()];
					}
				);

				Card sortedCards[ HandCardNum ];
				for( int i = 0 ; i < HandCardNum ; ++i )
					sortedCards[i] = cards[ idxOldMap[i] ];

				int idxTake[ CardTrickNum ];
				CardTrickHelper helper;
				trickInfo.power = helper.calcPower( sortedCards , HandCardNum , idxTake );
				
				for( int i = 0 ; i < CardTrickNum ; ++ i )
				{
					slotTrickInfo.index[ i ] = trickInfo.index[ i ] = idxOldMap[ idxTake[i] ];
				}
				slotTrickInfo.power = trickInfo.power;
				
			}
			else
			{
				trickInfo.power = 0;
			}
		}

		trickResult.numSlotTrick = numTrick;
		getTransfer().sendData( SLOT_SERVER , trickResult );

		std::sort( powerSorted , powerSorted + numPlayer , 
			[](CardTrickInfo* i1, CardTrickInfo* i2)
			{
				return i1->power > i2->power;
			}
		);

		int order = 0;
		int powerOrderCount[ MaxPlayerNum ];
		int power = powerSorted[0]->power;
		powerSorted[0]->powerOrder = order;
		powerOrderCount[0] = 1;
		
		for ( int i = 1 ; i < numPlayer ; ++i )
		{
			if ( powerSorted[i]->power != power )
			{
				++order;
				powerOrderCount[ order ] = 1;
				power = powerSorted[i]->power;
			}
			else
			{
				powerOrderCount[ order ] += 1;
			}
			powerSorted[i]->powerOrder = order;
		}

		if ( cmpMoneyCount >= 2 )
		{
			CardTrickInfo* betSorted[ MaxPlayerNum ];
			for ( int i = 0 ; i < numPlayer ; ++i )
			{
				betSorted[i] = &storage[i];
			}
			std::sort(betSorted, betSorted + numPlayer, 
				[](CardTrickInfo* i1, CardTrickInfo* i2)
				{
					return i1->betMoney < i2->betMoney;
				}
			);

			int betOrderLast = 0;
			int prevMoney = 0;
			int money = betSorted[0]->betMoney;
			betSorted[0]->betOrder = 0;
			int PotPool[ MaxPlayerNum ];
			int numBetPlayer = numPlayer;
			int numThisBet = 1;
			for( int i = 1 ; i < numPlayer ; ++i )
			{
				CardTrickInfo* info = betSorted[ i ];
				if ( info->betMoney != money )
				{
					PotPool[ betOrderLast ] = ( money - prevMoney ) * numBetPlayer;

					numBetPlayer -= numThisBet;

					++betOrderLast;
					info->betOrder = betOrderLast;
					prevMoney = money;
					money = info->betMoney;
					numThisBet = 1;
				}
				else
				{
					info->betOrder = betOrderLast;
					++numThisBet;
				}
			}

			PotPool[ betOrderLast ] = ( money - prevMoney ) * numBetPlayer;

			SDWinnerResult data;
			int countInfo = 0;
			for( int cb = 0 ; cb <= betOrderLast ; ++cb )
			{
				int pos[ MaxPlayerNum ];
				int winMoney[ MaxPlayerNum ];
				int numWinner = 0;

				int orderCur = 0;
				int idxOrderCur = 0;

				for( ;; )
				{
					for( int i = 0 ; i < powerOrderCount[orderCur] ; ++i )
					{
						int idx = idxOrderCur + i;
						if ( powerSorted[ idx ]->betOrder >= cb )
						{
							pos[ numWinner ] = powerSorted[ idx ]->pos;
							++numWinner;
						}
					}
					if ( numWinner )
						break;

					++orderCur;
					idxOrderCur += powerOrderCount[ orderCur ];
				}

				if ( numWinner != 1 )
				{
					std::sort( pos , pos + numWinner , std::less<int>() );
					divideMoney( PotPool[ cb ] , numWinner , pos, winMoney );
				}
				else
				{
					winMoney[0] = PotPool[cb];
				}
				data.numWinner[ cb ] = numWinner;
				for( int i = 0 ; i < numWinner ; ++i )
				{
					SDWinnerInfo& info = data.info[ countInfo ];
					++countInfo;
					info.pos   = pos[i];
					info.money = winMoney[i];
					doWinMoney( pos[i] , winMoney[i] );
				}
			}

			for( int i = betOrderLast + 1 ; i < MaxPlayerNum ; ++i )
				data.numWinner[ i ] = 0;
			data.numPot = betOrderLast + 1;
		 
			getTransfer().sendData( SLOT_SERVER , data , SDWinnerResult::BaseSize + countInfo * sizeof( SDWinnerInfo ) );
		}
		else
		{
			int totalMoney = 0;
			for( int i = 0 ; i < MaxPlayerNum ; ++i )
			{
				SlotInfo& info = getSlotInfo( i );
				if ( info.status != ESlotStatus::Playing )
					continue;
				totalMoney += info.totalBetMoney;

			}
			int pos[ MaxPlayerNum ];
			for( int i = 0 ; i < powerOrderCount[0] ; ++i )
			{
				pos[i] = powerSorted[i]->pos;
			}
			int winMoney[ MaxPlayerNum ];
			int numWinner = powerOrderCount[0];

			if ( numWinner != 1 )
			{
				std::sort( pos , pos + numWinner , std::less<int>() );
				divideMoney( totalMoney , numWinner , pos , winMoney );
			}
			else
			{
				winMoney[0] = totalMoney;
			}

			SDWinnerResult data;
			data.numPot = 1;
			data.numWinner[0] = numWinner;
			for( int i = 1 ; i < MaxPlayerNum ; ++i )
				data.numWinner[i] = 0;
			for( int i = 0 ; i < numWinner ; ++i )
			{
				data.info[i].pos   = pos[i];
				data.info[i].money = winMoney[i];
				doWinMoney( pos[i] , winMoney[i] );
			}

			getTransfer().sendData( SLOT_SERVER , data , SDWinnerResult::BaseSize + numWinner * sizeof( SDWinnerInfo ) );
		}
	}

	void ServerLevel::divideMoney( int totalMoney , int numWinner , int pos[] , int winMoney[] )
	{
#ifdef _DEBUG
		for( int i = 0 ; i < numWinner - 1 ; ++i )
			assert( pos[i] < pos[i+1] );
#endif
		int avgMoney = totalMoney / numWinner;
		int remainMoney = totalMoney - avgMoney * numWinner;

		for( int i = 0 ; i < numWinner ; ++i )
			winMoney[i] = avgMoney;

		if ( remainMoney != 0 )
		{
			int idxCur = 0;
			int posStart = getButtonPos();
			for( ; idxCur < numWinner ; ++idxCur )
			{
				if ( posStart < pos[ idxCur ] )
					break;
			}
			winMoney[ idxCur ] += remainMoney;
		}
	}

	void ServerLevel::addPlayer( unsigned playerId , int pos , int money )
	{
		SlotInfo& info = getSlotInfo( pos );
		info.playerId  = playerId;
		info.ownMoney  = money;
		info.status    = ESlotStatus::WaitNext;
	}

	ClientLevel::ClientLevel()
	{
		mListener = NULL;
	}

	void ClientLevel::procRecvData( int recvId , int dataId , void* data , int dataSize )
	{
		if ( recvId != SLOT_SERVER )
		{
			return;
		}

		switch( dataId )
		{
		case DATA2ID( SDGameInfo ):
			{
				SDGameInfo* myData = static_cast< SDGameInfo* >( data );
				doGameInit( myData->rule );

				int cur = 0;
				for( int i = 0 ; i < MaxPlayerNum  ; ++i )
				{
					if ( cur >= myData->numPlayer )
						break;

					SlotInfo& slot = getSlotInfo( i );

					if ( myData->info[ cur ].pos == i )
					{
						SDSlotInfo& slotData = myData->info[ cur ];
						++cur;
						if ( slotData.playerId > 0 )
						{
							slot.status    = ESlotStatus::Playing;
							slot.playerId = unsigned( slotData.playerId - 1 );

						}
						else if ( slotData.playerId < 0 )
						{
							slot.status    = ESlotStatus::WaitNext;
							slot.playerId = unsigned( -slotData.playerId - 1 );
						}
						slot.ownMoney = slotData.money;
					}
					else
					{
						slot.status    = ESlotStatus::Empty;
						slot.playerId = -1;
						slot.ownMoney = 0;
					}
				}
			}
			break;
		case DATA2ID( SDRoundInit ):
			{
				SDRoundInit* myData = static_cast< SDRoundInit* >( data );
				
				doInitNewRound();

				for( int i = 0 ; i < MaxPlayerNum ; ++i )
				{
					SlotInfo& info = getSlotInfo( i );

					info.lastAction = EBetAction::None;
					info.ownMoney = myData->ownMoney[i];
					switch ( myData->startBetMoney[i] )
					{
					case -1:
						info.status = ESlotStatus::Empty;
						info.betMoney[ STEP_HOLE_CARDS ] = 0;
						break;
					case -2:
						info.status = ESlotStatus::WaitNext;
						info.betMoney[ STEP_HOLE_CARDS ] = 0;
						break;
					default:
						info.status = ESlotStatus::Playing;
						info.betMoney[ STEP_HOLE_CARDS ] = myData->startBetMoney[i];
						break;
					}
				}

				doNewRound( myData->posButton , myData->posBet );
				
				for( int i = 0; i < PocketCardNum; ++i )
				{
					SlotInfo& slot = getPlayerSlot();
					slot.pocketCards[i] = myData->pocketCard[i];
				}
					
			}
			break;
		case DATA2ID( SDBetCall ):
			{
				SDBetCall* myData = static_cast< SDBetCall* >( data );
				mPosCurBet = myData->pos;
				if ( mListener )
					mListener->onBetCall( myData->pos );
			}
			break;
		case DATA2ID( SCDBetInfo ):
			{
				SCDBetInfo* myData = static_cast< SCDBetInfo* >( data );

				doBet( myData->pos , EBetAction( myData->type ) , myData->money );
				if ( mListener )
					mListener->onBetResult( myData->pos , EBetAction( myData->type ) , myData->money );
			}
			break;
		case DATA2ID( SDNextStep ):
			{
				SDNextStep* myData = static_cast< SDNextStep* >( data );
				doNextStep( BetStep( myData->step ) , myData->card );
				if ( mListener )
					mListener->onNextStep( BetStep( myData->step ) , myData->card );
			}
			break;
		case DATA2ID( SDTrickResult ):
			{
				SDTrickResult* myData = static_cast< SDTrickResult* >( data );

				if ( mListener )
					mListener->onShowDown( myData->info , myData->numSlotTrick );
			}
			break;
		case DATA2ID( SDWinnerResult ):
			{
				SDWinnerResult* myData = static_cast< SDWinnerResult* >( data );

				int countInfo = 0;
				for( int cb = 0; cb < myData->numPot ; ++cb )
				{
					for( int i = 0 ; i < myData->numWinner[ cb ] ; ++i )
					{
						SDWinnerInfo& info = myData->info[ countInfo + i ];
						doWinMoney( info.pos , info.money );
					}
					countInfo += myData->numWinner[ cb ];
				}
				doRoundEnd();
			}
			break;
		case DATA2ID( SDShowPocketCard ):
			{
				SDShowPocketCard* myData = static_cast<SDShowPocketCard*>(data);
				for( int i = 0; i < myData->numPlayer; ++i )
				{
					int pos = myData->pocketCardInfos[i].pos;
					SlotInfo& slot = getSlotInfo(pos);
					
					for( int n = 0 ; n < PocketCardNum ; ++n )
						slot.pocketCards[n] = myData->pocketCardInfos[i].cards[n];
				}

				if( mListener )
					mListener->onShowPocketCard();
			}
			break;
		}
	}

	void ClientLevel::betRequest( EBetAction type , int money )
	{
		if ( mPosCurBet != mPosPlayer )
			return;

		SCDBetInfo data;
		data.pos   = mPosPlayer;
		data.type  = uint8(type);
		data.step  = mBetStep;
		switch( type )
		{
		case EBetAction::Raise:
			data.money = getMaxBetMoney() + money;
			break;
		}
		getTransfer().sendData( SLOT_SERVER ,  data );
	}

}//namespace Holdem

