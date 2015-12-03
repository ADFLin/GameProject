#ifndef CFPluginManager_h__
#define CFPluginManager_h__

#include "CFBase.h"

namespace CFly
{
	class ILoadListener;
	class IFileImport;
	class IFileExport;
	class IFileLinker;

	enum LoaderFlag
	{
		LF_NO_DELETE = BIT( 0 ),
		LF_USE_CACHE = BIT( 1 ),
	};
	class PluginManager : public SingletonT< PluginManager >
	{
	public:
		bool registerLinker( char const* name , IFileLinker* factory , unsigned flag = 0 );
		void unregisterLinker( char const* name );

		bool load( World* world , Actor* actor , char const* fileName , char const* loaderName , ILoadListener* listener = nullptr );
		bool load( World* world , Object* object , char const* fileName , char const* loaderName , ILoadListener* listener = nullptr );
		bool load( World* world , Scene* scene , char const* fileName , char const* loaderName , ILoadListener* listener = nullptr );

		typedef uintptr_t DataType;
		void     setLoaderMeta( int id , DataType data ){ assert( id < MaxMetaNum ); mMeta[id] = data; }
		DataType getLoaderMeta( int id ){ assert( id < MaxMetaNum ); return mMeta[ id ]; }

	private:
		template< class T > 
		bool loadT( World* world , T* entity , unsigned dirBit , EntityType type , char const* fileName , char const* loaderName , ILoadListener* listener );

		struct LinkerInfo 
		{
			LinkerInfo( IFileLinker* factory , unsigned flag )
				:factory( factory ),flag( flag )
			{
				cacheImport = nullptr;
				cacheExport = nullptr;
			}
			IFileLinker*  factory;
			IFileImport*  cacheImport;
			IFileExport*  cacheExport;
			unsigned      flag;
		};
		LinkerInfo* findLoader( char const* loaderName, char const* fileName, EntityType type );
		typedef std::map< String , LinkerInfo > LinkerMap;
		static int const MaxMetaNum = 8;
		DataType  mMeta[ MaxMetaNum ];
		LinkerMap mLinkerMap;
	};
}

#endif // CFPluginManager_h__
