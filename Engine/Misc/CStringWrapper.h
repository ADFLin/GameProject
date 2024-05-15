#pragma once

#include "Core/TypeHash.h"
#include "CString.h"

template< typename T , bool bIgnoreCase = false >
struct TCStringWrapper
{
	TCStringWrapper(T const* str) :str(str) {}
	TCStringWrapper(TCStringWrapper const& rhs): str(rhs.str){}

	T const* str;
	bool operator == (TCStringWrapper const& rhs) const
	{
		if (str == nullptr || rhs.str == nullptr)
		{
			return str != rhs.str;
		}
		if constexpr (bIgnoreCase)
		{
			return FCString::CompareIgnoreCase(str, rhs.str) == 0;
		}
		else
		{
			return FCString::Compare(str, rhs.str) == 0;
		}
	}

	bool operator < (TCStringWrapper const& rhs) const
	{
		if (str == nullptr || rhs.str == nullptr)
		{
			return str < rhs.str;
		}
		if constexpr (bIgnoreCase)
		{
			return FCString::CompareIgnoreCase(str, rhs.str) < 0;
		}
		else
		{
			return FCString::Compare(str, rhs.str) < 0;
		}
	}
	operator T const* () const { return str; }
	uint32 getTypeHash() const
	{
		if constexpr (bIgnoreCase)
		{
			return FCString::StriHash(str);
		}
		else
		{
			return FCString::StrHash(str);
		}
	}
};

using CStringWrapper = TCStringWrapper<char>;
using WCStringWrapper = TCStringWrapper<wchar_t>;

EXPORT_MEMBER_HASH_TO_STD(CStringWrapper);
EXPORT_MEMBER_HASH_TO_STD(WCStringWrapper);
EXPORT_MEMBER_HASH_TO_STD(COMMA_SEPARATED(TCStringWrapper<char, true>));
EXPORT_MEMBER_HASH_TO_STD(COMMA_SEPARATED(TCStringWrapper<wchar_t, true>));