#pragma once
#ifndef HashString_h__
#define HashString_h__

#include "CoreShare.h"
#include "Core/IntegerType.h"
#include "Core/TypeHash.h"
#include "Template/StringView.h"
#include "Serialize/SerializeFwd.h"


enum class EName : uint8
{
	None = 0,
};

class HashString
{
public:
	static void Initialize();
	static CORE_API bool Find(StringView const& str, bool bCaseSensitive, HashString& outValue);

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

	HashString(char const* str, bool bCaseSensitive = false)
	{
		init(str, bCaseSensitive);
	}

	HashString(StringView const& str, bool bCaseSensitive = false)
	{
		init(str.data(), str.size(), bCaseSensitive);
	}
	bool operator == (HashString const& rhs) const
	{
		return mIndex == rhs.mIndex && mNumber == rhs.mNumber;
	}

	CORE_API bool operator == (char const* str ) const;
	bool operator != (char const* str) { return !operator == (str); }

	CORE_API char const* c_str() const;
	operator char const* () const { return c_str(); }

	bool operator == (EName name) const { return mIndex == uint32(name) << 1; }
	bool operator != (EName name) const { return !operator == ( name ); }

	bool empty() const {  return mIndex == 0;  }

	HashString& operator = (char const* other) { init(other); return *this; }
	uint32 getIndex() const { return mIndex;  }


	// support std::map
	bool operator < (HashString const& rhs) const
	{
		return mIndex < rhs.mIndex || (!(rhs.mIndex < mIndex) && mNumber < rhs.mNumber);
	}
	
	CORE_API uint32 getTypeHash() const;
	friend uint32 HashValue(HashString const & string) { return string.getTypeHash(); }
	template< class OP >
	void serialize(OP& op)
	{
		if (OP::IsLoading)
		{
			bool bCS;
			std::string str;
			op & bCS & str;
			init(str.c_str(), str.length(),bCS);
		}
		else
		{
			bool bCS = isCastSensitive();
			std::string str = c_str();
			op & bCS & str;
		}
	}
private:
	CORE_API void init(char const* str, bool bCaseSensitive = false);
	CORE_API void init(char const* str, int len , bool bCaseSensitive = false);
	uint32 getSlotIndex() const { return mIndex >> 1;  }
	bool   isCastSensitive() const { return !(mIndex & 0x1);  }
	CORE_API HashString(EName name, char const* str);
	uint32 mNumber;
	uint32 mIndex;

};

EXPORT_MEMBER_HASH_TO_STD(HashString);

#endif // HashString_h__