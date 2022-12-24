#include "TinyGamePCH.h"
#include "DataTransferImpl.h"

#include "GameServer.h"

void CTestDataTransfer::sendData( int recvId , int dataId , void* data , int num )
{
	if ( recvId == conTransfer->slotId || recvId == SLOT_SERVER )
	{
		conTransfer->mFunc( slotId , dataId , data , num );
	}
}

CWorkerDataTransfer::CWorkerDataTransfer( ComWorker* worker , int slotId )
{
	mWorker = worker;
	mSlotId = slotId;
	mWorker->getEvaluator().setUserFunc< GDPStream >( this , &CWorkerDataTransfer::procPacket );
}

void CWorkerDataTransfer::sendTcpCommand( int recvId , IComPacket* cp )
{
	mWorker->sendTcpCommand( cp );
}

void CWorkerDataTransfer::sendData( int recvId , int dataId , void* data , int num )
{
	mStream.buffer.clear();
	mStream.buffer.fillConv<int8>( mSlotId );
	mStream.buffer.fillConv<int8>( dataId  );
	mStream.buffer.fill( num );
	mStream.buffer.fill( data , num );
	sendTcpCommand( recvId , &mStream );
}

void CWorkerDataTransfer::procPacket( IComPacket* cp)
{
	GDPStream* com = cp->cast< GDPStream >();
	size_t pos = com->buffer.getUseSize();
	int slotId;
	int dataId;
	com->buffer.takeConv<int8>( slotId );
	com->buffer.takeConv<int8>( dataId );
	int dataSizeRecord;
	com->buffer.take(dataSizeRecord);
	char* data = com->buffer.getData() + com->buffer.getUseSize();
	int dataSize = com->buffer.getAvailableSize();
	if( dataSizeRecord == dataSize )
	{
		mFunc(slotId, dataId, data, dataSize);
	}
	else
	{
		//#TODO:Add log

	}
	com->buffer.setUseSize( pos );
}


CSVWorkerDataTransfer::CSVWorkerDataTransfer( NetWorker* worker ) 
	:CWorkerDataTransfer( worker , SLOT_SERVER )
{
	assert( worker->isServer() );
	mPlayerIdMap.resize(getServer()->getPlayerManager()->getPlayerNum(), ERROR_PLAYER_ID);
	for( auto iter = getServer()->getPlayerManager()->createIterator(); iter; ++iter )
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

