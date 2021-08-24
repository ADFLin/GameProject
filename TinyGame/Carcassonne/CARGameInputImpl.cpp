#include "CARGameInputImpl.h"

#include "CARDebug.h"
#include "CARGameLogic.h"

#include "CppVersion.h"

#include <fstream>

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
		mInputExecution = nullptr;
		reset();
	}

	void CGameInput::reset()
	{
		assert(mInputExecution == nullptr);
		mbReplayMode = false;
		mAction = ACTION_NONE;
		mActionData = nullptr;
		mRemoteSender = nullptr;
		mGameLogic = nullptr;
		mNumActionInput = 0;
		mRecordAction.clear();
	}

	void CGameInput::clearReplyAction()
	{
		mAction = ACTION_NONE;
		mActionData = nullptr;
	}

	void CGameInput::runLogic(GameLogic& gameLogic)
	{
		mGameLogic = &gameLogic;
		mGameLogicExecution = ExecType( std::bind( &CGameInput::LogicExecutionEntry, this , std::placeholders::_1 ) );
	}

	void CGameInput::LogicExecutionEntry(YeildType& yeild )
	{
		try 
		{
			mInputExecution = &yeild;
			mGameLogic->run( *this );
			mInputExecution = nullptr;
			mGameLogic = nullptr;
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
			onPrevAction( *this );

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
						executeSkipAction();
						break;
					}

					ActionCom com;
					com.action = header.id;
					com.bReply = header.reply;
					com.numParam = header.numParam;
					for( int i = 0 ; i < com.numParam ; ++i )
						mRecordAction.take( com.params[i] );

					if( !executeActionCom(com) )
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
			onAction( *this );
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
		assert( mInputExecution );
		mbWaitReply = true;
		(*mInputExecution)();
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

				auto* myData = static_cast< GamePlaceTileData* >( mActionData );
				myData->resultPos.x = com.getInt(0);
				myData->resultPos.y = com.getInt(1);
				myData->resultRotation = com.getInt(2);
			}
			break;
		case ACTION_DEPLOY_ACTOR:
			{
				if ( com.numParam != 2 )
					return false;

				auto* myData = static_cast< GameDeployActorData* >( mActionData );
				myData->resultIndex = com.getInt(0);
				myData->resultType  = (EActor::Type) com.getInt(1);
			}
			break;
		case ACTION_SELECT_ACTOR:
		case ACTION_SELECT_MAPTILE:
		case ACTION_SELECT_ACTOR_INFO:
			{
				if ( com.numParam != 1 )
					return false;

				auto* myData = static_cast< GameSelectActionData* >( mActionData );
				myData->resultIndex = com.getInt(0);
			}
			break;
		case ACTION_AUCTION_TILE:
			{
				auto* myData = static_cast< GameAuctionTileData* >( mActionData );

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

				auto* myData = static_cast<GameExchangeActorPosData*>(mActionData);
				myData->resultActorType = (EActor::Type)com.getInt(0);
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

				auto id = (TileId)com.getInt(0);
				if ( mGameLogic->changePlaceTile( id ) == false )
					return false;
			}
			break;

		default:
			return false;
		}

		executeAction( &com );
		return true;
	}

	void CGameInput::executeSkipAction()
	{
		if ( mActionData )
		{
			mActionData->bSkipActionForResult = true;
			executeAction( nullptr );
		}
	}

	void CGameInput::executeAction( ActionCom const* com )
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

		if (!mAutoSavePath.empty())
		{
			saveReplay(mAutoSavePath.c_str());
		}

		if ( com == nullptr || com->bReply == true )
		{
			clearReplyAction();
			if ( mbReplayMode == false )
			{
				executeGameLogic();
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
		commitActionCom( com );
	}


	void CGameInput::replyDeployActor(int index , EActor::Type type)
	{
		assert( mAction == ACTION_DEPLOY_ACTOR );

		ActionCom com;
		com.addParam( index );
		com.addParam( (int)type );
		commitActionCom( com );
	}

	void CGameInput::replySelection(int index)
	{
		assert( mAction == ACTION_SELECT_ACTOR || 
			mAction == ACTION_SELECT_MAPTILE ||
			mAction == ACTION_SELECT_ACTOR_INFO );

		ActionCom com;
		com.addParam( index );
		commitActionCom( com );
	}

	void CGameInput::replyAuctionTile( int riseScore , int index /*= -1 */)
	{
		assert( mAction == ACTION_AUCTION_TILE );
		ActionCom com;
		com.addParam( riseScore );
		if ( index != INDEX_NONE )
			com.addParam( index );
		commitActionCom( com );
	}

	void CGameInput::replyActorType(EActor::Type type)
	{
		assert(mAction == ACTION_EXCHANGE_ACTOR_POS);
		ActionCom com;
		com.addParam((int)type);
		commitActionCom(com);
	}

	void CGameInput::replyDoIt()
	{
		assert( mAction == ACTION_BUILD_CASTLE || 
			    mAction == ACTION_TRUN_OVER ||
				mAction == ACTION_BUY_AUCTIONED_TILE );
		ActionCom com;
		commitActionCom( com );
	}

	void CGameInput::replySkip()
	{
		if ( mRemoteSender )
		{
			SkipCom com;
			com.action = getReplyAction();
			mRemoteSender->sendData( SLOT_SERVER , com );
		}
		else
		{
			executeSkipAction();
		}
	}

	void CGameInput::setRemoteSender(IDataTransfer* transfer)
	{
		mRemoteSender = transfer;
		if (mRemoteSender)
		{
			mRemoteSender->setRecvFunc(RecvFunc(this, &CGameInput::handleRecvCommond));
		}
	}

	void CGameInput::handleRecvCommond(int slot , int dataId , void* data , int dataSize)
	{
		switch( dataId )
		{
		case DATA2ID( ActionCom ):
			{
				auto myData = static_cast< ActionCom* >( data );
				executeActionCom( *myData );
			}
			break;
		case DATA2ID( SkipCom ):
			{
				auto myData = static_cast< SkipCom* >( data );
				if ( myData->action == getReplyAction() )
				{
					executeSkipAction();
				}
			}
			break;
		}
	}

	void CGameInput::commitActionCom( ActionCom& com , bool bReply )
	{	
		com.bReply = bReply;
		if (bReply)
		{
			com.action = getReplyAction();
		}

		if ( mRemoteSender )
		{
			mRemoteSender->sendData( SLOT_SERVER , com );
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
		commitActionCom(com, false);
	}

	void CGameInput::executeGameLogic()
	{
		if (mInputExecution)
		{
			mGameLogicExecution();
		}
	}

	void CGameInput::exitGame()
	{
		if ( mActionData )
		{
			mGameLogic->mbNeedShutdown = true;
			executeGameLogic();
		}
	}

	void CGameInput::changePlaceTile(TileId id)
	{
		ActionCom com;
		com.action = ACTION_CHANGE_TILE;
		com.addParam( (int)id );
		commitActionCom( com , false );
	}

}//namespace CAR