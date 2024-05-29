#pragma once
#ifndef ConstString_H_4619B714_A202_464F_A699_E7D7CCA85A48
#define ConstString_H_4619B714_A202_464F_A699_E7D7CCA85A48

template< class CharT >
class TConstString
{
public:
	template<size_t N>
	constexpr TConstString(const TChar(&str)[N])
		: mSize(N - 1), mData(&str[0])
	{
	}

	constexpr size_t size() const
	{
		return mSize;
	}
	constexpr size_t length() const
	{
		return mSize;
	}
	constexpr const CharT* c_str() const
	{
		return mData;
	}
	constexpr const CharT* data() const
	{
		return mData;
	}

	constexpr CharT operator[](int index) const { return mData[index]; }

	size_t const mSize;
	CharT const *mData;
};

using ConstString = TConstString< char >;
using ConstStringW = TConstString< wchar_t >;

#endif // ConstString_H_4619B714_A202_464F_A699_E7D7CCA85A48