#ifndef DataTransferImpl_h__f3ef3722_73ac_49f9_bcf6_c987273f37cc
#define DataTransferImpl_h__f3ef3722_73ac_49f9_bcf6_c987273f37cc

#include "DataTransfer.h"
#include "GameNetPacket.h"

class ComWorker;
class NetWorker;
class ServerWorker;

class CTestDataTransfer : public IDataTransfer
{
public:
	void sendData( int recvId , int dataId , void* data , int num );
	void setRecvFunc( RecvFunc func ){ mFunc = func; }
	int                slotId;
	CTestDataTransfer* conTransfer;
private:
	RecvFunc            mFunc;
};

class CWorkerDataTransfer : public IDataTransfer
{
public:
	CWorkerDataTransfer( ComWorker* worker , int slotId );

	virtual void sendTcpCommand( int recvId , IComPacket* cp );
	void  sendData( int recvId , int dataId , void* data , int num );
	void  setRecvFunc( RecvFunc func ){  mFunc = func;  }

	//
	void  procPacket( IComPacket* cp );

	int        mSlotId;
	GDPStream  mStream;
	ComWorker* mWorker;
	RecvFunc   mFunc;
};

class CSVWorkerDataTransfer : public CWorkerDataTransfer
{
public:
	CSVWorkerDataTransfer( NetWorker* worker , int numPlayer );
	virtual void sendTcpCommand( int recvId , IComPacket* cp );
	ServerWorker* getServer();

	std::vector< unsigned > mPlayerIdMap;
};


#endif // DataTransferImpl_h__f3ef3722_73ac_49f9_bcf6_c987273f37cc
