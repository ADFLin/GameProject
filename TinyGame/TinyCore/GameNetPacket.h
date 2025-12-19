#ifndef GameNetPacket_h__
#define GameNetPacket_h__

#include "ComPacket.h"

#include "NetSocket.h"
#include "SocketBuffer.h"
#include "DataStreamBuffer.h"

#include "GameConfig.h"
#include "GamePlayer.h"
#include "GameStage.h"

#include <new>

class PacketFactory;

// 全局 PacketFactory 實例聲明（定義在 GameGlobal.cpp）
TINY_API extern PacketFactory GGamePacketFactory;


typedef unsigned SessionId;

enum NetPacketID
{
	//// Client Packet///////////
	CP_START_ID = 0 ,
	CP_LOGIN ,
	CP_ECHO  ,
	CP_UPD_CON ,
	CP_NET_CONTROL_REPLAY ,
	CP_NEXT_ID ,
	//// Client and Server ///////////
	CSP_START_ID = 100 ,
	CSP_ClOCK_SYN    ,
	CSP_PLAYER_STATE ,
	CSP_MSG ,
	CSP_COM_MSG ,
	CSP_RAW_DATA ,
	CSP_NEXT_ID ,
	//// Server Packet ///////////
	SP_START_ID = 200 ,
	SP_PLAYER_STATUS ,
	SP_CONNECT_MSG ,
	SP_LEVEL_INFO ,
	SP_SERVER_INFO ,
	SP_SLOT_STATE ,
	SP_NET_CONTROL_REQUEST ,
	
	SP_NEXT_ID ,
	///////////////////////////////
	GP_START_ID = 300 ,
	GP_GAME_SV_LIST ,
	GP_LADDER_RECORD ,
	GP_NEXT_ID ,

	/////////////////////////////
	GDP_START_ID = 400 ,
	GDP_FARME_STREAM ,
	GDP_STREAM  ,
	GDP_NEXT_ID ,

	////
	NET_PACKET_CUSTOM_ID = 500,
};


class AllocTakeOperator : public SocketBuffer::TakeOperator
{
public:
	AllocTakeOperator( SocketBuffer& buffer ):SocketBuffer::TakeOperator( buffer ){}

	static void* alloc( unsigned size ){ return ::malloc( size ); }
	static void  free( void* ptr ){ ::free( ptr ); }

	template< class T > 
	static T* newObj()
	{
		void* ptr = alloc( sizeof( T ) );
		new ptr T;
	}
};


inline SocketBuffer& operator >> ( SocketBuffer& buffer , DataStreamBuffer& dataBuffer )
{
	size_t size;
	buffer.take( size );
	dataBuffer.fill( buffer , size );
	return buffer;
}

inline SocketBuffer& operator << ( SocketBuffer& buffer , DataStreamBuffer& dataBuffer )
{
	size_t size = dataBuffer.getAvailableSize();
	buffer.fill( size );
	buffer.copy( dataBuffer , size );
	return buffer;
}

template< int N >
inline SocketBuffer& operator >> ( SocketBuffer& buffer , InlineString< N >& val )
{
	buffer.take( val , N );
	return buffer;
}

template< int N >
inline SocketBuffer& operator << ( SocketBuffer& buffer , InlineString< N > const& val )
{
	buffer.fill( val , N );
	return buffer;
}

template< class T >
struct TArrayData
{
public:
	bool bNeedFree;
	T*   data;
	int  length;

	TArrayData()
	{
		length = 0;
		data = nullptr;
		bNeedFree = false;
	}
	~TArrayData()
	{
		if( bNeedFree )
		{
			delete[] data;
		}
	}
};

template< class T >
inline SocketBuffer& operator >> (SocketBuffer& buffer, TArrayData< T >& val)
{
	buffer.take(val.length);
	if( val.length )
	{
		val.data = new T[val.length];
		val.bNeedFree = true;
	}
	buffer.take(SocketBuffer::MakeArrayData(val.data, val.length));
	return buffer;
}

template< class T >
inline SocketBuffer& operator << (SocketBuffer& buffer, TArrayData< T > const& val)
{
	buffer.fill(val.length);
	buffer.fill(SocketBuffer::MakeArrayData(val.data, val.length));
	return buffer;
}

template< class T , int ID >
class GamePacketT : public IComPacket
{
public:
	enum { PID = ID , };
	GamePacketT():IComPacket( ID ){}
	void doRead( SocketBuffer& buffer )
	{  
		AllocTakeOperator takeOp( buffer ); 
		static_cast<T*>( this )->operateBuffer( takeOp );      
	}
	void doWrite( SocketBuffer& buffer )
	{  
		SocketBuffer::FillOperator fillOp( buffer );
		static_cast<T*>( this )->operateBuffer( fillOp ); 
	}
	template < class BufferOP >
	void  operateBuffer( BufferOP& op ){}
};


class FramePacket : public IComPacket
{
public:
	FramePacket( ComID id ):IComPacket( id ){}
	int32  frame;
};

template< class T , int ID >
class GameFramePacketT : public FramePacket
{
public:
	enum { PID = ID , };
	GameFramePacketT():FramePacket( ID ){}

	void doRead( SocketBuffer& buffer )
	{  
		buffer.take( frame ); 
		AllocTakeOperator takeOp( buffer ); 
		static_cast<T*>( this )->operateBuffer( takeOp );      
	}
	void doWrite( SocketBuffer& buffer )
	{  
		buffer.fill( frame ); 
		SocketBuffer::FillOperator fillOp( buffer );
		static_cast<T*>( this )->operateBuffer( fillOp ); 
	}
	template < class BufferOP >
	void  operateBuffer( BufferOP& op ){}
};

template< typename T >
class TGamePacketRegister
{
public:
	TGamePacketRegister()
	{
		GGamePacketFactory.addFactory<T>();
	}
	~TGamePacketRegister()
	{
		GGamePacketFactory.removeFactory<T>();
	}
};

#define REGISTER_GAME_PACKET(TYPE)\
	static TGamePacketRegister<TYPE> ANONYMOUS_VARIABLE(GameRegister);


class GDPFrameStream : public GameFramePacketT< GDPFrameStream , GDP_FARME_STREAM >
{
public:
	DataStreamBuffer buffer;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op  & buffer;
	}
};

class GDPStream : public GamePacketT< GDPStream , GDP_STREAM >
{
public:
	DataStreamBuffer buffer;
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op  & buffer;
	}
};

class CPLogin : public GamePacketT< CPLogin , CP_LOGIN >
{
public:
	SessionId       id;
	InlineString< MAX_PLAYER_NAME_LENGTH > name;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & id & name;
	}
};


class CSPRawData : public GamePacketT< CSPRawData , CSP_RAW_DATA >
{
public:
	int   id;
	DataStreamBuffer buffer;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & id & buffer;
	}
};



class CPEcho : public GamePacketT< CPEcho , CP_ECHO >
{
public:
	InlineString< 256 > content;
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & content;
	}
};


class CSPMsg : public GamePacketT< CSPMsg , CSP_MSG >
{
public:

	enum
	{
		eSERVER ,
		ePLAYER ,
	};

	static int const MaxMsgLength = 128;
	uint8  type;
	unsigned  playerID;
	InlineString< MaxMsgLength > content;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & type;
		if ( type == ePLAYER )
			op & playerID;
		op & content;
	}
};

class SPSlotState : public GamePacketT< SPSlotState , SP_SLOT_STATE  >
{
public:
	int idx[ MAX_PLAYER_NUM ];
	int state[ MAX_PLAYER_NUM ];

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & idx & state;
	}
};

class CSPComMsg : public GamePacketT< CSPComMsg , CSP_COM_MSG >
{
public:
	static int const MaxMsgLength = 128;
	InlineString< MaxMsgLength > str;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & str;
	}
};

class CSPPlayerState : public GamePacketT< CSPPlayerState , CSP_PLAYER_STATE >
{
public:
	int8     playerId;
	uint8    state;
	//uint8    action;

	void setServerState( uint8 inState )
	{
		playerId = (int8)SERVER_PLAYER_ID;
		state = inState;
	}
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & playerId & state;
	}

};

class CSPClockSynd : public GamePacketT< CSPClockSynd , CSP_ClOCK_SYN >
{
public:
	enum
	{
		eSTART   , //Server
		eREQUEST , //Client
		eWAIT    , //Server
		eREPLY   , //Server
		eSYND    , //Server
		eDONE    , //Client
	};

	char code ;
	union
	{
		unsigned numSample;
		unsigned latency;
	};

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & code ;
		if ( code == eSTART )
			op & numSample;
		else if ( code == eDONE  )
			op & latency; 
	}
};


class SPServerInfo : public GamePacketT< SPServerInfo , SP_SERVER_INFO >
{
public:
	static int const MAX_NAME_LENGTH = 32;
	InlineString< MAX_NAME_LENGTH > name;
	InlineString< 32 > ip;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & name & ip;
	}
};

class SPLevelInfo : public GamePacketT< SPLevelInfo , SP_LEVEL_INFO >
	              , public GameLevelInfo
{
public:
	SPLevelInfo()
	{
		needFree = false;
	}
	~SPLevelInfo()
	{
		if ( needFree )
		{
			delete [] (char*)data;
		}
	}
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & seed & numData;
		if ( BufferOP::IsTake )
		{
			data = new char[ numData ];
			needFree = true;
		}
		op & SocketBuffer::RawData( data , numData );
	}
	bool needFree;
};

class CPUdpCon : public GamePacketT< CPUdpCon , CP_UPD_CON >
{
public:
	SessionId   id;
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & id;
	}
};

class SPConnectMsg : public GamePacketT< SPConnectMsg , SP_CONNECT_MSG >
{
public:
	enum
	{
		eNEW_CON ,
	};
	uint8     result;
	SessionId id;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & result;
		if ( result == eNEW_CON )
			op & id;
	}
};

class SPPlayerStatus : public GamePacketT< SPPlayerStatus , SP_PLAYER_STATUS >
{
public:
	SPPlayerStatus()
	{
		bAlloced = false;
	}

	~SPPlayerStatus()
	{
		if ( bAlloced )
			delete[] info[0];
	}

	uint8       numPlayer;
	int         flags[ MAX_PLAYER_NUM ];
	PlayerInfo* info[ MAX_PLAYER_NUM ];
	

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & numPlayer;
		op & flags;
		//op & SocketBuffer::MakeArrayData(flags, numPlayer);
		if ( BufferOP::IsTake )
		{
			PlayerInfo* newInfo = new PlayerInfo[ numPlayer ];
			for( unsigned i = 0 ; i < numPlayer ; ++i )
				info[i] = newInfo + i;
			bAlloced = true;
		}

		for( unsigned i = 0 ; i < numPlayer ; ++i )
		{
			PlayerInfo* pInfo = info[i];
			op & pInfo->name;
			op & pInfo->type;
			op & pInfo->playerId;
			op & pInfo->slot;
			op & pInfo->actionPort;
			op & pInfo->ping;
		}
	}

private:
	bool bAlloced;
};

class SPNetControlRequest : public GamePacketT< SPNetControlRequest, SP_NET_CONTROL_REQUEST >
{
public:
	uint8 action;
};

class CPNetControlReply : public GamePacketT< CPNetControlReply, CP_NET_CONTROL_REPLAY >
{
public:
	uint32 playerId;
	uint8  action;
	uint   result;
};

struct GameSvInfo
{
	uint32  ipv4;
	uint16  port;
};

class GPGameSvList : public GamePacketT< GPGameSvList , GP_GAME_SV_LIST  >
{
public:
	size_t getGameSvInfoNum(){  return gameSvInfo.size();  }
	TArray< GameSvInfo > gameSvInfo;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & gameSvInfo;
	}
};


#endif // GameNetPacket_h__