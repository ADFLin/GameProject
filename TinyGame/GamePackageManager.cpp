#include "TinyGamePCH.h"
#include "GamePackageManager.h"
#include "GameControl.h"
#include "Win32Header.h"

bool GamePackageManager::registerGame( IGamePackage* game )
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
	mGamePackages.push_back( game );
	return true;
}

void GamePackageManager::cleanup()
{
	if ( mCurGame )
		mCurGame->exit();
	mCurGame = NULL;

	struct CleanupVisit
	{
		bool visit( IGamePackage* game )
		{
			game->cleanup();
			game->deleteThis();
			return true;
		}
	};
	CleanupVisit visitor;
	visitInternal( visitor );
	mPackageMap.clear();
	mGamePackages.clear();
}

void GamePackageManager::classifyGame( int attrID , GamePackageVec& games )
{
	struct ClassifyVisit
	{
		ClassifyVisit( int id  , GamePackageVec& games )
			:attrValue( id ),games( games ){}
		bool visit( IGamePackage* game )
		{
			if ( game->getAttribValue( attrValue ) ) 
			{
				if ( attrValue.iVal )
				{
					games.push_back( game );
				}
			}
			return true;
		}
		GamePackageVec& games;
		AttribValue     attrValue;
	};

	ClassifyVisit visitor( attrID , games );
	visitInternal( visitor );
}

IGamePackage* GamePackageManager::findGame( char const* name )
{
	PackageMap::iterator iter = mPackageMap.find( name );
	if ( iter != mPackageMap.end() )
		return iter->second;
	return NULL;
}

IGamePackage* GamePackageManager::changeGame( char const* name )
{
	IGamePackage* game = findGame( name );
	if ( !game )
		return NULL;

	if ( mCurGame )
		mCurGame->exit();

	try
	{
		mCurGame = game;
		mCurGame->enter();
		return game;
	}
	catch( ... )
	{
		mCurGame = NULL;
		return NULL;
	}
}

GamePackageManager::GamePackageManager()
{
	mCurGame = NULL;
}

GamePackageManager::~GamePackageManager()
{
	cleanup();
}

bool GamePackageManager::loadGame( char const* path )
{
	HINSTANCE hInstance = ::LoadLibrary( path );
	if ( hInstance == NULL )
		return false;

	CREATEGAMEFUN createFun = (CREATEGAMEFUN)GetProcAddress( hInstance , GAME_CREATE_FUN_NAME );

	if ( !createFun )
		return false;

	IGamePackage* game = (*createFun)();
	if ( !game )
		return false;

	if ( !registerGame( game ) )
	{
		game->deleteThis();
		return false;
	}

	return true;
}
