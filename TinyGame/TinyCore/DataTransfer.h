#ifndef DataTransfer_h__
#define DataTransfer_h__

#include "FastDelegate/FastDelegate.h"
#include "Meta/FunctionCall.h"

#define SLOT_SERVER -1

DEFINE_HAVE_MEMBER_FUNC_IMPL(THaveGetSendSizeFuncImpl, getSendSize);

template< class T >
struct DataTransferTypeToId {};



template< class T >
struct THaveGetSendSizeFunc : Meta::HaveResult< THaveGetSendSizeFuncImpl<T>::Value >{};

typedef fastdelegate::FastDelegate< void ( int recvId , int dataId , void* data , int dataSize ) > RecvFunc;

class IDataTransfer
{
public:
	virtual void sendData( int recvId , int dataId , void* data , int num ) = 0;
	virtual void setRecvFunc( RecvFunc func ) = 0;

	template< class T >
	void sendData(int recvId, T& data)
	{
		sendData(recvId, ::DataTransferTypeToId< std::remove_reference< decltype(data) >::type >::Result , data );
	}

	template< class T >
	void sendData(int recvId, T& data , int dataSize )
	{
		sendData(recvId, ::DataTransferTypeToId< std::remove_reference< decltype(data) >::type >::Result, &data, dataSize );
	}

	template< class T >
	void sendData( int recvId , int dataId , T& data )
	{
		sendData( recvId , dataId , data , THaveGetSendSizeFunc<T>::Type() );
	}

	template< class T >
	void sendData(int recvId, int dataId, T& data , Meta::TureType)
	{
		sendData(recvId, dataId, &data, data.getSendSize() );
	}
	template< class T >
	void sendData(int recvId, int dataId, T& data, Meta::FalseType)
	{
		sendData(recvId, dataId, &data, sizeof(data));
	}
};


#define DATA2ID( CLASS ) DATA_##CLASS

#define ENUM_OP( A )  A,
#define DATA2ID_ENUM_OP( TYPE ) DATA2ID( TYPE ),
#define DATA2ID_MAP_OP( TYPE ) template<> struct ::DataTransferTypeToId< TYPE >{ static int const Result = DATA2ID( TYPE ); };
#define DEFINE_DATA2ID( DATA_LIST , COMMAND_LIST )\
	enum\
	{\
		DATA_LIST(DATA2ID_ENUM_OP)\
		COMMAND_LIST(ENUM_OP)\
	};\
	DATA_LIST(DATA2ID_MAP_OP)

#endif // DataTransfer_h__