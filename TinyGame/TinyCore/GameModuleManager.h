#pragma once
#ifndef GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
#define GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E

#include "GameModule.h"
#include <map>

typedef std::vector< IGameModule* > GameModuleVec;

class GameModuleManager
{
public:
	GameModuleManager();
	~GameModuleManager();

	TINY_API bool            loadGame( char const* path );
	TINY_API void            cleanup();
	TINY_API void            classifyGame( int attrID , GameModuleVec& games );
	TINY_API IGameModule*  changeGame( char const* name );
	TINY_API bool            changeGame(IGameModule* game);

	IGameModule*  getRunningGame(){ return mGameRunning; }

private:

	IGameModule*  findGame( char const* name );
#if SYS_PLATFORM_WIN
	bool            registerGame(IGameModule* game, HMODULE hModule);
#endif
	template< class Visitor >
	void  visitInternal( Visitor& visitor )
	{
		for( GameInfoVec::iterator iter = mGameInfos.begin() ;
			iter != mGameInfos.end() ;++iter )
		{
			if ( !visitor( *iter ) )
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
		IGameModule* instance;
		HMODULE        hModule;
	};
	typedef std::map< char const* , IGameModule* , StrCmp > PackageMap;

	typedef std::vector< GameInfo > GameInfoVec;
	GameInfoVec     mGameInfos;
	PackageMap      mPackageMap;
	IGameModule*  mGameRunning;
};


#endif // GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
