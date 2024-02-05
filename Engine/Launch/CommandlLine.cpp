#include "CommandlLine.h"
#include "StringParse.h"
#include "Core/Memory.h"
#include "CompilerConfig.h"
#include "PlatformConfig.h"
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
