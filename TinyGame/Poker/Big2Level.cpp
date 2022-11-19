#include "TinyGamePCH.h"
#include "Big2Level.h"

namespace Big2 {

#define DATA_LIST( op )\
	op(SDGameInit )\
	op(SDRoundInit)\
	op(CDShowCard)\
	op(SDSlotTurn)\
	op(SDNextCycle)\
	op(SDRoundEnd)

#define COMMON_ID_LIST(op)\
	op(DATA_SDShowFail)\
	op(DATA_CDPlayerPass)\
	op(DATA_SDGameOver)

DEFINE_DATA2ID( DATA_LIST , COMMON_ID_LIST )


#undef DATA_LIST
#undef COMMON_ID_LIST

	bool LevelBase::checkShowCard( CardDeck& cards , int pIndex[] , int num ,  TrickInfo& info )
	{
		if ( !FTrick::CheckCard( &cards[0] , (int)cards.size() , pIndex , num , info ) )
			return false;

		info.num   = num;
		info.power = FTrick::CalcPower( info.group , info.card );

		if ( mLastShowSlot == -1 )
		{
			if ( mCycleCount == 0 )
			{
				int idx = 0;
				for( idx = 0 ; idx < info.num ; ++idx )
				{
					if ( info.card[idx] == Card( Card::eCLUBS , Card::eN3 ) )
						break;
				}
				if ( idx == info.num )
					return false;
			}
		}
		else if ( !FTrick::CanSuppress( info , mLastShowCard ) )
		{
			return false;
		}

		return true;
	}

	void LevelBase::doNextCycle( int slotId )
	{
		++mCycleCount;
		mNextShowSlot = slotId;
		mLastShowSlot = INDEX_NONE;
		for( int i = 0 ; i < PlayerNum ; ++i )
			mSlotStatus[i].bPassed = false;
	}

	void LevelBase::doRoundInit( int slotId )
	{
		mCycleCount   = 0;
		mNextShowSlot = slotId ;
		mLastShowSlot = INDEX_NONE;
		for( int i = 0 ; i < PlayerNum ; ++i )
			mSlotStatus[i].bPassed = false;
	}

	void LevelBase::doSlotTurn( int slotId , bool beShow )
	{
		if ( beShow )
		{
			mLastShowSlot = slotId;
		}
		else
		{
			mSlotStatus[ slotId ].bPassed = true;
		}

		for( int i = 0 ; i < 4 ; ++i )
		{
			mNextShowSlot += 1;
			if ( mNextShowSlot == PlayerNum )
				mNextShowSlot = 0;
			if ( !mSlotStatus[ mNextShowSlot ].bPassed )
				break;
		}
	}

	void LevelBase::doRoundEnd()
	{
		mNextShowSlot = INDEX_NONE;
	}

	void LevelBase::sortCards( CardDeck& cards )
	{
		std::sort( cards.begin() , cards.end() , CardSortCmp() );
	}

	int LevelBase::calcScore( CardDeck& cards , uint32& reason )
	{
		int numBig2 = 0;

		TrickHelper helper;
		helper.parse( &cards[0]  , (int)cards.size() );

		numBig2 = helper.getFaceNum( Card::ToRank( Card::eN2 ) );

		int factor = 1;
		for( int i = 0 ; i < numBig2 ; ++i )
		{
			factor *= 2;
			reason |= ( SR_HAVE_1_BIG_2 << i );
		}

		if ( cards.size() >= 10 )
		{
			factor *= 2;
			reason |= SR_OVER_DOUBLE_NUM;
		}
		if ( helper.haveGroup( CG_FOUR_OF_KIND ) )
		{
			factor *= 2;
			reason |= SR_HAVE_FOUR_OF_KINDS;
		}
		if ( helper.haveGroup( CG_STRAIGHT_FLUSH ) )
		{
			factor *= 2;
			reason |= SR_HAVE_STRAIGHT_FLUSH;
		}
		return factor * (int)cards.size();
	}

	ServerLevel::ServerLevel() 
	{
		for( int i = 0 ; i < PlayerNum ; ++i )
		{
			mSlotStatus[i].slotId = i;
			mBot[i] = NULL;
		}
		mListener = NULL;
	}

	void ServerLevel::restrat( IRandom& random , bool beInit )
	{
		SDGameInit sData;
		for( int i = 0 ; i < PlayerNum ; ++i )
		{
			sData.playerId[i] = mSlotStatus[i].playerId;
			mSlotStatus[i].money = 500;
		}
		sData.startMoney  = 500;

		getTransfer().sendData( SLOT_SERVER , sData );
		if ( mListener )
			mListener->onGameInit();

		startNewRound( random );
	}

	void ServerLevel::startNewRound( IRandom& random )
	{
		for( int i = 0 ; i < PlayerNum ; ++i )
			getSlotOwnCards(i).clear();

		int cardIndex[ 52 ];
		for( int i = 0 ; i < 52 ; ++i )
			cardIndex[i] = i;
		for( int i = 0 ; i < 52 ; ++i )
			std::swap( cardIndex[i] , cardIndex[ random.getInt() % 52 ] );

		int idxClubs2 = Card::ToIndex( Card::eCLUBS , Card::eN3 );
		int startSlot = 0;
		for( int i = 0 ; i < 52 ; ++i )
		{
			int slot = i % PlayerNum;
			if ( cardIndex[i] == idxClubs2 )
			{
				startSlot  = slot;
			}
			getSlotOwnCards( slot ).push_back( Card( cardIndex[i] ) );
		}

		mPassCount = 0;
		doRoundInit( startSlot );

		SDRoundInit data;
		data.startSlot = startSlot;
		for( int i = 0; i < PlayerNum ; ++i )
		{
			CardDeck& cards = getSlotOwnCards( i );
			for( int n = 0 ; n < PlayerCardNum ; ++n )
			{
				data.card[n] = cards[n].getIndex();
			}
			getTransfer().sendData( i , data );
		}

		for( int i = 0 ; i < PlayerNum ; ++i )
		{
			sortCards( getSlotOwnCards( i ) );
		}

		if ( mListener )
			mListener->onRoundInit();

		updateBot();
	}


	bool ServerLevel::procSlotShowCard( int slotId , int pIndex[] , int numCard )
	{
		if ( getNextShowSlot() != slotId )
			return false;

		TrickInfo info;
		if ( !checkShowCard( getSlotOwnCards( slotId ) , pIndex , numCard , info ) )
		{
			getTransfer().sendData( slotId , DATA_SDShowFail , NULL , 0 );
			return false;
		}

		SDSlotTurn sData;
		sData.slotId = slotId;
		sData.group = info.group;
		sData.power = info.power;
		sData.num   = info.num;
		for( int i = 0 ; i < info.num ; ++i )
		{
			sData.card[i] = info.card[i].getIndex();
		}
		CardDeck& cards = getSlotOwnCards( slotId );
		RemoveCards( cards , pIndex , numCard );

		mLastShowCard = info;

		doSlotTurn( slotId , true );

		getTransfer().sendData( SLOT_SERVER , sData );
		if ( mListener )
			mListener->onSlotTurn( slotId , true );

		if ( cards.size() != 0 )
			return true;

		doRoundEnd();

		int factor = 1;
		uint32 reason = 0;
		switch ( mLastShowCard.group )
		{
		case CG_ONE_PAIR:
		case CG_STRAIGHT:
		case CG_SINGLE:
			if ( mLastShowCard.card[0].getFace() == Card::eN2 )
			{
				factor *= 2;
				reason |= SR_LAST_SHOW_BIG_2;
			}
			break;
		case CG_FULL_HOUSE:
			if ( mLastShowCard.card[4].getFace() == Card::eN2 )
			{
				factor *= 2;
				reason |= SR_LAST_SHOW_BIG_2;
			}
			break;
		case CG_FOUR_OF_KIND:
			factor *= 2;
			reason |= SR_LAST_FOUR_OF_KIND;
			if ( mLastShowCard.card[4].getFace() == Card::eN2 )
			{
				factor *= 2;
				reason |= SR_LAST_SHOW_BIG_2;
			}
			break;
		case CG_STRAIGHT_FLUSH:
			factor *= 2;
			reason |= SR_LAST_STRAIGHT_FLUSH;
			if ( mLastShowCard.card[0].getFace() == Card::eN2 )
			{
				factor *= 2;
				reason |= SR_LAST_SHOW_BIG_2;
			}
			break;
		}


		SDRoundEnd endData;
		endData.winner = slotId;
		endData.reason[slotId] = reason;
		int idxCard = 0;

		bool isGameOver = false;
		endData.score[ slotId ] = 0;
		for( int id = 0 ; id < PlayerNum ; ++id )
		{
			CardDeck& otherCards = getSlotOwnCards( id );

			if ( id != slotId )
			{
				auto& playerData = endData.players[id];
				playerData.reason = 0;
				int score = factor * calcScore( otherCards , endData.reason[id] );
				int decMoney = std::min( score , mSlotStatus[id].money );

				mSlotStatus[id].money -= decMoney;
				playerData.money = mSlotStatus[id].money;
				playerData.score = score;
				if ( mSlotStatus[id].money == 0 )
				{
					isGameOver = true;
				}

				mSlotStatus[slotId].money += decMoney;
				endData.score[ slotId ] += endData.score[id];
			}

			endData.numCard[ id ] = (int)otherCards.size();
			for( int n = 0 ; n < (int)endData.numCard[ id ] ; ++n )
			{
				endData.card[ idxCard++ ] = otherCards[ n ].getIndex();
			}
		}
		endData.isGameOver = isGameOver;
		endData.players[ slotId ].money = mSlotStatus[ slotId ].money; 
		getTransfer().sendData( SLOT_SERVER , endData );

		if ( mListener )
			mListener->onRoundEnd( isGameOver );

		if ( isGameOver )
		{
			getTransfer().sendData( SLOT_SERVER , DATA_SDGameOver , 0 , 0 );
		}

		return true;
	}

	bool ServerLevel::procSlotPass( int slotId )
	{
		if ( getNextShowSlot() != slotId )
			return false;

		if ( getLastShowSlot() == INDEX_NONE )
			return false;

		mPassCount += 1;
		doSlotTurn( slotId , false );

		SDSlotTurn sData;
		sData.slotId = slotId;
		sData.num    = 0;
		getTransfer().sendData( SLOT_SERVER , sData );
		if ( mListener )
			mListener->onSlotTurn( slotId , false );

		if ( mPassCount == PlayerNum - 1 )
		{
			int startSlot = getLastShowSlot();
			mPassCount = 0;

			doNextCycle( startSlot );

			SDNextCycle sData;
			sData.slotId = startSlot;
			getTransfer().sendData( SLOT_SERVER , sData );

			if ( mListener )
				mListener->onNewCycle();
		}
		return true;
	}

	void ServerLevel::handleRecvData( int slotId , int dataId , void* data , int dataSize)
	{
		switch( dataId )
		{
		case DATA_CDPlayerPass:
			if ( procSlotPass( slotId ) )
			{
				updateBot();
			}
			break;
		case DATA2ID( CDShowCard ):
			CDShowCard* myData = static_cast< CDShowCard* >( data );
			if ( procSlotShowCard( slotId , myData->index , myData->num ) )
			{
				updateBot();
			}
			break;
		}
	}

	void ServerLevel::doSetupTransfer()
	{
		getTransfer().setRecvFunc( RecvFunc( this , &ServerLevel::handleRecvData ) );
	}

	void ServerLevel::updateBot()
	{
		do 
		{
			int slot = getNextShowSlot();
			if ( slot == INDEX_NONE )
				break;

			if ( !updateBotImpl( slot ) )
				break;

		} while ( 1 );
	}

	bool ServerLevel::updateBotImpl( int slotId )
	{
		IBot* bot = mBot[ slotId ];
		if (  !bot )
			return false;

		int loopCount = 0;
		while( 1 )
		{
			int index[5];
			int num = bot->think( getLastShowSlot() , getLastShowCards() , index );

			bool isOk;
			if ( num == 0 )
				isOk = procSlotPass( slotId );
			else
				isOk = procSlotShowCard( slotId , index , num );

			bot->postThink( isOk , index , num );

			if ( isOk )
				break;
			if ( loopCount > 10 )
			{
				LogMsg("Poker Bot Error");
				return false;
			}
		}

		return true;
	}

	ClientLevel::ClientLevel() 
	{
		
	}

	void ClientLevel::restart( bool beInit )
	{

	}

	void ClientLevel::passCard()
	{
		if ( getNextShowSlot() != getPlayerStatus().slotId )
			return;

		getTransfer().sendData( SLOT_SERVER , DATA_CDPlayerPass , NULL , 0 );
	}

	bool ClientLevel::showCard( int idxSelect[] , int num )
	{
		if ( getNextShowSlot() != getPlayerStatus().slotId )
			return false;

		 TrickInfo info;
		if ( !checkShowCard( getOwnCards() , idxSelect , num , info ) )
			return false;

		CDShowCard sData;
		sData.num = num;
		for( int i = 0 ; i < num ; ++i )
		{
			sData.index[i] = idxSelect[i];
			mLastShowIndex[i] = idxSelect[i];
		}
		getTransfer().sendData( SLOT_SERVER , sData );

		return true;
	}

	void ClientLevel::handleRecvData( int slotId , int dataId , void* data , int dataSize )
	{
		switch( dataId )
		{
		case DATA2ID( SDGameInit ):
			{
				SDGameInit* myData = static_cast< SDGameInit* >( data );
				for( int i = 0 ; i < PlayerNum ; ++i )
				{
					SlotStatus& status = getSlotStatus( i );
					status.slotId    = i;
					status.playerId  = myData->playerId[i];
					status.money     = myData->startMoney;
				}
				mListener->onGameInit();
			}
			break;
		case DATA2ID( SDRoundInit ):
			{
				SDRoundInit* myData = static_cast< SDRoundInit* >( data );

				CardDeck& ownCards = getOwnCards();
				ownCards.clear();
				for( int i = 0 ; i < 13 ; ++i )
				{
					ownCards.push_back( Card( myData->card[i] ) );
				}

				for( int i = 0 ; i < PlayerNum ; ++i )
				{
					SlotStatus& status = getSlotStatus( i );
					mSlotCardNum[i] = PlayerCardNum;
				}
				doRoundInit( myData->startSlot );

				sortCards( mOwnCards );

				mListener->onRoundInit( *myData );
			}
			break;
		case DATA2ID( SDSlotTurn ):
			{
				SDSlotTurn* myData = static_cast< SDSlotTurn* >( data );

				bool beShow = myData->num != 0;

				if ( beShow )
				{
					mSlotCardNum[ myData->slotId ] -= myData->num;
					if ( myData->slotId == getPlayerStatus().slotId )
					{
						RemoveCards( getOwnCards() , mLastShowIndex , myData->num );
					}
					
					mLastShowCard.power = myData->power;
					mLastShowCard.group = CardGroup( myData->group );
					mLastShowCard.num   = myData->num;
					for( int i= 0 ; i < (int)myData->num ; ++i )
					{
						mLastShowCard.card[i] = Card( myData->card[i] );
					}
				}

				doSlotTurn( myData->slotId , beShow );

				mListener->onSlotTurn( myData->slotId , beShow );
			}
			break;
		case DATA2ID( SDNextCycle ):
			{
				SDNextCycle* myData = static_cast< SDNextCycle* >( data );
				doNextCycle( myData->slotId );
				mListener->onNewCycle();
			}
			break;
		case DATA2ID( SDRoundEnd ):
			{
				SDRoundEnd* myData = static_cast< SDRoundEnd* >( data );

				for( int i = 0 ; i < PlayerNum ; ++i )
				{
					SlotStatus& status = getSlotStatus( i );
					status.money = myData->players[i].money;
				}
				doRoundEnd();
				mListener->onRoundEnd( *myData );
			}
			break;
		case DATA_SDGameOver:
			{
				mListener->onGameOver();
			}
			break;
		}
	}


	void ClientLevel::doSetupTransfer()
	{
		getTransfer().setRecvFunc( RecvFunc( this , &ClientLevel::handleRecvData ) );
	}

	TablePos ClientLevel::getTablePos( int slotId )
	{
		int result = slotId - getPlayerStatus().slotId;
		if ( result < 0 )
			result += PlayerNum;
		return TablePos( result );
	}

	int ClientLevel::getSlotId( TablePos pos )
	{
		return ( pos + getPlayerStatus().slotId ) % PlayerNum;
	}


}//namespace Big2