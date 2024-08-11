#ifndef CacheAllocator_h__
#define CacheAllocator_h__

#include "Core/Memory.h"

class CacheAllocator
{
public:
	CacheAllocator( int maxCacheSize )
		:mNumCache(0)
	{
		mCache = (void**)FMemory::Alloc( maxCacheSize * sizeof( void* ) );
		FMemory::Zero( mCache , sizeof(mCache) * maxCacheSize );
	}

	~CacheAllocator()
	{
		for( int i = 0; i < mNumCache ; ++ i )
			FMemory::Free( mCache[i] );
		FMemory::Free( mCache );
	}

	void*  alloc( size_t size )
	{
		for( int i = 0; i < mNumCache ; ++size )
		{
			CacheInfo* cache = reinterpret_cast< CacheInfo* >( mCache[i] );
			if ( cache->size >= size )
			{
				mCache[i] = mCache[mNumCache - 1 ];
				--mNumCache;
				return cache;
			}
		}
		return FMemory::Alloc( size > sizeof( CacheInfo ) ? size : sizeof( CacheInfo ) );
	}

	void   dealloc( void* ptr )
	{
		if ( mNumCache >= mMaxCacheSize )
		{
			FMemory::Free( ptr );
			return;
		}

		CacheInfo* cache = reinterpret_cast< CacheInfo* >( ptr );
		cache->size = _msize( ptr );
		mCache[mNumCache] = cache;
		++mNumCache;		
	}

	void   clearCache()
	{
		for( int i = 0; i < mNumCache ; ++ i )
			FMemory::Free(mCache[i] );
		mNumCache = 0;
	}
private:
	struct CacheInfo
	{
		size_t     size;
		CacheInfo* next;
	};
	int    mNumCache;
	int    mMaxCacheSize;
	void** mCache;
};


#endif // CacheAllocator_h__
