#ifndef HoldemLevel_h__
#define HoldemLevel_h__

#include "PokerBase.h"
#include "IDataTransfer.h"
#include "FixVector.h"

namespace Poker { namespace Holdem {

	int const PocketCardNum = 2;
	int const CommunityCardNum = 5;
	int const MaxPlayerNum = 9;
	int const SpectatorPos  = -1;
	int const CardTrickNum = 5;

	enum CardGroup
	{
		CG_HIGH_HAND ,
		CG_PAIR ,
		CG_TWO_PAIRS ,
		CG_THREE_OF_A_KIND ,
		CG_STRAIGHT ,
		CG_FLUSH ,
		CG_FULL_HOUSE ,
		CG_FOUR_OF_A_KIND ,
		CG_STRAIGHT_FLUSH ,
		CG_ROYAL_FLUSH ,
	};
	struct Rule
	{	
		int bigBlind;
		int smallBlind;
	};

	struct SlotTrickInfo
	{
		int  power;
		char pos;
		char card[ PocketCardNum ];
		char index[ 5 ];
	};

	enum BetStep
	{
		STEP_SHOW_DOWN = -2,
		STEP_NO_BET = -1 ,
		STEP_HOLE_CARDS = 0,
		STEP_FLOP_CARDS ,
		STEP_TURN_CARD ,
		STEP_RIVER_CARD ,
		NUM_BET_STEP ,
	};

	enum BetType
	{
		BET_NONE  ,
		BET_FOLD  , //��P  
		BET_CALL  , //��P
		BET_RAISE , //�[�`
		BET_ALL_IN,
	};

	enum SlotState
	{
		SLOT_EMPTY ,
		SLOT_WAIT_NEXT ,
		SLOT_PLAY  ,
	};

	struct SlotInfo
	{
		SlotInfo()
		{
			state    = SLOT_EMPTY;
			playerId = -1;
			betType  = BET_NONE;
			ownMoney = 0;
		}
		int       pos;
		SlotState state;
		unsigned  playerId;
		BetType   betType;
		int       ownMoney;
		int       betMoney[ NUM_BET_STEP ];
		int       totalBetMoney;
		int       betMoneyOrder;
	};


	class LevelBase
	{
	public:
		LevelBase();

		IDataTransfer* getTransfer(){ return mTransfer; }
		void           setupTransfer( IDataTransfer* transfer );

		Rule const& getRule() const { return mRule; }
		int         getCommunityCardNum();
		Card const& getCommunityCard( int idx ){ return mCommunityCards[idx]; }

		SlotInfo&   getSlotInfo( int pos ){ return mSlotInfoVec[ pos ]; }
		int         getButtonPos() const { return mPosButton; }

		int         nextPlayingPos( int pos );
		int         prevPlayingPos( int pos );
		int         getMaxBetMoney(){ return mMaxBetMoney; }

		BetStep     getBetStep(){ return mBetStep; }
		int         getCurBetPos(){ return mPosCurBet; }

		int         getPotNum(){ return mIdxPotLast + 1; }
		int         getPotMoney( int idx ){ return mPotPool[ idx ]; }

	protected:

		virtual void procRecvData( int recvId , int dataId , void* data ) = 0;


		void      doGameInit( Rule const& rule );
		void      doNewRound( int posButton , int posBet );
		void      doNextStep( BetStep step , char cards[] );
		void      doBet( int pos , BetType type , int money );
		void      doWinMoney( int pos , int money );
		void      doRoundEnd();

		void      updatePotPool( bool beEnd );


		Rule       mRule;
		Card       mCommunityCards[ CommunityCardNum ];
		BetStep    mBetStep;
		int        mPosCurBet;
		int        mPosButton;
		int        mMaxBetMoney;

		int        mPotPool[ MaxPlayerNum ];
		int        mIdxPotLast;
		int        mPotBetMoney;

	private:

		SlotInfo       mSlotInfoVec[ MaxPlayerNum ];
		IDataTransfer* mTransfer;
	};

	class ServerLevel : public LevelBase
	{
	public:

		class Listener
		{
		public:
			virtual void onBetCall( int slot ){}
			virtual void onBetResult( int slot , BetType type , int money ){}
			virtual void onShowDown( SlotTrickInfo info[] , int num ){}
		};

		void startNewRound( IRandom& random );
		void setupGame( Rule const& rule );
		void addPlayer( unsigned playerId , int pos , int money );
		void removePlayer( int pos );

		void procBetRequest( int pos , BetType type , int money );

	private:

		void shuffle( IRandom& random );
		void nextStep();
		void calcRoundResult();

		void divideMoney( int totalMoney , int numWinner  , int pos[] , int winMoney[] );

		void procRecvData( int recvId , int dataId , void* data );

		Listener* mListener;
		FixVector< char , 52 > mDecks;
		int   mNumBet;
		int   mNumFinishBet;
		int   mNumCall;
		int   mNumAllIn;

		int   mPosLastBet;
		Card  mSlotPocketCards[ MaxPlayerNum ][ PocketCardNum ];

	};


	class ClientLevel : public LevelBase
	{
	public:
		ClientLevel();

		class Listener
		{
		public:
			virtual void onBetCall( int slot ){}
			virtual void onNextStep( BetStep step , char card[] ){}
			virtual void onBetResult( int slot , BetType type , int money ){}
			virtual void onShowDown( SlotTrickInfo info[] , int num ){}
		};

		void          setListener( Listener* listener ){ mListener = listener; }
		void          setPlayerPos( int pos ){ mPosPlayer = pos; }
		int           getPlayerPos(){ return mPosPlayer; }

		void          procRecvData( int recvId , int dataId , void* data );
		void          betRequest( BetType type , int money = 0 );
		Card const&   getPoketCard( int idx ){ return mPocketCards[ idx ]; }

	private:

		Card       mPocketCards[ PocketCardNum ];
		int        mPosPlayer;
		Listener*  mListener;
	};

}//namespace Holdem
}//namespace Poker

#endif // HoldemLevel_h__
