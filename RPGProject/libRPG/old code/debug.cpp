#include "debug.h"

#include <cstdio>
#include <cstring>

static TMsgProxy* g_OutputProxy[MSG_CHANEL_NUM] = { NULL };

TMsgProxy::TMsgProxy()
{
	m_showChanelName = true;
	m_level          = 0;
}

TMsgProxy::~TMsgProxy()
{
	for( int i = 0 ; i < MSG_CHANEL_NUM ; ++i )
	{
		if ( g_OutputProxy[i] == this )
			g_OutputProxy[i] = NULL;
	}
}

void TMsgProxy::setCurrentOutput( int chanel )
{
	g_OutputProxy[chanel] = this;
}

static void SendMsg( TMsgProxy* output , char const* addStr , char const* format ,va_list argptr )
{
	char str[1024];
	char* fstr = str;
	if ( output->m_showChanelName )
	{
		strcpy( str , addStr );
		fstr += strlen(addStr);
	}

	vsprintf( fstr , format , argptr );
	output->receive( str );
}


void Msg(char const* format , ... )
{
	TMsgProxy* output = g_OutputProxy[MSG_NORMAL];

	if ( !output )
		return;

	va_list argptr;
	va_start( argptr, format );
	SendMsg( output , "" , format , argptr );
	va_end( argptr );
}

void DevMsg( int level , char const* format , ... )
{
	TMsgProxy* output = g_OutputProxy[MSG_DEV];

	if ( !output )
		return;

	if ( level < output->getLevel() )
		return;

	va_list argptr;
	va_start( argptr, format );
	SendMsg( output , "Dev:" , format , argptr );
	va_end( argptr );
}

void WarmingMsg( int level , char const* format , ... )
{
	TMsgProxy* output = g_OutputProxy[MSG_WARNING];

	if ( !output )
		return;

	if ( level < output->getLevel() )
		return;

	va_list argptr;
	va_start( argptr, format );
	SendMsg( output , "Warming:" , format , argptr );
	va_end( argptr );

}


void ErrorMsg( char const* format , ... )
{
	TMsgProxy* output = g_OutputProxy[MSG_ERROR];

	if ( !output )
		return;

	va_list argptr;
	va_start( argptr, format );
	SendMsg( output , "Error:" , format , argptr );
	va_end( argptr );

}

