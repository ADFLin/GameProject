#ifndef ObjectFactory_h__
#define ObjectFactory_h__

#include <malloc.h>
#include <cstring>
#include <cassert>
#include <new>
#include "Singleton.h"
#include "Memory/CacheAllocator.h"

namespace Shoot2D
{

	class ObjectCreator
	{
	public:
		static size_t const MAX_CACHE_NUM = 128;

		template<class T >
		inline static void destroy(T* ptr)
		{
			ptr->~T();
			getAllocator().dealloc( ptr );
		}
		template<class T >
		inline static T* create()
		{ 
			T* ptr = ( T* ) getAllocator().alloc( sizeof(T) );
			new ptr T();
			return ptr;
		}

		template<class T , typename P1>
		inline static T* create( P1& p1 )
		{ 
			void* ptr = getAllocator().alloc( sizeof(T) );
			return new (ptr) T(p1);
		}
		template<class T , typename P1 , typename P2>
		inline static T* create( P1& p1, P2& p2 )
		{ 
			void* ptr = getAllocator().alloc( sizeof(T) );
			return new (ptr) T(p1,p2);
		}
		template<class T , typename P1 , typename P2 , typename P3>
		inline static T* create( P1& p1, P2& p2, P3& p3 )
		{ 
			void* ptr = getAllocator().alloc( sizeof(T) );
			return new (ptr) T(p1,p2,p3);
		}
		template<class T , typename P1 , typename P2 , typename P3 , typename P4 >
		inline static T* create( P1& p1, P2& p2, P3& p3, P4& p4)
		{ 
			void* ptr = getAllocator().alloc( sizeof(T) );
			return new (ptr) T(p1,p2,p3,p4);
		}

	protected:
		static CacheAllocator& getAllocator()
		{
			static CacheAllocator allocator( MAX_CACHE_NUM );
			return allocator;
		}
	};

}//namespace Shoot2D

#endif // ObjectFactory_h__