#ifndef LogSystem_h__
#define LogSystem_h__

#include <cstdarg>
#include <cassert>

#include "CoreShare.h"


CORE_API void Msg       ( char const* format , ... );
CORE_API void ErrorMsg  ( char const* format , ... );
CORE_API void DevMsg    ( int level , char const* format , ... );
CORE_API void WarmingMsg( int level , char const* format , ... );

CORE_API void Msg       ( char const* format , va_list argptr );
CORE_API void ErrorMsg  ( char const* format , va_list argptr );
CORE_API void DevMsg    ( int level , char const* format , va_list argptr );
CORE_API void WarmingMsg( int level , char const* format , va_list argptr );

enum LogChannel
{
	LOG_MSG   = 0,
	LOG_DEV,
	LOG_WARNING,
	LOG_ERROR,
	NUM_DEFAULT_LOG_CHANNEL ,
};

class ILogListener
{
public:
	CORE_API virtual ~ILogListener();
	CORE_API void addChannel(LogChannel channel);
	CORE_API void removeChannel(LogChannel channel);

	virtual bool filterLog(LogChannel channel, int level) { return true; }
	virtual void receiveLog(LogChannel channel, char const* str) = 0;

protected:
	friend class LogManager;
};

#endif // LogSystem_h__

