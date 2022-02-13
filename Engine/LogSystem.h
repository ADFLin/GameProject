#ifndef LogSystem_h__
#define LogSystem_h__

#include <cstdarg>
#include <cassert>

#include "CoreShare.h"
#include "CString.h"

#ifndef USE_LOG
#define USE_LOG 1
#endif

#if USE_LOG

CORE_API void LogMsg    (char const* str);
CORE_API void LogError  (char const* str);
CORE_API void LogDevMsg (int level, char const* str);
CORE_API void LogWarning(int level, char const* str);

#define LOG_BUFFER_SIZE 102400

template< class ...Arg >
FORCEINLINE void LogMsg(char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogMsg(buffer);
}

template< class ...Arg >
FORCEINLINE void LogError(char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogError(buffer);
}

template< class ...Arg >
FORCEINLINE void LogDevMsg(int level, char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogDevMsg(level, buffer);
}

template< class ...Arg >
FORCEINLINE void LogWarning(int level, char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogWarning(level, buffer);
}

CORE_API void LogMsgV    ( char const* format , va_list argptr );
CORE_API void LogErrorV  ( char const* format , va_list argptr );
CORE_API void LogDevMsgV ( int level , char const* format , va_list argptr );
CORE_API void LogWarningV( int level , char const* format , va_list argptr );

#else

#define  LogMsg(...)
#define  LogError(...) 
#define  LogDevMsg(...)
#define  LogWarning(...) 

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

