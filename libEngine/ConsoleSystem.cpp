#include "ConsoleSystem.h"

#include "MemorySecurity.h"
#include "LogSystem.h"
#include "StringParse.h"

#include <cstdlib>
#include <malloc.h>

#define STRING_BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 1024

ConsoleCommandBase::ConsoleCommandBase(char const* inName, TArrayView< ConsoleArgTypeInfo const > inArgs)
	:mName(inName)
	,mArgs(inArgs)
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
		if ( FCString::StrStr( iter->first , includeStr ) != NULL )
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
	unregisterCommandByName(commond->mName.c_str());
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
		mNameMap.insert(std::make_pair(com->mName.c_str(), com));
	if( !result.second )
		delete com;
}

bool ConsoleSystem::executeCommand(char const* comStr)
{
	ExecuteContext context;
	if (!context.init(comStr))
		return false;

	bool result = executeCommandImpl(context);
	if( !result )
	{
		LogMsg("Com : Fail \"%s\" : %s", comStr, context.errorMsg.c_str());
	}
	else
	{
		LogMsg("Execute Com : \"%s\"", comStr);
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

bool ConsoleSystem::executeCommandImpl(ExecuteContext& context)
{

	context.command = findCommand(context.commandString);
	if( context.command == nullptr )
	{
		context.errorMsg = "Unknown executeCommand";
		return false;
	}

#if 0
	if( context.command->args.size() != context.numArgs )
	{
		context.errorMsg = "Arg number is not match";
		return false;
	}
#endif

	int totalArgSize = 0;
	for (auto const& arg : context.command->mArgs)
	{
		totalArgSize += arg.size;
	}

#if 1
	uint8* dataBuffer = (uint8*)alloca(totalArgSize);
#else
	uint8 dataBuffer[DATA_BUFFER_SIZE];
#endif
	void*  argData[NumMaxParams];
	CHECK(context.command->mArgs.size() <= NumMaxParams);
	uint8* pData = dataBuffer;
	for( int argIndex = 0; argIndex < context.command->mArgs.size(); ++argIndex )
	{
		ConsoleArgTypeInfo const& arg = context.command->mArgs[argIndex];

		if (!fillParameterData(context, arg, pData))
		{
			if (context.command->mArgs.size() == 1)
			{
				auto var = context.command->asVariable();
				if (var && ( var->getFlags() & CVF_TOGGLEABLE ) )
				{				
					if (var->getTypeIndex() == typeid(bool))
					{
						bool val;
						var->getValue(&val);
						val = !val;
						var->setValue(&val);
						LogMsg("Toggle %s to \"%d\"", var->mName.c_str() , (int)val);
						return true;
					}
					else if (var->getTypeIndex() == typeid(int))
					{
						int val;
						var->getValue(&val);
						val = !val;
						var->setValue(&val);
						LogMsg("Toggle %s to \"%d\"", var->mName.c_str(), val);
						return true;
					}
				}
				else
				{
					LogMsg("Var %s = %s" , var->mName.c_str() , var->toString().c_str() );
					return true;
				}
			}
			return false;
		}

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
			context.errorMsg = "less param : ";
			context.errorMsg += context.command->mName;
			for ( int i = 0 ; i < context.command->mArgs.size() ; ++ i )
			{
				context.errorMsg += std::string(" ");

				char const* pStr = context.command->mArgs[i].format;
				switch( pStr[1] )
				{
				case 'd': context.errorMsg += "#int"; break;
				case 'f': context.errorMsg += "#float"; break;
				case 's': context.errorMsg += "#String"; break;
				case 'u': context.errorMsg += "#uint"; break;
				case 'l':
					if ( pStr[2] == 'f')
						context.errorMsg += "#double";
					break;
				}
			}
			return 0;
		}

		char const* paramString = context.paramStrings[context.numUsedParam];

		if ( paramString[0] =='$')
		{
			++paramString;
			ConsoleCommandBase* paramCom = findCommand( paramString );
			if ( !paramCom || paramCom->mArgs.size() != 1)
			{
				context.errorMsg = "No match ComVar";
				return 0;
			}
			if ( FCString::CompareN( paramCom->mArgs[0].format , fptr , FCString::Strlen(paramCom->mArgs[0].format) ) == 0 )
			{
				paramCom->getValue( outData );
			}
			else
			{
				context.errorMsg = "ComVar's param is not match function's param";
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
					context.errorMsg = "param error";
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
