#include "CoreShare.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "HashString.h"
#include "ConsoleSystem.h"


void CoreShareInitialize()
{
	HashString::Initialize();

	ConsoleSystem::Get().initialize();
}

void CoreShareFinalize()
{
	ConsoleSystem::Get().finalize();
}

