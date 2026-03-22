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

	template< class T >
	static void Construct(T* ptr)
	{
		if constexpr (!std::is_trivially_default_constructible_v<T>)
		{
			::new (ptr) T();
		}
	}

	template< class T >
	static void Construct(T* ptr, T const& val)
	{
		if constexpr (std::is_trivially_copy_constructible_v<T>)
		{
			*ptr = val;
		}
		else
		{
			::new (ptr) T(val);
		}
	}

	template< class T >
	static void Construct(void* ptr, T const& val)
	{
		Construct(static_cast<T*>(ptr), val);
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

	template< class T >
	static void ConstructSequence(T* ptr, size_t num)
	{
		if constexpr (!std::is_trivially_default_constructible_v<T>)
		{
			for (size_t i = 0; i < num; ++i)
			{
				Construct(ptr + i);
			}
		}
	}

	template< class T >
	static void ConstructSequence(T* ptr, size_t num, T const& val)
	{
		if constexpr (std::is_trivially_copy_constructible_v<T>)
		{
			std::fill_n(ptr, num, val);
		}
		else
		{
			for (size_t i = 0; i < num; ++i)
			{
				Construct(ptr + i, val);
			}
		}
	}

	template< typename T >
	static void ConstructSequence(T* ptr, size_t num, T const* ptrValue)
	{
		if constexpr (std::is_trivially_copy_constructible_v<T>)
		{
			FMemory::Copy(ptr, ptrValue, sizeof(T) * num);
		}
		else
		{
			for (size_t i = 0; i < num; ++i)
			{
				::new (ptr + i) T(ptrValue[i]);
			}
		}
	}

	template< typename T, typename Iter, TEnableIf_Type< TIsIterator<Iter>::Value, bool > = true >
	static void ConstructSequence(T* ptr, size_t num, Iter itBegin, Iter itEnd)
	{
		if constexpr (std::is_same_v< typename Meta::RemoveCVRef<Iter>::Type, T*> ||
			          std::is_same_v< typename Meta::RemoveCVRef<Iter>::Type, T const*>)
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
		if constexpr (std::is_trivially_copyable_v<T> || TBitwiseReallocatable<T>::Value)
		{
			*ptr = *from;
		}
		else
		{
			if (ptr != from)
			{
				::new (ptr) T(std::move(*from));
				Destruct(from);
			}
		}
	}

	template< class T >
	static void MoveAssign(T* ptr, T* from)
	{
		if constexpr (std::is_trivially_copy_assignable_v<T>)
		{
			*ptr = *from;
		}
		else
		{
			if (ptr != from)
			{
				*ptr = std::move(*from);
			}
		}
	}

	template< class T >
	static void MoveSequence(T* ptr, size_t num, T* from)
	{
		CHECK(from + num <= ptr || ptr < from );
		if constexpr (std::is_trivially_copyable_v<T> || TBitwiseReallocatable<T>::Value)
		{
			FMemory::Move(ptr, from, sizeof(T) * num);
		}
		else
		{
			for (size_t i = 0; i < num; ++i)
			{
				Construct(ptr + i, std::move(from[i]));
				Destruct(from + i);
			}
		}
	}

	template< class T >
	static void MoveRightOverlap(T* ptr, size_t num, T* from)
	{
		if constexpr (std::is_trivially_copyable_v<T> || TBitwiseReallocatable<T>::Value)
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
		*ptr = std::forward<Q>(val);
	}

	template< class T, class ...Args >
	static void Assign(T* ptr, Args&& ...args)
	{
		Assign(ptr , T(std::forward<Args>(args)...) );
	}

	template< class T >
	static void Destruct(T* ptr)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			ptr->~T();
		}
	}

	template< class T >
	static void Destruct(void* ptr)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			static_cast<T*>(ptr)->~T();
		}
	}

	template< class T >
	static void DestructSequence(T* ptr, size_t num)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			for (size_t i = num; i > 0; --i)
			{
				ptr[i - 1].~T();
			}
		}
	}
};

#endif // TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A
