#pragma once
#ifndef StringView_H_CA843BB9_3C9D_487D_9EC7_9D3F1EE8E800
#define StringView_H_CA843BB9_3C9D_487D_9EC7_9D3F1EE8E800

#include "CString.h"
#include "Core/StringConv.h"
#include "EnumCommon.h"
#include "Meta/EnableIf.h"
#include "Meta/MetaBase.h"


template< typename CharT, size_t BufferSize = 256 >
struct TCStringConvertible
{
	TCStringConvertible(CharT const* data, size_t num)
	{
		if (num)
		{
			CHECK(data);
			if (data[num] == 0)
			{
				mPtr = data;
			}
			else
			{
				CHECK(ARRAY_SIZE(mBuffer) > num + 1);
				FCString::CopyN(mBuffer, data, num);
				mBuffer[num] = 0;
				mPtr = mBuffer;
			}
		}
		else
		{
			mPtr = STRING_LITERAL(CharT, "");
		}
	}

	operator CharT const* () const { return mPtr; }

	CharT const* mPtr;
	CharT mBuffer[BufferSize];
};



template< typename CharT, size_t BufferSize>
struct TFormatArgPolicy < TCStringConvertible<CharT, BufferSize> >
{
	static char const* Convert(TCStringConvertible<CharT, BufferSize>& value)
	{
		return value;
	}
};

template < typename CharT >
struct TCStringMutable
{
	TCStringMutable(CharT const* data, size_t num)
	{
		if (num)
		{
			CHECK(data);
			mPtr = data;
			mEndPtr = const_cast<CharT*>(mPtr + num);
			mOldChar = *mEndPtr;
			*mEndPtr = 0;
		}
		else
		{
			mOldChar = 0;
			mPtr = &mOldChar;
			mEndPtr = &mOldChar;
		}
	}

	~TCStringMutable()
	{
		*mEndPtr = mOldChar;
	}
	operator CharT const* () const { return mPtr; }

	CharT const* mPtr;
	CharT* mEndPtr;
	CharT  mOldChar;
};


template< typename CharT >
struct TFormatArgPolicy < TCStringMutable<CharT> >
{
	static char const* Convert(TCStringMutable<CharT>& value)
	{
		return value;
	}
};

template < typename CharT >
class TStringView
{
public:
	using StdString = typename TStringTraits<CharT>::StdString;

	TStringView() = default;
	explicit TStringView( EForceInit ):mData(nullptr),mNum(0){}

	template< typename Q, TEnableIf_Type< Meta::IsSameType< CharT const*, Q >::Value , bool > = true >
	TStringView(Q str)
		:mData(str), mNum(FCString::Strlen(str)){}

	template< size_t N >
	constexpr TStringView(CharT const (&str)[N])
		:mData(str), mNum(N - 1){}

	TStringView(CharT const* str , size_t num )
		:mData(str),mNum(num){}

	TStringView(StdString const& str)
		:mData(str.data()),mNum(str.length()){ }

	bool operator == (CharT const* other) const
	{
		return FCString::CompareN( mData , other , mNum ) == 0;
	}
	bool operator != (CharT const* other) const
	{
		return !this->operator == (other);
	}

	bool operator < (TStringView const& rhs) const
	{
		return compare(rhs) < 0;
	}

	//operator T const* () const { return mData; }

	using iterator = CharT const*;
	iterator begin() { return mData; }
	iterator end() { return mData + mNum; }

	using const_iterator = CharT const*;
	const_iterator begin() const { return mData; }
	const_iterator end() const { return mData + mNum; }

	constexpr CharT operator[](size_t idx) const { assert(idx < mNum); return mData[idx]; }

	constexpr CharT const* data() const { return mData; }
	constexpr size_t size() const { return mNum; }
	constexpr size_t length() const { return mNum; }

	void trimStart()
	{
		if (mData)
		{
			while (mNum && FCString::IsSpace(*mData))
			{
				++mData;
				--mNum;
			}
		}
	}
	void trimEnd()
	{
		if (mData)
		{
			while (mNum && FCString::IsSpace(mData[mNum - 1]))
			{
				--mNum;
			}
		}
	}

	void trimStartAndEnd()
	{
		if (mData)
		{
			while (mNum && FCString::IsSpace(*mData))
			{
				++mData;
				--mNum;
			}
			while (mNum && FCString::IsSpace(mData[mNum - 1]))
			{
				--mNum;
			}
		}
	}

	bool operator == (TStringView const& rhs) const
	{
		return compare(rhs) == 0;
	}
	bool operator != (TStringView const& rhs) const
	{
		return compare(rhs) != 0;
	}

	int compare(TStringView const& other) const
	{
		if( other.size() < size() )
			return -other.compareInternal(data(), size());
		return compareInternal(other.data(), other.size());
	}

	int compare(CharT const* other) const
	{
		char const* p1 = data();
		char const* p2 = other;
		for( int i = size(); i; --i, ++p1, ++p2 )
		{
			CHECK(*p1 != 0);
			if( *p2 == 0 )
				return 1;
			if( *p1 != *p2 )
				return (*p1 > *p2) ? 1 : -1;
		}
		return (*p2 == 0) ? 0 : -1;
	}


	//void resize(size_t inSize) { assert(mNum >= inSize); mNum = inSize; }

	StdString toStdString() const { return StdString(mData, mNum); }

	template< size_t BufferSize = 256 >
	TCStringConvertible<CharT, BufferSize> toCString() const { return TCStringConvertible<CharT , BufferSize>(mData, mNum); }
	TCStringMutable<CharT> toMutableCString() const { return TCStringMutable<CharT>(mData, mNum); }

	template<class Q>
	Q toValue() const
	{
		return FStringConv::To<Q>(mData, (int)mNum);
	}
	template<class Q>
	bool toValueCheck(Q& outValue) const
	{
		return FStringConv::ToCheck<Q>(mData, (int)mNum, outValue);
	}

protected:

	int compareInternal(CharT const* other, size_t numOhter) const
	{
		CHECK(size() <= numOhter);
		CharT const* p1 = data();
		CharT const* p2 = other;
		for(size_t i = 0; i < size(); ++i, ++p1, ++p2 )
		{
			CHECK(*p1 != 0 && *p2 != 0);
			if( *p1 != *p2 )
				return (*p1 > *p2) ? 1 : -1;
		}
		return (size() == numOhter) ? 0 : -1;
	}


	CharT const* mData;
	size_t   mNum;
};


using StringView = TStringView<char>;
using WStringView = TStringView<wchar_t>;


#endif // StringView_H_CA843BB9_3C9D_487D_9EC7_9D3F1EE8E800