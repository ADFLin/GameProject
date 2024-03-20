#ifndef Bitset_h__
#define Bitset_h__

#include "Core/IntegerType.h"
#include "Meta/Unroll.h"
#include <algorithm>

namespace Private
{
    template < int N >
	struct TLog2{};
	template <>
	struct TLog2<  8 >{ enum{ Result = 3 }; };
	template <>
	struct TLog2< 16 >{ enum{ Result = 4 }; };
	template <>
	struct TLog2< 32 >{ enum{ Result = 5 }; };
	template <>
	struct TLog2< 64 >{ enum{ Result = 6 }; };
}

class Bitset
{
public:
	Bitset();
	Bitset( size_t size , bool val = false );
	~Bitset();

	bool    test ( size_t pos ) const;
	bool    any() const;
	bool    none() const { return !any(); }
	size_t  count() const;
	size_t  size() const { return mSize; }
	Bitset& set( size_t pos , bool val = true );
	void    flip();
	void    flip( size_t pos );

	void    resize( size_t size , bool val = false );

	void    fillRLECode( unsigned char* buf , int bufSize )
	{
		for (int n=0, nDComp=0; n < mSize && nDComp < bufSize; n++, nDComp++ )
		{
			if ( buf[nDComp] == 0 )
			{
				n += buf[++nDComp] - 1;
			}
			else
			{
				mStorage[n] = mStorage[nDComp];
			}
		}
	}
private:
	typedef unsigned char StorageType;


	enum
	{
		STORAGE_BIT_NUM    = sizeof( StorageType ) * 8 ,
		STORAGE_POS_OFFSET = Private::TLog2< STORAGE_BIT_NUM >::Result ,
	};
	static StorageType const BIT_POS_MASK = StorageType( ( ( STORAGE_POS_OFFSET ) << 1 ) - 1 );


	StorageType* mStorage;
	size_t mSize;
};


template< size_t N, typename ElementType >
struct TBitSet
{
	TBitSet() = default;

	enum class EIndexInit
	{
		Value,
	};

	enum
	{
		ElementBitCount = 8 * sizeof(ElementType),
	};


	TBitSet(ElementType value)
	{
		mElements[0] = value;
		if constexpr (N > 1)
		{
			std::fill_n(mElements + 1, N - 1u, 0);
		}
	}

	TBitSet(uint index, EIndexInit)
	{
		if constexpr (N > 1)
		{
			std::fill_n(mElements, N, 0);
			mElements[index / ElementBitCount] = ElementType(1) << ElementType(index % ElementBitCount);
		}
		else
		{
			mElements[0] = ElementType(1) << index;
		}
	}

	static constexpr bool DoLoop = N > 4;

	bool operator == (TBitSet const& rhs) const
	{
		return !this->operator!=(rhs);
	}

	bool operator != (TBitSet const& rhs) const
	{
		if constexpr (DoLoop)
		{
			for (int i = 0; i < N; ++i)
			{
				if (mElements[i] != rhs.mElements[i])
					return true;
			}
			return false;
		}
		else
		{
			return UnrollAny<N>([&](size_t index) { return mElements[index] != rhs.mElements[index]; });
		}
	}

	static TBitSet FromIndex(uint index)
	{
		return TBitSet(index, EIndexInit::Value);
	}


	void add(uint index)
	{
		if constexpr (N > 1)
		{
			return mElements[index / ElementBitCount] |= (ElementType(1) << ElementType(index % ElementBitCount));
		}
		else
		{
			return mElements[0] |= (ElementType(1) << index);
		}
	}
	void remove(uint index)
	{
		if constexpr (N > 1)
		{
			return mElements[index / ElementBitCount] &= ~(ElementType(1) << ElementType(index % ElementBitCount));
		}
		else
		{
			return mElements[0] &= ~(ElementType(1) << index);
		}
	}
	bool test(uint index) const
	{
		if constexpr (N > 1)
		{
			return !!(mElements[index / ElementBitCount] & (ElementType(1) << ElementType(index % ElementBitCount)));
		}
		else
		{
			return !!(mElements[0] && (ElementType(1) << index));
		}
	}

	bool testIntersection(TBitSet const& rhs) const
	{
		if constexpr (DoLoop)
		{
			for (int i = 0; i < N; ++i)
			{
				if (mElements[i] & rhs.mElements[i])
					return true;
			}
			return false;
		}
		else
		{
			return UnrollAny<N>([&](size_t index) { return !!(mElements[index] & rhs.mElements[index]); });
		}
	}

	TBitSet operator~() const
	{
		TBitSet result;
		if constexpr (DoLoop)
		{
			for (int i = 0; i < N; ++i)
			{
				result.mElements[i] = ~mElements[i];
			}
		}
		else
		{
			Unroll<N>([&](size_t index) { result.mElements[index] = ~mElements[index]; });
		}
		return result;
	}

	TBitSet& operator |= (TBitSet const& rhs)
	{
		if constexpr (DoLoop)
		{
			for (int i = 0; i < N; ++i)
			{
				mElements[i] |= rhs.mElements[i];
			}
		}
		else
		{
			Unroll<N>([&](size_t index) { mElements[index] |= rhs.mElements[index]; });
		}
		return *this;
	}
	TBitSet& operator &= (TBitSet const& rhs)
	{
		if constexpr (DoLoop)
		{
			for (int i = 0; i < N; ++i)
			{
				mElements[i] &= rhs.mElements[i];
			}
		}
		else
		{
			Unroll<N>([&](size_t index) { mElements[index] &= rhs.mElements[index]; });
		}
		return *this;
	}
	ElementType mElements[N];
};

#endif //Bitset_h__
