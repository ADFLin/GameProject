#include "TinyGamePCH.h"
#include "CSyncFrameManager.h"

#include "GameServer.h"
#include "GameClient.h"
#include "GameAction.h"
#include "GameNetPacket.h"

#define USE_UDP_FRAME_DATA 1
#define RESET_DATA_FRAME (-1)
int const UseChannel = CHANNEL_GAME_NET_UDP_CHAIN;


FrameDataManager::FrameDataManager()
{
	mCurFrame = 0;
	mLastDataFrame = 0;
}

void FrameDataManager::addFrameData(int32 frame , DataStreamBuffer& buffer )
{
	if ( mLastDataFrame < frame )
		mLastDataFrame = frame;

	mDataList.push_front( std::move(buffer) );

	FrameData fd;
	fd.frame = frame;
	fd.iter  = mDataList.begin();
	mDataQueue.push( fd );
}

bool FrameDataManager::canAdvanceFrame()
{
	if ( mDataQueue.empty() )
		return false;

	FrameData const& data = mDataQueue.top();

	if ( data.frame == getFrame() + 1 )
		return true;

	return false;
}


void FrameDataManager::beginFrame()
{
	++mCurFrame;

	do
	{
		FrameData const& data =  mDataQueue.top();

		if ( data.frame > getFrame() )
			break;

		mProcessData.push_back( data );
		mDataQueue.pop();
	}
	while( !mDataQueue.empty() );
}


void FrameDataManager::restoreData( IFrameActionTemplate* actionTemp  )
{
	for( FrameDataVec::iterator iter = mProcessData.begin();
		iter != mProcessData.end() ; ++iter )
	{
		auto& buffer = *iter->iter;
		auto dataSteam = MakeBufferSerializer(*(iter->iter));
		actionTemp->restoreFrameData( dataSteam , buffer.getAvailableSize() != 0);

	}

	actionTemp->debugMessage(mCurFrame);
}

void FrameDataManager::endFrame()
{
	for( FrameDataVec::iterator iter = mProcessData.begin();
		iter != mProcessData.end() ; ++iter )
	{
		mDataList.erase( iter->iter );
	}
	mProcessData.clear();
}

CSyncFrameManager::CSyncFrameManager( IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector) 
{
	mActionTemplate = actionTemp;
	mFrameCollector = frameCollector;
	mProcessor.setLanucher( this );

	mFrameCollector->setupAction(mProcessor);
	bClearData = false;
	
}

bool CSyncFrameManager::scanInput( bool beUpdateFrame )
{
	if ( !beUpdateFrame )
		return false;
	if ( !mFrameMgr.haveFrameData() )
		return false;

	mFrameMgr.restoreData( mActionTemplate );
	return true;
}

bool CSyncFrameManager::checkAction( ActionParam& param )
{
	return mActionTemplate->checkAction( param );
}

int CSyncFrameManager::evalFrame( IFrameUpdater& updater , int updateFrames , int maxDelayFrames )
{
	if( bClearData )
	{
		bClearData = false;
		return 0;
	}

	int frameCount = 0;

	int deltaFrame = mFrameMgr.getLastDataFrame() - mFrameMgr.getFrame();
	if (  deltaFrame > maxDelayFrames + updateFrames )
		++updateFrames;

	sendFrameData();

	if ( mFrameMgr.canAdvanceFrame() )
	{
		do
		{
			mFrameMgr.beginFrame();
			updater.tick();
			mFrameMgr.endFrame();

			++frameCount;

			if ( frameCount >= updateFrames )
				break;

			sendFrameData();
		}
		while ( mFrameMgr.canAdvanceFrame() );
	}

	updater.updateFrame( frameCount );
	return mFrameMgr.getFrame();
}


SVSyncFrameManager::SVSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector ) 
	:CSyncFrameManager( actionTemp , frameCollector )
	,mFrameStream( new GDPFrameStream )
{
	assert( worker->isServer() );
	mWorker = static_cast< ServerWorker* >( worker );

	mWorker->setNetListener( this );
	mWorker->setComListener( frameCollector );
	mWorker->getEvaluator().setUserFun< GDPFrameStream >( this , &SVSyncFrameManager::procFrameData );

	frameCollector->reflashPlayer( *mWorker->getPlayerManager() );

	mCheckDataBits = 0;
	mLocalDataBits = 0;
	mUpdateDataBit = 0;

	for( auto iter = worker->getPlayerManager()->createIterator(); iter ; ++iter )
	{
		ServerPlayer* player = static_cast< ServerPlayer*>( iter.getElement() );

		mCheckDataBits |= BIT( player->getId() );
		if ( !player->isNetwork() )
			mLocalDataBits |= BIT( player->getId() );

		player->lastUpdateFrame = 0;
		player->lastRecvFrame   = 0;
		player->lastSendFrame   = 0;
	}

	mUpdateDataBit = mCheckDataBits;
	mCountDataDelay = 0;
}

SVSyncFrameManager::~SVSyncFrameManager()
{
	mWorker->getEvaluator().removeProcesserFun( this );
	mWorker->setNetListener( NULL );
	mWorker->setComListener( NULL );
}

bool SVSyncFrameManager::sendFrameData()
{
	for( ClientFrameDataList::iterator iter = mFrameDataList.begin();
		 iter != mFrameDataList.end() ; )
	{
		//LogDevMsg(0, "Process Player Frame Data Start" );

		ClientFrameData& data = *iter;

		ServerPlayer* player = mWorker->getPlayerManager()->getPlayer( data.id );

		//Preserve null when Player disconnect 
		if( player != nullptr )
		{
			if( mFrameMgr.getFrame() - data.recvFrame < 10 )
			{
				if( (BIT(data.id) & mUpdateDataBit) &&
				   data.recvFrame >= player->lastRecvFrame &&
				   data.sendFrame > player->lastSendFrame )
				{
					++iter;
					continue;
				}
			}

			bool isOk = true;
			if( data.buffer.getAvailableSize() )
			{
				try
				{
					
					mFrameCollector->processClientFrameData(data.id, data.buffer);
				}
				catch( BufferException& )
				{
					isOk = false;
				}
			}

			if( isOk )
			{
				mUpdateDataBit |= BIT(data.id);
				player->lastRecvFrame = data.recvFrame;
				player->lastSendFrame = data.sendFrame;
			}
		}

		iter = mFrameDataList.erase( iter );
	}

	mUpdateDataBit |= mLocalDataBits;

	mProcessor.beginAction( CTF_BLOCK_ACTION );

	int const MaxWaitDiffFrame = 30;
	if ( mCheckDataBits != mUpdateDataBit )
	{
		bool bNeedFreezeFrame = false;

		for( auto iter = mWorker->getPlayerManager()->createIterator(); iter; ++iter )
		{
			ServerPlayer* player = static_cast< ServerPlayer*>( iter.getElement() );

			if ( BIT( player->getId() ) & mLocalDataBits )
				continue;

			if ( (int)mFrameMgr.getFrame() - player->lastUpdateFrame > MaxWaitDiffFrame  )
			{
				//LogMsg( "Data Delay %d (%d %d)" , mCountDataDelay , player->lastUpdateFrame ,(int) mFrameMgr.getFrame() );
				bNeedFreezeFrame = true;
				break;
			}
		}

		if( bNeedFreezeFrame )
		{
#if DEBUG_SHOW_FRAME_DATA_TRANSITION
			LogDevMsg(0, "Freeze Frame !!!");
#endif
			mUpdateDataBit = 0;
			++mCountDataDelay;
			mProcessor.endAction();
			return false;
		}
	}

	mCountDataDelay = 0;

	int32 frame = mFrameMgr.getFrame() + 1;
	mFrameStream->buffer.clear();
	mFrameStream->frame = frame;

	auto dataSteam = MakeBufferSerializer(mFrameStream->buffer);
	if( mFrameCollector->haveFrameData(frame) )
	{
		mFrameCollector->collectFrameData(dataSteam);
	}
#if DEBUG_SHOW_FRAME_DATA_TRANSITION
	//LogDevMsg( 0 ,"Send Frame Data : frame = %u" , frame);
#endif
	mWorker->sendCommand( UseChannel , mFrameStream.get() , WSF_IGNORE_LOCAL );

	DataStreamBuffer buffer;
	buffer.copy( mFrameStream->buffer );

	mFrameMgr.addFrameData( mFrameStream->frame , buffer );
	mUpdateDataBit = 0;

	mProcessor.endAction();
	return true;
}

void SVSyncFrameManager::procFrameData( IComPacket* cp )
{
	if( bClearData )
		return;

	NetClientData* info = static_cast< NetClientData* >( cp->getUserData() );
	if ( !info )
	{
		return;
	}


	PlayerId id = info->ownerId;
	GDPFrameStream* fp = cp->cast< GDPFrameStream >();
	ServerPlayer* player = mWorker->getPlayerManager()->getPlayer( id );

#if 1 || DEBUG_SHOW_FRAME_DATA_TRANSITION
	//if( fp->buffer.getAvailableSize() )
	{
		LogDevMsg(0, "==Recv Frame Data : %d %d %d==", (int)mFrameMgr.getFrame(), (int)fp->frame, (int)fp->buffer.getAvailableSize());
	}
	
#endif

	int maxDiscardDifFrame = 5;
	if( player->lastUpdateFrame >= fp->frame + maxDiscardDifFrame ||
	    fp->frame >= player->lastUpdateFrame + maxDiscardDifFrame )
	{
		return;
	}
	static int count = 0;
	if ( fp->buffer.getAvailableSize() && count <= 10  )
	{
		count += 1;
	}

	ClientFrameData data;
	data.id = id;
	data.recvFrame = mFrameMgr.getFrame();
	data.sendFrame = fp->frame;
	data.buffer    = std::move(fp->buffer);
	mFrameDataList.push_back(std::move(data));

	if ( player->lastUpdateFrame < fp->frame )
		player->lastUpdateFrame = fp->frame;


}

unsigned SVSyncFrameManager::calcLocalPlayerBit()
{
	unsigned result = 0;
	for( auto iter = mWorker->getPlayerManager()->createIterator(); iter; ++iter )
	{
		ServerPlayer* sPlayer = static_cast< ServerPlayer* >( iter.getElement() );
		if ( !sPlayer->isNetwork() )
			result |= BIT( sPlayer->getId() );
	}
	return result;
}

void SVSyncFrameManager::refreshPlayerState()
{
	mLocalDataBits = 0;
	mCheckDataBits = 0;
	for( auto iter = mWorker->getPlayerManager()->createIterator(); iter; ++iter )
	{
		ServerPlayer* sPlayer = static_cast<ServerPlayer*>(iter.getElement());
		mCheckDataBits |= BIT(sPlayer->getId());
		if( !sPlayer->isNetwork() )
			mLocalDataBits |= BIT(sPlayer->getId());
	}
}

void SVSyncFrameManager::onPlayerStateMsg( unsigned pID , PlayerStateMsg state )
{
	mFrameCollector->reflashPlayer( *mWorker->getPlayerManager() );
	refreshPlayerState();
}

void SVSyncFrameManager::onChangeActionState( NetActionState state )
{
	if ( state == NAS_LEVEL_INIT || state == NAS_LEVEL_RESTART )
	{

	}
}

void SVSyncFrameManager::fireAction( ActionTrigger& trigger )
{
	for( auto iter = mWorker->getPlayerManager()->createIterator(); iter; ++iter )
	{
		GamePlayer* player = iter.getElement();
		if ( mLocalDataBits & BIT( player->getId() ) )
		{
			if ( player->getInfo().actionPort == ERROR_ACTION_PORT )
				continue;

			mActionTemplate->firePortAction(player->getInfo().actionPort,  trigger );
		}
	}
}

void SVSyncFrameManager::resetFrameData()
{
	mFrameDataList.clear();
	mFrameMgr.clearData();
	mCountDataDelay = 0;
}

void SVSyncFrameManager::release()
{
	delete this;
}

CLSyncFrameManager::CLSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector ) 
	:CSyncFrameManager( actionTemp , frameCollector )
	,mCalcuator( 20 )
	,mFrameStream( new GDPFrameStream )
{
	assert( !worker->isServer() );
	mWorker = worker;
	mWorker->getEvaluator().setUserFun< GDPFrameStream >( this , &CLSyncFrameManager::procFrameData );
	mUserPlayer = mWorker->getPlayerManager()->getUser();

	mLastSendDataFrame = 0;
	mLastRecvDataFrame = 0;
}

CLSyncFrameManager::~CLSyncFrameManager()
{
	mWorker->getEvaluator().removeProcesserFun( this );
}


bool CLSyncFrameManager::sendFrameData()
{
	mProcessor.beginAction( CTF_BLOCK_ACTION );

	int32 frame = mFrameMgr.getFrame() + 1;
	mFrameStream->frame = frame;
	mFrameStream->buffer.clear();

	if ( mFrameCollector->haveFrameData(frame) )
	{
		auto dataStream = MakeBufferSerializer(mFrameStream->buffer);
		mFrameCollector->collectFrameData(dataStream);
	}


#if DEBUG_SHOW_FRAME_DATA_TRANSITION
	//if( (mFrameStream->frame % 40) == 0 )
	if ( mFrameStream->buffer.getAvailableSize() )
	{
		LogDevMsg(0, "Send Frame Data : frame = %d", mFrameStream->frame);
	}
	
#endif

	mWorker->sendCommand( UseChannel , mFrameStream.get() , 0 );

	if ( mLastSendDataFrame != mFrameMgr.getFrame() )
	{
		mCalcuator.markRequest();
		mLastSendDataFrame = mFrameMgr.getFrame();
	}

	mProcessor.endAction();
	return true;
}

void CLSyncFrameManager::procFrameData( IComPacket* cp)
{
	GDPFrameStream* data = cp->cast< GDPFrameStream >();

	if( bClearData )
		return;

	if ( data->frame <= mFrameMgr.getFrame() )
		return;

	if ( data->frame == mLastSendDataFrame + 1 )
	{
		mCalcuator.markReply();
		if ( mCalcuator.getSampleNum() >= 30 )
		{
			LogMsg("Lactency = %u" , mCalcuator.calcResult() );
			mCalcuator.clear();
		}
	}

	//if( mFrameStream->frame % 40 == 0 )
	{
#if DEBUG_SHOW_FRAME_DATA_TRANSITION
		//LogDevMsg(10, "Recv Frame Data frame = %u %d %d", data->frame, mFrameMgr.getFrame(), data->frame - mFrameMgr.getFrame());
#endif
	}
	mLastRecvDataFrame = data->frame;
	mFrameMgr.addFrameData( data->frame , data->buffer );
	return;
}

void CLSyncFrameManager::fireAction( ActionTrigger& trigger )
{
	if ( mUserPlayer->getInfo().actionPort == ERROR_ACTION_PORT )
		return;

	mActionTemplate->firePortAction(mUserPlayer->getInfo().actionPort , trigger );
}

void CLSyncFrameManager::resetFrameData()
{
	bClearData = true;
	mFrameMgr.clearData();
	mLastSendDataFrame = 0;
	mLastRecvDataFrame = 0;
}

void CLSyncFrameManager::release()
{
	delete this;
}
