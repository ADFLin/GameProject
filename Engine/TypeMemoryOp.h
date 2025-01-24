#pragma once
#ifndef TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A
#define TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A

#include "Meta/MetaBase.h"
#include "Core/Memory.h"
#include "Meta/EnableIf.h"

#include <utility>
#include <type_traits>
#include <algorithm>


class FTypeMemoryOp
{
public:

	template< class T, TEnableIf_Type< std::is_trivially_copy_constructible_v<T>, bool > = true >
	static void Construct(void* ptr, T const& val) {}
	template< class T, TEnableIf_Type< !std::is_trivially_copy_constructible_v<T>, bool > = true >
	static void Construct(void* ptr, T const& val)
	{
		Construct(static_cast<T*>(ptr));
	}

	template< class T, TEnableIf_Type< std::is_trivially_default_constructible_v<T>, bool > = true >
	static void Construct(T* ptr){}
	template< class T, TEnableIf_Type< !std::is_trivially_default_constructible_v<T>, bool > = true >
	static void Construct(T* ptr)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			*ptr = T();
		}
		else
		{
			::new (ptr) T();
		}
	}
	template< class T >
	static void Construct(T* ptr, T const& val)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			*ptr = val;
		}
		else
		{
			::new (ptr) T(val);
		}
	}

	template< class T, class ...Args >
	static void Construct(void* ptr, Args&& ...args)
	{
		::new (ptr) T(std::forward<Args>(args)...);
	}

	template< class T, class ...Args >
	static void Construct(T* ptr, Args&& ...args)
	{
		::new (ptr) T(std::forward<Args>(args)...);
	}

	template< class T , TEnableIf_Type< std::is_trivially_default_constructible_v<T> , bool > = true >
	static void ConstructSequence(T* ptr, size_t num){}
	template< class T, TEnableIf_Type< !std::is_trivially_default_constructible_v<T>, bool > = true >
	static void ConstructSequence(T* ptr, size_t num)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			std::fill_n(ptr, num, T());
		}
		else
		{
			while (num)
			{
				Construct(ptr);
				++ptr;
				--num;
			}
		}
	}

	template< class T >
	static void ConstructSequence(T* ptr, size_t num, T const& val)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			std::fill_n(ptr, num, val);
		}
		else
		{
			while (num)
			{
				Construct(ptr, val);
				++ptr;
				--num;
			}
		}
	}
	template< typename T >
	static void ConstructSequence(T* ptr, size_t num, T const* ptrValue)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			FMemory::Copy(ptr, ptrValue, sizeof(T) * num);
		}
		else
		{
			while (num)
			{
				new (ptr) T(*ptrValue);
				++ptr;
				++ptrValue;
				--num;
			}
		}
	}

	template< typename T, typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	static void ConstructSequence(T* ptr, size_t num, Iter itBegin, Iter itEnd)
	{
		if constexpr (std::is_same_v< Meta::RemoveCVRef<Iter>::Type, T*> ||
			          std::is_same_v< Meta::RemoveCVRef<Iter>::Type, T const*>)
		{
			ConstructSequence(ptr, num, itBegin);
		}
		else
		{
			for (; itBegin != itEnd; ++itBegin)
			{
				Construct(ptr, *itBegin);
				++ptr;
			}
		}
	}

	template< class T >
	static void Move(T* ptr, T* from)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			*ptr = *from;
		}
		else
		{
			new (ptr) T(std::move(*from));
			Destruct(from);
		}
	}

	template< class T >
	static void MoveSequence(T* ptr, size_t num, T* from)
	{
		CHECK(from + num <= ptr || ptr < from );
		if constexpr (Meta::IsPod< T >::Value)
		{
			FMemory::Move(ptr, from, sizeof(T)*num);
		}
		else
		{
			while (num)
			{
				Construct(ptr, std::move(*from));
				Destruct(from);
				++from;
				++ptr;
				--num;
			}
		}
	}

	template< class T >
	static void MoveRightOverlap(T* ptr, size_t num, T* from)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{
			FMemory::Move(ptr, from, sizeof(T)*num);
		}
		else
		{
			ptr += num - 1;
			from += num - 1;
			while (num)
			{
				Construct(ptr, std::move(*from));
				Destruct(from);
				--from;
				--ptr;
				--num;
			}
		}
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

	template< class T, TEnableIf_Type< std::is_trivially_destructible_v<T>, bool > = true >
	static void Destruct(T* ptr){}

	template< class T, TEnableIf_Type< !std::is_trivially_destructible_v<T>, bool > = true >
	static void Destruct(T* ptr)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{

		}
		else
		{
			ptr->~T();
		}
	}

	template< class T, TEnableIf_Type< std::is_trivially_destructible_v<T>, bool > = true >
	static void Destruct(void* ptr) {}

	template< class T, TEnableIf_Type< !std::is_trivially_destructible_v<T>, bool > = true >
	static void Destruct(void* ptr)
	{
		if constexpr (Meta::IsPod< T >::Value)
		{

		}
		else
		{
			static_cast<T*>(ptr)->~T();
		}
	}

	template< class T, TEnableIf_Type< std::is_trivially_destructible_v<T>, bool > = true >
	static void DestructSequence(T* ptr, size_t num)
	{
		(void)ptr;
		(void)num;
	}

	template< class T , TEnableIf_Type< !std::is_trivially_destructible_v<T> , bool > = true >
	static void DestructSequence(T* ptr, size_t num)
	{
		while (num)
		{
			ptr->~T();
			++ptr;
			--num;
		}
	}
};

#endif // TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A
