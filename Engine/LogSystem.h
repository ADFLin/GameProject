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

CORE_API void LogMsgImpl(char const* str);
CORE_API void LogErrorImpl(char const* str);
CORE_API void LogDevMsgImpl (int level, char const* str);
CORE_API void LogWarningImpl(int level, char const* str);

#define LOG_BUFFER_SIZE 102400

template< class ...Arg >
FORCEINLINE void LogMsgImpl(char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogMsgImpl(buffer);
}

template< class ...Arg >
FORCEINLINE void LogErrorImpl(char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogErrorImpl(buffer);
}

template< class ...Arg >
FORCEINLINE void LogDevMsgImpl(int level, char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogDevMsgImpl(level, buffer);
}

template< class ...Arg >
FORCEINLINE void LogWarningImpl(int level, char const* format, Arg ...arg)
{
	char buffer[LOG_BUFFER_SIZE];
	FCString::PrintfT(buffer, format, arg...);
	LogWarningImpl(level, buffer);
}

CORE_API void LogMsgVImpl( char const* format , va_list argptr );
CORE_API void LogErrorVImpl( char const* format , va_list argptr );
CORE_API void LogDevMsgVImpl( int level , char const* format , va_list argptr );
CORE_API void LogWarningVImpl( int level , char const* format , va_list argptr );


#define  LogMsg(...) LogMsgImpl(__VA_ARGS__)
#define  LogError(...) LogErrorImpl(__VA_ARGS__)
#define  LogDevMsg(...) LogDevMsgImpl(__VA_ARGS__)
#define  LogWarning(...) LogWarningImpl(__VA_ARGS__)

#define  LogMsgV(...) LogMsgVImpl(__VA_ARGS__)
#define  LogErrorV(...) LogErrorVImpl(__VA_ARGS__)
#define  LogDevMsgV(...) LogDevMsgVImpl(__VA_ARGS__)
#define  LogWarningV(...) LogWarningVImpl(__VA_ARGS__)

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

	void addDefaultChannels()
	{
		for (int i = 0; i < NUM_DEFAULT_LOG_CHANNEL; ++i)
		{
			addChannel(LogChannel(i));
		}
	}

	virtual bool filterLog(LogChannel channel, int level) { return true; }
	virtual void receiveLog(LogChannel channel, char const* str) = 0;

protected:
	friend class LogManager;
};

#endif // LogSystem_h__

