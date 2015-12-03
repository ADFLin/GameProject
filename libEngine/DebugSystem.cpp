#include "DebugSystem.h"

#include "Singleton.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

#include <list>

class MsgManager : public SingletonT< MsgManager >
{
public:

	typedef std::list< IMsgListener* > MsgListenerList;
	MsgListenerList chanelListenerList[ NUM_MSG_CHANNEL ];
	char buffer[1024];

	void   addListener( MsgChannel channel , IMsgListener* listener )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];
		MsgListenerList::iterator iter = std::find( 
			listenList.begin() , listenList.end() , listener );

		if ( iter != listenList.end() )
			return;

		listenList.push_back( listener );
	}

	void   removeListener( MsgChannel channel , IMsgListener* listener )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];
		if( listenList.empty() ) 
			return;

		MsgListenerList::iterator iter = std::find( 
			listenList.begin() , listenList.end() , listener );

		if ( iter != listenList.end() )
			listenList.erase( iter );
	}


	void   sendMessage( MsgChannel channel , char const* format , va_list argptr , int level )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];

		vsprintf_s( buffer , format , argptr );

		MsgListenerList::iterator itEnd = listenList.end();
		for( MsgListenerList::iterator iter = listenList.begin() ; 
			iter != itEnd ; ++iter )
		{
			IMsgListener* listener = *iter;
			if ( level != 0 && level > listener->getLevel() )
				continue;

			listener->receive( channel , buffer );
		}
	}


	void   sendDevMessage( MsgChannel channel , char const* format , va_list argptr , int level )
	{
		MsgListenerList& listenList = chanelListenerList[ channel ];

		vsprintf_s( buffer , format , argptr );

		MsgListenerList::iterator itEnd = listenList.end();
		for( MsgListenerList::iterator iter = listenList.begin() ; 
			iter != itEnd ; ++iter )
		{
			IMsgListener* listener = *iter;

			if ( level != listener->getLevel() )
				continue;

			listener->receive( channel , buffer );
		}
	}
};


IMsgListener::IMsgListener()
{
	m_level = 0;
}

IMsgListener::~IMsgListener()
{
	for( int i = 0 ; i < NUM_MSG_CHANNEL ; ++i )
		removeChannel( MsgChannel(i) );
}

void IMsgListener::addChannel( MsgChannel channel )
{
	MsgManager::getInstance().addListener( channel , this );
}

void IMsgListener::removeChannel( MsgChannel channel )
{
	MsgManager::getInstance().removeListener( channel , this );
}


void Msg(char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	MsgManager::getInstance().sendMessage( MSG_NORMAL , format , argptr , 0 );
	va_end( argptr );
}

void DevMsg( int level , char const* format , ... )
{
	//#ifdef _DEBUG
	va_list argptr;
	va_start( argptr, format );
	MsgManager::getInstance().sendDevMessage( MSG_DEV , format , argptr , level );
	va_end( argptr );
	//#endif
}

void WarmingMsg( int level , char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	MsgManager::getInstance().sendMessage( MSG_WARNING , format , argptr , level );
	va_end( argptr );

}

void ErrorMsg( char const* format , ... )
{
	va_list argptr;
	va_start( argptr, format );
	MsgManager::getInstance().sendMessage( MSG_ERROR , format , argptr , 0 );
	va_end( argptr );
}

void Msg       ( char const* format , va_list argptr )
{
	MsgManager::getInstance().sendMessage( MSG_NORMAL , format , argptr , 0 );
}

void ErrorMsg  ( char const* format , va_list argptr )
{
	MsgManager::getInstance().sendMessage( MSG_ERROR , format , argptr , 0 );
}

void DevMsg    ( int level , char const* format , va_list argptr )
{
	MsgManager::getInstance().sendMessage( MSG_DEV , format , argptr , level );
}

void WarmingMsg( int level , char const* format , va_list argptr )
{
	MsgManager::getInstance().sendMessage( MSG_WARNING , format , argptr , level );
}
