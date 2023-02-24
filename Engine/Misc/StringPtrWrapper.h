#pragma once

#include "Core/TypeHash.h"
#include "CString.h"

template< typename T , bool bIgnoreCase = false >
struct TStringPtrWrapper
{
	TStringPtrWrapper(T const* str) :str(str) {}

	T const* str;
	bool operator == (TStringPtrWrapper const& rhs) const
	{
		if constexpr (bIgnoreCase)
		{
			return FCString::CompareIgnoreCase(str, rhs.str) == 0;
		}
		else
		{
			return FCString::Compare(str, rhs.str) == 0;
		}

	}

	bool operator < (TStringPtrWrapper const& rhs) const
	{
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

using StringPtrWrapper = TStringPtrWrapper<char>;
using WStringPtrWrapper = TStringPtrWrapper<wchar_t>;

EXPORT_MEMBER_HASH_TO_STD(StringPtrWrapper);
EXPORT_MEMBER_HASH_TO_STD(WStringPtrWrapper);