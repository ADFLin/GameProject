#include "CARGameInput.h"

#include "CARDebug.h"

#include "GameGlobal.h"
#include "PropertyKey.h"

#include "CppVersion.h"

namespace CAR
{
	union ActionHeader 
	{
		struct
		{
			uint8 id;
			uint8 numParam;
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
		mModule = nullptr;
		mNumActionInput = 0;
	}

	void CGameInput::clearReplyAction()
	{
		mAction = ACTION_NONE;
		mActionData = nullptr;
	}

	void CGameInput::run(GameModule& module)
	{
		mModule = &module;
		mImpl = ImplType( std::bind( &CGameInput::execEntry , this , std::placeholders::_1 ) );
	}

	void CGameInput::execEntry(YeildType& yeild )
	{
		try 
		{
			mYeild = &yeild;
			mModule->runLogic( *this );
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
			onPrevAction( *mModule , *this );

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

					doActionCom( com );
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
			onAction( *mModule , *this );
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

		if ( ::Global::getSetting().getIntValue( "AutoSaveGame" , "CAR" , 1 ) )
		{
			saveReplay( ::Global::getSetting().getStringValue( "AutoSaveGameName" , "CAR" , "car_record_temp" ) );
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

		size_t size;
		fs.read( (char*)&size , sizeof( size ) );
		if ( size )
		{
			mRecordAction.resize( size );
			fs.read( mRecordAction.getData() , size );
			mRecordAction.setFillSize( size );
		}

		mbReplayMode = true;
		return true;
	}

	bool CGameInput::saveReplay(char const* file)
	{
		std::ofstream fs( file , std::ios::binary );
		if ( !fs.is_open() )
			return false;

		size_t size = mRecordAction.getFillSize();
		fs.write( (char const*)&size , sizeof( size ) );
		if ( size )
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

	void CGameInput::replySelect(int index)
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

	void CGameInput::replyDoIt()
	{
		assert( mAction == ACTION_BUILD_CASTLE || 
			    mAction == ACTION_TRUN_OVER ||
				mAction == ACTION_BUY_AUCTIONED_TILE );
		ActionCom com;
		replyAction( com );
	}

	void CGameInput::doActionCom( ActionCom const& com  )
	{
		if ( com.bReply && com.action != getReplyAction() )
		{
			return;
		}

		switch( com.action )
		{
		case ACTION_PLACE_TILE:
			{
				if ( com.numParam != 3 )
					return;

				GamePlaceTileData* myData = static_cast< GamePlaceTileData* >( mActionData );
				myData->resultPos.x = com.params[0].iValue;
				myData->resultPos.y = com.params[1].iValue;
				myData->resultRotation = com.params[2].iValue;
			}
			break;
		case ACTION_DEPLOY_ACTOR:
			{
				if ( com.numParam != 2 )
					return;
				GameDeployActorData* myData = static_cast< GameDeployActorData* >( mActionData );
				myData->resultIndex = com.params[0].iValue;
				myData->resultType  = (ActorType) com.params[1].iValue;
			}
			break;
		case ACTION_SELECT_ACTOR:
		case ACTION_SELECT_MAPTILE:
		case ACTION_SELECT_ACTOR_INFO:
			{
				if ( com.numParam != 1 )
					return;

				GameSelectActionData* myData = static_cast< GameSelectActionData* >( mActionData );
				myData->resultIndex = com.params[0].iValue;
			}
			break;
		case ACTION_AUCTION_TILE:
			{
				GameAuctionTileData* myData = static_cast< GameAuctionTileData* >( mActionData );

				if ( com.numParam != ( myData->playerId == myData->pIdRound ? 2 : 1 ) )
					return;

				myData->resultRiseScore = com.params[0].iValue;
				if ( myData->playerId == myData->pIdRound )
				{
					myData->resultIndexTileSelect = com.params[1].iValue;
				}
			}
			break;
		case ACTION_BUILD_CASTLE:
		case ACTION_BUY_AUCTIONED_TILE:
		case ACTION_TRUN_OVER:
			{
				if ( com.numParam != 0 )
					return;
			}
			break;
		case ACTION_BUILD_BRIDGE:
			{
				if ( com.numParam != 3 )
					return;
				Vec2i pos;
				pos.x = com.params[0].iValue;
				pos.y = com.params[1].iValue;
				int dir = com.params[2].iValue;

				if ( mModule->buildBridge( pos , dir ) == false )
					return;
			}
		case ACTION_CHANGE_TILE:
			{
				if ( com.numParam != 1 )
					return;

				TileId id = (TileId)com.params[0].iValue;
				if ( mModule->changePlaceTile( id ) == false )
					return;
			}
			break;
		default:
			return;
		}

		postActionCom( &com );
	}

	void CGameInput::replySkip()
	{
		if ( mDataTransfer )
		{
			SkipCom com;
			com.action = getReplyAction();
			mDataTransfer->sendData( SLOT_SERVER , DATA2ID(SkipCom) , com );
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
				doActionCom( *myData );
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
			com.send( SLOT_SERVER , DATA2ID(ActionCom) , *mDataTransfer );
		}
		else
		{
			doActionCom( com );
		}
	}


	void CGameInput::replyAction( ActionCom& com )
	{
		com.action = getReplyAction();
		com.bReply = true;

		if ( mDataTransfer )
		{
			com.send( SLOT_SERVER , DATA2ID(ActionCom) , *mDataTransfer );
		}
		else
		{
			doActionCom( com );
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
			mImpl();
	}

	void CGameInput::exitGame()
	{
		if ( mActionData )
		{
			mActionData->resultExitGame = true;
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