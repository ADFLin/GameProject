#ifndef Flag_h__
#define Flag_h__

#include "MetaBase.h"
#include "Core/IntegerType.h"

#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif

template < class T = uint32 >
class FlagValue
{
public:
	FlagValue() { clear(); }
	FlagValue(T value) :mValue(value) {}

	T    getValue() const { return mValue; }
	void setValue(T value) { mValue = value; }
	void clear() { mValue = 0; }

	void addBits(T value) { mValue |= value; }
	void removeBits(T value) { mValue &= ~value; }
	bool checkBits(T value) const { return !!(mValue & value); }

	void add(unsigned bitPos) { mValue |= BIT(bitPos); }
	void remove(unsigned bitPos) { mValue &= ~BIT(bitPos); }
	bool check(unsigned bitPos) const { return !!(mValue & BIT(bitPos)); }


	operator T() const { return mValue; }
	FlagValue& operator = (T value) { mValue = value; return *this; }

	FlagValue operator | (FlagValue const& rhs) const { return FlagValue(mValue | rhs.mValue); }
	FlagValue operator & (FlagValue const& rhs) const { return FlagValue(mValue & rhs.mValue); }
	FlagValue operator ^ (FlagValue const& rhs) const { return FlagValue(mValue ^ rhs.mValue); }
	FlagValue& operator |= (FlagValue const& rhs) { mValue |= rhs.mValue; return *this; }
	FlagValue& operator &= (FlagValue const& rhs) { mValue &= rhs.mValue; return *this; }
	FlagValue& operator ^= (FlagValue const& rhs) { mValue ^= rhs.mValue; return *this; }
	
private:
	T    mValue;
};

template < int BitNum , class T = uint32 >
class FlagValueArray
{
	static int const TypeBitNum = sizeof(T) * 8;
	static int const StorageSize = (BitNum - 1) / TypeBitNum + 1;
public:
	FlagValueArray() { clear(); }
	void clear()
	{
		for( int i = 0; i < StorageSize; ++i )
			mStorage[i] = 0;
	}

	void add(unsigned bitPos)
	{
		mStorage[bitPos / TypeBitNum] |= BIT(bitPos % TypeBitNum);
	}
	void remove(unsigned bitPos)
	{
		mStorage[bitPos / TypeBitNum] &= ~BIT(bitPos % TypeBitNum);
	}
	bool check(unsigned bitPos) const
	{
		return !!(mStorage[bitPos / TypeBitNum] & BIT(bitPos % TypeBitNum));
	}
	T    getValue(int idx) const { return mStorage[idx]; }

	FlagValueArray operator | (FlagValueArray const& rhs) const { FlagValueArray result; for( int i = 0; i < StorageSize; ++i ) { result.mStorage[i] = mStorage[i] | rhs.mStorage[i]; } return result; }
	FlagValueArray operator & (FlagValueArray const& rhs) const { FlagValueArray result; for( int i = 0; i < StorageSize; ++i ) { result.mStorage[i] = mStorage[i] & rhs.mStorage[i]; } return result; }
	FlagValueArray operator ^ (FlagValueArray const& rhs) const { FlagValueArray result; for( int i = 0; i < StorageSize; ++i ) { result.mStorage[i] = mStorage[i] ^ rhs.mStorage[i]; } return result; }
	FlagValueArray& operator |= (FlagValueArray const& rhs) { for( int i = 0; i < StorageSize; ++i ) { mStorage[i] |= rhs.mStorage[i]; } return *this; }
	FlagValueArray& operator &= (FlagValueArray const& rhs) { for( int i = 0; i < StorageSize; ++i ) { mStorage[i] &= rhs.mStorage[i]; } return *this; }
	FlagValueArray& operator ^= (FlagValueArray const& rhs) { for( int i = 0; i < StorageSize; ++i ) { mStorage[i] ^= rhs.mStorage[i]; } return *this; }
private:
	T  mStorage[StorageSize];
};

namespace Private
{
	template< int BitNum, class T  >
	struct FlagBitsSelector
	{
		static int const N = 8 * sizeof(T);
		typedef typename Meta::Select<
			BitNum <= N, FlagValue< T >, FlagValueArray< BitNum , T >
		>::Type Type;
	};

}

template < int BitNum  , class T = uint32 >
class FlagBits : public Private::FlagBitsSelector< BitNum , T >::Type
{

};

#endif // Flag_h__