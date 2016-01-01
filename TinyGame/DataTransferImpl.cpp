#include "TinyGamePCH.h"
#include "DataTransferImpl.h"

#include "GameServer.h"

void CTestDataTransfer::sendData( int recvId , int dataId , void* data , int num )
{
	if ( recvId == conTransfer->slotId || recvId == SLOT_SERVER )
	{
		conTransfer->mFun( slotId , dataId , data , num );
	}
}

CWorkerDataTransfer::CWorkerDataTransfer( ComWorker* worker , int slotId )
{
	mWorker = worker;
	mSlotId = slotId;
	mWorker->getEvaluator().setUserFun< GDPStream >( this , &CWorkerDataTransfer::procPacket );
}

void CWorkerDataTransfer::sendTcpCommand( int recvId , IComPacket* cp )
{
	mWorker->sendTcpCommand( cp );
}

void CWorkerDataTransfer::sendData( int recvId , int dataId , void* data , int num )
{
	mStream.buffer.clear();
	mStream.buffer.fill( mSlotId );
	mStream.buffer.fill( dataId  );
	mStream.buffer.fill( data , num );
	sendTcpCommand( recvId , &mStream );
}

void CWorkerDataTransfer::procPacket( IComPacket* cp )
{
	GDPStream* com = cp->cast< GDPStream >();
	int slotId;
	int dataId;
	size_t pos = com->buffer.getUseSize();
	com->buffer.take( slotId );
	com->buffer.take( dataId );
	char* data = com->buffer.getData() + com->buffer.getUseSize();
	int dataSize = com->buffer.getAvailableSize();
	mFun( slotId , dataId , data , dataSize );

	com->buffer.setUseSize( pos );
}


CSVWorkerDataTransfer::CSVWorkerDataTransfer( NetWorker* worker , int numPlayer ) 
	:CWorkerDataTransfer( worker , SLOT_SERVER )
	,mPlayerIdMap( numPlayer , ERROR_PLAYER_ID )
{
	assert( worker->isServer() );
	for( IPlayerManager::Iterator iter = getServer()->getPlayerManager()->getIterator();
		iter.haveMore() ; iter.goNext())
	{
		GamePlayer* player = iter.getElement();
		if ( player->getActionPort() != ERROR_ACTION_PORT )
			mPlayerIdMap[ player->getActionPort() ] = player->getId();
	}
}

ServerWorker* CSVWorkerDataTransfer::getServer()
{
	return static_cast< ServerWorker* >( mWorker );
}

void CSVWorkerDataTransfer::sendTcpCommand( int recvId , IComPacket* cp )
{
	if ( recvId == SLOT_SERVER )
	{
		getServer()->sendTcpCommand( cp );
	}
	else
	{
		ServerPlayer* player = getServer()->getPlayerManager()->getPlayer( mPlayerIdMap[ recvId ]);
		if ( player )
		{
			player->sendTcpCommand( cp );
		}
		else
		{


		}
	}
}

