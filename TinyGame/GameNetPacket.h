#ifndef GameNetPacket_h__
#define GameNetPacket_h__

#include "ComPacket.h"

#include "TSocket.h"
#include "SocketBuffer.h"
#include "DataStreamBuffer.h"

#include "GamePlayer.h"

#include <new>

typedef unsigned SessionId;

enum NetPacketID
{
	//// Client Packet///////////
	CP_START_ID = 0 ,
	CP_LOGIN ,
	CP_ECHO  ,
	CP_UPD_CON ,
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
	SP_CON_SETTING ,
	SP_LEVEL_INFO ,
	SP_SERVER_INFO ,
	SP_SLOT_STATE ,
	
	SP_NEXT_ID ,
	///////////////////////////////
	GP_START_ID = 300 ,
	GP_GAME_SV_LIST ,
	GP_LADDER_RECORD ,
	GP_NEXT_ID ,

	/////////////////////////////
	GDP_START_ID = 500 ,
	GDP_FARME_STREAM ,
	GDP_STREAM  ,
	GDP_NEXT_ID ,
};


class AllocTake : public SBuffer::Take
{
public:
	AllocTake( SBuffer& buffer ):SBuffer::Take( buffer ){}

	static void* alloc( unsigned size ){ return malloc( size ); }
	static void  free( void* ptr ){ ::free( ptr ); }

	template< class T > 
	static T* newObj()
	{
		void* ptr = alloc( sizeof( T ) );
		new ptr T;
	}
};


inline SBuffer& operator >> ( SBuffer& buffer , DataStreamBuffer& dataBuffer )
{
	size_t size;
	buffer.take( size );
	dataBuffer.fill( buffer , size );
	return buffer;
}

inline SBuffer& operator << ( SBuffer& buffer , DataStreamBuffer& dataBuffer )
{
	size_t size = dataBuffer.getAvailableSize();
	buffer.fill( size );
	buffer.copy( dataBuffer , size );
	return buffer;
}

template< int N >
inline SBuffer& operator >> ( SBuffer& buffer , FixString< N >& val )
{
	buffer.take( val , N );
	return buffer;
}

template< int N >
inline SBuffer& operator << ( SBuffer& buffer , FixString< N > const& val )
{
	buffer.fill( val , N );
	return buffer;
}


template< class T , int ID >
class GamePacket : public IComPacket
{
public:
	enum { PID = ID , };
	GamePacket():IComPacket( ID ){}
	void doTake( SBuffer& buffer )
	{  
		AllocTake takeOp( buffer ); 
		static_cast<T*>( this )->operateBuffer( takeOp );      
	}
	void doFill( SBuffer& buffer )
	{  
		SBuffer::Fill fillOp( buffer );
		static_cast<T*>( this )->operateBuffer( fillOp ); 
	}
	template < class BufferOP >
	void  operateBuffer( BufferOP& op ){}
};

class FramePacket : public IComPacket
{
public:
	FramePacket( ComID id ):IComPacket( id ){}
	long  frame;
};

template< class T , int ID >
class GameFramePacket : public FramePacket
{
public:
	enum { PID = ID , };
	GameFramePacket():FramePacket( ID ){}

	void doTake( SBuffer& buffer )
	{  
		buffer.take( frame ); 
		AllocTake takeOp( buffer ); 
		static_cast<T*>( this )->operateBuffer( takeOp );      
	}
	void doFill( SBuffer& buffer )
	{  
		buffer.fill( frame ); 
		SBuffer::Fill fillOp( buffer );
		static_cast<T*>( this )->operateBuffer( fillOp ); 
	}
	template < class BufferOP >
	void  operateBuffer( BufferOP& op ){}
};


class GDPFrameStream : public GameFramePacket< GDPFrameStream , GDP_FARME_STREAM >
{
public:
	DataStreamBuffer buffer;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op  & buffer;
	}
};

class GDPStream : public GamePacket< GDPStream , GDP_STREAM >
{
public:
	DataStreamBuffer buffer;
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op  & buffer;
	}
};

class CPLogin : public GamePacket< CPLogin , CP_LOGIN >
{
public:
	SessionId       id;
	FixString< 32 > name;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & id & name;
	}
};


class CSPRawData : public GamePacket< CSPRawData , CSP_RAW_DATA >
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



class CPEcho : public GamePacket< CPEcho , CP_ECHO >
{
public:
	FixString< 256 > content;
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & content;
	}
};


class CSPMsg : public GamePacket< CSPMsg , CSP_MSG >
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
	FixString< MaxMsgLength > content;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & type;
		if ( type == ePLAYER )
			op & playerID;
		op & content;
	}
};

class SPSlotState : public GamePacket< SPSlotState , SP_SLOT_STATE  >
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

class CSPComMsg : public GamePacket< CSPComMsg , CSP_COM_MSG >
{
public:
	static int const MaxMsgLength = 128;
	FixString< MaxMsgLength > str;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & str;
	}
};



class CSPPlayerState : public GamePacket< CSPPlayerState , CSP_PLAYER_STATE >
{
public:
	unsigned playerID;
	uint8     state;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & playerID & state;
	}

};

class CSPClockSynd : public GamePacket< CSPClockSynd , CSP_ClOCK_SYN >
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


class SPServerInfo : public GamePacket< SPServerInfo , SP_SERVER_INFO >
{
public:
	static int const MAX_NAME_LENGTH = 32;
	FixString< MAX_NAME_LENGTH > name;
	FixString< 32 > ip;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & name & ip;
	}
};

class CPUdpCon : public GamePacket< CPUdpCon , CP_UPD_CON >
{
public:
	SessionId   id;
	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & id;
	}
};
class SPConSetting : public GamePacket< SPConSetting , SP_CON_SETTING >
{
public:
	enum
	{
		eNEW_CON ,
	};
	char      result;
	SessionId id;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & result;
		if ( result == eNEW_CON )
			op & id;
	}
};

class SPPlayerStatus : public GamePacket< SPPlayerStatus , SP_PLAYER_STATUS >
{
public:
	SPPlayerStatus()
	{
		needFree = false;
	}

	~SPPlayerStatus()
	{
		if ( needFree )
			delete[] info[0];
	}

	uint8        numPlayer;
	int         flag[ MAX_PLAYER_NUM ];
	PlayerInfo* info[ MAX_PLAYER_NUM ];


	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		op & numPlayer & flag;
		if ( BufferOP::IsTake )
		{
			PlayerInfo* newInfo = new PlayerInfo[ numPlayer ];
			for( unsigned i = 0 ; i < numPlayer ; ++i )
				info[i] = newInfo + i;
			needFree = true;
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
	bool needFree;
};


struct GameSvInfo
{
	uint32  ipv4;
	uint16  port;
};

class GPGameSvList : public GamePacket< GPGameSvList , GP_GAME_SV_LIST  >
{
public:
	size_t getGameSvInfoNum(){  return gameSvInfo.size();  }
	std::vector< GameSvInfo > gameSvInfo;

	template < class BufferOP >
	void  operateBuffer( BufferOP& op )
	{
		size_t size;

		if ( BufferOP::IsTake )
		{
			op & size;
			gameSvInfo.resize( size );
		}
		else
		{
			size = gameSvInfo.size();
			op & size;
		}

		for( int i = 0 ; i < (int)size ; ++i )
		{
			op & gameSvInfo[i];
		}
	}
};


#endif // GameNetPacket_h__