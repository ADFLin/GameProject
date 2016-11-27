#ifndef GameManager_h__
#define GameManager_h__

#include "GamePackage.h"
#include <map>

typedef std::vector< IGamePackage* > GamePackageVec;

class GamePackageManager
{
public:
	GamePackageManager();
	~GamePackageManager();

	GAME_API bool           loadGame( char const* path );
	GAME_API bool           registerGame( IGamePackage* game );
	GAME_API void           cleanup();
	GAME_API void           classifyGame( int attrID , GamePackageVec& games );

	GAME_API IGamePackage*  changeGame( char const* name );
	IGamePackage*  getCurGame(){ return mCurGame; }

private:
	IGamePackage*  findGame( char const* name );

	template< class Visitor >
	void  visitInternal( Visitor& visitor )
	{
		for( GamePackageVec::iterator iter = mGamePackages.begin() ; 
			iter != mGamePackages.end() ;++iter )
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

	typedef std::map< char const* , IGamePackage* , StrCmp > PackageMap;
	GamePackageVec mGamePackages;
	PackageMap     mPackageMap;
	IGamePackage*  mCurGame;
};








#endif // GameManager_h__
