#include "BigInteger.h"

#include <cassert>

BigUintBase::BaseType BigUintBase::tryDivMethod( BaseType u2 , BaseType u1 , BaseType u0 , BaseType v1 , BaseType v0 )
{
	TBigUint< 2 > temp;

	temp.elements[ 1 ] = u2;
	temp.elements[ 0 ] = u1;

	BaseType m = temp.div( v1 );

	bool done;
	do 
	{
		bool wantDec = false;

		//assert( temp.ele[1] == 0 );

		if ( temp.elements[1] == 1 )
			wantDec = true;
		else
		{
			CompType comp;
			comp.val = ( ExtendType ) temp.elements[0] * v0;

			if ( ( comp.high > m ) || 
				 ( comp.high == m && comp.low > u0 ) )
			{
				wantDec =true;
			}	
		}

		done = true;

		if ( wantDec )
		{
			temp.sub( 1 );
			m += v1;
			if ( m >= v1 )
				done = false;
		}
	}
	while( !done );

	return temp.elements[0];
}

void BigUintBase::reverseStr( char* str , int len )
{
	char* ch = str + len - 1;
	while ( ch - str > 0 )
	{
		std::swap( *ch , *str );

		--ch;
		++str;
	}
}


struct IntBit 
{
	static unsigned findHighestNonZeroBit( uint32 n )
	{
		if ( n == 0 ) return 0;

		unsigned result = 31;

		if ( n >> 16 == 0){ result -= 16; n<<=16; } 
		if ( n >> 24 == 0){ result -=  8; n<<= 8; }
		if ( n >> 28 == 0){ result -=  4; n<<= 4; }
		if ( n >> 30 == 0){ result -=  2; n<<= 2; }

		return result + ( n >> 31 );
	}

	static unsigned findHighestNonZeroBit( uint64 n )
	{
		if ( n == 0 ) return 0;

		unsigned result = 63;

		if ( n >> 32 == 0){ result -= 32; n<<=32; }
		if ( n >> 48 == 0){ result -= 16; n<<=16; } 
		if ( n >> 56 == 0){ result -=  8; n<<= 8; }
		if ( n >> 60 == 0){ result -=  4; n<<= 4; }
		if ( n >> 62 == 0){ result -=  2; n<<= 2; }

		return result + ( n >> 63 );
	}
};

unsigned BigUintBase::findHighestNonZeroBit( BaseType n )
{
	//if ( n == 0 )
	//	return 0;
	//unsigned bit = BaseTypeBits;
	//while( ( n & BaseTypeHighestBit ) == 0 )
	//{
	//	n <<= 1;
	//	--bit;
	//}
	//return bit;

	return IntBit::findHighestNonZeroBit( n );
}

unsigned BigUintBase::findLowestNonZeroBit( BaseType n )
{
	if ( !n )
		return 0;

	unsigned result = 1;
	while( ! ( n & 0x1 ) )
	{
		n >>= 1;
		++result;
	}
	return result;
}

BigUintBase::BaseType BigUintBase::convertCharToNum( char c )
{
	assert( ('0' <= c && c <= '9') || 
		    ('a' <= c && c <= 'f') );
	return ('0' <= c && c <= '9') ? BaseType( c - '0' ) : BaseType( c + 10 - 'a' );
}

char BigUintBase::convertNumToChar( BaseType n )
{
	assert(  0 <= n && n <= 15  );
	return ( n < 10 ) ? ( '0' + n ) : ( 'a' + ( n - 10 ) );
}

bool BigUintBase::checkStr( char const* str )
{
	return true;
}
