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
	template< class T, TEnableIf_Type< std::is_trivially_default_constructible_v<T>, bool > = true >
	static void Construct(T* ptr){}
	template< class T, TEnableIf_Type< !std::is_trivially_default_constructible_v<T>, bool > = true >
	static void Construct(T* ptr)
	{
		ConstructInternal(static_cast<T*>(ptr), Meta::IsPod< T >::Type());
	}

	template< class T, TEnableIf_Type< std::is_trivially_copy_constructible_v<T>, bool > = true >
	static void Construct(void* ptr, T const& val){}
	template< class T, TEnableIf_Type< !std::is_trivially_copy_constructible_v<T>, bool > = true >
	static void Construct(void* ptr, T const& val)
	{
		ConstructInternal(static_cast<T*>(ptr), val, Meta::IsPod< T >::Type());
	}

	template< class T >
	static void Construct(T* ptr, T const& val)
	{
		ConstructInternal(static_cast<T*>(ptr), val, Meta::IsPod< T >::Type());
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
		ConstructSequenceInternal(ptr, num, Meta::IsPod< T >::Type());
	}

	template< class T >
	static void ConstructSequence(T* ptr, size_t num, T const& val)
	{
		ConstructSequenceInternal(ptr, num, val, Meta::IsPod< T >::Type());
	}
	template< typename T >
	static void ConstructSequence(T* ptr, size_t num, T const* ptrValue)
	{
		ConstructSequenceInternal(ptr, num, ptrValue, Meta::IsPod< T >::Type());
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
				FTypeMemoryOp::Construct(ptr, *itBegin);
				++ptr;
			}
		}
	}

	template< class T >
	static void Move(T* ptr, T* from)
	{
		MoveInternal(ptr, from, Meta::IsPod< T >::Type());
	}
	template< class T >
	static void MoveSequence(T* ptr, size_t num, T* from)
	{
		CHECK(from + num <= ptr || ptr < from );
		MoveSequenceInternal(ptr, num , from, Meta::IsPod< T >::Type());
	}

	template< class T >
	static void MoveRightOverlap(T* ptr, size_t num, T* from)
	{
		CHECK(from + num > ptr || num == 1);
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

private:
	template< class T >
	static void ConstructInternal(T* ptr, Meta::TureType)
	{
		*ptr = T();
	}
	template< class T >
	static void ConstructInternal(T* ptr, Meta::FalseType)
	{
		::new (ptr) T();
	}
	template< class T >
	static void ConstructInternal(T* ptr, T const& val, Meta::TureType)
	{
		*ptr = val;
	}
	template< class T >
	static void ConstructInternal(T* ptr, T const& val , Meta::FalseType )
	{
		::new (ptr) T(val);
	}
	template< class T >
	static void ConstructSequenceInternal(T* ptr, size_t num, Meta::TureType)
	{
		std::fill_n(ptr, num, T());
	}
	template< class T >
	static void ConstructSequenceInternal(T* ptr, size_t num, Meta::FalseType)
	{
		while (num)
		{
			Construct(ptr);
			++ptr;
			--num;
		}
	}
	template< class T >
	static void ConstructSequenceInternal(T* ptr, size_t num, T const& val, Meta::TureType)
	{
		std::fill_n(ptr, num, val);
	}
	template< class T >
	static void ConstructSequenceInternal(T* ptr, size_t num, T const& val, Meta::FalseType)
	{
		while (num)
		{
			Construct(ptr, val);
			++ptr;
			--num;
		}
	}
	template< class T >
	static void ConstructSequenceInternal(T* ptr, size_t num, T const* ptrValue, Meta::TureType)
	{
		FMemory::Copy(ptr, ptrValue, sizeof(T) * num);
	}
	template< class T >
	static void ConstructSequenceInternal(T* ptr, size_t num, T const* ptrValue, Meta::FalseType)
	{
		while (num)
		{
			new (ptr) T(*ptrValue);
			++ptr;
			++ptrValue;
			--num;
		}
	}

	template< class T >
	static void MoveInternal(T* ptr, T* from, Meta::TureType)
	{
		*ptr = *from;
	}
	template< class T >
	static void MoveInternal(T* ptr, T* from, Meta::FalseType)
	{
		new (ptr) T(std::move(*from));
		Destruct(from);
	}
	template< class T >
	static void MoveSequenceInternal(T* ptr, size_t num, T* from, Meta::TureType)
	{
		FMemory::Move(ptr, from, sizeof(T)*num);
	}
	template< class T >
	static void MoveSequenceInternal(T* ptr, size_t num, T* from, Meta::FalseType)
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
};
#endif // TypeConstruct_H_CF8B1CE1_FBAC_450E_B932_B8C76DF1B46A
