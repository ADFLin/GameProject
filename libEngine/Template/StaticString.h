#pragma once
#ifndef StaticString_H_A4C090CC_B466_4139_BFE1_959659499B9F
#define StaticString_H_A4C090CC_B466_4139_BFE1_959659499B9F

template< class TChar >
class TStaticString
{
public:
	template<size_t N>
	constexpr TStaticString(const TChar(&str)[N])
		: mSize(N - 1), mData(&str[0])
	{
	}

	constexpr size_t size() const
	{
		return mSize;
	}

	constexpr const TChar *c_str() const
	{
		return mData;
	}

	constexpr TChar operator[](int index) const { return mData[index]; }

	const size_t mSize;
	const TChar *mData = nullptr;
};

typedef TStaticString< char > StaticString;
typedef TStaticString< wchar_t > WStaticString;

#endif // StaticString_H_A4C090CC_B466_4139_BFE1_959659499B9F