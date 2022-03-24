#include "TinyGamePCH.h"
#include "GameModuleManager.h"
#include "GameControl.h"

#include "FileSystem.h"
#include "Core/ScopeGuard.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif
#include "StringParse.h"

bool GameModuleManager::registerModule( IModuleInterface* module , char const* moduleName 
#if SYS_PLATFORM_WIN
									   , HMODULE hModule 
#endif
)
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
	mModuleDataList.push_back(
		{   module
#if SYS_PLATFORM_WIN
          , hModule
#endif
		});

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
}

void GameModuleManager::cleanupModuleMemory()
{
#if SYS_PLATFORM_WIN
	visitInternal([](ModuleData& info) ->bool
	{
		assert(info.instance == nullptr);
		::FreeLibrary(info.hModule);
		return true;
	});
#endif

	mModuleDataList.clear();
	mNameToModuleMap.clear();
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
	cleanupModuleInstances();
}

bool GameModuleManager::loadModule( char const* path )
{
#if SYS_PLATFORM_WIN
	HMODULE hModule = ::LoadLibraryA( path );
	if (hModule == NULL)
	{
		char* lpMsgBuf;
		DWORD dw = ::GetLastError();
		::FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		LogWarning(0, "Load Module %s Fail : %s", path , lpMsgBuf);

		::LocalFree(lpMsgBuf);
		return false;
	}

	ON_SCOPE_EXIT
	{
		if (hModule)
		{
			::FreeLibrary(hModule);
		}
	};

	char const* funName = CREATE_MODULE_STR;
	auto createFunc = (CreateModuleFunc)GetProcAddress(hModule, CREATE_MODULE_STR);
#else
	CreateModuleFunc createFunc = nullptr;
#endif 
	if( !createFunc )
		return false;

	IModuleInterface* module = (*createFunc)();
	if( !module )
		return false;

	StringView loadName = FFileUtility::GetBaseFileName(path);
	if (registerModule(module, loadName.toCString()
#if SYS_PLATFORM_WIN
		, hModule
#endif
	))
	{

	}
	else
	{
		module->deleteThis();
		return false;
	}

#if SYS_PLATFORM_WIN
	hModule = NULL;
#endif

	LogMsg("Module Loaded: %s", (char const*)loadName.toCString());
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
		if (moduleName.size() == 0)
			break;

		if (moduleDir.size())
		{
			InlineString<MAX_PATH + 1> path = moduleDir;
			path += "/";
			path += moduleName;
			result |= loadModule(path);
		}
		else
		{
			result |= loadModule(moduleName.toCString());
		}
	}

	return true;
}
