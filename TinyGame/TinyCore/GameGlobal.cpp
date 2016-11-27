#include "TinyGamePCH.h"
#include "GameGlobal.h"

#include "PropertyKey.h"
#include "DrawEngine.h"
#include "Random.h"
#include "GamePackageManager.h"
#include "GameGUISystem.h"
#include "UserProfile.h"

#include <cstdlib>

GAME_API IGameNetInterface* gGameNetInterfaceImpl = nullptr;

uint64 generateRandSeed()
{
	uint64 result ;
	__asm
	{
		rdtsc;
		mov dword ptr result , eax;
		mov dword ptr result + 4, eax;
	}
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

	Well512::uint32 s[ 16 ];
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


PropertyKey& Global::GameSetting()
{
	static PropertyKey settingKey;
	return settingKey;
}

DrawEngine* Global::getDrawEngine()
{
	static DrawEngine drawEngine;
	return &drawEngine;
}

Graphics2D& Global::getGraphics2D()
{
	return getDrawEngine()->getScreenGraphics();
}

GamePackageManager& Global::GameManager()
{
	static GamePackageManager manager;
	return manager;
}

IGameNetInterface& Global::GameNet()
{
	assert(gGameNetInterfaceImpl);
	return *gGameNetInterfaceImpl;
}

GUISystem& Global::GUI()
{
	static GUISystem system;
	return system;
}

GLGraphics2D& Global::getGLGraphics2D()
{
	return getDrawEngine()->getGLGraphics();
}

IGraphics2D& Global::getIGraphics2D()
{
	return getDrawEngine()->getIGraphics();
}

UserProfile& Global::getUserProfile()
{
	static UserProfile profile;
	return profile;
}
