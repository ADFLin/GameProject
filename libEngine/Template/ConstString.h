#pragma once
#ifndef ConstString_H_4619B714_A202_464F_A699_E7D7CCA85A48
#define ConstString_H_4619B714_A202_464F_A699_E7D7CCA85A48

template< class TChar >
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

	constexpr const TChar *c_str() const
	{
		return mData;
	}

	constexpr TChar operator[](int index) const { return mData[index]; }

	const size_t mSize;
	const TChar *mData = nullptr;
};

using ConstString = TConstString< char >;
using ConstStringW = TConstString< wchar_t >;

#endif // ConstString_H_4619B714_A202_464F_A699_E7D7CCA85A48