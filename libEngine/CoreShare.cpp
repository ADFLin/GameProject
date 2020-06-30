#include "CoreShare.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "HashString.h"
#include "ConsoleSystem.h"


#include "RHI/MeshImportor.h"

void EngineInitialize()
{

}

void EngineFinalize()
{
	Render::MeshImporterRegistry::Get().cleanup();
}

void CoreShareInitialize()
{
	HashString::Initialize();

	ConsoleSystem::Get().initialize();
}

void CoreShareFinalize()
{
	ConsoleSystem::Get().finalize();
}

