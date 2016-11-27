#ifndef BigUint_h__9a27ca99_d140_44ca_a443_886396289370
#define BigUint_h__9a27ca99_d140_44ca_a443_886396289370

#include "IntegerType.h"

#include <limits>
#include <string>
#include <iostream>
#include <algorithm>
#include <cassert>

#define MIN_NUM_WORD_CHECK_WORD 4

class BigUintBase
{
public:
	typedef uint32 BaseType;
	typedef uint64 ExtendType;

	static unsigned const BaseTypeBits       = std::numeric_limits< BaseType >::digits;
	static unsigned const BaseTypeHighestBit = 1 << ( BaseTypeBits - 1 );
	static unsigned const BaseTypeMaxValue   = -1;

	static unsigned findHighestNonZeroBit( BaseType n );
	static unsigned findLowestNonZeroBit( BaseType n );

protected:

	union CompType
	{
		struct 
		{
			BaseType low;
			BaseType high;
		};
		ExtendType val;
	};

	static BaseType tryDivMethod( BaseType u2 , BaseType u1 , BaseType u0 , BaseType v1 , BaseType v0 );
	static void     reverseStr( char* str , int len );


	static bool     checkStr( char const* str );
	static BaseType convertCharToNum( char c );
	static char     convertNumToChar( BaseType n );
};



template < int NumWord >
class TBigUint : public BigUintBase
{
public:
	static const int NumWord = NumWord;
	typedef TBigUint< 2 * NumWord > MulRType;

	TBigUint(){}

	TBigUint( char const* str , unsigned base = 10 ){  setFromStr( str , base );  }
	TBigUint( unsigned n ){  setZero();  setElement( 0 , n );  }
	TBigUint( unsigned n , unsigned exp )
	{
		setZero();
		setElement( 0 , 10 );
		pow( exp ); 
		mul( n );
	}


	void setValue( TBigUint const& rh )
	{
		for( int i = 0 ; i < NumWord ; ++i )
			elements[i] = rh.elements[i];
	}

	bool getString( char* str , int maxLen , unsigned base = 10 ) const;
	void getString( std::string& str , unsigned base = 10 ) const;
	void setFromStr( char const* str , unsigned base = 10 );

	void setZero(){ fillElements(0); }
	void setMax(){ fillElements( BaseTypeMaxValue ); }
	bool isZero() const;

	unsigned add( TBigUint const& rh );
	unsigned add( BaseType n ){ return addElement( 0 , n ); }
	
	unsigned sub( TBigUint const& rh );
	unsigned sub( BaseType n ){ return subElement( 0 , n ); }

	unsigned mul( BaseType n );
	unsigned mul( TBigUint const& rh );
	BaseType div( BaseType divisor );
	void     div( TBigUint const& divisor , TBigUint& mod );

	unsigned pow( BaseType n );
	void     sqrt();

	TBigUint const operator + ( TBigUint const& rh ) const{  TBigUint temp(*this); temp += rh; return temp;  }
	TBigUint const operator - ( TBigUint const& rh ) const{  TBigUint temp(*this); temp -= rh; return temp;  }
	TBigUint const operator * ( TBigUint const& rh ) const{  TBigUint temp(*this); temp *= rh; return temp;  }
	TBigUint const operator / ( TBigUint const& rh ) const{  TBigUint temp(*this); temp /= rh; return temp;  }
	TBigUint const operator % ( TBigUint const& rh ) const{  TBigUint temp(*this); temp %= rh; return temp;  }

	TBigUint& operator -= ( TBigUint const& rh ){  sub( rh ); return *this; }
	TBigUint& operator += ( TBigUint const& rh ){  add( rh ); return *this; }
	TBigUint& operator *= ( TBigUint const& rh ){  mul( rh ); return *this; }
	TBigUint& operator /= ( TBigUint const& rh ){  TBigUint mod; div( rh , mod ); return *this;  }
	TBigUint& operator %= ( TBigUint const& rh ){  TBigUint mod; div( rh , mod ); *this = mod ; return *this; }


	TBigUint const operator + ( unsigned n ) const{  TBigUint temp(*this); temp += n; return temp;  }
	TBigUint const operator - ( unsigned n ) const{  TBigUint temp(*this); temp -= n; return temp;  }
	TBigUint const operator * ( unsigned n ) const{  TBigUint temp(*this); temp *= n; return temp;  }
	TBigUint const operator / ( unsigned n ) const{  TBigUint temp(*this); temp /= n; return temp;  }
	unsigned    operator % ( unsigned n ) const{  TBigUint temp(*this); return temp.div( n );  }

	TBigUint& operator *= ( unsigned n ){ mul( n ); return *this; }
	TBigUint& operator /= ( unsigned n ){ div( n ); return *this; }
	TBigUint& operator -= ( unsigned n ){ sub( n ); return *this; }
	TBigUint& operator += ( unsigned n ){ add( n ); return *this; }
	TBigUint& operator %= ( unsigned n ){  *this = div( n ); return *this;  }

	bool big( TBigUint const& rh ) const;
	bool equal( TBigUint const& rh ) const;
	bool operator >  ( TBigUint const& rh ) const { return big(rh);}
	bool operator <= ( TBigUint const& rh ) const { return !big(rh); }
	bool operator <  ( TBigUint const& rh ) const { return rh.big( *this ); }
	bool operator >= ( TBigUint const& rh ) const { return !rh.big( *this ); }
	
	bool operator == ( TBigUint const& rh ) const { return equal( rh ); }
	bool operator != ( TBigUint const& rh ) const { return !equal( rh ); }

	TBigUint& operator <<=( unsigned shift ){  shiftLeftBit( shift , 0 ); return *this; }
	TBigUint& operator >>=( unsigned shift ){  shiftRightBit( shift , 0 ); return *this; }

	TBigUint& operator &= ( TBigUint const& rh ){  bitAND( rh ); return *this; }
	TBigUint& operator |= ( TBigUint const& rh ){  bitOR( rh );  return *this;  }
	TBigUint& operator ^= ( TBigUint const& rh ){  bitXOR( rh ); return *this; }


	TBigUint const operator ~ (){ TBigUint temp(*this); temp.bitNOT(); return temp; }
	TBigUint const operator & ( TBigUint const& rh ) const{  TBigUint temp(*this); temp &= rh; return temp;  }
	TBigUint const operator | ( TBigUint const& rh ) const{  TBigUint temp(*this); temp |= rh; return temp;  }
	TBigUint const operator ^ ( TBigUint const& rh ) const{  TBigUint temp(*this); temp ^= rh; return temp;  }

	friend std::istream& operator >> ( std::istream& s , TBigUint& v )
	{  v.inputSteam( s ); return s; }

	friend std::ostream& operator << ( std::ostream& s , TBigUint const& v )
	{  v.outputSteam( s ); return s; }

	unsigned getElement( unsigned idx ) const { return elements[ idx ];  }
	void     setElement( unsigned idx , BaseType val ){ elements[ idx ] = val; }

protected:

	void setValueByTable( unsigned const* table , unsigned num  )
	{
		assert( num >= NumWord );
		num = NumWord;
		for( int i = 0 ; i < NumWord ; ++i )
		{
			elements[i] = table[--num]; 
		}
	}

	void     fillElements( BaseType val )
	{
		std::fill_n( elements , NumWord , val );
	}

	void shiftLeftBit( unsigned wordOffset , unsigned bitOffset , unsigned fill )
	{


	}
	void shiftRightBit( unsigned wordOffset , unsigned bitOffset , unsigned fill )
	{

	}

	void shiftLeftElement( unsigned offset , unsigned fill );
	void shiftRightElement( unsigned offset , unsigned fill );
	void shiftLeftBit( unsigned offset , unsigned fill );
	void shiftRightBit( unsigned offset , unsigned fill );
	void shiftLeftBitLessElementBit( unsigned offset , unsigned fill );
	void shiftRightBitLessElementBit( unsigned offset , unsigned fill );

	int  getHighestNonZeroElement() const;
	int  getLowestNonZeroElement() const
	{
		for( int i = 0 ; i < NumWord ; ++i )
		{
			if ( elements[i] ) return i;
		}
		return -1;
	}

	void doMul( TBigUint const& rh  , MulRType& result );


	void bitOR( TBigUint& rh );
	void bitXOR( TBigUint& rh );
	void bitAND( TBigUint& rh );
	void bitNOT();


	void fillBit( unsigned word , unsigned bit );

	template< int N >
	void clone( TBigUint< N >& v ) const;

	
	unsigned mulElement( unsigned idx , BaseType n );
	unsigned addElement( unsigned idx , BaseType n );
	unsigned addElement( unsigned idx , BaseType n1 , BaseType n0 );
	unsigned subElement( unsigned idx , BaseType n );

	unsigned mod2(){ return elements[0] & 0x01; }

	static void convertToString( TBigUint& temp , std::string& str , unsigned base , unsigned fNum = 0);
	static bool convertToString( TBigUint& temp , char* str , int maxLen  , unsigned base  );

	void outputSteam( std::ostream& s ) const;
	void inputSteam( std::istream& s );

	BaseType elements[ NumWord ];

private:

	friend class BigUintBase;
	template < int N >  friend class TBigUint;
	template < int N , int M >  friend class TBigFloat;
};


template < int NumWord >
bool TBigUint<NumWord>::big( TBigUint const& rh ) const
{
	for( int i = NumWord - 1; i >= 0 ; --i )
	{
		if ( elements[i] != rh.elements[i] )
			return elements[i] > rh.elements[i];
	}
	return false;
}

template < int NumWord >
bool TBigUint<NumWord>::equal( TBigUint const& rh ) const
{
	for( int i = 0; i < NumWord ; ++i )
	{
		if ( elements[i] != rh.elements[i] )
			return false;
	}
	return true;
}


template < int NumWord >
unsigned TBigUint<NumWord>::sub( TBigUint const& rh )
{
	BaseType carry = 0;
	BaseType val;
	for( unsigned i = 0 ; i < NumWord ; ++i )
	{
		val = elements[i];
		if ( carry )
		{
			elements[i] -= rh.elements[i] + 1;
			carry = ( elements[i] >= val ) ? 1 : 0;
		}
		else
		{
			elements[i] -= rh.elements[i];
			carry = ( elements[i] > val ) ? 1 : 0;
		}
	}
	return carry;
}

template < int NumWord >
unsigned TBigUint<NumWord>::add( TBigUint const& rh )
{
	BaseType carry = 0;
	BaseType val;
	for( unsigned i = 0 ; i < NumWord ; ++i )
	{
		val = elements[i];

		if ( carry )
		{
			elements[i] += rh.elements[i] + 1;
			carry = ( elements[i] <= val ) ? 1 : 0;
		}
		else
		{
			elements[i] += rh.elements[i];
			carry = ( elements[i] < val ) ? 1 : 0;
		}
	}
	return carry;
}

template < int NumWord >
unsigned TBigUint<NumWord>::mul( TBigUint const& rh )
{
	MulRType  result;
	doMul( rh , result );

	int i;
	for ( i = 0 ; i < NumWord ; ++i )
		elements[i] = result.elements[i];

	for( ; i < 2 * NumWord ; ++ i )
	{
		if ( result.elements[i] )
			return 1;
	}

	return 0;
}

template < int NumWord >
unsigned TBigUint<NumWord>::addElement( unsigned idx , BaseType n1 , BaseType n0 )
{
	assert( idx < NumWord - 1 );

	BaseType val = elements[ idx ];
	elements[idx] += n0;

	if ( elements[idx] < val )
	{
		n1 += 1;
		assert( n1 > n1-1 );
	}
	return addElement( idx + 1 , n1 );
}

template < int NumWord >
void TBigUint<NumWord>::doMul( TBigUint const& rh , MulRType& result )
{
	result.setZero();
	CompType temp;

	int n1 , n2;
	if ( NumWord > MIN_NUM_WORD_CHECK_WORD )
	{
		n1 = getHighestNonZeroElement();
		n2 = rh.getHighestNonZeroElement();
	}
	else
	{
		n1 = NumWord - 1;
		n2 = NumWord - 1;
	}

	for( int i = 0 ; i <= n1 ; ++i )
	for( int j = 0 ; j <= n2 ; ++j )
	{
		temp.val = (ExtendType) elements[i] * rh.elements[j];
		result.addElement( i + j , temp.high , temp.low );
	}
}

template < int NumWord >
bool TBigUint<NumWord>::isZero() const
{
	for( int i = 0 ;i < NumWord ; ++i )
	{
		if ( elements[i] )  
			return false;
	}
	return true;
}

template < int NumWord >
unsigned TBigUint<NumWord>::mul( BaseType n )
{
	int dn;
	if ( NumWord > MIN_NUM_WORD_CHECK_WORD )
		dn = getHighestNonZeroElement();
	else
		dn = NumWord - 1;

	unsigned c = mulElement( dn , BaseType(n) );
	for( int i = dn - 1; i >= 0  ; --i )
		c += mulElement( i , BaseType(n) );

	return ( dn == NumWord - 1 ) ? c : 0;
}

template < int NumWord >
BigUintBase::BaseType TBigUint<NumWord>::div( BaseType divisor )
{
	int dn;
	if ( NumWord > MIN_NUM_WORD_CHECK_WORD )
		dn = getHighestNonZeroElement();
	else
		dn = NumWord - 1;

	BaseType m = 0;
	for(int i = dn ; i >= 0 ; --i )
	{
		CompType temp;
		temp.high = m;
		temp.low  = elements[ i ];

		m       = BaseType( temp.val % divisor );
		elements[i]  = BaseType( temp.val / divisor );
	}

	return m;
}

template < int NumWord >
int TBigUint<NumWord>::getHighestNonZeroElement() const
{
	for( int i = NumWord - 1 ; i >= 0 ; --i )
		if ( elements[i] ) return i;

	return -1;
}

template < int NumWord >
void TBigUint<NumWord>::shiftLeftElement( unsigned offset , unsigned fill )
{
	if ( offset == 0 )
		return;

	assert( offset < NumWord );
	int i;
	for ( i = NumWord - 1 ; i >= offset ; --i )
	{
		elements[ i ] = elements[ i - offset ];
	}
	for ( ; i >= 0 ; --i )
	{
		elements[ i ] = fill;
	}
}

template < int NumWord >
void TBigUint<NumWord>::shiftRightElement( unsigned offset , unsigned fill )
{
	if ( offset == 0 )
		return;

	assert( offset < NumWord );
	int i;
	for ( i = 0 ; i < NumWord - offset ; ++i )
	{
		elements[ i ] = elements[ i + offset ];
	}
	for ( ; i < NumWord  ; ++i )
	{
		elements[ i ] = fill;
	}
}

template < int NumWord >
void TBigUint<NumWord>::shiftLeftBitLessElementBit( unsigned offset , unsigned fill )
{
	assert( offset < BaseTypeBits );
	if ( offset == 0 )
		return;

	unsigned of2 = BaseTypeBits - offset;
	int i;
	for( i = NumWord - 1; i > 0  ; --i )
	{
		elements[ i ] <<= offset;
		elements[ i ] |= ( elements[ i - 1 ] >> of2 );
	}
	elements[i] <<= offset;
	elements[i] |=  ( fill >> of2 );
}

template < int NumWord >
void TBigUint<NumWord>::shiftRightBitLessElementBit( unsigned offset , unsigned fill )
{
	assert( offset < BaseTypeBits );

	if ( offset == 0 )
		return;

	unsigned of2 = BaseTypeBits - offset;
	int i ;
	for( i = 0 ; i < NumWord - 1 ; ++i )
	{
		elements[ i ] >>= offset;
		elements[ i ] |= ( elements[ i + 1 ] << of2 );
	}
	elements[i] >>= offset;
	elements[i] |=  ( fill << of2 );
}

template < int NumWord >
void TBigUint<NumWord>::shiftLeftBit( unsigned offset , unsigned fill )
{
	unsigned mWord = offset / BaseTypeBits;
	unsigned mBit  = offset % BaseTypeBits;

	assert( mWord < NumWord );

	shiftLeftBitLessElementBit( mBit , fill );
	shiftLeftElement( mWord , fill );
}

template < int NumWord >
void TBigUint<NumWord>::shiftRightBit( unsigned offset , unsigned fill )
{
	unsigned mWord = offset / BaseTypeBits;
	unsigned mBit  = offset % BaseTypeBits;

	assert( mWord < NumWord );

	shiftRightBitLessElementBit( mBit , fill );
	shiftRightElement( mWord , fill );
}

template < int NumWord >
template < int N >
void TBigUint<NumWord>::clone( TBigUint< N >& v ) const
{
	assert( N > NumWord );

	int i;
	for( i = 0 ; i < NumWord ; ++i )
		v.elements[i] = elements[i];  

	for( ; i < N ; ++i )
		v.elements[ NumWord ] = 0;
}


template < int NumWord >
void TBigUint<NumWord>::div( TBigUint const& divisor , TBigUint& mod )
{
	int dN = getHighestNonZeroElement();
	int rN = divisor.getHighestNonZeroElement();

	if ( ( dN < rN ) || ( dN == rN && divisor > *this )  )
	{
		mod = *this;
		setZero();
		return;
	}

	assert( rN != -1 );

	if ( rN == 0 )
	{
		mod.elements[0] = div( divisor.elements[0] );
		return;
	}

	mod = *this;

	setZero();

	TBigUint< NumWord + 1 > copyDR;
	divisor.clone( copyDR );

	BaseType u2 = 0;

	// normalize divisor////////////////////////////
	unsigned bit = mod.findHighestNonZeroBit( copyDR.elements[rN] );
	unsigned offset = BaseTypeBits - bit;

	copyDR.shiftLeftBitLessElementBit( offset , 0 );

	BaseType te = mod.elements[ dN ];
	unsigned maxOff = BaseTypeBits - findHighestNonZeroBit( te );

	if ( maxOff < offset )
	{
		if ( dN == NumWord )
			u2 = te >> ( offset - maxOff );
		else
			++dN; 
	}
	mod.shiftLeftBitLessElementBit( offset , 0 );
	assert( dN == mod.getHighestNonZeroElement() );
	///////////////////////////////////////////

	BaseType v1 = copyDR.elements[ rN ];
	BaseType v0 = copyDR.elements[ rN - 1 ];

	TBigUint< NumWord + 1 > dr_sub;
	TBigUint< NumWord + 1 > md_copy;

	for( int dif = dN - rN ; dif >= 0 ; --dif )
	{
		BaseType u1 = mod.elements[ dN ];
		BaseType u0 = mod.elements[ dN - 1 ];

		BaseType qd = tryDivMethod( u2 , u1 , u0 , v1 ,v0 );

		if ( qd )
		{
			int idx;

			dr_sub = copyDR;
			dr_sub.mul( qd );

			for( idx = 0 ; idx <= rN ; ++idx )
				md_copy.elements[idx] = mod.elements[ idx + dif ];
			
			md_copy.elements[idx] = u2;

			for( ++idx ; idx < NumWord + 1 ; ++idx )
				md_copy.elements[idx] = 0;

			if ( md_copy.sub( dr_sub ) )
			{
				--qd;
				md_copy.add( copyDR );
			}

			for(idx=0 ; idx <= rN; ++idx)
				mod.elements[ idx + dif ] = md_copy.elements[idx];

			int temp = idx + dif;
			if( temp < NumWord )
				mod.elements[ temp ] = md_copy.elements[idx];

			elements[ dif ] = qd;
		}

		u2 = mod.elements[ dN ];
		--dN;
	}

	mod.shiftRightBitLessElementBit( offset , 0 );
}

template < int NumWord >
unsigned TBigUint<NumWord>::addElement( unsigned idx , BaseType n )
{
	assert( idx < NumWord );

	BaseType val = elements[idx];
	elements[idx] += n;

	if ( elements[ idx ] >= val )
		return 0;

	for ( idx += 1 ; idx < NumWord ; ++idx )
	{
		BaseType val = elements[idx];
		elements[idx] += 1;

		if ( elements[ idx ] >= val )
			return 0;
	}
	return 1;
}

template < int NumWord >
unsigned TBigUint<NumWord>::subElement( unsigned idx , BaseType n )
{
	assert( idx < NumWord );

	BaseType val = elements[idx];
	elements[idx] -= n;

	if ( elements[ idx ] <= val )
		return 0;

	for ( idx += 1 ; idx < NumWord ; ++ idx )
	{
		val = elements[idx];
		elements[idx] -= 1;

		if ( elements[ idx ] <= val )
			return 0;
	}
	return 1;
}

template < int NumWord >
unsigned TBigUint<NumWord>::mulElement( unsigned idx , BaseType n )
{
	assert( idx < NumWord );

	CompType temp;
	temp.val = (ExtendType) elements[idx] * n;
	elements[idx] = temp.low;

	if ( idx < NumWord - 1 )
		return addElement( idx + 1 , temp.high );

	return temp.high;
}

template < int NumWord >
void TBigUint<NumWord>::getString( std::string& str , unsigned base /*= 10 */ ) const
{
	TBigUint temp(*this);
	convertToString( temp , str , base );
}

template < int NumWord >
void TBigUint<NumWord>::convertToString( TBigUint& temp , std::string& str , unsigned base , unsigned fNum )
{
	assert( base >= 2 && base <= 16 );
	unsigned i = 0;
	do
	{
		BaseType mod = temp.div( BaseType(base) );
		str.push_back( convertNumToChar( mod ) );
		++i;

		if ( i == fNum )
			str.push_back('.');
	}
	while ( !temp.isZero() );
	std::reverse( str.begin() , str.end() );
}

template < int NumWord >
bool TBigUint<NumWord>::getString( char* str , int maxLen , unsigned base /*= 10 */ ) const
{
	assert( base >= 2 && base <= 16 );
	TBigUint temp(*this);

	return convertToString( temp , str, maxLen , base );
}

template < int NumWord >
bool TBigUint<NumWord>::convertToString( TBigUint& temp , char* str , int maxLen , unsigned base ) 
{
	assert( base >= 2 && base <= 16 );

	int i = 0;
	do
	{
		if ( i >= maxLen )
			return false;

		BaseType mod = temp.div( BaseType(base) );
		str[i] = convertNumToChar( mod );
		++i;
	}
	while ( !temp.isZero() );

	str[i] = '\0';
	std::reverse( str ,  str + i );
	return true;
}

template < int NumWord >
void TBigUint<NumWord>::setFromStr( char const* str , unsigned base /*= 10 */ )
{
	if( !checkStr( str ) )
		return;

	setZero();

	char const* p = str;
	while ( *p != '\0' )
	{
		mul( BaseType( base ) );
		BaseType n = convertCharToNum( *p );
		add( n );
		++p;
	}
}

template < int NumWord >
void TBigUint<NumWord>::bitOR( TBigUint& rh )
{
	for ( int i = 0 ; i < NumWord ; ++i )
		elements[i] |= rh.elements[i];
}

template < int NumWord >
void TBigUint<NumWord>::bitXOR( TBigUint& rh )
{
	for ( int i = 0 ; i < NumWord ; ++i )
		elements[i] ^= rh.elements[i];
}

template < int NumWord >
void TBigUint<NumWord>::bitAND( TBigUint& rh )
{
	for ( int i = 0 ; i < NumWord ; ++i )
		elements[i] &= rh.elements[i];
}

template < int NumWord >
void TBigUint<NumWord>::bitNOT()
{
	for ( int i = 0 ; i < NumWord ; ++i )
		elements[i] = ~elements[i];
}

template < int NumWord >
void TBigUint<NumWord>::inputSteam( std::istream& s )
{
	std::string str;
	s >> str;
	setFromStr( str.c_str() , 10 );
}

template < int NumWord >
void TBigUint<NumWord>::outputSteam( std::ostream& s ) const
{
	std::string str;
	getString( str , 10 );
	s << str;
}

template < int NumWord >
unsigned TBigUint<NumWord>::pow( BaseType n )
{
	if( n == 0 && isZero() )
		return 2;

	TBigUint start  = *this;
	
	*this = 1u;
	
	unsigned c = 0;

	while( !c )
	{
		if ( n & 1 )
			c += mul( start );

		n >>= 1;

		if ( n == 0 )
			break;

		c += start.mul( start );
	}
	return ( c ) ? 1 : 0;
}


template < int NumWord >
void TBigUint<NumWord>::sqrt()
{
	if( isZero() )
		return;

	TBigUint val = *this;
	setZero();

	int n = val.getHighestNonZeroElement();
	
	TBigUint temp;
	temp.setZero();
	temp.elements[ n ] = BaseTypeHighestBit >> 1;
	
	while( temp > val )
	{
		temp.shiftRightBitLessElementBit( 2 , 0 );
	}
	
	n = temp.getHighestNonZeroElement();
	int pos = n * BaseTypeBits + findHighestNonZeroBit( temp.elements[ n ] ) - 1;

	//while( !bit.isZero() )
	while( pos >= 0 )
	{
		unsigned word = pos / BaseTypeBits;
		unsigned bp   = pos % BaseTypeBits;

		temp = *this;
		
		//temp.add( bit );
		temp.fillBit( word , bp );

		shiftRightBitLessElementBit( 1 , 0 );

		if(  val >= temp )
		{
			val.sub(temp);
			//add( bit );
			fillBit( word , bp );
		}
		//bit.RSBitSmallWord( 2 , 0 );
		pos -= 2;
	}
}

template < int NumWord >
void TBigUint<NumWord>::fillBit( unsigned word , unsigned bit )
{
	elements[ word ] |= ( 1 << bit );
}


template < int NumWord >
class TBigInt : private TBigUint< NumWord >
{
public:

	TBigInt(){}
	TBigInt( int n ){ setValue( n ); }
	TBigInt( unsigned n ){ setValue( n ); }
	TBigInt( char const* str , unsigned base = 10 ){ setFromStr( str , base ); }

	void     setValue( int n );
	unsigned setValue( TBigUint< NumWord > const& rh )
	{  
		TBigUint::setValue( rh );
		if ( elements[ NumWord - 1 ] & BaseTypeHighestBit )
			return 1;
		return 0;
	}
	unsigned setValue( unsigned n );
	
	void setMax()
	{
		fillElements( BaseTypeMaxValue );
		elements[ NumWord - 1 ] &= ~BaseTypeHighestBit;  
	}

	void setMin()
	{
		fillElements( 0 );
		elements[ NumWord - 1 ] |= BaseTypeHighestBit;
	}

	
	TBigInt const operator + ( TBigInt const& rh ) const{  TBigInt temp(*this); temp += rh; return temp;  }
	TBigInt const operator - ( TBigInt const& rh ) const{  TBigInt temp(*this); temp -= rh; return temp;  }
	TBigInt const operator * ( TBigInt const& rh ) const{  TBigInt temp(*this); temp *= rh; return temp;  }
	//BUInt const operator / ( BInt const& rh ) const{  BInt temp(*this); temp /= rh; return temp;  }
	//BUInt const operator % ( BInt const& rh ) const{  BInt temp(*this); temp %= rh; return temp;  }

	TBigInt& operator += ( TBigInt const& rh ){  add( rh ); return *this; }
	TBigInt& operator -= ( TBigInt const& rh ){  sub( rh ); return *this; }
	TBigInt& operator *= ( TBigInt const& rh ){  mul( rh ); return *this; }
	//BInt& operator /= ( BInt const& rh ){  BUInt mod; div( rh , mod ); return *this;  }
	//BInt& operator %= ( BInt const& rh ){  BUInt mod; div( rh , mod ); *this = mod ; return *this; }

	TBigInt const operator + ( unsigned n ) const{  TBigInt temp(*this); temp += n; return temp;  }
	TBigInt const operator - ( unsigned n ) const{  TBigInt temp(*this); temp -= n; return temp;  }
	TBigInt const operator * ( unsigned n ) const{  TBigInt temp(*this); temp *= n; return temp;  }
	//BUInt const operator / ( unsigned n ) const{  BInt temp(*this); temp /= n; return temp;  }
	//unsigned    operator % ( unsigned n ) const{  BInt temp(*this); return temp.div( n );  }

	TBigInt& operator += ( unsigned n ){ add( n ); return *this; }
	TBigInt& operator -= ( unsigned n ){ sub( n ); return *this; }
	TBigInt& operator *= ( unsigned n ){ mul( n ); return *this; }
	//BInt& operator /= ( unsigned n ){ div( n ); return *this; }
	//BInt& operator %= ( unsigned n ){  *this = div( n ); return *this;  }

	TBigInt& operator *= ( int n ){ mul( n ); return *this; }
	TBigInt& operator /= ( int n ){ div( n ); return *this; }
	TBigInt& operator -= ( int n ){ sub( n ); return *this; }


	bool operator  > ( TBigInt const& rh ) const{ return  big(rh); }
	bool operator <= ( TBigInt const& rh ) const{ return !big(rh); }
	bool operator  < ( TBigInt const& rh ) const{ return  rh.big(*this); }
	bool operator >= ( TBigInt const& rh ) const{ return !rh.big(*this); }


	TBigUint< NumWord >& castUInt(){ return static_cast< TBigUint< NumWord >& >(*this); }
	bool  big( TBigInt const& rh ) const;
	TBigInt& operator = ( TBigUint< NumWord > const& rh ){ setValue( rh ); return *this; }

	TBigInt& operator = ( char const* str ){ setFromStr( str ); return *this; }
	TBigInt& operator = ( int rh ){ setValue( rh ); return *this; }

	void  getElements( TBigUint< NumWord >&lh ) const
	{  
		lh.setValue( *this );  
	}

	bool equal( TBigInt const& rh ) const { return TBigUint::equal( rh ); }
	bool operator == ( TBigInt const& rh ) const { return equal( rh ); }
	bool operator != ( TBigInt const& rh ) const { return !equal( rh ); }

	bool isZero() const { return TBigUint::isZero(); }
	void setZero(){  TBigUint::setZero(); }

	bool convertToInt( int& val ) const;

	unsigned add( int n );

	unsigned add( unsigned n )
	{
		bool bNS = false;
		bool bBS = isSign();

		TBigUint::add( n );

		return checkAddCarry( bNS , bBS );
	}

	unsigned add( TBigInt const& n )
	{
		bool bNS = n.isSign();
		bool bBS = isSign();

		TBigUint::add( n );

		return checkAddCarry( bNS , bBS );
	}

	unsigned sub( int n )
	{ 
		if ( n >= 0 ) 
			return sub( unsigned(n) );
		else
			return add( unsigned(-n) );
	}


	unsigned sub( unsigned n )
	{
		bool bNS = false;
		bool bBS = isSign();

		TBigUint::sub( n );

		return checkSubCarry( bNS , bBS );
	}


	unsigned sub( TBigInt const& n )
	{
		bool bNS = n.isSign();
		bool bBS = isSign();

		TBigUint::sub( n );

		return checkSubCarry( bNS , bBS );
	}



	unsigned mul( int n )
	{
		if ( n >=  0 )
			return mul( unsigned(n) );
		else 
		{
			unsigned c = mul( unsigned(-n) );
			applyComplement();
			return c;
		}
	}

	void div( TBigInt const& rh , TBigInt& mod )
	{
		TBigInt& ncRh = const_cast< TBigInt& >( rh );

		bool beSS = ( isSign() == ncRh.isSign() );
		bool beRS = ncRh.isSign();
		if ( isSign() )
			applyComplement();
		if ( ncRh.isSign() )
			applyComplement();

		TBigUint::div( rh , mod );

		if ( !beSS )
			applyComplement();

		if ( beRS )
		{
			mod.applyComplement();
			ncRh.applyComplement();
		}
	}

	//BInt& operator += ( int n ){ add( n ); return *this; }
	//BInt& operator %= ( int n ){  *this = div( n ); return *this;  }


	unsigned mul( unsigned n )
	{
		bool bNS = true;
		bool bBS = isSign();
		TBigUint::mul( n );

		return checkMulCarry( bNS , bBS );
	}

	unsigned mul( TBigInt const& n )
	{
		bool bNS = n.isSign();
		bool bBS = isSign();

		TBigUint::mul(n);

		return checkMulCarry( bNS , bBS );
	}

	unsigned checkMulCarry( bool bNS , bool bBS )
	{
		if ( bNS == bBS )
			return isSign() ? 1 : 0;
		else
			return isSign() ? 0 : 1;
	}

	unsigned checkAddCarry( bool bNS , bool bBS )
	{
		if ( bNS == bBS && bBS != isSign() )
			return 1;

		return 0;
	}

	unsigned checkSubCarry( bool bNS , bool bBS )
	{
		if ( bNS != bBS && bBS != isSign() )
			return 1;

		return 0;
	}


	bool isMin()
	{
		if ( elements[ NumWord - 1 ] != BaseTypeHighestBit )
			return false;

		for ( int i = 0 ; i < NumWord - 1 ; ++i )
			if ( elements[i] ) return false;

		return true;
	}
	unsigned abs()
	{
		doAbs();
		return isMin() ? 1 : 0;
	}

	void applyComplement(){  bitNOT();  TBigUint::add( 1 );  }
	void getString( std::string& str , unsigned base = 10 ) const
	{
		TBigInt temp = *this;

		bool beS = temp.isSign();

		if ( beS )
			temp.applyComplement();

		convertToString( temp , str , base );

		if ( beS )
			str.insert( str.begin() ,  '-' );
	}
	bool getString( char* str , int maxLen , unsigned base = 10 ) const
	{
		TBigInt temp = *this;

		if ( temp.isSign() )
		{
			*( str++ ) = '-';
			temp.applyComplement();
			--maxLen;
		}
		return convertToString( temp , str , maxLen , base );
	}

	void setFromStr( char const* str , unsigned base = 10 )
	{
		bool haveSignal = false;
		if ( str[0] == '-' )
		{
			++str;
			haveSignal = true;
		}
		TBigUint::setFromStr( str , base );

		if ( haveSignal )
			applyComplement();
	}

	void setSign( bool beS )
	{
		if ( beS )
			elements[ NumWord - 1 ] &= ~BaseTypeHighestBit; 
		else
			elements[ NumWord - 1 ] |=  BaseTypeHighestBit; 
	}

	bool isSign() const
	{
		return ( elements[ NumWord - 1 ] & BaseTypeHighestBit ) != 0;
	}

	friend std::istream& operator >> ( std::istream& s , TBigInt& v )
	{  v.inputSteam( s ); return s; }

	friend std::ostream& operator << ( std::ostream& s , TBigInt const& v )
	{  v.outputSteam( s ); return s; }


protected:
	void doAbs()
	{
		if ( isSign() )
			applyComplement();
	}

	void outputSteam( std::ostream& s ) const
	{
		std::string str;
		getString( str , 10 );
		s << str;
	}
	void inputSteam( std::istream& s )
	{
		std::string str;
		s >> str;
		setFromStr( str.c_str() , 10 );
	}

	template < int N , int M >  friend class TBigFloat;
};

template < int NumWord >
bool TBigInt<NumWord>::convertToInt(int& val) const
{
	TBigInt temp = *this;
	if ( temp.abs() )
		return false;

	if ( temp.getHighestNonZeroElement() > 0 )
		return false;

	val = temp.getElement( 0 );

	if ( NumWord != 1 )
	{
		if (  val & BaseTypeHighestBit )
			return false;
	}

	if ( isSign() )
		val = -val;

	return true;
}

template < int NumWord >
bool TBigInt<NumWord>::big(TBigInt const& rh) const
{
	int lv = int ( elements[ NumWord - 1] );
	int rv = int ( rh.elements[ NumWord - 1 ] );

	if ( lv != rv )
		return lv > rv;

	for ( int i = NumWord - 2  ; i >= 0 ; --i )
	{
		if ( elements[i] != rh.elements[i] )
			return elements[i] > rh.elements[i];
	}
	return false;
}


template < int NumWord >
unsigned TBigInt<NumWord>::add(int n)
{

	if ( n >= 0 )
		return add( unsigned(n) );
	else
		return sub( unsigned(-n) );
}

template < int NumWord >
unsigned TBigInt<NumWord>::setValue(unsigned n)
{
	fillElements( 0 );
	elements[0] = n ;
	if ( NumWord == 1 && ( n & BaseTypeHighestBit ) )
		return 1;
	return 0;
}

template < int NumWord >
void TBigInt<NumWord>::setValue(int n)
{
	unsigned fill = ( n < 0 ) ? (unsigned)~0 : 0;
	fillElements( fill );
	elements[0] = unsigned( n );
}

#endif // BigUint_h__9a27ca99_d140_44ca_a443_886396289370
