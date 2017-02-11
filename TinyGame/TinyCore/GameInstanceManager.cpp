#include "TinyGamePCH.h"
#include "GameInstanceManager.h"
#include "GameControl.h"
#include "Win32Header.h"

bool GameInstanceManager::registerGame( IGameInstance* game , HMODULE hModule )
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

void GameInstanceManager::cleanup()
{
	if ( mGameRunning )
		mGameRunning->exit();
	mGameRunning = NULL;

	struct CleanupVisit
	{
		bool visit( GameInfo& info )
		{
			info.instance->cleanup();
			info.instance->deleteThis();
			::FreeLibrary(info.hModule);
			return true;
		}
	};
	CleanupVisit visitor;
	visitInternal( visitor );
	mPackageMap.clear();
	mGameInfos.clear();
}

void GameInstanceManager::classifyGame( int attrID , GameInstanceVec& games )
{
	struct ClassifyVisit
	{
		ClassifyVisit( int id  , GameInstanceVec& games )
			:attrValue( id ),games( games ){}
		bool visit(GameInfo& info)
		{
			if ( info.instance->getAttribValue( attrValue ) ) 
			{
				if ( attrValue.iVal )
				{
					games.push_back(info.instance);
				}
			}
			return true;
		}
		GameInstanceVec& games;
		AttribValue     attrValue;
	};

	ClassifyVisit visitor( attrID , games );
	visitInternal( visitor );
}

IGameInstance* GameInstanceManager::findGame( char const* name )
{
	PackageMap::iterator iter = mPackageMap.find( name );
	if ( iter != mPackageMap.end() )
		return iter->second;
	return NULL;
}

IGameInstance* GameInstanceManager::changeGame( char const* name )
{
	IGameInstance* game = findGame( name );

	if( changeGame(game) )
		return game;

	return nullptr;
}

bool GameInstanceManager::changeGame(IGameInstance* game)
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

GameInstanceManager::GameInstanceManager()
{
	mGameRunning = NULL;
}

GameInstanceManager::~GameInstanceManager()
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



bool GameInstanceManager::loadGame( char const* path )
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

	CreateGameFun createFun = (CreateGameFun)GetProcAddress(hModule, GAME_CREATE_FUN_NAME);

	if( !createFun )
		return false;

	IGameInstance* game = (*createFun)();
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
