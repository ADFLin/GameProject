#ifndef CARGameInput_h__4f6924ee_9b2e_4404_b84f_1209a9c6d58e
#define CARGameInput_h__4f6924ee_9b2e_4404_b84f_1209a9c6d58e

#include "CARGameInput.h"

#include "Async/Coroutines.h"
#include "DataStreamBuffer.h"
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
		ACTION_EXCHANGE_ACTOR_POS ,
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

#define ACTION_COM_TYPE_CHECK _DEBUG
	struct ActionCom
	{
		ActionCom()
		{
			numParam = 0;
			bReply   = false;
#if ACTION_COM_TYPE_CHECK
			floatParamMask = 0;
#endif
		}
		void addParam( int value )
		{ 
			assert( numParam < MaxParamNum );
			params[ numParam++ ].iValue = value; 
		}
		void addParam( float value )
		{ 
			assert( numParam < MaxParamNum );
#if ACTION_COM_TYPE_CHECK
			floatParamMask |= BIT(numParam);
#endif
			params[ numParam++ ].fValue = value;
		}

		float getFloat(int index) const 
		{ 
			assert(index < numParam);
#if ACTION_COM_TYPE_CHECK
			assert(floatParamMask & BIT(index));
#endif
			return params[index].fValue;
		}
		int   getInt(int index) const
		{ 
			assert(index < numParam); 
			return params[index].iValue; 
		}
		static int const MaxParamNum = 16;
		uint16 action;
		uint8  numParam : 5;
		uint8  bReply   : 1;
#if ACTION_COM_TYPE_CHECK
		uint16 floatParamMask;
#endif

		int getSendSize() const 
		{
			return sizeof(*this) - (MaxParamNum - numParam) * sizeof(Param);
		}

		union Param
		{
			int   iValue;
			float fValue;
		};
		Param params[MaxParamNum];

	};


#define DATA_LIST( op )\
	op(SkipCom)\
	op(ActionCom)

#define COMMAND_LIST(op)

	DEFINE_DATA2ID( DATA_LIST , COMMAND_LIST)

#undef DATA_LIST
#undef COMMAND_LIST


	class GameInput : public IGameInput
	{
		using ExecType  = boost::coroutines2::asymmetric_coroutine< void >::pull_type;
		using YeildType = boost::coroutines2::asymmetric_coroutine< void >::push_type;

	public:
		GameInput();
		~GameInput()
		{
			delete mGameLogicExecution;
		}

		void runLogic( GameLogic& gameLogic );
		void setRemoteSender( IDataTransfer* transfer );

		void reset();
		

		bool isReplayMode(){ return mbReplayMode; }
		
		PlayerAction    getReplyAction(){ return mAction; }
		GameActionData* getReplyData(){ return mActionData; }
		void clearReplyAction();

		std::function< void ( GameInput& ) > onAction;
		std::function< void ( GameInput& ) > onPrevAction;
		void waitReply();

		void replyPlaceTile( Vec2i const& pos , int rotation );
		void replyDeployActor( int index , EActor::Type type );
		void replySelection( int index );
		void replyAuctionTile( int riseScore , int index = INDEX_NONE );
		void replyActorType(EActor::Type type);

		void replyDoIt();
		void replySkip();
	
		void buildBridge( Vec2i const& pos , int dir );
		void changePlaceTile( TileId id );
		void exitGame();
		void executeGameLogic();

	private:

		void commitActionCom( ActionCom& com, bool bReply = true );
		void handleRecvCommand(int slot, int dataId, void* data, int dataSize);
		bool executeActionCom( ActionCom const& com );
		void executeSkipAction();
	public:


	private:
		void requestActionImpl(PlayerAction action, GameActionData& data);
#define REQUEST_ACTION( FUNC , DATA , DATA_ID )\
	virtual void FUNC( DATA& data ) override {  requestActionImpl( DATA_ID , data ); }

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
		REQUEST_ACTION( requestExchangeActorPos, GameExchangeActorPosData, ACTION_EXCHANGE_ACTOR_POS );

#undef REQUEST_ACTION


		void executeAction( ActionCom const* com );
		void LogicExecutionEntry( YeildType& yeild );
	public:

		bool loadReplay( char const* file );
		bool saveReplay( char const* file );


		std::string      mAutoSavePath;
		//Networking
		IDataTransfer*   mRemoteSender;
		//Replay
		DataStreamBuffer mRecordAction;
		bool             mbReplayMode;

		bool             mbWaitReply;
		int              mNumActionInput;
		GameLogic*       mGameLogic;
		PlayerAction     mAction;
		GameActionData*  mActionData;
		ExecType*        mGameLogicExecution = nullptr;
		YeildType*       mInputExecution;
	};






}//namespace CAR

#endif // CARGameInput_h__4f6924ee_9b2e_4404_b84f_1209a9c6d58e
