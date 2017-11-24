#ifndef Big2Level_h__
#define Big2Level_h__

#include "PokerBase.h"
#include "Big2Utility.h"

#include "FixVector.h"
#include "Core/IntegerType.h"
#include "DataTransfer.h"

#include <algorithm>
#include <functional>

namespace Poker { namespace Big2 {

	int const PlayerNum = 4;
	int const MaxPlayerCardNum = 26;
	int const PlayerCardNum = 13;

	typedef FixVector< Card , 13 > CardDeck;
	inline void removeCards( CardDeck& cards , int* pIndex , int num )
	{
		int temp[ 52 ];
		std::copy( pIndex , pIndex + num , temp );
		std::sort( temp , temp + num , std::greater< int >() );
		for ( int i = 0 ; i < num ; ++i )
		{
			cards.erase( cards.begin() + temp[i] );
		}
	}

	struct SlotStatus
	{
		int      slotId;
		unsigned playerId;
		int      money;
		int      score;
		bool     bePass;
	};


	struct CDShowCard
	{
		int num;
		int index[5];
	};

	struct SDSlotTurn
	{
		char slotId;
		char card[5];
		int  power;
		char num;
		char group;
	};

	struct SDNextCycle
	{
		int slotId;
	};

	struct SDRoundEnd
	{
		bool   isGameOver;
		char   winner;
		int32  money[4];
		int32  score[4];
		uint32 reason[4];
		uint8  numCard[4];
		uint8  card[52];
	};

	enum ScoreReason
	{
		SR_OVER_DOUBLE_NUM     = BIT(0) ,
		SR_HAVE_1_BIG_2        = BIT(1) ,
		SR_HAVE_2_BIG_2        = BIT(2) ,
		SR_HAVE_3_BIG_2        = BIT(3) ,
		SR_HAVE_4_BIG_2        = BIT(4) ,
		SR_HAVE_FOUR_OF_KINDS  = BIT(5) ,
		SR_HAVE_STRAIGHT_FLUSH = BIT(6) ,

		SR_LAST_SHOW_BIG_2     = BIT(7) ,
		SR_LAST_FOUR_OF_KIND  = BIT(8) ,
		SR_LAST_STRAIGHT_FLUSH = BIT(9) ,
	};


	struct SDGameInit
	{
		unsigned playerId[ 4 ];
		short    startMoney;
	};

	struct SDRoundInit
	{
		char startSlot;
		char card[13];
	};

	class LevelBase
	{
	public:
		TrickInfo const* getLastShowCards(){ return ( mLastShowSlot != -1 )? &mLastShowCard : NULL; }
		int              getLastShowSlot(){ return mLastShowSlot; }
		int              getNextShowSlot(){ return mNextShowSlot; }
		bool             checkShowCard( CardDeck& cards , int pIndex[] , int num , TrickInfo& info );

		void             setupTransfer( IDataTransfer* transfer ){ assert( transfer ); mTransfer = transfer; doSetupTransfer(); }
		IDataTransfer*   getTransfer(){ return mTransfer; }

		SlotStatus&      getSlotStatus( int slotId ){ return mSlotStatus[ slotId ]; }
	protected:
		virtual void     doSetupTransfer(){}
		void             doNextCycle( int slotId );
		void             doRoundInit( int slotId );
		void             doSlotTurn( int slotId , bool beShow );
		void             doRoundEnd();

		static void      sortCards( CardDeck& cards );
		static int       calcScore( CardDeck& cards , uint32& reason );
		TrickInfo       mLastShowCard;
		SlotStatus      mSlotStatus[PlayerNum];
	private:

		
		IDataTransfer*  mTransfer;
		int             mNextShowSlot;
		int             mCycleCount;
		int             mLastShowSlot;
		
	};


	class IBot
	{
	public:
		virtual ~IBot(){}
		virtual void  init( int slotId , CardDeck& cards ) = 0;
		virtual int   think( int lastShowId , TrickInfo const* lastTrick , int index[] ) = 0;
		virtual void  postThink( bool beOk , int index[] , int num ) = 0;
		virtual void  onSlotShowCard( int slotId , TrickInfo const* info ) = 0;
	};

	class ServerLevel : public LevelBase
	{
	public:

		ServerLevel();

		class Listener
		{
		public:
			virtual void onGameInit(){}
			virtual void onRoundInit(){}
			virtual void onSlotTurn( int slotId , bool beShow ){}
			virtual void onNewCycle(){}
			virtual void onRoundEnd( bool isGameOver ){}
			virtual void onGameOver(){}
			
		};
		void        restrat( IRandom& random , bool beInit );
		void        startNewRound( IRandom& random );
		bool        procSlotShowCard( int slotId , int* pIndex , int numCard );
		bool        procSlotPass( int slotId );


		void        setEventListener( Listener* listener ){ mListener = listener; }
		CardDeck&   getSlotOwnCards( int slotId ){ return mSlotOwnCards[ slotId ]; }
		void        setSlotBot( int slotId , IBot* bot ){ mBot[ slotId ] = bot;  }
		IBot*       getSlotBot( int slotId ){ return mBot[ slotId ];  }


	private:
		
		void        onRecvData( int slotId , int dataId , void* data , int dataSize );
		void        doSetupTransfer();
		void        updateBot();
		bool        updateBotImpl( int slotId );

	private:
		Listener*  mListener;
		CardDeck   mSlotOwnCards[ PlayerNum ];
		IBot*      mBot[ PlayerNum ];
		int        mPassCount;
	};

	class CBot : public IBot
	{
	public:
		virtual void  init( int slotId , CardDeck& cards )
		{

		}
		virtual int   think( int lastShowId , TrickInfo const* lastTrick , int index[] )
		{
			if ( lastTrick )
			{
				
				return 0;
			}

			index[0] = 0;
			return 1;
		}
		virtual void  postThink( bool beOk ,int index[] , int num )
		{

		}
		virtual void  onSlotShowCard( int slotId , TrickInfo const* info ){}

		TrickHelper mHelper;
	};

	enum TablePos
	{
		TPOS_ME    = 0,
		TPOS_RIGHT ,
		TPOS_FRONT ,
		TPOS_LEFT  ,
	};

	class ClientLevel : public LevelBase
	{
	public:
		ClientLevel();

		class Listener
		{
		public:
			virtual void onGameInit(){}
			virtual void onRoundInit( SDRoundInit const& initData ){}
			virtual void onSlotTurn( int slotId , bool beShow ){}
			virtual void onNewCycle(){}
			virtual void onRoundEnd( SDRoundEnd const& endData ){}
			virtual void onGameOver(){}
		};

		void          restart( bool beInit );
		void          passCard();
		bool          showCard( int idxSelect[] , int num );

		void          setPlayerSlotId( int id ){ mPlayerSlotId = id; }
		void          setListener( Listener& listener ){  mListener = &listener;  }


		CardDeck&     getOwnCards(){ return mOwnCards; }
		SlotStatus&   getPlayerStatus(){ return mSlotStatus[ mPlayerSlotId ]; }
		int           getSlotCardNum( int slotId ){ return mSlotCardNum[ slotId ]; }

		TablePos      getTablePos( int slotId );
		int           getSlotId( TablePos pos );

		void          onRecvData( int playerId , int dataId , void* data , int dataSize );


		void          doSetupTransfer();
		
	private:
		Listener*      mListener;
	
		int            mSlotCardNum[PlayerNum];
		int            mLastShowIndex[5];
		SlotStatus     mUser;
		CardDeck       mOwnCards;
		int            mPlayerSlotId;
		
	};


}//namespace Big2
}//namespace Poker


#endif // Big2Level_h__
