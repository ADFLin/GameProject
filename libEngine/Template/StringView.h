#pragma once
#ifndef StringView_H_CA843BB9_3C9D_487D_9EC7_9D3F1EE8E800
#define StringView_H_CA843BB9_3C9D_487D_9EC7_9D3F1EE8E800

#include "CString.h"

template < class T >
class TStringView
{
public:
	typedef typename TStringTraits<T>::StdString StdString;

	TStringView():mData(nullptr),mNum(0){}

	TStringView(T const* str)
		:mData(str), mNum( str ? FCString::Strlen(str) : 0 ){}

	TStringView( T const* str , size_t num )
		:mData(str),mNum(num){}

	bool operator == (T const* other) const
	{
		return FCString::CompareN( mData , other , mNum ) == 0;
	}
	bool operator != (T const* other) const
	{
		return !this->operator == (other);
	}

	bool operator < (TStringView const& rhs) const
	{
		return compare(rhs) < 0;
	}

	//operator T const* () const { return mData; }

	typedef T const* iterator;
	iterator begin() { return mData; }
	iterator end() { return mData + mNum; }

	typedef T const* const_iterator;
	const_iterator begin() const { return mData; }
	const_iterator end() const { return mData + mNum; }

	T  operator[](size_t idx) const { assert(idx < mNum); return mData[idx]; }

	T const* data() const { return mData; }
	size_t   size() const { return mNum; }
	size_t   length() const { return mNum; }


	int compare(TStringView const& other) const
	{
		if( other.size() < size() )
			return -other.compareInternal(data(), size());
		return compareInternal(other.data(), other.size());
	}

	int compare(T const* other) const
	{
		char const* p1 = data();
		char const* p2 = other;
		for( int i = size(); i; --i, ++p1, ++p2 )
		{
			assert(*p1 != 0);
			if( *p2 == 0 )
				return 1;
			if( *p1 != *p2 )
				return (*p1 > *p2) ? 1 : -1;
		}
		return (*p2 == 0) ? 0 : -1;
	}


	//void resize(size_t inSize) { assert(mNum >= inSize); mNum = inSize; }

	StdString toStdString() const { return StdString(mData, mNum); }

	template< size_t BufferSize >
	struct TCStringConvable
	{
		TCStringConvable(T const* data, size_t num)
		{
			if( data == nullptr || num == 0 )
			{
				mPtr = STRING_LITERAL(T, "");
			}
			else if( data[num] == 0 )
			{
				mPtr = data;
			}
			else
			{
				assert(ARRAY_SIZE(mBuffer) > num + 1);
				FCString::CopyN(mBuffer, data, num);
				mBuffer[num] = 0;
				mPtr = mBuffer;
			}
		}

		operator T const* () const { return mPtr; }

		T const* mPtr;
		T mBuffer[BufferSize];
	};

	template< size_t BufferSize = 256 >
	TCStringConvable< BufferSize > toCString() const { return TCStringConvable<BufferSize>(mData, mNum); }

	bool toFloat(float& value) const
	{
		//TODO : optimize
		StdString temp = toStdString();
		T* endPtr;
		value = FCString::Strtof(temp.c_str(), &endPtr);
		return endPtr == &temp.back() + 1;
	}
protected:

	int compareInternal(char const* other, int numOhter) const
	{
		assert(size() <= numOhter);
		T const* p1 = data();
		T const* p2 = other;
		for( int i = 0; i < size(); ++i, ++p1, ++p2 )
		{
			assert(*p1 != 0 && *p2 != 0);
			if( *p1 != *p2 )
				return (*p1 > *p2) ? 1 : -1;
		}
		return (size() == numOhter) ? 0 : -1;
	}


	T const* mData;
	size_t   mNum;
};

typedef TStringView<char>    StringView;
typedef TStringView<wchar_t> WStringView;


#endif // StringView_H_CA843BB9_3C9D_487D_9EC7_9D3F1EE8E800