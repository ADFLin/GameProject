#ifndef DebugSystem_h__
#define DebugSystem_h__

#include <cstdarg>
#include <cassert>

#include "ShareLibConfig.h"


CORE_API void Msg       ( char const* format , ... );
CORE_API void ErrorMsg  ( char const* format , ... );
CORE_API void DevMsg    ( int level , char const* format , ... );
CORE_API void WarmingMsg( int level , char const* format , ... );

CORE_API void Msg       ( char const* format , va_list argptr );
CORE_API void ErrorMsg  ( char const* format , va_list argptr );
CORE_API void DevMsg    ( int level , char const* format , va_list argptr );
CORE_API void WarmingMsg( int level , char const* format , va_list argptr );


class IMsgListener;
void SendMsg( IMsgListener* output , char const* addStr , char const* format ,va_list argptr );

enum MsgChannel
{
	MSG_NORMAL   = 0,
	MSG_DEV,
	MSG_WARNING,
	MSG_ERROR,

	NUM_MSG_CHANNEL ,
};

class IMsgListener
{
public:
	CORE_API IMsgListener();
	CORE_API virtual ~IMsgListener();
	CORE_API void addChannel   ( MsgChannel channel );
	CORE_API void removeChannel( MsgChannel channel );

	virtual void receive( MsgChannel channel , char const* str ) = 0;

	void setLevel( int level ){ m_level = level; }
	int  getLevel() const { return m_level; }
protected:
	friend class MsgManager;
	friend void SendMsg( IMsgListener* output , char const* addStr , char const* format ,va_list argptr );
	int  m_level;
};

#endif // DebugSystem_h__

