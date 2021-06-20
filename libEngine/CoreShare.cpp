#include "CoreShare.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "HashString.h"
#include "ConsoleSystem.h"


#include "Renderer/MeshImportor.h"

void EngineInitialize()
{
	ConsoleSystem::Get().initialize();
}

void EngineFinalize()
{
	ConsoleSystem::Get().finalize();

	Render::MeshImporterRegistry::Get().cleanup();
}

void CoreShareInitialize()
{
	HashString::Initialize();


}

void CoreShareFinalize()
{

}

