#ifndef IDataTransfer_h__
#define IDataTransfer_h__

#include "FastDelegate/FastDelegate.h"

#define SLOT_SERVER -1
#define DATA2ID( CLASS ) DATA_##CLASS

namespace Poker
{
	typedef fastdelegate::FastDelegate< void ( int , int , void* ) > RecvFun;

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

}

#endif // IDataTransfer_h__