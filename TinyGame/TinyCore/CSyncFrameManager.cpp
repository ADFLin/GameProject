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

void FrameDataManager::addFrameData( long frame , DataSteamBuffer& buffer )
{
	if ( mLastDataFrame < frame )
		mLastDataFrame = frame;

	mDataList.push_front( buffer );

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
		auto dataSteam = MakeBufferDataSteam(*(iter->iter));
		actionTemp->restoreData( DataSerializer( dataSteam ) );
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

CSyncFrameManager::CSyncFrameManager( IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator) 
{
	mActionTemplate = actionTemp;
	mFrameGenerator = frameGenerator;
	mProcessor.setLanucher( this );
	mProcessor.addListener( *frameGenerator );
	bClearData = false;
	
}

bool CSyncFrameManager::scanInput( bool beUpdateFrame )
{
	if ( !beUpdateFrame )
		return false;
	if ( !mFrameMgr.haveFrameData() )
		return false;

	mFrameMgr.restoreData( mActionTemplate );
	mActionTemplate->prevCheckAction();
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


SVSyncFrameManager::SVSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator ) 
	:CSyncFrameManager( actionTemp , frameGenerator )
	,mFrameStream( new GDPFrameStream )
{
	assert( worker->isServer() );
	mWorker = static_cast< ServerWorker* >( worker );

	mWorker->setNetListener( this );
	mWorker->setComListener( frameGenerator );
	mWorker->getEvaluator().setUserFun< GDPFrameStream >( this , &SVSyncFrameManager::procFrameData );

	frameGenerator->reflashPlayer( *mWorker->getPlayerManager() );

	mCheckDataBits = 0;
	mLocalDataBits = 0;
	mUpdateDataBit = 0;

	for( auto iter = worker->getPlayerManager()->createIterator(); iter ; ++iter )
	{
		ServerPlayer* player = static_cast< ServerPlayer*>( iter.getElement() );

		mCheckDataBits |= BIT( player->getId() );
		if ( !player->isNetwork() )
			mLocalDataBits |= BIT( player->getId() );

		player->lastUpdateFrame = -1;
		player->lastRecvFrame   = -1;
		player->lastSendFrame   = -1;
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
					mFrameGenerator->recvClientData(data.id, data.buffer);

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
		bool needStop = false;

		for( auto iter = mWorker->getPlayerManager()->createIterator(); iter; ++iter )
		{
			ServerPlayer* player = static_cast< ServerPlayer*>( iter.getElement() );

			if ( BIT( player->getId() ) & mLocalDataBits )
				continue;

			if ( (int)mFrameMgr.getFrame() - player->lastUpdateFrame > MaxWaitDiffFrame  )
			{
				//Msg( "Data Delay %d (%d %d)" , mCountDataDelay , player->lastUpdateFrame ,(int) mFrameMgr.getFrame() );
				needStop = true;
				break;
			}
		}

		if(  needStop )
		{
			mUpdateDataBit = 0;
			++mCountDataDelay;
			mProcessor.endAction();
			return false;
		}
	}

	mCountDataDelay = 0;

	mFrameStream->buffer.clear();
	mFrameStream->frame = mFrameMgr.getFrame() + 1;

	auto dataSteam = MakeBufferDataSteam(mFrameStream->buffer);
	mFrameGenerator->generate( DataSerializer( dataSteam ) );

	//DevMsg( 10 ,"Send Frame Data frame = %d" , fp->frame  );
	mWorker->sendCommand( UseChannel , mFrameStream.get() , WSF_IGNORE_LOCAL );

	DataSteamBuffer buffer;
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

	PlayerId id  = info->player->getId();
	GDPFrameStream* fp = cp->cast< GDPFrameStream >();
	ServerPlayer* player = mWorker->getPlayerManager()->getPlayer( id );

	int maxDiscardDifFrame = 5;
	if ( player->lastUpdateFrame >= fp->frame + maxDiscardDifFrame ||
		 fp->frame >= player->lastUpdateFrame + maxDiscardDifFrame )
		return;

	static int count = 0;
	if ( fp->buffer.getAvailableSize() && count <= 10  )
	{
		LogMsg( "==%d %d %d==" , (int)mFrameMgr.getFrame() , (int)fp->frame , (int)fp->buffer.getAvailableSize() );
		count += 1;
	}

	ClientFrameData data;
	data.recvFrame = mFrameMgr.getFrame();
	data.sendFrame = fp->frame;
	data.buffer    = fp->buffer;
	data.id        = id;


	mFrameDataList.push_back( data );

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
	mFrameGenerator->reflashPlayer( *mWorker->getPlayerManager() );
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

			trigger.setPort( player->getInfo().actionPort );
			mActionTemplate->firePortAction( trigger );
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

CLSyncFrameManager::CLSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator ) 
	:CSyncFrameManager( actionTemp , frameGenerator )
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

	mFrameStream->frame = mFrameMgr.getFrame() + 1;
	mFrameStream->buffer.clear();

	auto dataStream = MakeBufferDataSteam(mFrameStream->buffer);
	mFrameGenerator->generate( DataSerializer(dataStream) );


#if 0
	DevMsg( 10 ,"Send Frame Data frame = %d" , mFrameStream->frame   );
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

	if ( mFrameStream->frame % 40 == 0 )
		LogDevMsg(  10 , "Recv Frame Data frame = %d %d %d" , data->frame , mFrameMgr.getFrame() , data->frame - mFrameMgr.getFrame() );
	mLastRecvDataFrame = data->frame;
	mFrameMgr.addFrameData( data->frame , data->buffer );
	return;
}

void CLSyncFrameManager::fireAction( ActionTrigger& trigger )
{
	if ( mUserPlayer->getInfo().actionPort == ERROR_ACTION_PORT )
		return;
	trigger.setPort( mUserPlayer->getInfo().actionPort );
	mActionTemplate->firePortAction( trigger );
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
