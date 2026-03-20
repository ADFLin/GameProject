#include "CoreShare.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "HashString.h"
#include "ConsoleSystem.h"


#include "Renderer/MeshImportor.h"
#include "Module/ModuleManager.h"
#include "RHI/Font.h"

void EngineInitialize()
{
	ModuleManager::Initialize();

	IConsoleSystem::Get().initialize();

	Render::FontCharCache::Get().initialize();

}

void EngineFinalize()
{
	Render::FontCharCache::Get().finalize();
	Render::MeshImporterRegistry::Get().cleanup();

	IConsoleSystem::Get().finalize();
	ModuleManager::Finalize();
}

void CoreShareInitialize()
{
	HashString::Initialize();


}

void CoreShareFinalize()
{

}

