#ifndef LogSystem_h__
#define LogSystem_h__

#include <cstdarg>
#include <cassert>

#include "CoreShare.h"

#ifndef USE_LOG
#define USE_LOG 1
#endif

#if USE_LOG

CORE_API void LogMsg    (char const* str);
CORE_API void LogError  (char const* str);
CORE_API void LogDevMsg (int level, char const* str);
CORE_API void LogWarning(int level, char const* str);

CORE_API void LogMsgF    ( char const* format , ... );
CORE_API void LogErrorF  ( char const* format , ... );
CORE_API void LogDevMsgF ( int level , char const* format , ... );
CORE_API void LogWarningF( int level , char const* format , ... );

CORE_API void LogMsgV    ( char const* format , va_list argptr );
CORE_API void LogErrorV  ( char const* format , va_list argptr );
CORE_API void LogDevMsgV ( int level , char const* format , va_list argptr );
CORE_API void LogWarningV( int level , char const* format , va_list argptr );

#else

#define  LogMsg(...)
#define  LogError(...) 
#define  LogDevMsg(...)
#define  LogWarning(...) 

#define  LogMsgF(...) 
#define  LogErrorF(...) 
#define  LogDevMsgF(...) 
#define  LogWarningF(...) 

#define  LogMsgV(...) 
#define  LogErrorV(...) 
#define  LogDevMsgV(...) 
#define  LogWarningV(...) 

#endif

enum LogChannel
{
	LOG_MSG   = 0,
	LOG_DEV,
	LOG_WARNING,
	LOG_ERROR,
	NUM_DEFAULT_LOG_CHANNEL ,
};

class LogOutput
{
public:
	CORE_API virtual ~LogOutput();
	CORE_API void addChannel(LogChannel channel);
	CORE_API void removeChannel(LogChannel channel);

	virtual bool filterLog(LogChannel channel, int level) { return true; }
	virtual void receiveLog(LogChannel channel, char const* str) = 0;

protected:
	friend class LogManager;
};

#endif // LogSystem_h__

