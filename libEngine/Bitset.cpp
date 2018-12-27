#include "Bitset.h"

#include "BitUtility.h"

Bitset::Bitset()
{
	mSize = 0;
	mStorage = 0;
}

Bitset::Bitset( size_t size , bool val )
{
	resize( size , val );
}

Bitset::~Bitset()
{
	delete [] mStorage;
}

bool Bitset::test( size_t pos ) const
{
	size_t idx = pos >> STORAGE_POS_OFFSET;
	StorageType bit =  1 << ( pos & BIT_POS_MASK );

	return !!( mStorage[ idx ] & bit );
}

Bitset& Bitset::set( size_t pos , bool val /*= true */ )
{
	size_t idx = pos >> STORAGE_POS_OFFSET;
	StorageType bit =  1 << ( pos & BIT_POS_MASK );

	if ( val )
		mStorage[ idx ] |=  bit;
	else
		mStorage[ idx ] &= ~bit;

	return *this;
}

void Bitset::resize( size_t size , bool val /*= false */ )
{
	delete[] mStorage;
	mSize = size;
	size_t allocSize = size / STORAGE_BIT_NUM;
	size_t numBit = mSize % STORAGE_BIT_NUM;
	
	if ( numBit )
		++allocSize;
	
	mStorage = new StorageType[ allocSize ];

	StorageType fillValue = ( val ) ? ~StorageType(0) : StorageType(0);
	std::fill( mStorage , mStorage + allocSize , fillValue );
}

void Bitset::flip()
{
	size_t size = mSize / STORAGE_BIT_NUM;
	for( size_t i = 0 ; i < size ; ++i )
		mStorage[i] = ~mStorage[i];
}

void Bitset::flip( size_t pos )
{
	size_t idx = pos >> STORAGE_POS_OFFSET;
	StorageType bit =  1 << ( pos & BIT_POS_MASK );

	if ( mStorage[ pos ] & bit )
		mStorage[ pos ] &= ~bit;
	else
		mStorage[ pos ] |= bit;
}

size_t Bitset::count() const
{
	size_t result = 0;

	size_t size = mSize / STORAGE_BIT_NUM;
	size_t numBit = mSize % STORAGE_BIT_NUM;
	for( size_t i = 0 ; i < size ; ++i )
		result += BitUtility::CountSet( mStorage[i] );
	
	if ( numBit )
		result += BitUtility::CountSet(StorageType( mStorage[ size ] & (( 1 << (numBit+1) )- 1 )) );

	return result;
}

bool Bitset::any() const
{
	size_t size = mSize / STORAGE_BIT_NUM;
	size_t numBit = mSize % STORAGE_BIT_NUM;
	for( size_t i = 0 ; i < size ; ++i )
	{
		if ( mStorage[i] )
			return true;
	}

	if ( numBit )
		return ( mStorage[ size ] & ( ( 1 << (numBit+1) )- 1 ) ) != 0;

	return false;
}

#include "UnitTest.h"

class BitsetTest : public UnitTest::ClassTestT< BitsetTest , Bitset >
{
public:
	RunResult run()
	{
		Bitset b( 15 , false );
		b.set( 0 , true );
		b.set( 10 , true );

		UT_ASSERT( b.count() == 2 );
		UT_ASSERT( b.test( 0 ) && b.test( 15 ) );
		return RR_SUCCESS; 
	}
};

UT_REG_TEST_CLASS( BitsetTest )


