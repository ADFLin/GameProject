#ifndef String_h__
#define String_h__

#include "CString.h"
#include "RefCount.h"
#include "ThreadSafeCounter.h"
#include "Core/TypeHash.h"

#include <algorithm>
#include <cassert>
#include <new>
#include <type_traits>
#include <utility>
#include <iostream>
#include <string>
#include "Template/StringView.h"

template< typename CharT >
class TString
{
public:
	using SizeType = uint32;
	
	enum ESizeCategory
	{
		Small,
		Middle,
		Large,
	};
	
	struct MediumLarge
	{
		CharT* ptr;
		size_t size;
		size_t capacity;
	};

	struct RefCountedHeader
	{
		ThreadSafeCounter refCount;
	};

	// 24 bytes for 64-bit architecture
	union 
	{
		uint8 bytes[24];
		CharT buffer[24 / sizeof(CharT)];
		MediumLarge ml;
	};

	static constexpr size_t kStorageBytes = 24;
	static constexpr size_t kMaxSmallSize = (kStorageBytes - 1) / sizeof(CharT);
	static constexpr size_t kMaxMediumSize = 255;
	
	// Category bits stored in the high bits of the last byte
	static constexpr uint8 kCategorySmall  = 0x00;
	static constexpr uint8 kCategoryMedium = 0x80;
	static constexpr uint8 kCategoryLarge  = 0x40;
	static constexpr uint8 kCategoryMask   = 0xC0;

public:
	TString()
	{
		initSmall(0);
	}

	TString(const CharT* str)
	{
		size_t len = FCString::Strlen(str);
		construct(str, len);
	}
	
	TString(const CharT* str, size_t len)
	{
		construct(str, len);
	}

	TString(const std::basic_string<CharT>& str)
	{
		construct(str.c_str(), str.length());
	}

	TString(TStringView<CharT> str)
	{
		construct(str.data(), str.length());
	}

	TString(const TString& other)
	{
		copyFrom(other);
	}

	TString(TString&& other) noexcept
	{
		moveFrom(std::move(other));
	}

	~TString()
	{
		destroy();
	}

	TString& operator=(const TString& other)
	{
		if (this != &other)
		{
			// Optimization: check if already sharing same buffer (for Large)
			if (category() == Large && other.category() == Large && ml.ptr == other.ml.ptr)
			{
				return *this;
			}
			destroy();
			copyFrom(other);
		}
		return *this;
	}

	TString& operator=(TString&& other) noexcept
	{
		if (this != &other)
		{
			destroy();
			moveFrom(std::move(other));
		}
		return *this;
	}

	TString& operator=(const CharT* str)
	{
		size_t len = FCString::Strlen(str);
		assign(str, len);
		return *this;
	}

	TString& operator=(const std::basic_string<CharT>& str)
	{
		assign(str.c_str(), str.length());
		return *this;
	}

	TString& operator=(TStringView<CharT> str)
	{
		assign(str.data(), str.length());
		return *this;
	}

	operator TStringView<CharT>() const
	{
		return TStringView<CharT>(data(), size());
	}

	TString& operator+=(const TString& other)
	{
		append(other.data(), other.size());
		return *this;
	}

	TString& operator+=(const CharT* str)
	{
		append(str, FCString::Strlen(str));
		return *this;
	}

	TString& operator+=(CharT c)
	{
		append(&c, 1);
		return *this;
	}
	
	void append(const CharT* str, size_t len)
	{
		if (len == 0) return;
		
		// Check for aliasing
		bool isAliased = (str >= data() && str < data() + size());
		size_t offset = isAliased ? (str - data()) : 0;

		size_t curSize = size();
		size_t newSize = curSize + len;
		
		if (category() != Small)
			ensureUnique();

		if (newSize > capacity())
		{
			reserve(newSize);
		}
		
		CharT* p = data();
		
		// Adjust str if it was aliased and buffer potentially moved
		if (isAliased)
		{
			str = p + offset;
		}

		FCString::CopyN_Unsafe(p + curSize, str, len); 
		p[newSize] = CharT(0);
		setSize(newSize);
	}

	const CharT* c_str() const
	{
		return data();
	}

	const CharT* data() const
	{
		if (category() == Small)
		{
			return buffer;
		}
		return ml.ptr;
	}
	
	CharT* data()
	{
		if (category() == Small)
		{
			return buffer;
		}
		// For Medium/Large, we might need to unshare if Large
		ensureUnique();
		return ml.ptr;
	}

	bool empty() const
	{
		return size() == 0;
	}

	size_t length() const
	{
		return size();
	}

	size_t size() const
	{
		if (category() == Small)
		{
			// Last byte stores (kMaxSmallSize - size)
			return kMaxSmallSize - bytes[kStorageBytes - 1];
		}
		return ml.size;
	}

	size_t capacity() const
	{
		if (category() == Small)
		{
			return kMaxSmallSize;
		}
		// Mask out category bits from capacity
		return ml.capacity & ~(static_cast<size_t>(kCategoryMask) << ((sizeof(size_t) - 1) * 8));
	}

	// STL Container Support
	bool operator==(const TString& other) const
	{
		return size() == other.size() && FCString::CompareN(data(), other.data(), size()) == 0;
	}

	bool operator!=(const TString& other) const
	{
		return !(*this == other);
	}

	bool operator<(const TString& other) const
	{
		return FCString::Compare(data(), other.data()) < 0;
	}

	uint32 getTypeHash() const
	{
		return FCString::StrHash(data());
	}
	
	ESizeCategory category() const
	{
		uint8 lastByte = bytes[kStorageBytes - 1];
		if ((lastByte & kCategoryMask) == kCategoryMedium) return Middle;
		if ((lastByte & kCategoryMask) == kCategoryLarge) return Large;
		return Small;
	}

	void resize(size_t newSize)
	{
		resize(newSize, CharT(0));
	}
	
	void resize(size_t newSize, CharT fill)
	{
		size_t currentSize = size();
		if (newSize == currentSize) return;

		if (newSize < currentSize)
		{
			if (category() != Small)
			{
				// Ensure unique before modifying
				ensureUnique();
			}

			// Shrink
			if (category() == Small)
			{
				setSmallSize(newSize);
				buffer[newSize] = CharT(0);
			}
			else
			{
				// Medium or Large (now unique)
				ml.size = newSize;
				ml.ptr[newSize] = CharT(0);
			}
		}
		else
		{
			// Grow
			if (category() != Small)
			{
				ensureUnique(); 
			}
			
			if (newSize > capacity())
			{
				reserve(newSize);
			}
			
			// We might have changed category in reserve, assume generic access
			CharT* p = data(); // ensureUnique called inside if needed, but we called reserve which ensures unique
			for (size_t i = currentSize; i < newSize; ++i)
			{
				p[i] = fill;
			}
			setSize(newSize);
			p[newSize] = CharT(0);
		}
	}

	void reserve(size_t newCap)
	{
		if (newCap <= capacity()) return;

		if (category() == Small && newCap <= kMaxSmallSize) return;

		size_t curSize = size();
		// Reallocate
		// If currently Large and shared, we MUST allocate new buffer anyway.
		// If currently Large and unique, we verify if realloc is needed (checked above).
		
		size_t allocCap = std::max(newCap, curSize + curSize / 2);
		
		CharT* newPtr = nullptr;
		ESizeCategory newCat;

		if (allocCap > kMaxMediumSize)
		{
			newCat = Large;
			newPtr = allocLarge(allocCap);
		}
		else
		{
			newCat = Middle;
			newPtr = new CharT[allocCap + 1];
		}

		// Copy existing data
		const CharT* oldPtr = (category() == Small) ? buffer : ml.ptr;
		if (curSize > 0)
		{
			FCString::CopyN_Unsafe(newPtr, oldPtr, curSize);
		}
		newPtr[curSize] = CharT(0);

		destroy(); // Decrement ref count / delete old
		
		ml.ptr = newPtr;
		ml.size = curSize;
		setCapacity(allocCap, newCat);
	}
	
	void clear()
	{
		destroy();
		initSmall(0);
	}
	
	void assign(const CharT* str, size_t len)
	{
		// Basic implementation: destroy and construct
		// Optimization possible
		destroy();
		construct(str, len);
	}

private:
	void construct(const CharT* str, size_t len)
	{
		if (len <= kMaxSmallSize)
		{
			initSmall(len);
			if (len > 0) FCString::CopyN_Unsafe(buffer, str, len);
			buffer[len] = CharT(0);
		}
		else if (len <= kMaxMediumSize)
		{
			// Medium
			size_t cap = len;
			CharT* newPtr = new CharT[cap + 1];
			FCString::CopyN_Unsafe(newPtr, str, len);
			newPtr[len] = CharT(0);
			
			ml.ptr = newPtr;
			ml.size = len;
			setCapacity(cap, Middle);
		}
		else
		{
			// Large
			size_t cap = len;
			CharT* newPtr = allocLarge(cap);
			FCString::CopyN_Unsafe(newPtr, str, len);
			newPtr[len] = CharT(0);
			
			ml.ptr = newPtr;
			ml.size = len;
			setCapacity(cap, Large);
		}
	}

	void destroy()
	{
		if (category() == Small)
		{
			// nothing
		}
		else if (category() == Middle)
		{
			delete[] ml.ptr;
		}
		else // Large
		{
			releaseLarge(ml.ptr);
		}
	}
	
	void copyFrom(const TString& other)
	{
		ESizeCategory cat = other.category();
		if (cat == Small)
		{
			for(int i=0; i<kStorageBytes; ++i) bytes[i] = other.bytes[i];
		}
		else if (cat == Middle)
		{
			// Deep copy
			size_t len = other.ml.size;
			size_t cap = other.capacity();
			CharT* newPtr = new CharT[cap + 1];
			FCString::CopyN_Unsafe(newPtr, other.ml.ptr, len);
			newPtr[len] = CharT(0);
			
			ml.ptr = newPtr;
			ml.size = len;
			setCapacity(cap, Middle);
		}
		else // Large
		{
			// CoW share
			RefCountedHeader* header = getHeader(other.ml.ptr);
			header->refCount.add(1);
			
			ml.ptr = other.ml.ptr;
			ml.size = other.ml.size;
			// Copy capacity (which includes category bits)
			ml.capacity = other.ml.capacity; 
		}
	}

	void moveFrom(TString&& other)
	{
		for(int i=0; i<kStorageBytes; ++i) bytes[i] = other.bytes[i];
		other.initSmall(0);
	}

	void initSmall(size_t s)
	{
		for(int i=0; i<kStorageBytes; ++i) bytes[i] = 0;
		setSmallSize(s);
		buffer[0] = CharT(0);
	}

	void setSmallSize(size_t s)
	{
		uint8 val = static_cast<uint8>(kMaxSmallSize - s);
		bytes[kStorageBytes - 1] = val;
	}

	void setCapacity(size_t cap, ESizeCategory cat)
	{
		ml.capacity = cap;
		size_t shift = (sizeof(size_t) - 1) * 8;
		size_t mask = 0;
		if (cat == Middle) mask = static_cast<size_t>(kCategoryMedium) << shift;
		else if (cat == Large) mask = static_cast<size_t>(kCategoryLarge) << shift;
		
		ml.capacity |= mask;
	}
	
	void setSize(size_t s)
	{
		if (category() == Small)
		{
			setSmallSize(s);
		}
		else
		{
			ml.size = s;
		}
	}
	
	// Large/CoW helpers
	RefCountedHeader* getHeader(CharT* ptr) const
	{
		return reinterpret_cast<RefCountedHeader*>(ptr) - 1;
	}

	CharT* allocLarge(size_t cap)
	{
		size_t allocSize = sizeof(RefCountedHeader) + (cap + 1) * sizeof(CharT);
		uint8* mem = new uint8[allocSize];
		RefCountedHeader* header = new (mem) RefCountedHeader();
		header->refCount.set(1);
		
		return reinterpret_cast<CharT*>(header + 1);
	}
	
	void releaseLarge(CharT* ptr)
	{
		RefCountedHeader* header = getHeader(ptr);
		if (header->refCount.add(-1) == 1) // returns OLD value
		{
			// count dropped to 0
			// Manually call destructor if RefCountedHeader had non-trivial one? 
			// ThreadSafeCounter trivial? yes.
			delete[] reinterpret_cast<uint8*>(header);
		}
	}
	
	void ensureUnique()
	{
		if (category() == Large)
		{
			RefCountedHeader* header = getHeader(ml.ptr);
			if (header->refCount.get() > 1)
			{
				// Unshare
				size_t curSize = ml.size;
				size_t curCap = capacity();
				
				CharT* newPtr = allocLarge(curCap);
				FCString::CopyN_Unsafe(newPtr, ml.ptr, curSize);
				newPtr[curSize] = CharT(0);
				
				// Release old
				releaseLarge(ml.ptr);
				
				ml.ptr = newPtr;
				// ml.size/capacity unchanged (capacity might change if we chose to optimize)
			}
		}
	}

};

template< typename CharT >
TString<CharT> operator+(const TString<CharT>& lhs, const TString<CharT>& rhs)
{
	TString<CharT> ret;
	ret.reserve(lhs.size() + rhs.size());
	ret += lhs;
	ret += rhs;
	return ret;
}

template< typename CharT >
TString<CharT> operator+(const TString<CharT>& lhs, const CharT* rhs)
{
	TString<CharT> ret;
	size_t rhsLen = FCString::Strlen(rhs);
	ret.reserve(lhs.size() + rhsLen);
	ret += lhs;
	ret.append(rhs, rhsLen);
	return ret;
}

template< typename CharT >
TString<CharT> operator+(const CharT* lhs, const TString<CharT>& rhs)
{
	TString<CharT> ret;
	size_t lhsLen = FCString::Strlen(lhs);
	ret.reserve(lhsLen + rhs.size());
	ret.append(lhs, lhsLen);
	ret += rhs;
	return ret;
}

template< typename CharT >
TString<CharT> operator+(const TString<CharT>& lhs, CharT rhs)
{
	TString<CharT> ret;
	ret.reserve(lhs.size() + 1);
	ret += lhs;
	ret += rhs;
	return ret;
}

template< typename CharT >
std::basic_ostream<CharT>& operator<<(std::basic_ostream<CharT>& os, const TString<CharT>& str)
{
	os << str.c_str();
	return os;
}

template< typename CharT >
std::basic_istream<CharT>& operator>>(std::basic_istream<CharT>& is, TString<CharT>& str)
{
	std::basic_string<CharT> temp;
	is >> temp;
	str = temp.c_str();
	return is;
}

using StringA = TString< char >;
using StringW = TString< wchar_t >;
using String  = TString< TChar >;

namespace std
{
	template<typename CharT>
	struct hash< TString<CharT> >
	{
		size_t operator()(const TString<CharT>& str) const
		{
			return str.getTypeHash();
		}
	};
}

#endif // String_h__