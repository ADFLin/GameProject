#include "TinyGamePCH.h"
#include "GameGlobal.h"

#include "PropertyKey.h"
#include "DrawEngine.h"
#include "Random.h"
#include "GameModuleManager.h"
#include "GameGUISystem.h"
#include "UserProfile.h"
#include "Asset.h"

#include "PlatformThread.h"

#include <cstdlib>

TINY_API IGameNetInterface* gGameNetInterfaceImpl = nullptr;
TINY_API IDebugInterface* gDebugInterfaceImpl = nullptr;
TINY_API uint32 gGameThreadId = 0;

bool IsInGameThead()
{
	return PlatformThread::GetCurrentThreadId() == gGameThreadId;
}

uint64 generateRandSeed()
{
	uint64 result = __rdtsc();
	return result;
}


static int g_RandCount = 0;
using Random::Well512;
static Well512 gWellRng;

int Global::RandomNet()
{
	++g_RandCount;
	//Msg("rand count = %d" ,g_RandCount );
	int rt =  gWellRng.rand();
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

	gWellRng.init( s );
}

int Global::Random()
{
	++g_RandCount;
	//Msg("rand count = %d" ,g_RandCount );

	return std::rand();
}

void Global::RandSeed(unsigned seed )
{
	std::srand( seed );
}


PropertyKey& Global::GameConfig()
{
	static PropertyKey settingKey;
	return settingKey;
}

DrawEngine* Global::GetDrawEngine()
{
	static DrawEngine drawEngine;
	return &drawEngine;
}

Graphics2D& Global::GetGraphics2D()
{
	return GetDrawEngine()->getScreenGraphics();
}

GameModuleManager& Global::GameManager()
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
	assert(gGameNetInterfaceImpl);
	return *gGameNetInterfaceImpl;
}

IDebugInterface& Global::Debug()
{
	assert(gDebugInterfaceImpl);
	return *gDebugInterfaceImpl;
}

GUISystem& Global::GUI()
{
	static GUISystem system;
	return system;
}

GLGraphics2D& Global::GetRHIGraphics2D()
{
	return GetDrawEngine()->getGLGraphics();
}

IGraphics2D& Global::GetIGraphics2D()
{
	return GetDrawEngine()->getIGraphics();
}

UserProfile& Global::GetUserProfile()
{
	static UserProfile profile;
	return profile;
}

