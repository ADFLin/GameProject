#include "CommandlLine.h"

#include "CompilerConfig.h"
#include "PlatformConfig.h"

#include "StringParse.h"
#include "Core/Memory.h"
#include "Core/StringConv.h"

#include <cstdlib>
#include <algorithm>


#if CORE_SHARE_CODE

namespace
{
	TChar GCmdLine[1024];

#if SYS_PLATFORM_WIN

#else
	TChar GCmdArgBuffer[1024];
	TChar const* GArgv[128];
	int    GArgc = 0;
#endif
}

void FCommandLine::Initialize(TChar const* initStr)
{
	FCString::Copy(GCmdLine, initStr);

#if SYS_PLATFORM_WIN
	return;
#else
	if (FCString::Strlen(initStr) >= GCmdArgBuffer)
	{
		LogWarning(0, "CommandLine lenght large than CmdArgBuffer %d", ARRAY_SIZE(GCmdArgBuffer));
		return;
	}
	GArgc = Parse(initStr, GCmdArgBuffer, ARRAY_SIZE(GCmdArgBuffer), GArgv, ARRAY_SIZE(GArgv));
	if (GArgc == -1)
	{
		LogWarning(0, "CommandLine Args Count large than MaxArgv %d", ARRAY_SIZE(GArgv));
		GArgc = 0;
	}
#endif
}

TChar const* FCommandLine::Get()
{
	return GCmdLine;
}

TChar const** FCommandLine::GetArgs(int& outNumArg)
{
#if SYS_PLATFORM_WIN
	outNumArg = __argc;
	return (TChar const**)__argv;
#else
	outNumArg = GArgc;
	return GArgv;
#endif
}
#endif


int FCommandLine::Parse(TChar const* content, TChar buffer[], int bufferLen, TChar const* outArgs[], int maxNumArgs)
{
	TChar* cmdStart = buffer;
	TChar* cmdCur = cmdStart;

#define WRITE_BUFFER(c)\
	if (cmdCur - buffer >= bufferLen)\
		return -1;\
	*(cmdCur++) = c;

	bool bInQuote = false;
	int argc = 0;
	for (TChar const* token = content; *token != 0; ++token)
	{
		char c = *token;
		switch (c)
		{
		case TSTR(' '):
			if (bInQuote)
			{
				WRITE_BUFFER(c);
			}
			else if (cmdCur != cmdStart)
			{
				if (argc >= maxNumArgs)
					return -1;

				WRITE_BUFFER(0);
				outArgs[argc++] = cmdStart;
				cmdStart = cmdCur;
			}
			break;
		case TSTR('\"'):
			if (token[1] == TSTR('\"'))
			{
				WRITE_BUFFER('\"');
				++token;
			}
			else
			{
				bInQuote = !bInQuote;
			}
			break;
		default:
			{
				WRITE_BUFFER(c);
			}
			break;
		}
	}

	if (cmdCur != cmdStart)
	{
		if (argc >= maxNumArgs)
			return -1;

		WRITE_BUFFER(0);
		outArgs[argc++] = cmdStart;
	}

	return argc;

#undef WRITE_BUFFER
}

//========================================================================
// CommandLineArgs Implementation
//========================================================================

CommandLineArgs::CommandLineArgs()
{
	parse(FCommandLine::Get());
}

CommandLineArgs::CommandLineArgs(TChar const* initStr)
{
	if (initStr)
	{
		parse(initStr);
	}
}

CommandLineArgs::CommandLineArgs(int argc, TChar const* argv[])
{
	parseArgs(argc, argv);
}

void CommandLineArgs::parse(TChar const* cmdLine)
{
	if (!cmdLine || *cmdLine == 0)
		return;
	
	TChar buffer[1024];
	TChar const* args[128];
	int argc = FCommandLine::Parse(cmdLine, buffer, sizeof(buffer), args, 128);
	
	if (argc > 0)
	{
		parseArgs(argc, args);
	}
}

void CommandLineArgs::parseArgs(int argc, TChar const* argv[])
{
	mArgs.clear();
	mKeyValues.clear();
	mFlags.clear();
	mPositionalArgs.clear();
	
	for (int i = 0; i < argc; ++i)
	{
		TChar const* arg = argv[i];
		if (!arg || *arg == 0)
			continue;
		
		mArgs.push_back(arg);
		
		// Check if this is a flag/option (starts with - or --)
		if (arg[0] == TSTR('-'))
		{
			TChar const* key = arg;
			// Skip leading dashes
			while (*key == TSTR('-'))
				++key;
			
			if (*key == 0)
				continue;
			
			// Check for -key=value format
			TChar const* equalSign = FCString::Strchr(key, TSTR('='));
			if (equalSign)
			{
				TString keyStr(key, equalSign - key);
				TString valueStr(equalSign + 1);
				mKeyValues[keyStr] = valueStr;
			}
			// Check if next argument is a value (doesn't start with -)
			else if (i + 1 < argc && argv[i + 1] && argv[i + 1][0] != TSTR('-'))
			{
				mKeyValues[key] = argv[i + 1];
				++i; // Skip the value argument
				mArgs.push_back(argv[i]);
			}
			else
			{
				// It's a boolean flag
				mFlags.push_back(key);
			}
		}
		else
		{
			// Positional argument
			mPositionalArgs.push_back(arg);
		}
	}
}

bool CommandLineArgs::hasFlag(TChar const* flag) const
{
	if (!flag)
		return false;
	
	// Skip leading dashes for comparison
	while (*flag == TSTR('-'))
		++flag;
	
	// Check in flags list
	for (auto const& f : mFlags)
	{
		if (FCString::CompareIgnoreCase(f.c_str(), flag) == 0)
			return true;
	}
	
	// Also check in key-values (flag with value counts as having flag)
	auto it = mKeyValues.find(flag);
	return it != mKeyValues.end();
}

bool CommandLineArgs::hasValue(TChar const* key) const
{
	if (!key)
		return false;
	
	// Skip leading dashes
	while (*key == TSTR('-'))
		++key;
	
	auto it = mKeyValues.find(key);
	return it != mKeyValues.end();
}

TChar const* CommandLineArgs::getValue(TChar const* key, TChar const* defaultValue) const
{
	if (!key)
		return defaultValue;
	
	// Skip leading dashes
	while (*key == TSTR('-'))
		++key;
	
	auto it = mKeyValues.find(key);
	if (it != mKeyValues.end())
	{
		return it->second.c_str();
	}
	return defaultValue;
}

int CommandLineArgs::getIntValue(TChar const* key, int defaultValue) const
{
	TChar const* value = getValue(key, nullptr);
	if (value)
	{
		return FStringConv::To<int>(value);
	}
	return defaultValue;
}

float CommandLineArgs::getFloatValue(TChar const* key, float defaultValue) const
{
	TChar const* value = getValue(key, nullptr);
	if (value)
	{
		return (float)FStringConv::To<float>(value);
	}
	return defaultValue;
}

