#include "ConsoleSystem.h"

#include "MemorySecurity.h"
#include "LogSystem.h"
#include <cstdlib>
#include <malloc.h>


#define STRING_BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 1024


ConsoleCommandBase::ConsoleCommandBase(char const* inName, TArrayView< ConsoleArgTypeInfo const > inArgs)
	:name(inName)
	,args(inArgs)
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
	mNameMap.clear();
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
		LogMsg("Com : \"%s\"", comStr);
	}
	else
	{
		LogMsg("Com : Fail \"%s\" : %s", comStr, mLastErrorMsg.c_str());
	}
	return result;
}


ConsoleCommandBase* ConsoleSystem::findCommand(char const* str)
{
	CommandMap::iterator iter = mNameMap.find(str);
	if (iter == mNameMap.end())
		return nullptr;
	return iter->second;
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

#if 0
	if( context.command->args.size() != context.numArgs )
	{
		mLastErrorMsg = "Arg number is not match";
		return false;
	}
#endif

	int totalArgSize = 0;
	for (auto const& arg : context.command->args)
	{
		totalArgSize += arg.size;
	}

#if 0
	uint8* dataBuffer = (uint8*)alloca(totalArgSize);
#else
	uint8 dataBuffer[DATA_BUFFER_SIZE];
#endif
	void*  argData[NumMaxParams];
	assert(context.command->args.size() <= NumMaxParams);
	uint8* pData = dataBuffer;
	for( int argIndex = 0; argIndex < context.command->args.size(); ++argIndex )
	{
		ConsoleArgTypeInfo const& arg = context.command->args[argIndex];

		if (!fillParameterData(context, arg, pData))
			return false;

		argData[argIndex] = pData;
		pData += arg.size;
	}
	context.command->execute(argData);

	return true;
}

bool ConsoleSystem::fillParameterData(ExecuteContext& context, ConsoleArgTypeInfo const& arg, uint8* outData)
{
	char const* fptr = arg.format;
	int fillSize = 0;
	while( 1 )
	{

		while( fptr[0] != '%' &&
			   fptr[0] != '\0')
		{
			++fptr;
		}

		if ( *fptr =='\0' )
			break;

		if ( context.numUsedParam >= context.numArgs )
		{
			mLastErrorMsg = "less param : ";
			mLastErrorMsg += context.command->name;
			for ( int i = 0 ; i < context.command->args.size() ; ++ i )
			{
				mLastErrorMsg += std::string(" ");

				char const* pStr = context.command->args[i].format;
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
			if ( !cdata || cdata->args.size() != 1)
			{
				mLastErrorMsg = "No match ComVar";
				return 0;
			}
			if ( !strncmp( cdata->args[0].format , fptr , strlen(cdata->args[0].format) ) )
			{
				cdata->getValue( outData );
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
				int num = sscanf_s( paramString , fptr , outData );
				if ( num == 0 )
				{
					mLastErrorMsg = "param error";
					return 0;
				}
			}
			else
			{
				char const** ptr = ( char const** )outData;
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

		debugPtr.vptr = outData;

		int elementSize = 0;
		switch( fptr[1] )
		{
		case 'd': elementSize = sizeof(int); break;
		case 'f': elementSize = sizeof(float); break;
		case 's': elementSize = sizeof(char*); break;
		case 'u': elementSize = sizeof(unsigned); break;
		case 'l':
			if ( fptr[2] == 'f')
				elementSize = sizeof(double);
			break;
		}

		outData += elementSize;
		fillSize += elementSize;

		++context.numUsedParam;
		++fptr;
	}

	return fillSize != 0;
}


#include "StringParse.h"

bool ConsoleSystem::ExecuteContext::init(char const* inCommandString)
{
	numArgs = 0;
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
		paramStrings[ numArgs ] = token.data();
		++numArgs;
	}

	return true;
}
