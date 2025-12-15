#include "ConsoleSystem.h"

#include "PlatformConfig.h"
#include "MemorySecurity.h"
#include "LogSystem.h"
#include "StringParse.h"
#include "Launch/CommandlLine.h"
#include "FileSystem.h"

#include "InlineString.h"
#include "Misc/CStringWrapper.h"
#include "Core/StringConv.h"

#include <cstdlib>
#include <malloc.h>

#include <map>
#include <unordered_map>

#define STRING_BUFFER_SIZE 256
#define DATA_BUFFER_SIZE 1024

ConsoleCommandBase::ConsoleCommandBase(char const* inName, TArrayView< ConsoleArgTypeInfo const > inArgs, uint32 flags)
	:mName(inName)
	,mFlags(flags)
{

}


class ConsoleSystem : public IConsoleSystem
{
public:
	ConsoleSystem();
	~ConsoleSystem();

	bool        initialize();
	void        finalize();

	bool        executeCommand(char const* inCmdText);
	int         getAllCommandNames(char const* buffer[], int bufSize);
	int         findCommandName(char const* includeStr, char const** findStr, int maxNum);
	int         findCommandName2(char const* includeStr, char const** findStr, int maxNum);

	void        unregisterCommand(ConsoleCommandBase* commond);
	void        unregisterCommandByName(char const* name);
	void        unregisterAllCommandsByObject(void* objectPtr);

	ConsoleCommandBase* findCommand(char const* comName);


	bool       insertCommand(ConsoleCommandBase* com);
	bool       registerAlias(char const* aliasName, char const* commandName);

	template < class T >
	auto* registerVar(char const* name, T* obj)
	{
		auto* command = new TVariableConsoleCommad<T>(name, obj);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc, class T >
	auto* registerCommand(char const* name, TFunc func, T* obj, uint32 flags = 0)
	{
		auto* command = new TMemberFuncConsoleCommand<TFunc, T >(name, func, obj, flags);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc >
	auto* registerCommand(char const* name, TFunc func, uint32 flags = 0)
	{
		auto* command = new TBaseFuncConsoleCommand<TFunc>(name, func, flags);
		if (!insertCommand(command))
			return decltype(command)(nullptr);
		return command;
	}

	template < class TFunc >
	void  visitAllVariables(TFunc&& visitFunc)
	{
		for (auto const& pair : mNameMap)
		{
			auto variable = pair.second->asVariable();
			if (variable)
			{
				visitFunc(variable);
			}
		}
	}


	void  visitAllVariables(std::function<void(VariableConsoleCommadBase*)> func)
	{
		for (auto const& pair : mNameMap)
		{
			auto variable = pair.second->asVariable();
			if (variable)
			{
				func(variable);
			}
		}
	}
protected:
	void unregisterAliases(ConsoleCommandBase* command);
	void cleanupCommands();
	static int const NumMaxParams = 16;
	struct ExecuteContext
	{
		TArray<char> buffer;
		ConsoleCommandBase* command;

		char const* text;
		char const* name;
		char const* args[NumMaxParams];
		int  numArgs;
		int  numArgUsed;
		bool parseText();

		std::string errorMsg;
	};

	bool executeCommandImpl(ExecuteContext& context);

	using CommandMap = std::unordered_map< TCStringWrapper<char, true>, ConsoleCommandBase* >;
	CommandMap   mNameMap;
	CommandMap   mAliasMap;
	friend class ConsoleCommandBase;
};


#if CORE_SHARE_CODE

IConsoleSystem& IConsoleSystem::Get()
{
	static ConsoleSystem sInstance;
	return sInstance;
}

#endif //CORE_SHARE_CODE

ConsoleSystem::ConsoleSystem()
{

}

ConsoleSystem::~ConsoleSystem()
{

}

bool ConsoleSystem::initialize()
{
	mbInitialized = true;
	return true;
}

void ConsoleSystem::finalize()
{
	mbInitialized = false;
	cleanupCommands();
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
		if (FCString::StrIStr((char const*)pair.first, includeStr) != NULL)
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
		unregisterAliases(iter->second);

		ConsoleCommandBase* command = iter->second;
		mNameMap.erase(iter);
		delete command;
	}
}

void ConsoleSystem::unregisterAllCommandsByObject(void* objectPtr)
{
	CommandMap::iterator iter = mNameMap.begin();
	while (iter != mNameMap.end())
	{
		ConsoleCommandBase* command = iter->second;

		if (command->isHoldObject(objectPtr))
		{
			unregisterAliases(command);

			iter = mNameMap.erase(iter);
			delete command;
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

bool ConsoleSystem::registerAlias(char const* aliasName, char const* commandName)
{
	ConsoleCommandBase* command = findCommand(commandName);
	if (!command)
	{
		return false;
	}

	if (findCommand(aliasName))
	{
		LogWarning(0, "Alias %s is already defined", aliasName);
		return false;
	}

	mAliasMap.insert(std::make_pair(aliasName, command));
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
	if (iter != mNameMap.end())
		return iter->second;

	iter = mAliasMap.find(str);
	if (iter != mAliasMap.end())
		return iter->second;

	return nullptr;
}

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

	// Convert args to StringView view
	TArray<StringView> viewArgs;
	viewArgs.reserve(context.numArgs);
	for (int i = 0; i < context.numArgs; ++i)
	{
		viewArgs.push_back(StringView(context.args[i]));
	}

	LogMsg("Execute Com : \"%s\"", context.text);

	if (!context.command->execute(viewArgs))
	{
		std::string usageStr = "Usage : ";
		usageStr += context.command->mName;

		for (int i = 0; i < context.command->mArgs.size(); ++i)
		{
			usageStr += std::string(" ");
			char const* pStr = context.command->mArgs[i].format;
			
			// Handle multi-component formats like %f%f
			do 
			{
				// pStr points to start, likely '%', check next char
				if (*pStr == '%')
				{
					switch (pStr[1])
					{
					case 'd': usageStr += "#int"; break;
					case 'f': usageStr += "#float"; break;
					case 's': usageStr += "#String"; break;
					case 'u': usageStr += "#uint"; break;
					case 'c': usageStr += "#char"; break;
					case 'h':
						if (pStr[2] == 'u') usageStr += "#uint16";
						else if (pStr[2] == 'd') usageStr += "#int16";
						break;
					case 'l':
						if (pStr[2] == 'f') usageStr += "#double";
						break;
					}
				}
				
				pStr = FStringParse::FindChar(pStr + 1, '%');
			}
			while( *pStr != 0 );
		}
		LogWarning(0, "%s", usageStr.c_str());

		context.errorMsg = "Execute Failed";
		return false;
	}

	return true;
}

void ConsoleSystem::unregisterAliases(ConsoleCommandBase* command)
{
	// Remove any aliases pointing to this command
	for (auto itAlias = mAliasMap.begin(); itAlias != mAliasMap.end(); )
	{
		if (itAlias->second == command)
		{
			itAlias = mAliasMap.erase(itAlias);
		}
		else
		{
			++itAlias;
		}
	}
}

void ConsoleSystem::cleanupCommands()
{
	for (CommandMap::iterator iter = mNameMap.begin();
		iter != mNameMap.end(); ++iter)
	{
		delete iter->second;
	}

	mNameMap.clear();
	mAliasMap.clear();
}



bool ConsoleSystem::ExecuteContext::parseText()
{
	numArgs = 0;
	numArgUsed = 0;

	buffer.resize(FCString::Strlen(text) + 1);

	char const* tempText = text;
	StringView token;
	if (!FStringParse::StringToken(tempText, " ", token))
		return false;

	FCString::CopyN_Unsafe(buffer.data(), token.data(), token.length());
	buffer[token.length()] = 0;
	name = buffer.data();
	int offset = token.length() + 1;
	numArgs = FCommandLine::Parse(tempText, buffer.data() + offset, buffer.size() - offset, args, ARRAY_SIZE(args));

	if (numArgs < 0)
		return false;

	return true;
}

bool FConsole::ExecuteScript(char const* path)
{
	std::vector< std::string > lines;
	if (!FFileUtility::LoadLines(path, lines))
		return false;

	for(auto const& line : lines)
	{
		IConsoleSystem::Get().executeCommand(line.c_str());
	}
	return true;
}
