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

#include "PacketFactory.h"


#include <cstdlib>
#include "ProfileSystem.h"



TINY_API IGameNetInterface* GGameNetInterfaceImpl = nullptr;
TINY_API IDebugInterface* GDebugInterfaceImpl = nullptr;
TINY_API uint32 GGameThreadId = 0;
TINY_API IEditor* GEditor = nullptr;

DataCacheInterface* GGameDataCache = nullptr;


TINY_API PacketFactory GGamePacketFactory;

bool IsInGameThead()
{
	return PlatformThread::GetCurrentThreadId() == GGameThreadId;
}

uint64 GenerateRandSeed()
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
	PlatformThread::SetThreadName(GGameThreadId, "GameThread");
	GGameDataCache = DataCacheInterface::Create("DataCache");
	ProfileSystem::Get().setThreadName(GGameThreadId, "GameThread");
}

void Global::Finalize()
{
	GGameDataCache->release();
	GGameDataCache = nullptr;
}

#include <cstdio>

int Global::RandomNet()
{
	++GRandCount;
	int rt =  GWellRng.rand();
	if ( rt < 0 )
		rt = -rt;
	
	// LogMsg("RandomNet: C=%d Val=%d", GRandCount, rt);
	return rt;
}

void Global::RandSeedNet( uint64 seed )
{
	LogMsg("RandSeedNet: Seed=%llu", seed);
	::srand( (unsigned)seed );

	uint32 s[ 16 ];
	for( int i = 0 ; i < 16 ; ++i )
		s[i] = rand();

	GWellRng.init( s );
	GRandCount = 0;
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

RHIGraphics2D& Global::GetRHIGraphics2D_RenderThread()
{
	return GetDrawEngine().getRHIGraphics_RenderThread();
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

IEditor* Global::Editor()
{
#if TINY_WITH_EDITOR
	return GEditor;
#else
	return nullptr;
#endif
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