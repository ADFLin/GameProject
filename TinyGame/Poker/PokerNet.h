#ifndef PokerNet_h__
#define PokerNet_h__

#include "IDataTransfer.h"
#include "GameNetPacket.h"

class ComWorker;
class NetWorker;
class ServerWorker;

namespace Poker {


	class CTestDataTransfer : public IDataTransfer
	{
	public:
		void sendData( int recvId , int dataId , void* data , int num );
		void setRecvFun( RecvFun fun ){ mFun = fun; }
		int                slotId;
		CTestDataTransfer* conTransfer;
	private:
		RecvFun            mFun;
	};


	class CWorkerDataTransfer : public IDataTransfer
	{
	public:
		CWorkerDataTransfer( ComWorker* worker , int slotId );

		virtual void sendTcpCommand( int recvId , IComPacket* cp );
		void  sendData( int recvId , int dataId , void* data , int num );
		void  setRecvFun( RecvFun fun ){  mFun = fun;  }
		void  procPacket( IComPacket* cp );

		int        mSlotId;
		GDPStream  mStream;
		ComWorker* mWorker;
		RecvFun    mFun;
	};

	class CSVWorkerDataTransfer : public CWorkerDataTransfer
	{
	public:
		CSVWorkerDataTransfer( NetWorker* worker , int numPlayer );
		virtual void sendTcpCommand( int recvId , IComPacket* cp );
		ServerWorker* getServer();

		std::vector< unsigned > mPlayerIdMap;
	};

}//namespace Poker

#endif // PokerNet_h__
