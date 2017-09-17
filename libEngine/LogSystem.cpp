#include "LogSystem.h"

#include "Singleton.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#include <vector>



class LogManager : public SingletonT< LogManager >
{
public:

	typedef std::vector< ILogListener* > MsgListenerList;
	MsgListenerList chanelListenerList[ NUM_DEFAULT_LOG_CHANNEL ];
	

	void   addListener( LogChannel channel , ILogListener* listener )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];
		MsgListenerList::iterator iter = std::find( 
			listenList.begin() , listenList.end() , listener );

		if ( iter != listenList.end() )
			return;

		listenList.push_back( listener );
	}

	void   removeListener( LogChannel channel , ILogListener* listener )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];
		if( listenList.empty() ) 
			return;

		MsgListenerList::iterator iter = std::find( 
			listenList.begin() , listenList.end() , listener );

		if ( iter != listenList.end() )
			listenList.erase( iter );
	}


	void   sendMessage( LogChannel channel , char const* format , va_list argptr , int level )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];

		char buffer[10240];
		vsprintf_s( buffer , format , argptr );

		MsgListenerList::iterator itEnd = listenList.end();
		for( MsgListenerList::iterator iter = listenList.begin() ; 
			iter != itEnd ; ++iter )
		{
			ILogListener* listener = *iter;
			if ( !listener->filterLog( channel , level ) )
				continue;

			listener->receiveLog( channel , buffer );
		}
	}
};


ILogListener::~ILogListener()
{
	for( int i = 0 ; i < NUM_DEFAULT_LOG_CHANNEL ; ++i )
		removeChannel( LogChannel(i) );
}

void ILogListener::addChannel( LogChannel channel )
{
	LogManager::getInstance().addListener( channel , this );
}

void ILogListener::removeChannel( LogChannel channel )
{
	LogManager::getInstance().removeListener( channel , this );
}


void Msg(char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	LogManager::getInstance().sendMessage( LOG_MSG , format , argptr , 0 );
	va_end( argptr );
}

void DevMsg( int level , char const* format , ... )
{
	//#ifdef _DEBUG
	va_list argptr;
	va_start( argptr, format );
	LogManager::getInstance().sendMessage( LOG_DEV , format , argptr , level );
	va_end( argptr );
	//#endif
}

void WarmingMsg( int level , char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	LogManager::getInstance().sendMessage( LOG_WARNING , format , argptr , level );
	va_end( argptr );

}

void LogAddListenChannel(ILogListener& listener, LogChannel channel)
{
	LogManager::getInstance().addListener(channel, &listener);
}

void LogRemoveListenLogChannel(ILogListener& listener, LogChannel channel)
{
	LogManager::getInstance().addListener(channel, &listener);
}

void ErrorMsg( char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	LogManager::getInstance().sendMessage( LOG_ERROR , format , argptr , 0 );
	va_end( argptr );
}

void Msg       ( char const* format , va_list argptr )
{
	LogManager::getInstance().sendMessage( LOG_MSG , format , argptr , 0 );
}

void ErrorMsg  ( char const* format , va_list argptr )
{
	LogManager::getInstance().sendMessage( LOG_ERROR , format , argptr , 0 );
}

void DevMsg    ( int level , char const* format , va_list argptr )
{
	LogManager::getInstance().sendMessage( LOG_DEV , format , argptr , level );
}

void WarmingMsg( int level , char const* format , va_list argptr )
{
	LogManager::getInstance().sendMessage( LOG_WARNING , format , argptr , level );
}
