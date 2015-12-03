#ifndef Bitset_h__
#define Bitset_h__

#include <algorithm>

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
	template < int N >
	struct Log2{};
	template <>
	struct Log2<  8 >{ enum{ Result = 3 }; };
	template <>
	struct Log2< 16 >{ enum{ Result = 4 }; };
	template <>
	struct Log2< 32 >{ enum{ Result = 5 }; };
	template <>
	struct Log2< 64 >{ enum{ Result = 6 }; };

	enum
	{
		STORAGE_BIT_NUM    = sizeof( StorageType ) * 8 ,
		STORAGE_POS_OFFSET = Log2< STORAGE_BIT_NUM >::Result ,
	};
	static StorageType const BIT_POS_MASK = StorageType( ( ( STORAGE_POS_OFFSET ) << 1 ) - 1 );


	StorageType* mStorage;
	size_t mSize;
};


#endif //Bitset_h__