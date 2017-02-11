#pragma once
#ifndef TypeFactory_H_BC9060AA_A057_4EF9_A087_C2D68BFF78F4
#define TypeFactory_H_BC9060AA_A057_4EF9_A087_C2D68BFF78F4

#include "MetaBase.h"
class TypeConstructHelpler
{
public:
	template< class T >
	static void construct(T* ptr, T const& val = T() )
	{
		constructInternal(ptr, val, Meta::IsPod< T >::ResultType());
	}
	template< class T >
	static void construct(T* ptr, size_t num, T const& val = T())
	{
		constructInternal(ptr, num, val, Meta::IsPod< T >::ResultType());
	}
	template< class T >
	static void construct(T* ptr, size_t num, T* ptrValue)
	{
		constructInternal(ptr, num, ptrValue, Meta::IsPod< T >::ResultType());
	}
	template< class T >
	static void move(T* ptr, T* from)
	{
		moveInternal(ptr, from, Meta::IsPod< T >::ResultType());
	}
	template< class T >
	static void move(T* ptr, size_t num, T* from)
	{
		moveInternal(ptr, num , from, Meta::IsPod< T >::ResultType());
	}
	template< class T >
	static void destruct(T* ptr)
	{
		ptr->~T();
	}
	template< class T >
	static void destruct(T* ptr, size_t num )
	{
		destructInternal(ptr, num, Meta::IsPod< T >::ResultType());
	}
private:
	template< class T >
	static void constructInternal(T* ptr, T const& val, Meta::TrueType)
	{
		*ptr = val;
	}
	template< class T >
	static void constructInternal(T* ptr, T const& val , Meta::FalseType )
	{
		::new (ptr) T(val);
	}
	template< class T >
	static void constructInternal(T* ptr, size_t num, T const& val, Meta::TrueType)
	{
		std::fill_n(ptr, num, val);
	}
	template< class T >
	static void constructInternal(T* ptr, size_t num, T const& val, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end; ++ptr )
		{
			construct(ptr, val);
		}
	}
	template< class T >
	static void constructInternal(T* ptr, size_t num, T* ptrValue, Meta::TrueType)
	{
		::memcpy(ptr, ptrValue, sizeof(T) * num);
	}
	template< class T >
	static void constructInternal(T* ptr, size_t num, T* ptrValue, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end; ++ptr )
		{
			new (ptr) T(*ptrValue);
			++ptrValue;
		}
	}
	template< class T >
	static void destructInternal(T* ptr, size_t num, Meta::TrueType)
	{
		(void*)ptr;
	}
	template< class T >
	static void destructInternal(T* ptr, size_t num, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end; ++ptr )
		{
			ptr->~T();
		}
	}
	template< class T >
	static void moveInternal(T* ptr, T* from, Meta::TrueType)
	{
		*ptr = *from;

	}
	template< class T >
	static void moveInternal(T* ptr, T* from, Meta::FalseType)
	{
		new (ptr) (std::move(*from));
		from->~T();
	}
	template< class T >
	static void moveInternal(T* ptr, size_t num, T* from, Meta::TrueType)
	{
		memcpy(ptr, from, sizeof(T)*num);
	}
	template< class T >
	static void moveInternal(T* ptr, size_t num, T* from, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end ; ++ptr )
		{
			construct(ptr, *from);
			destruct(from);
			++from;
		}
	}
};
#endif // TypeFactory_H_BC9060AA_A057_4EF9_A087_C2D68BFF78F4
