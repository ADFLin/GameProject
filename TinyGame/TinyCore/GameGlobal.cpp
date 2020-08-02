#include "TinyGamePCH.h"
#include "GameGlobal.h"

#include "PropertySet.h"
#include "DrawEngine.h"
#include "Random.h"
#include "GameModuleManager.h"
#include "GameGUISystem.h"
#include "UserProfile.h"
#include "DataCacheInterface.h"
#include "PlatformThread.h"
#include "Asset.h"

#include <cstdlib>


TINY_API IGameNetInterface* GGameNetInterfaceImpl = nullptr;
TINY_API IDebugInterface* GDebugInterfaceImpl = nullptr;
TINY_API uint32 GGameThreadId = 0;

DataCacheInterface* GGameDataCache = nullptr;

bool IsInGameThead()
{
	return PlatformThread::GetCurrentThreadId() == GGameThreadId;
}

uint64 generateRandSeed()
{
	uint64 result = __rdtsc();
	return result;
}


static int GRandCount = 0;
using Random::Well512;
static Well512 GWellRng;

void Global::Initialize()
{
	GGameThreadId = PlatformThread::GetCurrentThreadId();
	GGameDataCache = DataCacheInterface::Create("DataCache");
}

void Global::Finalize()
{
	GGameDataCache->release();
	GGameDataCache = nullptr;
}

int Global::RandomNet()
{
	++GRandCount;
	//Msg("rand count = %d" ,g_RandCount );
	int rt =  GWellRng.rand();
	if ( rt < 0 )
		rt = -rt;
	return rt;
}

void Global::RandSeedNet( uint64 seed )
{
	::srand( (unsigned)seed );

	uint32 s[ 16 ];
	for( int i = 0 ; i < 16 ; ++i )
		s[i] = rand();

	GWellRng.init( s );
}

int Global::Random()
{
	++GRandCount;
	//Msg("rand count = %d" ,GRandCount );

	return std::rand();
}

void Global::RandSeed(unsigned seed )
{
	std::srand( seed );
}


Vec2i Global::GetScreenSize()
{
	return GetDrawEngine().getScreenSize();
}

PropertySet& Global::GameConfig()
{
	static PropertySet settingKey;
	return settingKey;
}

DrawEngine& Global::GetDrawEngine()
{
	static DrawEngine drawEngine;
	return drawEngine;
}

Graphics2D& Global::GetGraphics2D()
{
	return GetDrawEngine().getPlatformGraphics();
}

RHIGraphics2D& Global::GetRHIGraphics2D()
{
	return GetDrawEngine().getRHIGraphics();
}

IGraphics2D& Global::GetIGraphics2D()
{
	return GetDrawEngine().getIGraphics();
}

GameModuleManager& Global::ModuleManager()
{
	static GameModuleManager manager;
	return manager;
}

IGameInstance* Global::GameInstacne()
{
	return nullptr;
}

AssetManager& Global::GetAssetManager()
{
	static AssetManager manager;
	return manager;
}

IGameNetInterface& Global::GameNet()
{
	assert(GGameNetInterfaceImpl);
	return *GGameNetInterfaceImpl;
}

IDebugInterface& Global::Debug()
{
	assert(GDebugInterfaceImpl);
	return *GDebugInterfaceImpl;
}

GUISystem& Global::GUI()
{
	static GUISystem system;
	return system;
}

UserProfile& Global::GetUserProfile()
{
	static UserProfile profile;
	return profile;
}

DataCacheInterface& Global::DataCache()
{
	return *GGameDataCache;
}

