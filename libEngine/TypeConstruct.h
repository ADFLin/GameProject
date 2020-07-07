#pragma once
#ifndef TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A
#define TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A

#include "Meta/MetaBase.h"

class TypeDataHelper
{
public:
	template< class T >
	static void Construct(void* ptr, T const& val = T())
	{
		ConstructInternal(static_cast<T*>(ptr), val, Meta::IsPod< T >::Type());
	}

	template< class T , class Q >
	static void Construct(T* ptr, Q&& val )
	{
		::new (ptr) T(std::forward<Q>(val));
	}

	template< class T, class ...Args >
	static void Construct(T* ptr, Args&& ...args)
	{
		::new (ptr) T(std::forward<Args>(args)...);
	}

	template< class ...Args >
	static void Construct(void* ptr, Args&& ...args)
	{
		::new (ptr) T(std::forward<Args>(args)...);
	}

	template< class T >
	static void Construct(T* ptr, size_t num, T const& val = T())
	{
		ConstructInternal(ptr, num, val, Meta::IsPod< T >::Type());
	}
	template< class T >
	static void Construct(T* ptr, size_t num, T* ptrValue)
	{
		ConstructInternal(ptr, num, ptrValue, Meta::IsPod< T >::Type());
	}
	template< class T >
	static void Move(T* ptr, T* from)
	{
		MoveInternal(ptr, from, Meta::IsPod< T >::Type());
	}
	template< class T >
	static void Move(T* ptr, size_t num, T* from)
	{
		MoveInternal(ptr, num , from, Meta::IsPod< T >::Type());
	}

	template< class T, class Q >
	static void Assign(T* ptr, Q&& val)
	{
		*ptr = val;
	}

	template< class T, class ...Args >
	static void Assign(T* ptr, Args&& ...args)
	{
		Assign(ptr , T(std::forward<Args>(args)...) );
	}

	template< class T >
	static void Destruct(T* ptr)
	{
		ptr->~T();
	}

	template< class T >
	static void Destruct(void* ptr)
	{
		static_cast< T* >( ptr )->~T();
	}

	template< class T >
	static void Destruct(T* ptr, size_t num )
	{
		DestructInternal(ptr, num, Meta::IsPod< T >::Type());
	}

private:
	template< class T >
	static void ConstructInternal(T* ptr, T const& val, Meta::TrueType)
	{
		*ptr = val;
	}
	template< class T >
	static void ConstructInternal(T* ptr, T const& val , Meta::FalseType )
	{
		::new (ptr) T(val);
	}
	template< class T >
	static void ConstructInternal(T* ptr, size_t num, T const& val, Meta::TrueType)
	{
		std::fill_n(ptr, num, val);
	}
	template< class T >
	static void ConstructInternal(T* ptr, size_t num, T const& val, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end; ++ptr )
		{
			Construct(ptr, val);
		}
	}
	template< class T >
	static void ConstructInternal(T* ptr, size_t num, T* ptrValue, Meta::TrueType)
	{
		::memcpy(ptr, ptrValue, sizeof(T) * num);
	}
	template< class T >
	static void ConstructInternal(T* ptr, size_t num, T* ptrValue, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end; ++ptr )
		{
			new (ptr) T(*ptrValue);
			++ptrValue;
		}
	}
	template< class T >
	static void DestructInternal(T* ptr, size_t num, Meta::TrueType)
	{
		(void*)ptr;
	}
	template< class T >
	static void DestructInternal(T* ptr, size_t num, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end; ++ptr )
		{
			ptr->~T();
		}
	}
	template< class T >
	static void MoveInternal(T* ptr, T* from, Meta::TrueType)
	{
		*ptr = *from;

	}
	template< class T >
	static void MoveInternal(T* ptr, T* from, Meta::FalseType)
	{
		new (ptr) (std::move(*from));
		from->~T();
	}
	template< class T >
	static void MoveInternal(T* ptr, size_t num, T* from, Meta::TrueType)
	{
		memcpy(ptr, from, sizeof(T)*num);
	}
	template< class T >
	static void MoveInternal(T* ptr, size_t num, T* from, Meta::FalseType)
	{
		for( T* end = ptr + num; ptr != end ; ++ptr )
		{
			Construct(ptr, *from);
			Destruct(from);
			++from;
		}
	}
};
#endif // TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A
