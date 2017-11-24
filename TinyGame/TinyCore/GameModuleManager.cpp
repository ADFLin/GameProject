#include "TinyGamePCH.h"
#include "GameModuleManager.h"
#include "GameControl.h"
#include "WindowsHeader.h"

bool GameModuleManager::registerGame( IGameModule* game , HMODULE hModule )
{
	assert( game );

	if ( findGame( game->getName() ) )
		return false;

	if ( !game->initialize() )
		return false;

	if ( !mPackageMap.insert( std::make_pair( game->getName() , game ) ).second )
	{
		game->cleanup();
		return false;
	}

	AttribValue attrSetting( ATTR_CONTROLLER_DEFUAULT_SETTING );
	game->getAttribValue( attrSetting );
	mGameInfos.push_back({ game , hModule });
	return true;
}

void GameModuleManager::cleanup()
{
	if ( mGameRunning )
		mGameRunning->exit();
	mGameRunning = NULL;

	visitInternal( [](GameInfo& info) ->bool
	{
		info.instance->cleanup();
		info.instance->deleteThis();
		::FreeLibrary(info.hModule);
		return true;
	});

	mPackageMap.clear();
	mGameInfos.clear();
}

void GameModuleManager::classifyGame( int attrID , GameModuleVec& games )
{
	visitInternal( [ attrID , &games ](GameInfo& info)-> bool
	{
		AttribValue    attrValue(attrID);
		if( info.instance->getAttribValue(attrValue) )
		{
			if( attrValue.iVal )
			{
				games.push_back(info.instance);
			}
		}
		return true;
	});
}

IGameModule* GameModuleManager::findGame( char const* name )
{
	PackageMap::iterator iter = mPackageMap.find( name );
	if ( iter != mPackageMap.end() )
		return iter->second;
	return NULL;
}

IGameModule* GameModuleManager::changeGame( char const* name )
{
	IGameModule* game = findGame( name );

	if( changeGame(game) )
		return game;

	return nullptr;
}

bool GameModuleManager::changeGame(IGameModule* game)
{
	if( !game )
		return false;

	if( mGameRunning != game )
	{
		if( mGameRunning )
			mGameRunning->exit();

		mGameRunning = game;

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
	mGameRunning = NULL;
}

GameModuleManager::~GameModuleManager()
{
	cleanup();
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



bool GameModuleManager::loadGame( char const* path )
{
#if SYS_PLATFORM_WIN
	HMODULE hModule = ::LoadLibrary( path );
	if ( hModule == NULL )
		return false;

	struct ModouleReleasePolicy
	{
		static void Release(HMODULE handle) { ::FreeLibrary(handle); }
	};

	TScopeRelease< HMODULE, ModouleReleasePolicy > scopeRelease( hModule );

	char const* funName = CREATE_GAME_MODULE_STR;
	CreateGameModuleFun createFun = (CreateGameModuleFun)GetProcAddress(hModule, CREATE_GAME_MODULE_STR);

	if( !createFun )
		return false;

	IGameModule* game = (*createFun)();
	if( !game )
		return false;

	if( !registerGame( game , hModule ) )

	{
		game->deleteThis();
		return false;
	}

	scopeRelease.release();
	
#else
#error "Not impl yet"
#endif 

	return true;
}
