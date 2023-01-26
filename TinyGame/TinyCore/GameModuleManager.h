#pragma once
#ifndef GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
#define GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E

#include "GameModule.h"
#include "Platform/PlatformModule.h"

#include <map>

typedef std::vector< IGameModule* > GameModuleVec;

using ModuleHandle = FPlatformModule::Handle;

class GameModuleManager
{
public:
	GameModuleManager();
	~GameModuleManager();

	TINY_API void         cleanupModuleInstances();
	TINY_API void         cleanupModuleMemory();
	TINY_API void         classifyGame( int attrID , GameModuleVec& games );
	TINY_API IGameModule* changeGame( char const* name );
	TINY_API bool         changeGame(IGameModule* game);

	IGameModule*  getRunningGame(){ return mGameRunning; }

private:

	template< class Visitor >
	void  visitGameInternal(Visitor&& visitor)
	{
		for (auto* gameMdoule : mGameModules)
		{
			if (!visitor(gameMdoule))
				return;
		}
	}

	void handleFeatureEvent(IModularFeature* feature, bool bRemove);

	std::vector< IGameModule* > mGameModules;
	IGameModule*     mGameRunning;
};


#endif // GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
