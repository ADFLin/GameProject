#ifndef CARGameInput_h__4f6924ee_9b2e_4404_b84f_1209a9c6d58e
#define CARGameInput_h__4f6924ee_9b2e_4404_b84f_1209a9c6d58e

#include "CARGameInput.h"

#include "Coroutine.h"
#include "DataSteamBuffer.h"
#include "DataTransfer.h"


namespace CAR
{
	enum PlayerAction
	{
		ACTION_NONE ,
		ACTION_PLACE_TILE ,
		ACTION_DEPLOY_ACTOR ,
		ACTION_SELECT_MAPTILE ,
		ACTION_SELECT_ACTOR ,
		ACTION_SELECT_ACTOR_INFO ,
		ACTION_SELECT_MAP_POS ,
		ACTION_SELECT_ACTION_OPTION ,
		ACTION_AUCTION_TILE ,
		ACTION_BUY_AUCTIONED_TILE ,
		ACTION_BUILD_CASTLE ,
		///
		ACTION_BUILD_BRIDGE ,
		ACTION_TRUN_OVER ,
		ACTION_CHANGE_TILE ,
		ACTION_PLACE_TARGET_TILE ,
	};

	class IInptActionCommand;

	struct SkipCom
	{
		uint8 action;
	};

	struct ActionCom
	{
		ActionCom()
		{
			numParam = 0;
		}
		void addParam( int value ){ assert( numParam < MaxParamNum ); params[ numParam++ ].iValue = value; }
		void addParam( float value ){ assert( numParam < MaxParamNum ); params[ numParam++ ].fValue = value; }
		void send( IDataTransfer& transfer , int slot , int dataId )
		{
			transfer.sendData( slot , dataId , this , sizeof( ActionCom ) - ( MaxParamNum - numParam ) * sizeof( Param ) );
		}
		static int const MaxParamNum = 16;
		uint16 action;
		uint8  numParam;
		uint8  bReply : 1;
		union Param
		{
			int   iValue;
			float fValue;
		};
		Param params[ MaxParamNum ];

		int getSendSize() const 
		{
			return sizeof(*this) - (MaxParamNum - numParam) * sizeof(Param);
		}
	};


#define DATA_LIST( op )\
	op(SkipCom)\
	op(ActionCom)

#define COMMAND_LIST(op)

	DEFINE_DATA2ID( DATA_LIST , COMMAND_LIST)

#undef DATA_LIST
#undef COMMAND_LIST


	class CGameInput : public IGameInput
	{
		typedef boost::coroutines::asymmetric_coroutine< void >::pull_type ExecType;
		typedef boost::coroutines::asymmetric_coroutine< void >::push_type YeildType;

	public:
		CGameInput();

		void runLogic( GameLogic& gameLogic );
		void setDataTransfer( IDataTransfer* transfer );

		void reset();
		

		bool isReplayMode(){ return mbReplayMode; }
		
		PlayerAction    getReplyAction(){ return mAction; }
		GameActionData* getReplyData(){ return mActionData; }
		void clearReplyAction();

		std::function< void ( GameLogic& , CGameInput& ) > onAction;
		std::function< void ( GameLogic& , CGameInput& ) > onPrevAction;
		void waitReply();

		void replyPlaceTile( Vec2i const& pos , int rotation );
		void replyDeployActor( int index , ActorType type );
		void replySelect( int index );
		void replyAuctionTile( int riseScore , int index = -1 );

		void replyDoIt();
		void replySkip();
	
		void buildBridge( Vec2i const& pos , int dir );
		void changePlaceTile( TileId id );
		void exitGame();
		void returnGame();

	private:

		void replyAction( ActionCom& com );
		void applyAction( ActionCom& com );
		void onRecvCommon( int slot , int dataId , void* data , int dataSize );
		bool executeActionCom( ActionCom const& com );
		void doSkip();
	public:

	private:

#define REQUEST_ACTION( FUN , DATA , DATA_ID )\
	virtual void FUN( DATA& data ) override {  requestActionImpl( DATA_ID , data ); }

		REQUEST_ACTION( requestPlaceTile , GamePlaceTileData , ACTION_PLACE_TILE );
		REQUEST_ACTION( requestDeployActor , GameDeployActorData , ACTION_DEPLOY_ACTOR );
		REQUEST_ACTION( requestSelectActor , GameSelectActorData , ACTION_SELECT_ACTOR );
		REQUEST_ACTION( requestSelectActorInfo , GameSelectActorInfoData , ACTION_SELECT_ACTOR_INFO );
		REQUEST_ACTION( requestSelectMapTile , GameSelectMapTileData , ACTION_SELECT_MAPTILE );
		REQUEST_ACTION( requestAuctionTile , GameAuctionTileData , ACTION_AUCTION_TILE );
		REQUEST_ACTION( requestBuyAuctionedTile , GameAuctionTileData , ACTION_BUY_AUCTIONED_TILE );
		REQUEST_ACTION( requestBuildCastle , GameBuildCastleData ,  ACTION_BUILD_CASTLE );
		REQUEST_ACTION( requestTurnOver , GameActionData , ACTION_TRUN_OVER );
		REQUEST_ACTION( requestSelectMapPos , GameSelectMapPosData , ACTION_SELECT_MAP_POS );
		REQUEST_ACTION( requestSelectActionOption , GameSelectActionOptionData , ACTION_SELECT_ACTION_OPTION );


#undef REQUEST_ACTION



		void requestActionImpl( PlayerAction action , GameActionData& data );

		void postActionCom( ActionCom const* com );
		void execEntry( YeildType& yeild );
	public:

		bool loadReplay( char const* file );
		bool saveReplay( char const* file );

		//Networking
		IDataTransfer*   mDataTransfer;
		//Replay
		DataSteamBuffer  mRecordAction;
		bool             mbReplayMode;

		bool             mbWaitReply;
		int              mNumActionInput;
		GameLogic*       mGameLogic;
		PlayerAction     mAction;
		GameActionData*  mActionData;
		ExecType         mExec;
		YeildType*       mYeild;
	};






}//namespace CAR

#endif // CARGameInput_h__4f6924ee_9b2e_4404_b84f_1209a9c6d58e
