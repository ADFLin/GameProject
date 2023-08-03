#include "ConsoleSystem.h"

#include "MemorySecurity.h"
#include "LogSystem.h"
#include "StringParse.h"
#include "Launch/CommandlLine.h"

#include <cstdlib>
#include <malloc.h>

#define STRING_BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 1024

ConsoleCommandBase::ConsoleCommandBase(char const* inName, TArrayView< ConsoleArgTypeInfo const > inArgs, uint32 flags)
	:mName(inName)
	,mArgs(inArgs)
	,mFlags(flags)
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


int ConsoleSystem::getAllCommandNames(char const* buffer[], int bufSize)
{
	int count = 0;
	for (auto& pair : mNameMap )
	{
		buffer[count] = pair.first;
		++count;
		if (count == bufSize)
		{
			break;
		}
	}
	return count;
}


int ConsoleSystem::findCommandName2( char const* includeStr , char const** findStr , int maxNum )
{
	int findNum = 0;

	size_t len = FCString::Strlen( includeStr );
	for (auto& pair : mNameMap)
	{
		if ( FCString::CompareIgnoreCaseN( pair.first , includeStr , len ) == 0 )
		{
			findStr[ findNum ] = pair.first;
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
	for (auto& pair : mNameMap)
	{
		if ( FCString::StrStr( pair.first , includeStr ) != NULL )
		{
			findStr[ findNum ] = pair.first;
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

void ConsoleSystem::unregisterAllCommandsByObject(void* objectPtr)
{
	CommandMap::iterator iter = mNameMap.begin();
	while (iter != mNameMap.end())
	{
		ConsoleCommandBase* command = iter->second;

		if (command->isHoldObject(objectPtr))
		{
			iter = mNameMap.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

bool ConsoleSystem::insertCommand(ConsoleCommandBase* com)
{
	std::pair<CommandMap::iterator, bool> result =
		mNameMap.insert(std::make_pair(com->mName.c_str(), com));
	if (!result.second)
	{
		LogWarning(0, "Cmd %s is defined", com->mName.c_str());
		delete com;
		return false;
	}

	return true;
}

bool ConsoleSystem::executeCommand(char const* inCmdText)
{
	ExecuteContext context;
	context.text = inCmdText;
	bool result = executeCommandImpl(context);
	if (!result)
	{
		LogMsg("Com : Fail \"%s\" : %s", inCmdText, context.errorMsg.c_str());
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
	if (!context.parseText())
	{
		context.errorMsg = "parse Command fail";
		return false;
	}

	context.command = findCommand(context.name);
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

	if (context.command->mArgs.size() == 1 && context.numArgs == 0)
	{
		auto var = context.command->asVariable();
		if (var)
		{
			if (var->getFlags() & CVF_TOGGLEABLE)
			{
				if (var->getTypeIndex() == typeid(bool))
				{
					bool val;
					var->getValue(&val);
					val = !val;
					var->setValue(&val);
					LogMsg("Toggle %s to \"%d\"", var->mName.c_str(), (int)val);
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
				LogMsg("Var %s = %s", var->mName.c_str(), var->toString().c_str());
				return true;
			}
		}
	}

	int totalArgSize = 0;
	for (auto const& arg : context.command->mArgs)
	{
		totalArgSize += Math::Max(arg.size, 4);
	}

#if 1
	uint8* dataBuffer = (uint8*)alloca(totalArgSize);
#else
	uint8 dataBuffer[DATA_BUFFER_SIZE];
#endif
	void*  argData[NumMaxParams];
	CHECK(context.command->mArgs.size() <= NumMaxParams);
	uint8* pData = dataBuffer;

	bool bAllowIgnoreArgs = !!(CVF_ALLOW_IGNORE_ARGS & context.command->getFlags() );
	for( int argIndex = 0; argIndex < context.command->mArgs.size(); ++argIndex )
	{
		ConsoleArgTypeInfo const& arg = context.command->mArgs[argIndex];

		if (!fillArgumentData(context, arg, pData, bAllowIgnoreArgs))
		{
			return false;
		}

		argData[argIndex] = pData;
		pData += arg.size;
	}

	LogMsg("Execute Com : \"%s\"", context.text);
	context.command->execute(argData);

	return true;
}

bool ConsoleSystem::fillArgumentData(ExecuteContext& context, ConsoleArgTypeInfo const& arg, uint8* outData, bool bAllowIgnoreArgs)
{
	char const* format = arg.format;
	int fillSize = 0;
	while( 1 )
	{

		while( format[0] != '%' &&
			   format[0] != '\0')
		{
			++format;
		}

		if ( *format =='\0' )
			break;

		char const* argText;

		if ( context.numArgUsed >= context.numArgs )
		{
			if (bAllowIgnoreArgs)
			{
				argText = "0";
			}
			else
			{
				context.errorMsg = "less param : ";
				context.errorMsg += context.name;
				for (int i = 0; i < context.command->mArgs.size(); ++i)
				{
					context.errorMsg += std::string(" ");

					char const* pStr = context.command->mArgs[i].format;
					switch (pStr[1])
					{
					case 'd': context.errorMsg += "#int"; break;
					case 'f': context.errorMsg += "#float"; break;
					case 's': context.errorMsg += "#String"; break;
					case 'u': context.errorMsg += "#uint"; break;
					case 'l':
						if (pStr[2] == 'f')
							context.errorMsg += "#double";
						break;
					}
				}
				return false;
			}
		}
		else
		{
			argText = context.args[context.numArgUsed];
		}


		if ( argText[0] =='$')
		{
			++argText;
			ConsoleCommandBase* argCmd = findCommand( argText );
			if ( !argCmd || argCmd->mArgs.size() != 1)
			{
				context.errorMsg = "No match ComVar";
				return false;
			}
			if ( FCString::CompareN( argCmd->mArgs[0].format , format , FCString::Strlen(argCmd->mArgs[0].format) ) == 0 )
			{
				argCmd->getValue( outData );
			}
			else
			{
				context.errorMsg = "ComVar's param is not match function's param";
				return false;
			}
		}
		else
		{
			if ( format[1] != 's' )
			{
#if CPP_COMPILER_MSVC
				int num = sscanf_s( argText , format , outData );
#else
				int num = sscanf(argText, format, outData);
#endif
				if ( num == 0 )
				{
					context.errorMsg = "arg error";
					return false;
				}
			}
			else
			{
				char const** ptr = ( char const** )outData;
				*ptr = argText;
			}
		}

		union
		{
			float* fptr;
			int  * iptr;
			char * cptr;
			void * vptr;
		}debugView;

		debugView.vptr = outData;

		int elementSize = 0;
		switch( format[1] )
		{
		case 'd': elementSize = sizeof(int32); break;
		case 'f': elementSize = sizeof(float); break;
		case 's': elementSize = sizeof(char*); break;
		case 'u': elementSize = sizeof(uint32); break;
		case 'l':
			if ( format[2] == 'f')
				elementSize = sizeof(double);
			break;
		case 'h':
			if (format[2] == 'u')
				elementSize = sizeof(uint16); 
			break;
		}

		outData += elementSize;
		fillSize += elementSize;

		++context.numArgUsed;
		++format;
	}

	return fillSize != 0;
}

bool ConsoleSystem::ExecuteContext::parseText()
{
	numArgs = 0;
	numArgUsed = 0;
	StringView token;

	char const* tempText = text;
	if (!FStringParse::StringToken(tempText, " ", token))
		return false;

	FCString::CopyN_Unsafe(buffer, token.data(), token.length());
	buffer[token.length()] = 0;
	name = buffer;
	numArgs = FCommandLine::Parse(tempText, buffer + token.length() + 1, args, ARRAY_SIZE(args));

	if (numArgs < 0)
		return false;

	return true;
}
