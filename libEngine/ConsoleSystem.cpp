#include "ConsoleSystem.h"

#include "MemorySecurity.h"
#include "LogSystem.h"
#include <cstdlib>


#define STRING_BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 1024


ConsoleCommandBase::ConsoleCommandBase(char const* inName, int inNumParam, char const** inParmFormat)
	:name(inName)
	,numParam(inNumParam)
	,paramFormat(inParmFormat)
{

}

ConsoleSystem::ConsoleSystem()
{

}

ConsoleSystem::~ConsoleSystem()
{

}


#if CORE_SHARE_CODE


ConsoleSystem& ConsoleSystem::Get()
{
	static ConsoleSystem sInstance;
	return sInstance;
}

bool ConsoleSystem::initialize()
{
	mbInitialized = true;
	return true;
}

void ConsoleSystem::finalize()
{
	mbInitialized = false;
	for( CommandMap::iterator iter = mNameMap.begin();
		iter != mNameMap.end(); ++iter )
	{
		delete iter->second;
	}
}


int ConsoleSystem::findCommandName2( char const* includeStr , char const** findStr , int maxNum )
{
	int findNum = 0;

	size_t len = strlen( includeStr );
	for( CommandMap::iterator iter = mNameMap.begin();
		iter != mNameMap.end() ; ++iter )
	{
		if ( strnicmp( iter->first , includeStr , len ) == 0 )
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

void ConsoleSystem::unregisterCommand(ConsoleCommandBase* commond)
{
	unregisterCommandByName(commond->name.c_str());
}

void ConsoleSystem::unregisterCommandByName(char const* name)
{
	CommandMap::iterator iter = mNameMap.find(name);
	if( iter != mNameMap.end() )
	{
		delete iter->second;
	}
	mNameMap.erase(iter);
}
void ConsoleSystem::insertCommand(ConsoleCommandBase* com)
{
	std::pair<CommandMap::iterator, bool> result =
		mNameMap.insert(std::make_pair(com->name.c_str(), com));
	if( !result.second )
		delete com;
}


bool ConsoleSystem::executeCommand(char const* comStr)
{
	bool result = executeCommandImpl(comStr);
	if( result )
	{
		::LogMsg("Com : \"%s\"", comStr);
	}
	else
	{
		::LogMsg("Com : Fail \"%s\" : %s", comStr, mLastErrorMsg.c_str());
	}
	return result;
}

#endif //CORE_SHARE_CODE

bool ConsoleSystem::executeCommandImpl(char const* comStr)
{
	ExecuteContext context;

	if( !context.init(comStr) )
		return false;

	context.command = findCommand(context.commandString);
	if( context.command == nullptr )
	{
		mLastErrorMsg = "Unknown executeCommand";
		return false;
	}

	if( context.command->numParam != context.numParam )
	{
		mLastErrorMsg = "Parameter number is not match";
		return false;
	}

	void* comParamData[NumMaxParams];
	uint8 dataBuffer[DATA_BUFFER_SIZE];
	uint8* pData = dataBuffer;
	for( int i = 0; i < context.command->numParam; ++i )
	{
		char const* format = context.command->paramFormat[i];
		int num = fillParameterData(context, pData, format);
		if( num == 0 )
			return false;

		comParamData[i] = pData;
		pData += num;
	}
	context.command->execute(comParamData);

	return true;
}

int  ConsoleSystem::fillParameterData(ExecuteContext& context , uint8* data , char const* format)
{
	char const* fptr = format;
	int result = 0;
	while( 1 )
	{
		while( fptr[0] != '%' &&
			   fptr[0] != '\0')
		{
			++fptr;
		}

		if ( *fptr =='\0' )
			break;

		if ( context.numUsedParam >= context.numParam )
		{
			mLastErrorMsg = "less param : ";
			mLastErrorMsg += context.command->name;
			for ( int i = 0 ; i < context.command->numParam ; ++ i )
			{
				mLastErrorMsg += std::string(" ");

				char const* pStr = context.command->paramFormat[i];
				switch( pStr[1] )
				{
				case 'd': mLastErrorMsg += "#int"; break;
				case 'f': mLastErrorMsg += "#float"; break;
				case 's': mLastErrorMsg += "#String"; break;
				case 'u': mLastErrorMsg += "#unsigned"; break;
				case 'l':
					if ( pStr[2] == 'f')
						mLastErrorMsg += "#double";
					break;
				}
			}
			return 0;
		}

		char const* paramString = context.paramStrings[context.numUsedParam];

		if ( paramString[0] =='$')
		{
			++paramString;
			ConsoleCommandBase* cdata = findCommand( paramString );
			if ( !cdata )
			{
				mLastErrorMsg = "No match ComVar";
				return 0;
			}
			if ( !strncmp( cdata->paramFormat[0] , fptr , strlen(cdata->paramFormat[0]) ) )
			{
				cdata->getValue( data );
			}
			else
			{
				mLastErrorMsg = "ComVar's param is not match function's param";
				return 0;
			}
		}
		else
		{
			if ( fptr[1] != 's' )
			{
				int num = sscanf_s( paramString , fptr , data );
				if ( num == 0 )
				{
					mLastErrorMsg = "param error";
					return 0;
				}
			}
			else
			{
				char const** ptr = ( char const** ) data;
				*ptr = paramString;
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
		case 'd': result += sizeof(int); break;
		case 'f': result += sizeof(float); break;
		case 's': result += sizeof(char*); break;
		case 'u': result += sizeof(unsigned); break;
		case 'l':
			if ( fptr[2] == 'f')
				result += sizeof(double);
			break;
		}

		++context.numUsedParam;
		++fptr;
	}
	return result;
}



ConsoleCommandBase* ConsoleSystem::findCommand( char const* str )
{
	CommandMap::iterator iter = mNameMap.find( str );
	if ( iter == mNameMap.end() )
		return nullptr;
	return iter->second;
}

#include "StringParse.h"

bool ConsoleSystem::ExecuteContext::init(char const* inCommandString)
{
	numParam = 0;
	numUsedParam = 0;
	buffer = inCommandString;
	StringView token;
	char const* data = buffer;
	if( !FStringParse::StringToken(data, " ", token) )
		return false;

	if( *data != 0 )
	{
		*const_cast<char*>(token.data() + token.length()) = 0;
		++data;
	}
	commandString = token.data();

	while( FStringParse::StringToken(data, " ", token))
	{
		if( *data != 0 )
		{
			*const_cast<char*>(token.data() + token.length()) = 0;
			++data;
		}
		paramStrings[ numParam ] = token.data();
		++numParam;
	}

	return true;
}
