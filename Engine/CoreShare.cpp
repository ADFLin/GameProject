#include "CoreShare.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "HashString.h"
#include "ConsoleSystem.h"


#include "Renderer/MeshImportor.h"
#include "Module/ModuleManager.h"

void EngineInitialize()
{
	ModuleManager::Initialize();

	ConsoleSystem::Get().initialize();
}

void EngineFinalize()
{
	ConsoleSystem::Get().finalize();

	Render::MeshImporterRegistry::Get().cleanup();

	ModuleManager::Finalize();
}

void CoreShareInitialize()
{
	HashString::Initialize();


}

void CoreShareFinalize()
{

}

