#ifndef HoldemLevel_h__
#define HoldemLevel_h__

#include "PokerBase.h"
#include "DataTransfer.h"
#include "DataStructure/FixedArray.h"

namespace Holdem {

	using namespace Poker;

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

	enum class EBetAction : uint8
	{
		None  ,
		Fold  , //±óµP  
		Call  , //¸òµP
		Raise , //¥[ª`
		AllIn ,
	};

	enum class ESlotStatus : uint8
	{
		Empty ,
		WaitNext ,
		Playing  ,
	};

	struct SlotInfo
	{
		SlotInfo()
		{
			status    = ESlotStatus::Empty;
			playerId = -1;
			lastAction  = EBetAction::None;
			ownMoney = 0;
		}
		int         pos;
		ESlotStatus status;
		unsigned    playerId;
		EBetAction  lastAction;
		int         ownMoney;
		int         betMoney[ NUM_BET_STEP ];
		int         totalBetMoney;
		int         betMoneyOrder;
		char        pocketCards[2];
	};


	class LevelBase
	{
	public:
		LevelBase();

		IDataTransfer& getTransfer(){ return *mTransfer; }
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

		bool        isPlaying() { return mBetStep >= 0; }

	protected:

		virtual void procRecvData( int recvId , int dataId , void* data , int dataSize ) = 0;


		void      doGameInit( Rule const& rule );
		void      doInitNewRound();
		void      doNewRound( int posButton , int posBet );
		void      doNextStep( BetStep step , char cards[] );
		void      doBet( int pos , EBetAction type , int money );
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
		ServerLevel();

		class Listener
		{
		public:
			virtual void onBetCall( int slot ){}
			virtual void onBetResult( int slot , EBetAction type , int money ){}
			virtual void onShowDown( SlotTrickInfo info[] , int num ){}
			virtual void onRoundEnd(){}
			virtual void onPlayerLessBetMoney(int slot) {}
		};

		void setListener(Listener* listener) { mListener = listener; }
		void startNewRound( IRandom& random );
		void setupGame( Rule const& rule );
		void addPlayer( unsigned playerId , int pos , int money );
		void removePlayer( int pos );

		void procBetRequest( int pos , EBetAction type , int money );

	private:
		void showHandCard( uint32 betTypeMask );
		void shuffle( IRandom& random );
		void nextStep();
		void calcRoundResult();

		void divideMoney( int totalMoney , int numWinner  , int pos[] , int winMoney[] );

		void procRecvData( int recvId , int dataId , void* data , int dataSize );

		Listener* mListener;
		TFixedArray< char , 52 > mDecks;
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
			virtual void onBetResult( int slot , EBetAction type , int money ){}
			virtual void onShowDown( SlotTrickInfo info[] , int num ){}
			virtual void onShowPocketCard(){}
		};

		void          setListener( Listener* listener ){ mListener = listener; }
		void          setPlayerPos( int pos ){ mPosPlayer = pos; }
		int           getPlayerPos(){ return mPosPlayer; }

		SlotInfo&     getPlayerSlot() { return getSlotInfo(mPosPlayer); }

		void          procRecvData( int recvId , int dataId , void* data , int dataSize );
		void          betRequest( EBetAction type , int money = 0 );
		Card          getPoketCard( int idx ){ return Card( getPlayerSlot().pocketCards[ idx ] ); }


	private:
		int        mPosPlayer;
		Listener*  mListener;
	};

}//namespace Holdem

#endif // HoldemLevel_h__
