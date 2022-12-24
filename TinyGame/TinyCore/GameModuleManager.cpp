#include "TinyGamePCH.h"
#include "GameModuleManager.h"
#include "GameControl.h"

#include "FileSystem.h"
#include "Core/ScopeGuard.h"

#include "WindowsHeader.h"
#include "StringParse.h"

bool GameModuleManager::registerModule( IModuleInterface* module , char const* moduleName, ModuleHandle hModule )
{
	assert(module);

	IGameModule* gameModule = nullptr;
	if( module->isGameModule() )
	{
		gameModule = static_cast<IGameModule*>(module);
	}

	char const* registerName = ( gameModule ) ? gameModule->getName() : moduleName;
	if( findModule(registerName) )
		return false;

	bool bInserted = mNameToModuleMap.insert(std::make_pair(registerName, module)).second;
	assert(bInserted);
	mModuleDataList.push_back({ module ,hModule });

	module->startupModule();

	if( gameModule )
	{
		GameAttribute attrSetting(ATTR_INPUT_DEFUAULT_SETTING);
		gameModule->queryAttribute(attrSetting);
	}

	return true;
}

void GameModuleManager::cleanupModuleInstances()
{
	if ( mGameRunning )
	{
		mGameRunning->exit();
		mGameRunning = nullptr;
	}

	visitInternal([](ModuleData& info) ->bool
	{
		info.instance->shutdownModule();
		info.instance->deleteThis();
		info.instance = nullptr;
		return true;
	});
	mNameToModuleMap.clear();
}

void GameModuleManager::cleanupModuleMemory()
{
	visitInternal([](ModuleData& info) ->bool
	{
		CHECK(info.instance == nullptr);
		FPlatformModule::Release(info.hModule);
		return true;
	});
	mModuleDataList.clear();
}

void GameModuleManager::classifyGame( int attrID , GameModuleVec& games )
{
	visitInternal( [ attrID , &games ](ModuleData& info)-> bool
	{
		GameAttribute    attrValue(attrID);

		if( info.instance->isGameModule() )
		{
			IGameModule* gameModule = static_cast<IGameModule*>(info.instance);
			if( gameModule->queryAttribute(attrValue) )
			{
				if( attrValue.iVal )
				{
					games.push_back(gameModule);
				}
			}
		}
		return true;
	});
}

IModuleInterface* GameModuleManager::findModule( char const* name )
{
	auto iter = mNameToModuleMap.find( name );
	if ( iter != mNameToModuleMap.end() )
		return iter->second;
	return nullptr;
}

IGameModule* GameModuleManager::changeGame( char const* name )
{
	IModuleInterface* module = findModule( name );

	if ( module && module->isGameModule() )
	{
		auto* gameModule = static_cast<IGameModule*>(module);
		if( changeGame(gameModule) )
			return gameModule;
	}

	return nullptr;
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

GameModuleManager::GameModuleManager()
{
	mGameRunning = nullptr;
}

GameModuleManager::~GameModuleManager()
{
	CHECK(mModuleDataList.empty());
	CHECK(mNameToModuleMap.empty());
}

bool GameModuleManager::loadModule( char const* path )
{
	StringView::TCStringConvertible<> loadName = FFileUtility::GetBaseFileName(path).toCString();

	if (mNameToModuleMap.find((char const*)loadName) != mNameToModuleMap.end())
		return true;

	FPlatformModule::Handle hModule = FPlatformModule::Load(path);
	if (hModule == NULL)
		return false;

	ON_SCOPE_EXIT
	{
		if (hModule)
		{
			FPlatformModule::Release(hModule);
		}
	};

	auto createFunc = (CreateModuleFunc)FPlatformModule::GetFunctionAddress(hModule, CREATE_MODULE_STR);
	if( !createFunc )
		return false;

	IModuleInterface* module = (*createFunc)();
	if( !module )
		return false;


	if (registerModule(module, loadName, hModule))
	{
		hModule = NULL;
	}
	else
	{
		module->deleteThis();
		return false;
	}
	LogMsg("Module Loaded: %s", (char const*)loadName);
	return true;
}

bool GameModuleManager::loadModulesFromFile(char const* path)
{
	std::vector< uint8 > buffer;
	if (!FFileUtility::LoadToBuffer(path, buffer, true, false))
	{
		LogWarning(0 , "Can't Load Module File : %s", path);
		return false;
	}

	StringView moduleDir = FFileUtility::GetDirectory(path);
	char const* text = (char const*)buffer.data();
	bool result = true;
	for (;;)
	{
		StringView moduleName = FStringParse::StringTokenLine(text);
		moduleName.trimStartAndEnd();
		if (moduleName.size() == 0)
			break;

		if (moduleDir.size())
		{
			InlineString<MAX_PATH + 1> path = moduleDir;
			path += "/";
			path += moduleName;
			result &= loadModule(path);
		}
		else
		{
			result &= loadModule(moduleName.toMutableCString());
		}
	}

	return result;
}
