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

	TINY_API bool            loadModule( char const* path );
	TINY_API void            cleanup();
	TINY_API void            classifyGame( int attrID , GameModuleVec& games );
	TINY_API IGameModule*    changeGame( char const* name );
	TINY_API bool            changeGame(IGameModule* game);

	IGameModule*  getRunningGame(){ return mGameRunning; }

private:

	IModuleInterface*  findModule( char const* name );

	bool          registerModule(
		IModuleInterface* game,
		char const* loadedModuleName ,
#if SYS_PLATFORM_WIN
		HMODULE hModule
#endif
	);

	template< class Visitor >
	void  visitInternal( Visitor& visitor )
	{
		for( auto& data : mModuleDataList )
		{
			if ( !visitor( data ) )
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

	struct ModuleData
	{
		IModuleInterface* instance;
#if SYS_PLATFORM_WIN
		HMODULE      hModule;
#endif
	};
	typedef std::map< HashString , IModuleInterface* > ModuleMap;

	std::vector< ModuleData >   mModuleDataList;
	ModuleMap        mNameToModuleMap;
	IGameModule*     mGameRunning;
};


#endif // GameModuleManager_H_CE50FE41_ACA5_4EEA_B3A9_C52AF32EF34E
