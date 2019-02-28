#include "CFlyPCH.h"
#include "CFPluginManager.h"

#include "CFFileLoader.h"
#include "CFWorld.h"

#include "FileSystem.h"


namespace CFly
{
	int const MAX_PATH_STR_SIZE = 256;


	bool PluginManager::registerLinker( char const* name , IFileLinker* linker , unsigned flag )
	{
		if ( !mLinkerMap.insert( std::make_pair( String( name ) , LinkerInfo( linker , flag ) ) ).second )
			return false;
		return true;
	}

	void PluginManager::unregisterLinker( char const* name )
	{
		LinkerMap::iterator iter = mLinkerMap.find( name );
		if ( iter == mLinkerMap.end() )
			return;

		LinkerInfo& info = iter->second;
		if ( !( info.flag & LF_NO_DELETE ) )
			info.factory->deleteThis();
		if ( info.cacheImport )
			info.cacheImport->deleteThis();
		if ( info.cacheExport )
			info.cacheExport->deleteThis();
	}

	PluginManager::LinkerInfo* PluginManager::findLoader( char const* loaderName, char const* fileName, EntityType type )
	{
		LinkerInfo* pInfo = nullptr;
		if ( loaderName )
		{
			LinkerMap::iterator iter = mLinkerMap.find( loaderName );
			if ( iter != mLinkerMap.end() )
			{
				pInfo = &iter->second;
			}
		}
		else if ( fileName )
		{
			char const* subName = FileUtility::GetExtension( fileName );
			for ( LinkerMap::iterator iter( mLinkerMap.begin() ) , end( mLinkerMap.end() );
				iter != end ; ++iter )
			{
				LinkerInfo& info = iter->second;
				if ( info.factory->canLoad( type , subName  ) )
				{
					pInfo = &info;
					break;
				}
			}
		}
		return pInfo;
	}

	template< class T >
	bool PluginManager::loadT( World* world , T* entity , unsigned dirBit , EntityType type , char const* fileName , char const* loaderName , ILoadListener* listener )
	{
		LinkerInfo* pInfo = findLoader( loaderName , fileName , type );
		if ( !pInfo )
			return false;

		world->_setUseDirBit( dirBit );
		char path[ MAX_PATH_STR_SIZE ];
		if ( !world->getPath( path , fileName ) )
			return false;

		IFileImport* loader = pInfo->cacheImport;
		if ( !loader )
			loader = pInfo->factory->createLoader();


		bool result = loader->load( path , entity , listener );

		if ( pInfo->flag & LF_USE_CACHE )
		{
			pInfo->cacheImport = loader;
		}
		else
		{
			loader->deleteThis();
		}
		return result;
	}

	bool PluginManager::load( World* world , Object* object , char const* fileName , char const* loaderName , ILoadListener* listener )
	{
		return loadT( world , object ,BIT(DIR_OBJECT) | BIT(DIR_SCENE) , ET_OBJECT , fileName , loaderName , listener );
	}

	bool PluginManager::load( World* world , Scene* scene , char const* fileName , char const* loaderName , ILoadListener* listener )
	{
		return loadT( world , scene , BIT(DIR_SCENE) , ET_SCENE , fileName , loaderName , listener );
	}

	bool PluginManager::load( World* world , Actor* actor , char const* fileName , char const* loaderName , ILoadListener* listener )
	{
		return loadT( world , actor , BIT(DIR_ACTOR) , ET_ACTOR , fileName , loaderName , listener );
	}

}//namespace CFly