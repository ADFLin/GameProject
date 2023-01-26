#include "TinyGamePCH.h"
#include "GameModuleManager.h"
#include "GameControl.h"

#include "FileSystem.h"
#include "Core/ScopeGuard.h"

#include "WindowsHeader.h"
#include "StringParse.h"
#include "StdUtility.h"
#include "Module/ModularFeature.h"
#include "Module/ModuleManager.h"


GameModuleManager::GameModuleManager()
{
	mGameRunning = nullptr;
	IModularFeatures::Get().addEvent(IGameModule::FeatureName, ModularFeatureEvent(this, &GameModuleManager::handleFeatureEvent));
}

GameModuleManager::~GameModuleManager()
{
	IModularFeatures::Get().removeEvent(IGameModule::FeatureName);
}

void GameModuleManager::cleanupModuleInstances()
{
	if ( mGameRunning )
	{
		mGameRunning->exit();
		mGameRunning = nullptr;
	}
	ModuleManager::Get().cleanupModuleInstances();
}

void GameModuleManager::cleanupModuleMemory()
{
	ModuleManager::Get().cleanupModuleMemory();
}

void GameModuleManager::classifyGame( int attrID , GameModuleVec& games )
{
	visitGameInternal( [ attrID , &games ](IGameModule* gameModule)-> bool
	{
		GameAttribute    attrValue(attrID);
		if (gameModule->queryAttribute(attrValue))
		{
			if (attrValue.iVal)
			{
				games.push_back(gameModule);
			}
		}
		return true;
	});
}

void GameModuleManager::handleFeatureEvent(IModularFeature* feature, bool bRemove)
{
	IGameModule* gameModule = static_cast<IGameModule*>(feature);
	if (bRemove)
	{
		RemoveValue(mGameModules, gameModule);
	}
	else
	{
		GameAttribute attrSetting(ATTR_INPUT_DEFUAULT_SETTING);
		gameModule->queryAttribute(attrSetting);
		mGameModules.push_back(gameModule);
	}
}

IGameModule* GameModuleManager::changeGame( char const* name )
{
	IGameModule* result = nullptr;
	visitGameInternal([name, this, & result](IGameModule* gameModule)-> bool
	{
		if (FCString::Compare(name, gameModule->getName()) == 0)
		{
			if (changeGame(gameModule))
			{
				result = gameModule;
				return false;
			}
		}
		return true;
	});
	return result;
}

bool GameModuleManager::changeGame(IGameModule* gameModule)
{
	if( !gameModule )
		return false;

	if( mGameRunning != gameModule )
	{
		if( mGameRunning )
			mGameRunning->exit();

		mGameRunning = gameModule;

		try
		{
			mGameRunning->enter();
		}
		catch( ... )
		{
			mGameRunning = nullptr;
			return false;
		}

	}
	return true;
}
