#include "TinyGamePCH.h"
#include "GameModuleManager.h"
#include "GameControl.h"
#include "WindowsHeader.h"

#include "FileSystem.h"


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

	if( !module->initialize() )
		return false;

	bool bInserted = mNameToModuleMap.insert(std::make_pair(registerName, module)).second;
	assert(bInserted);
	mModuleDataList.push_back(
		{   module
#if SYS_PLATFORM_WIN
          , hModule
#endif
		});

	LogMsg("Module Registered : %s", registerName);

	if( gameModule )
	{
		GameAttribute attrSetting(ATTR_CONTROLLER_DEFUAULT_SETTING);
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
		info.instance->cleanup();
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


template< class T , class Policy >
class TScopeRelease
{
public:
	TScopeRelease(T& object)
		:mObject(&object)
	{

	}

	~TScopeRelease()
	{
		if( mObject )
		{
			Policy::Release(*mObject);
		}
	}

	void release()
	{
		mObject = nullptr;
	}
	T* mObject;
};



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
	struct ModouleReleasePolicy
	{
		static void Release(HMODULE handle) { ::FreeLibrary(handle); }
	};

	TScopeRelease< HMODULE, ModouleReleasePolicy > scopeRelease( hModule );
	char const* funName = CREATE_MODULE_STR;
	auto createFun = (CreateModuleFun)GetProcAddress(hModule, CREATE_MODULE_STR);
#else

	CreateModuleFun createFun = nullptr;
#endif 
	if( !createFun )
		return false;

	IModuleInterface* module = (*createFun)();
	if( !module )
		return false;

	StringView loadName = FFileUtility::GetBaseFileName(path);
	if( !registerModule( module , loadName.toCString() 
#if SYS_PLATFORM_WIN
						, hModule 
#endif
	) )
	{
		module->deleteThis();
		return false;
	}

#if SYS_PLATFORM_WIN
	scopeRelease.release();
#endif

	return true;
}
