#include "LogSystem.h"

#include "Singleton.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#include <vector>

#if USE_LOG

class LogManager : public SingletonT< LogManager >
{
public:

	typedef std::vector< LogOutput* > LogOutputList;
	LogOutputList chanelListenerList[ NUM_DEFAULT_LOG_CHANNEL ];
	

	void   addListener( LogChannel channel , LogOutput* listener )
	{
		LogOutputList& outputList = chanelListenerList[ channel ];
		LogOutputList::iterator iter = std::find( 
			outputList.begin() , outputList.end() , listener );

		if ( iter != outputList.end() )
			return;

		outputList.push_back( listener );
	}

	void   removeListener( LogChannel channel , LogOutput* listener )
	{
		LogOutputList& outputList = chanelListenerList[ channel ];
		if( outputList.empty() ) 
			return;

		LogOutputList::iterator iter = std::find( 
			outputList.begin() , outputList.end() , listener );

		if ( iter != outputList.end() )
			outputList.erase( iter );
	}


	void   sendMessage(LogChannel channel, int level , char const* str)
	{
		LogOutputList& outputList = chanelListenerList[channel];

		for(LogOutput* output : outputList)
		{
			if( !output->filterLog(channel, level) )
				continue;

			output->receiveLog(channel, str);
		}
	}

	void   sendMessageV( LogChannel channel , char const* format, int level , va_list argptr  )
	{
		char buffer[10240];
		FCString::PrintfV(buffer, format, argptr);
		sendMessage(channel, level , buffer);
	}
};

void LogAddListenChannel(LogOutput& listener, LogChannel channel)
{
	LogManager::Get().addListener(channel, &listener);
}

void LogRemoveListenLogChannel(LogOutput& listener, LogChannel channel)
{
	LogManager::Get().addListener(channel, &listener);
}

#if CORE_SHARE_CODE
void LogMsg(char const* str)
{
	LogManager::Get().sendMessage(LOG_MSG, 0, str);
}

void LogError(char const* str)
{
	LogManager::Get().sendMessage(LOG_ERROR, 0, str);
}

void LogDevMsg(int level, char const* str)
{
	LogManager::Get().sendMessage(LOG_DEV, level, str);
}

void LogWarning(int level, char const* str)
{
	LogManager::Get().sendMessage(LOG_WARNING, level, str);
}

void LogMsgV(char const* format, va_list argptr)
{
	LogManager::Get().sendMessageV(LOG_MSG, format, 0, argptr);
}

void LogErrorV(char const* format, va_list argptr)
{
	LogManager::Get().sendMessageV(LOG_ERROR, format, 0, argptr);
}

void LogDevMsgV(int level, char const* format, va_list argptr)
{
	LogManager::Get().sendMessageV(LOG_DEV, format, level, argptr);
}

void LogWarningV(int level, char const* format, va_list argptr)
{
	LogManager::Get().sendMessageV(LOG_WARNING, format, level, argptr);
}


LogOutput::~LogOutput()
{
	for( int i = 0; i < NUM_DEFAULT_LOG_CHANNEL; ++i )
		removeChannel(LogChannel(i));
}

void LogOutput::addChannel(LogChannel channel)
{
	LogManager::Get().addListener(channel, this);
}

void LogOutput::removeChannel(LogChannel channel)
{
	LogManager::Get().removeListener(channel, this);
}

#endif //CORE_SHARE_CODE

#else

LogOutput::~LogOutput(){}
void LogOutput::addChannel(LogChannel channel){}
void LogOutput::removeChannel(LogChannel channel){}

#endif

