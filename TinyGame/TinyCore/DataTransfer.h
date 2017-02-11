#ifndef DataTransfer_h__
#define DataTransfer_h__

#include "FastDelegate/FastDelegate.h"

#define SLOT_SERVER -1
#define DATA2ID( CLASS ) DATA_##CLASS

typedef fastdelegate::FastDelegate< void ( int recvId , int dataId , void* data , int dataSize ) > RecvFun;

class IDataTransfer
{
public:
	virtual void sendData( int recvId , int dataId , void* data , int num ) = 0;
	virtual void setRecvFun( RecvFun fun ) = 0;

	template< class T >
	void sendData( int recvId , int dataId , T& data )
	{
		sendData( recvId , dataId , &data , sizeof( data ) );
	}
};

#endif // DataTransfer_h__