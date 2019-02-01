#include "CARGameInputImpl.h"

#include "CARDebug.h"
#include "CARGameLogic.h"

#include "GameGlobal.h"
#include "PropertyKey.h"

#include "CppVersion.h"

namespace CAR
{

#define CAR_REPLAY_MAGIC MAKE_MAGIC_ID('C','A','R','R')

	struct ReplayHeader
	{
		uint32 magic;
		uint32 version;
		uint32 dataSize;
	};

	namespace ReplayVerion
	{
		enum
		{
			Init,

			LastVersionPlusOne,
			LastVersion = LastVersionPlusOne - 1,
		};
	}

	union ActionHeader 
	{
		struct
		{
			uint8  id;
			uint16 numParam : 5;
			uint16 reply : 1;
			uint16 skip  : 1;
			uint16 noInput: 1;
		};
		uint32 value;
	};

	static_assert( sizeof(ActionHeader) == sizeof( uint32 ) , "Action Header Size Check Fail");

	CGameInput::CGameInput()
	{
		reset();
	}

	void CGameInput::reset()
	{
		mbReplayMode = false;
		mAction = ACTION_NONE;
		mActionData = nullptr;
		mDataTransfer = nullptr;
		mGameLogic = nullptr;
		mNumActionInput = 0;
	}

	void CGameInput::clearReplyAction()
	{
		mAction = ACTION_NONE;
		mActionData = nullptr;
	}

	void CGameInput::runLogic(GameLogic& gameLogic)
	{
		mGameLogic = &gameLogic;
		mExec = ExecType( std::bind( &CGameInput::execEntry , this , std::placeholders::_1 ) );
	}

	void CGameInput::execEntry(YeildType& yeild )
	{
		try 
		{
			mYeild = &yeild;
			mGameLogic->run( *this );
			mYeild = nullptr;
		} 
		catch(const boost::coroutines::detail::forced_unwind&) 
		{
			CAR_LOG("Catch forced_unwind Exception!");
			throw;
		} 
		catch(...) 
		{
			// possibly not re-throw pending exception
		}
	}


	void CGameInput::requestActionImpl( PlayerAction action , GameActionData& data )
	{
		mAction = action;
		mActionData = &data;

		if ( mbReplayMode && mRecordAction.getAvailableSize() == 0 )
		{
			mbReplayMode = false;
		}

		if ( onPrevAction )
			onPrevAction( *mGameLogic , *this );

		if ( mbReplayMode )
		{
			size_t lastUsePos;
			try
			{
				ActionHeader header;
				do
				{
					lastUsePos = mRecordAction.getUseSize();
					mRecordAction.take( header.value );

					if ( header.reply && header.id != action )
					{
						CAR_LOG( "Record Buffer Error Format" );
						mRecordAction.setFillSize( lastUsePos );
						mbReplayMode = false;
					}

					if ( header.noInput )
					{
						break;
					}
					if ( header.skip )
					{
						doSkip();
						break;
					}

					ActionCom com;
					com.action = header.id;
					com.bReply = header.reply;
					com.numParam = header.numParam;
					for( int i = 0 ; i < com.numParam ; ++i )
						mRecordAction.take( com.params[i] );

					if( ! executeActionCom(com) )
					{
						CAR_LOG("Record Buffer Error Format");
						mRecordAction.setFillSize(lastUsePos);
						mbReplayMode = false;
					}
				}
				while( header.reply == false );
			}
			catch ( BufferException& e )
			{
				CAR_LOG( "Record Buffer Error : %s" , e.what() );
				mRecordAction.setFillSize( lastUsePos );
				mbReplayMode = false;
			}
		}
		
		if ( mbReplayMode == false )
		{
			mbWaitReply = false;
			onAction( *mGameLogic , *this );
			if ( mbWaitReply == false )
			{
				ActionHeader header;
				header.value = 0;
				header.reply  = 1;
				header.noInput = 1;
				header.id     = action;
				mRecordAction.fill( header.value );
			}
		}
	}



	void CGameInput::waitReply()
	{
		assert( mYeild );
		mbWaitReply = true;
		(*mYeild)();
	}

	void CGameInput::postActionCom( ActionCom const* com )
	{

		++mNumActionInput;

		if ( mbReplayMode == false )
		{
			ActionHeader header;
			header.value = 0;
			if ( com == nullptr )
			{
				header.reply = 1;
				header.skip  = 1;
				header.id    = getReplyAction();
				mRecordAction.fill( header.value );
			}
			else
			{
				header.reply = com->bReply;
				header.numParam = com->numParam;
				header.id = com->action;
				mRecordAction.fill( header.value );
				for( int i = 0 ; i < com->numParam ; ++i )
				{
					mRecordAction.fill( com->params[i] );
				}
			}
		}

		if ( ::Global::GameConfig().getIntValue( "AutoSaveGame" , "CAR" , 1 ) )
		{
			saveReplay( ::Global::GameConfig().getStringValue( "AutoSaveGameName" , "CAR" , "car_record_temp" ) );
		}

		if ( com == nullptr || com->bReply == true )
		{
			clearReplyAction();
			if ( mbReplayMode == false )
			{
				returnGame();
			}
		}
	}

	bool CGameInput::loadReplay(char const* file)
	{
		std::ifstream  fs;
		fs.open( file , std::ios::in | std::ios::binary );
		if ( !fs.is_open() )
			return false;

		ReplayHeader header;
		fs.read( (char*)&header, sizeof(header) );
		if( !fs )
			return false;

		if( header.magic != CAR_REPLAY_MAGIC )
			return false;

		if ( header.dataSize )
		{
			mRecordAction.resize(header.dataSize);
			fs.read( mRecordAction.getData() , header.dataSize );
			mRecordAction.setFillSize(header.dataSize);
		}

		mbReplayMode = true;
		return true;
	}

	bool CGameInput::saveReplay(char const* file)
	{
		std::ofstream fs( file , std::ios::binary );
		if ( !fs.is_open() )
			return false;

		ReplayHeader header;
		header.magic = CAR_REPLAY_MAGIC;
		header.version = ReplayVerion::LastVersion;
		header.dataSize = mRecordAction.getFillSize();
		fs.write((char const*) &header , sizeof(header));
		if ( header.dataSize )
		{
			fs.write( mRecordAction.getData() , mRecordAction.getFillSize() );
		}

		return true;
	}

	void CGameInput::replyPlaceTile(Vec2i const& pos , int rotation)
	{
		assert( mAction == ACTION_PLACE_TILE );

		ActionCom com;
		com.addParam( pos.x );
		com.addParam( pos.y );
		com.addParam( rotation );
		replyAction( com );
	}


	void CGameInput::replyDeployActor(int index , ActorType type)
	{
		assert( mAction == ACTION_DEPLOY_ACTOR );

		ActionCom com;
		com.addParam( index );
		com.addParam( (int)type );
		replyAction( com );
	}

	void CGameInput::replySelection(int index)
	{
		assert( mAction == ACTION_SELECT_ACTOR || 
			mAction == ACTION_SELECT_MAPTILE ||
			mAction == ACTION_SELECT_ACTOR_INFO );

		ActionCom com;
		com.addParam( index );
		replyAction( com );
	}

	void CGameInput::replyAuctionTile( int riseScore , int index /*= -1 */)
	{
		assert( mAction == ACTION_AUCTION_TILE );
		ActionCom com;
		com.addParam( riseScore );
		if ( index != -1 )
			com.addParam( index );
		replyAction( com );
	}

	void CGameInput::replyActorType(ActorType type)
	{
		assert(mAction == ACTION_EXCHANGE_ACTOR_POS);
		ActionCom com;
		com.addParam((int)type);
		replyAction(com);
	}

	void CGameInput::replyDoIt()
	{
		assert( mAction == ACTION_BUILD_CASTLE || 
			    mAction == ACTION_TRUN_OVER ||
				mAction == ACTION_BUY_AUCTIONED_TILE );
		ActionCom com;
		replyAction( com );
	}

	bool CGameInput::executeActionCom( ActionCom const& com  )
	{
		if ( com.bReply && com.action != getReplyAction() )
		{
			return false;
		}

		switch( com.action )
		{
		case ACTION_PLACE_TILE:
			{
				if ( com.numParam != 3 )
					return false;

				GamePlaceTileData* myData = static_cast< GamePlaceTileData* >( mActionData );
				myData->resultPos.x = com.getInt(0);
				myData->resultPos.y = com.getInt(1);
				myData->resultRotation = com.getInt(2);
			}
			break;
		case ACTION_DEPLOY_ACTOR:
			{
				if ( com.numParam != 2 )
					return false;

				GameDeployActorData* myData = static_cast< GameDeployActorData* >( mActionData );
				myData->resultIndex = com.getInt(0);
				myData->resultType  = (ActorType) com.getInt(1);
			}
			break;
		case ACTION_SELECT_ACTOR:
		case ACTION_SELECT_MAPTILE:
		case ACTION_SELECT_ACTOR_INFO:
			{
				if ( com.numParam != 1 )
					return false;

				GameSelectActionData* myData = static_cast< GameSelectActionData* >( mActionData );
				myData->resultIndex = com.getInt(0);
			}
			break;
		case ACTION_AUCTION_TILE:
			{
				GameAuctionTileData* myData = static_cast< GameAuctionTileData* >( mActionData );

				if ( com.numParam != ( myData->playerId == myData->pIdRound ? 2 : 1 ) )
					return false;

				myData->resultRiseScore = com.getInt(0);
				if ( myData->playerId == myData->pIdRound )
				{
					myData->resultIndexTileSelect = com.getInt(1);
				}
			}
			break;
		case ACTION_EXCHANGE_ACTOR_POS:
			{
				if( com.numParam != 1 )
					return false;

				GameExchangeActorPosData* myData = static_cast<GameExchangeActorPosData*>(mActionData);
				myData->resultActorType = (ActorType)com.getInt(0);
			}
			break;
		case ACTION_BUILD_CASTLE:
		case ACTION_BUY_AUCTIONED_TILE:
		case ACTION_TRUN_OVER:
			{
				if ( com.numParam != 0 )
					return false;
			}
			break;
		case ACTION_BUILD_BRIDGE:
			{
				if ( com.numParam != 3 )
					return false;
				Vec2i pos;
				pos.x = com.getInt(0);
				pos.y = com.getInt(1);
				int dir = com.getInt(2);

				if ( mGameLogic->buildBridge( pos , dir ) == false )
					return false;
			}
			break;
		case ACTION_CHANGE_TILE:
			{
				if ( com.numParam != 1 )
					return false;

				TileId id = (TileId)com.getInt(0);
				if ( mGameLogic->changePlaceTile( id ) == false )
					return false;
			}
			break;

		default:
			return false;
		}

		postActionCom( &com );
		return true;
	}

	void CGameInput::replySkip()
	{
		if ( mDataTransfer )
		{
			SkipCom com;
			com.action = getReplyAction();
			TRANSFER_SEND( *mDataTransfer ,  SLOT_SERVER , com );
		}
		else
		{
			doSkip();
		}
	}

	void CGameInput::doSkip()
	{
		if ( mActionData )
		{
			mActionData->resultSkipAction = true;
			postActionCom( nullptr );
		}
	}

	void CGameInput::setDataTransfer(IDataTransfer* transfer)
	{
		mDataTransfer = transfer;
		mDataTransfer->setRecvFun( RecvFun( this , &CGameInput::onRecvCommon ) );
	}

	void CGameInput::onRecvCommon(int slot , int dataId , void* data , int dataSize)
	{
		switch( dataId )
		{
		case DATA2ID( ActionCom ):
			{
				ActionCom* myData = static_cast< ActionCom* >( data );
				executeActionCom( *myData );
			}
			break;
		case DATA2ID( SkipCom ):
			{
				SkipCom* myData = static_cast< SkipCom* >( data );
				if ( myData->action == getReplyAction() )
				{
					doSkip();
				}
			}
			break;
		}
	}

	void CGameInput::applyAction( ActionCom& com )
	{
		com.bReply = false;
		if ( mDataTransfer )
		{
			TRANSFER_SEND( *mDataTransfer , SLOT_SERVER , com );
		}
		else
		{
			executeActionCom( com );
		}
	}


	void CGameInput::replyAction( ActionCom& com )
	{
		com.action = getReplyAction();
		com.bReply = true;

		if ( mDataTransfer )
		{
			TRANSFER_SEND(*mDataTransfer, SLOT_SERVER , com );
		}
		else
		{
			executeActionCom( com );
		}
	}

	void CGameInput::buildBridge(Vec2i const& pos , int dir)
	{
		ActionCom com;
		com.action = ACTION_BUILD_BRIDGE;
		com.addParam( pos.x );
		com.addParam( pos.y );
		com.addParam( dir );
		applyAction( com );
	}

	void CGameInput::returnGame()
	{
		if ( mYeild )
			mExec();
	}

	void CGameInput::exitGame()
	{
		if ( mActionData )
		{
			mGameLogic->mbNeedShutdown = true;
			returnGame();
		}
	}

	void CGameInput::changePlaceTile(TileId id)
	{
		ActionCom com;
		com.action = ACTION_CHANGE_TILE;
		com.addParam( (int)id );
		applyAction( com );
	}

}//namespace CAR