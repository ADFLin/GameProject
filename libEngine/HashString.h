#pragma once
#ifndef HashString_h__
#define HashString_h__

#include "CoreShare.h"
#include "Core/IntegerType.h"



enum class EName : uint8
{
	None = 0,
};

class HashString
{
public:
	static void Initialize();

	HashString()
	{
		mIndex = 0;
		mNumber = 0;
	}
	HashString(EName name)
	{
		mIndex = uint32(name) << 1;
		mNumber = 0;
	}

	CORE_API HashString(char const* str, bool bCaseSensitive = true);

	bool operator == (HashString const& rhs) const
	{
		return mIndex == rhs.mIndex && mNumber == rhs.mNumber;
	}
	CORE_API bool operator == (char const* str ) const;
	bool operator != (char const* str) { return !operator == (str); }

	CORE_API char const* toString() const;
	operator char const* () const { return toString(); }

	bool operator == (EName name) const { return mIndex == uint32(name) << 1; }
	bool operator != (EName name) const { return !operator == ( name ); }


	uint32 getIndex() const { return mIndex;  }


	// support std::map
	bool operator < (HashString const& rhs) const
	{
		return mIndex < rhs.mIndex || (!(rhs.mIndex < mIndex) && mNumber < rhs.mNumber);
	}
	
	CORE_API friend uint32 hash_value(HashString const & string);
private:
	uint32 getSlotIndex() const { return mIndex >> 1;  }
	bool   isCastSensitive() const { return !(mIndex & 0x1);  }
	CORE_API HashString(EName name, char const* str);
	uint32 mNumber;
	uint32 mIndex;


};

#include <functional>

namespace std
{
	template<>
	struct hash< HashString >
	{
		size_t operator()(const HashString& value ) const
		{
			return hash_value(value);
		}
	};
}

#endif // HashString_h__