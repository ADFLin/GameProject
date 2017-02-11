#pragma once
#ifndef GameInstanceManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
#define GameInstanceManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E

#include "GameInstance.h"
#include <map>

typedef std::vector< IGameInstance* > GameInstanceVec;

class GameInstanceManager
{
public:
	GameInstanceManager();
	~GameInstanceManager();

	GAME_API bool            loadGame( char const* path );
	GAME_API void            cleanup();
	GAME_API void            classifyGame( int attrID , GameInstanceVec& games );
	GAME_API IGameInstance*  changeGame( char const* name );
	GAME_API bool            changeGame(IGameInstance* game);

	IGameInstance*  getRunningGame(){ return mGameRunning; }

private:

	IGameInstance*  findGame( char const* name );
#if SYS_PLATFORM_WIN
	bool            registerGame(IGameInstance* game, HMODULE hModule);
#endif
	template< class Visitor >
	void  visitInternal( Visitor& visitor )
	{
		for( GameInfoVec::iterator iter = mGameInfos.begin() ;
			iter != mGameInfos.end() ;++iter )
		{
			if ( !visitor.visit( *iter ) )
				return;
		}
	}

	struct StrCmp
	{
		bool operator() ( char const* s1 , char const* s2 ) const
		{ 
			return ::strcmp( s1 , s2 ) < 0;  
		}
	};

	struct GameInfo
	{
		IGameInstance* instance;
		HMODULE        hModule;
	};
	typedef std::map< char const* , IGameInstance* , StrCmp > PackageMap;

	typedef std::vector< GameInfo > GameInfoVec;
	GameInfoVec     mGameInfos;
	PackageMap      mPackageMap;
	IGameInstance*  mGameRunning;
};


#endif // GameInstanceManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
