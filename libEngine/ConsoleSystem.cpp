#include "ConsoleSystem.h"

#include "MemorySecurity.h"
#include <cstdlib>


static ConsoleSystem* g_console = NULL;
ComBase* ComBase::s_unRegCom = NULL;


#define STRING_BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 64
ConsoleSystem* getConsole()
{
	return g_console;
}

ComBase::ComBase( char const* _name , void* _ptr ,
				  int _num , char const** _pararm )
	:name(_name)
	,ptr(_ptr)
	,numParam(_num)
	,Param(_pararm)
{
	if ( getConsole() )
		getConsole()->insertCommand( this );
	else
	{
		this->next = s_unRegCom;
		s_unRegCom = this;
	}
}

ConsoleSystem::ConsoleSystem()
{
	m_dataBuffer = (char*) malloc( DATA_BUFFER_SIZE * sizeof(char) );
	m_strBuffer  = (char*) malloc( STRING_BUFFER_SIZE * sizeof(char) );
}

ConsoleSystem::~ConsoleSystem()
{
	g_console = NULL;

	free( m_dataBuffer );
	free( m_strBuffer );

	for( CommandMap::iterator iter = mNameMap.begin();
		iter != mNameMap.end() ; ++iter )
	{
		delete iter->second;
	}
}

bool ConsoleSystem::init()
{
	ComBase* data = ComBase::s_unRegCom;

	while( data != NULL )
	{
		insertCommand( data );
		data = data->next;
	}
	ComBase::s_unRegCom = NULL;

	g_console = this;

	return true;
}

bool ConsoleSystem::command( char const* comStr )
{
	size_t maxLen = strlen( comStr );

	strcpy_s( m_strBuffer , STRING_BUFFER_SIZE , comStr );

	m_numStr = 0;
	m_numParam = 0;
	m_numUsedParam = 0;

	char* context;
	char* bfPtr = strtok_s( m_strBuffer , " " , &context );

	while( bfPtr != NULL )
	{
		m_paramStr[m_numParam] = bfPtr;
		++m_numParam;
		bfPtr = strtok_s( NULL , " " , &context );
	}

	if ( m_numParam == 0 )
		return false;

	ComBase* com = findCommand( m_paramStr[0] );
	++m_numUsedParam;

	if ( com == NULL )
	{
		m_errorMsg = "unknown command";
		return false;
	}

	m_curCom = com;

	char* data = m_dataBuffer;
	m_dataMap[0] = data;
	for( int i = 0 ; i < com->numParam ; ++i )
	{
		char const* fptr = com->Param[i];
		data = fillVarData( data , fptr );

		if ( data == NULL )
			return false;

		m_dataMap[i+1] = data;
	}
	com->exec(  m_dataMap );

	return true;
}

int ConsoleSystem::findCommandName2( char const* includeStr , char const** findStr , int maxNum )
{
	int findNum = 0;

	size_t len = strlen( includeStr );
	for( CommandMap::iterator iter = mNameMap.begin();
		iter != mNameMap.end() ; ++iter )
	{
		if ( strncmp( iter->first , includeStr , len ) == 0 )
		{
			findStr[ findNum ] = iter->first;
			++findNum;
			if ( findNum == maxNum )
			{
				return maxNum;
			}
		}
	}
	return findNum;
}

int ConsoleSystem::findCommandName( char const* includeStr , char const** findStr , int maxNum )
{
	int findNum = 0;
	for( CommandMap::iterator iter = mNameMap.begin();
		iter != mNameMap.end() ; ++iter )
	{
		if ( strstr( iter->first , includeStr ) != NULL )
		{
			findStr[ findNum ] = iter->first;
			++findNum;

			if ( findNum == maxNum )
			{
				return maxNum;
			}
		}
	}

	return findNum;
}

char* ConsoleSystem::fillVarData( char* data , char const* format )
{
	char const* fptr = format;

	while( 1 )
	{
		while( fptr[0] != '%' &&
			   fptr[0] != '\0')
		{
			++fptr;
		}

		if ( *fptr =='\0' )
			break;

		if ( m_numUsedParam >= m_numParam )
		{
			m_errorMsg = "less param : ";
			m_errorMsg += m_curCom->name;
			for ( int i = 0 ; i < m_curCom->numParam ; ++ i )
			{
				m_errorMsg += std::string(" ");

				char const* pStr = m_curCom->Param[i];
				switch( pStr[1] )
				{
				case 'd': m_errorMsg += "#int"; break;
				case 'f': m_errorMsg += "#float"; break;
				case 's': m_errorMsg += "#String"; break;
				case 'u': m_errorMsg += "#unsigned"; break;
				case 'l':
					if ( pStr[2] == 'f')
						m_errorMsg += "#double";
					break;
				}
			}
			return NULL;
		}

		char* paramStr = m_paramStr[m_numUsedParam];

		if ( paramStr[0] =='$')
		{
			++paramStr;
			ComBase* cdata = findCommand( paramStr );
			if ( !cdata )
			{
				m_errorMsg = "No match ComVar";
				return NULL;
			}
			if ( !strncmp( cdata->Param[0] , fptr , strlen(cdata->Param[0]) ) )
			{
				cdata->getVal( data );
			}
			else
			{
				m_errorMsg = "ComVar's param is not match function's param";
				return NULL;
			}
		}
		else
		{
			if ( fptr[1] != 's' )
			{
				int num = sscanf_s( paramStr , fptr , data );
				if ( num == 0 )
				{
					m_errorMsg = "param error";
					return NULL;
				}
			}
			else
			{
				m_str[ m_numStr ] = paramStr;
				char** ptr = ( char** ) data;
				*ptr = m_str[m_numStr];
				++m_numStr;
			}
		}

		union
		{
			float* fptr;
			int  * iptr;
			char * cptr;
			void * vptr;
		}debugPtr;

		debugPtr.vptr = data;

		switch( fptr[1] )
		{
		case 'd': data += sizeof(int); break;
		case 'f': data += sizeof(float); break;
		case 's': data += sizeof(char*); break;
		case 'u': data += sizeof(unsigned); break;
		case 'l':
			if ( fptr[2] == 'f')
				data += sizeof(double);
			break;
		}

		++m_numUsedParam;
		++fptr;
	}
	return data;
}

void ConsoleSystem::unregisterByName( char const* name )
{
	CommandMap::iterator iter = mNameMap.find( name );
	if ( iter != mNameMap.end() )
	{
		delete iter->second;
	}
	mNameMap.erase( iter );
}

ComBase* ConsoleSystem::findCommand( char const* str )
{
	CommandMap::iterator iter = mNameMap.find( str );
	if ( iter == mNameMap.end() )
		return NULL;
	return iter->second;
}

void ConsoleSystem::insertCommand( ComBase* com )
{
	std::pair<CommandMap::iterator,bool> result =
		mNameMap.insert( std::make_pair( com->name.c_str() , com ) );
	if ( !result.second )
		delete com;
}
