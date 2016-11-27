#ifndef BFloat_h__
#define BFloat_h__

#include "BigInteger.h"

class BigFloatBase
{
protected:

	enum
	{
		STATS_SIGN     = 0x001 ,
		STATS_NAN      = 0x002 ,
		STATS_INFINITY = 0x004 ,
		STATS_ZERO     = 0x008 ,
	};

	typedef BigUintBase::BaseType BaseType;

	static unsigned const BaseTypeBits       = BigUintBase::BaseTypeBits;
	static unsigned const BaseTypeHighestBit = BigUintBase::BaseTypeHighestBit;
	static unsigned const BaseTypeMaxValue   = BigUintBase::BaseTypeMaxValue;

	unsigned  stats;

	static unsigned const MaxVarTableNum = 256;
	static unsigned const TableLn2[];
	static unsigned const TableLn10[];


	static int const DFLOAT_EXP_BIAS  = 1023;
	static int const DFLOAT_EXP_ERROR = 2047;

	struct DFloatFormat
	{
		uint32 lmant ;
		uint32 hmant  :20;
		uint32 exp    :11;
		uint32 sign   : 1;
	};

	static int const SFLOAT_EXP_BIAS  = 127;
	static int const SFLOAT_EXP_ERROR = 255;

	struct  SFloatFormat
	{
		uint32 mant  :23;
		uint32 exp   : 8;
		uint32 sign  : 1;
	};

public:
	void abs()       { stats &= ~STATS_SIGN; }
	void setSign()   { stats |= STATS_SIGN; }
	void setNan()    { stats = STATS_NAN; }
	void changeSign(){ stats = ( (~stats) & STATS_SIGN ) | ( stats & (~STATS_SIGN) ); }

	bool isNan()      const { return ( stats & STATS_NAN ) != 0; }
	bool isSign()     const { return ( stats & STATS_SIGN ) != 0; }
	bool isInfinity() const { return ( stats & STATS_INFINITY ) != 0; }

};


template < int MantNum , int ExpNum >
class TBigFloat : public BigFloatBase
{
private:
	//static int const MantNum = MantNum;
	//static int const ExpNum = ExpNum;

	typedef TBigInt< ExpNum >   ExpType;
	typedef TBigUint< MantNum > MantType;

	static unsigned const MantTotalBits = MantNum * BaseTypeBits;
	
	ExpType   exponent;
	MantType  mantissa;

public:
	TBigFloat(){}
	TBigFloat( char const*  str ,unsigned base = 10 ){  setFromString( str , base ); }
	TBigFloat( float  val ){  setValue( val ); }
	TBigFloat( double val ){  setValue( val ); }
	TBigFloat( TBigFloat const& val ){ setValue( val ); }
	TBigFloat( unsigned val ){ setValue( val ); }

	template< int N >
	void setValue( TBigInt< N > const& val );
	void setValue( float val );
	void setValue( double val );
	void setValue( TBigFloat const& val );
	void setValue( unsigned val );

	void setFromString( char const* str , unsigned base = 10 );
	void getString( std::string& str , unsigned base = 10 );

	bool convertToUInt( unsigned& val );
	bool convertToFloat( float& val );
	bool convertToDouble( double& val );

	void setZero();
	void setOne();
	void setLn10();
	void setLn2();

	bool operator >  ( TBigFloat const& rh ) const { return big(rh);}
	bool operator <= ( TBigFloat const& rh ) const { return !big(rh); }
	bool operator <  ( TBigFloat const& rh ) const { return rh.big( *this ); }
	bool operator >= ( TBigFloat const& rh ) const { return !rh.big( *this ); }

	unsigned add( TBigFloat const& rh ){  TBigFloat temp = rh; return addDirty( temp ); }
	unsigned sub( TBigFloat const& rh ){  TBigFloat temp = rh; return subDirty( temp ); }


	unsigned mul( unsigned  rh );

	void inverse();

	unsigned getMantissaVaildBitNum();

	bool convertMantissaToUInt();

	template < int N >
	unsigned powDirty( TBigInt< N >& rh );

	template < int N >
	unsigned powDirty( TBigUint< N >& rh );

	unsigned pow( TBigFloat const& val );


	unsigned exp( TBigFloat const& val );
	void     expFraction( TBigFloat const& val );


	void rejectDecimal();


	unsigned mul( TBigFloat const& rh );

	// this = this ^ ( 2 ^ rh )
	template < int N >
	unsigned pow2NDirty( TBigUint< N >& rh );

	bool bigWithoutSign( TBigFloat const& rh ) const;
	bool big( TBigFloat const& rh ) const;


	// val = m 2^e  -> result( this) = ln(val) = ln( m ) + e ln(2)
	unsigned ln( TBigFloat const& val );
	unsigned ln();

	unsigned div( TBigFloat const& rh );


	unsigned subDirty( TBigFloat& rh );
	unsigned addDirty( TBigFloat& rh );

	unsigned doMantissaAdd( TBigFloat const& rh);
	unsigned doMantissaSub( TBigFloat& rh );

	void getIntegerPart( TBigFloat& rh ) const;

	bool isZero() const
	{ 
		//assert( mantissa.isZero() && exponent.isZero() );
		return mantissa.isZero(); 
	}

	bool isStandardize() const 
	{ 
		return  mantissa.isZero() || ( mantissa.getElement( MantNum - 1 ) & BaseTypeHighestBit )!= 0; 
	}

	unsigned standardize();
	bool     normalize( TBigFloat& rh );
	bool     isInteger();

};

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::inverse()
{
	TBigFloat temp = *this;
	setOne();
	unsigned c = div( temp );
	assert( c == 0 );
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::getMantissaVaildBitNum()
{
	if ( isZero() )
		return 0;

	int word = mantissa.getLowestNonZeroElement();
	unsigned bit = BigUintBase::findLowestNonZeroBit( mantissa.getElement( word ) );

	assert( bit != 0 );
	return ( MantNum - word ) * BaseTypeBits  - bit + 1;
}

template < int MantNum , int ExpNum >
template < int N >
unsigned TBigFloat<MantNum, ExpNum>::powDirty(TBigInt< N >& rh)
{
	if ( rh.isSign() )
	{
		rh.applyComplement();
		unsigned result = powDirty( rh.castUInt() );
		inverse();

		return result;
	}
	else
	{
		return powDirty( rh.castUInt() );
	}
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::pow(TBigFloat const& val)
{
	TBigFloat temp;
	temp.ln( *this );
	temp.mul( val );

	return exp( temp );
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::doMantissaSub( TBigFloat& rh )
{
	unsigned carry;
	if ( mantissa >= rh.mantissa )
	{
		carry = mantissa.sub( rh.mantissa );
	}
	else
	{
		carry = rh.mantissa.sub( mantissa );
		mantissa = rh.mantissa;
		changeSign();
	}
	assert( carry == 0 );
	standardize();

	return carry;
}

template < int MantNum , int ExpNum  >
unsigned TBigFloat<MantNum, ExpNum>::doMantissaAdd( TBigFloat const& rh )
{
	unsigned carry = mantissa.add( rh.mantissa );
	if ( carry )
	{
		mantissa.shiftRightBitLessElementBit( 1 , 1 );
		carry  = exponent.add( 1 );
	}
	assert ( isStandardize() );
	return carry;
}

template < int MantNum , int ExpNum  >
unsigned TBigFloat<MantNum, ExpNum>::subDirty( TBigFloat& rh )
{
	if ( ! normalize( rh ) )
		return 0;

	if ( isSign() != rh.isSign() )
		return doMantissaAdd( rh );
	else
		return doMantissaSub( rh );
}

template < int MantNum , int ExpNum  >
unsigned TBigFloat<MantNum, ExpNum>::addDirty( TBigFloat& rh )
{
	if ( ! normalize( rh ) )
		return 0;

	if ( isSign() == rh.isSign() )
		return doMantissaAdd( rh );
	else
		return doMantissaSub( rh );
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::mul( TBigFloat const& rh )
{
	MantType::MulRType result;

	mantissa.doMul( rh.mantissa , result );

	int word = result.getHighestNonZeroElement();
	if ( word == -1 ) // result == 0;
	{
		setZero();
		return 0;
	}
	assert( word == ( 2 * MantNum - 1 ) );

	unsigned carry = 0;

	unsigned val = result.getElement( 2 * MantNum - 1 );

	assert( BigUintBase::findHighestNonZeroBit( val ) == BaseTypeBits || 
		    BigUintBase::findHighestNonZeroBit( val ) == BaseTypeBits - 1 );

	carry += exponent.add( rh.exponent );

	if ( val & BaseTypeHighestBit )
		carry += exponent.add( 1 );
	else
		result.shiftLeftBitLessElementBit( 1 , 0 );

	for( int i = 0 ; i < MantNum  ; ++i )
	{
		unsigned val = result.getElement( i + MantNum ); 
		mantissa.setElement( i , val );
	}

	if ( isSign() == rh.isSign() )
		abs();
	else
		setSign();

	assert( isStandardize() );
	return carry;
}


template < int MantNum , int ExpNum >
template< int N >
void TBigFloat<MantNum, ExpNum>::setValue( TBigInt< N > const& val )
{
	assert( N <= MantNum );

	if ( val.isZero() )
	{
		setZero();
		return;
	}

	TBigInt< N > temp = val;

	stats = 0;
	if ( temp.isSign() )
	{
		temp.applyComplement();
		setSign();
	}

	int word = temp.getHighestNonZeroElement();
	assert( word != -1 );

	unsigned move = BaseTypeBits - BigUintBase::findHighestNonZeroBit( temp.getElement(word) );
	if ( move )
		temp.shiftLeftBitLessElementBit( move , 0 );

	exponent.setValue( ( word + 1 ) * BaseTypeBits - move - 1 );

	int idx = MantNum - 1;
	for ( int i = word  ; i >= 0  ; --i , --idx)
		mantissa.setElement( idx , temp.getElement( i ) );

	for ( ; idx >= 0 ; --idx )
		mantissa.setElement( idx , 0 );
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::div( TBigFloat const& rh )
{
	if ( rh.isZero() )
	{
		return 1;
	}

	MantType::MulRType md;
	MantType::MulRType mr;
	MantType::MulRType mod;

	for ( int i = 0 ; i < MantNum ; ++i )
	{
		md.setElement( i + MantNum , mantissa.getElement(i) );
		mr.setElement( i , rh.mantissa.getElement(i) );
	}

	for ( int i = 0 ; i < MantNum ; ++i )
	{
		md.setElement( i , 0 );
		mr.setElement( i + MantNum , 0 );
	}

	md.div( mr , mod );

	unsigned carry = 0;

	int word = md.getHighestNonZeroElement();
	assert ( word == MantNum || word == MantNum - 1 );

	carry += exponent.sub( rh.exponent );

	if ( word == MantNum )
	{
		assert( md.getElement( MantNum ) == 1 );
		md.shiftRightBitLessElementBit( 1 , 0 );
	}
	else
	{
		assert( BigUintBase::findHighestNonZeroBit( md.getElement( MantNum - 1 ) ) == BaseTypeBits );
		carry += exponent.sub( 1 );
	}


	for( int i = 0 ; i < MantNum ; ++i )
	{
		mantissa.setElement( i , md.getElement(i) );
	}

	if ( isSign() != rh.isSign() )
		setSign();
	else
		abs();

	assert( isStandardize() );
	return carry;
}


template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::normalize( TBigFloat& rh )
{
	if ( isZero() )
	{
		exponent = rh.exponent;
		return true;
	}
		
	ExpType dif = exponent;
	dif.sub( rh.exponent );

	int ne;
	if ( !dif.convertToInt( ne ) )
		return false;

	if ( ne < 0 )
	{
		unsigned move = -ne;
		if ( move > MantTotalBits )
			return false;

		mantissa.shiftRightBit( move , 0 );
		exponent.add( move );
	}
	else
	{
		unsigned move = ne;

		if ( move >= MantTotalBits )
			return false;

		rh.mantissa.shiftRightBit( move , 0 );
		rh.exponent.add( move );
	}

	assert( exponent == rh.exponent );
	return true;
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setLn10()
{
	if ( MantTotalBits > MaxVarTableNum )
	{
		setValue( 10.0f );
		ln();
	}
	else
	{
		stats = 0;
		mantissa.setValueByTable( TableLn10 , MaxVarTableNum );
		exponent.setValue( 1 );
	}
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setLn2()
{
	if ( MantTotalBits > MaxVarTableNum )
	{
		setValue( 2.0f );
		ln();
	}
	else
	{
		stats = 0;
		mantissa.setValueByTable( TableLn2 , MaxVarTableNum );
		exponent.setValue( -1 );
	}
}


template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::standardize()
{
	int word = mantissa.getHighestNonZeroElement();

	if ( word == -1 ) // mant == 0
	{
		exponent.setZero();
		abs();
		return 0;
	}

	unsigned bit = BigUintBase::findHighestNonZeroBit( mantissa.getElement( word ) );
	unsigned move = BaseTypeBits * ( MantNum - word ) - bit;
	mantissa.shiftLeftBit( move , 0 );
	return exponent.sub( move );
}



template < int MantNum , int ExpNum >
template < int N >
unsigned TBigFloat<MantNum, ExpNum>::pow2NDirty( TBigUint< N >& rh )
{
	if( rh.isZero() && isZero() )
		return 2;

	unsigned c = 0;

	while ( !c )
	{
		c += mul(*this);

		rh.sub( 1 );
		if( rh.isZero() )
			break;
	}

	return ( c ) ? 1 : 0;
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::bigWithoutSign( TBigFloat const& rh ) const
{
	assert( isStandardize() && rh.isStandardize() );
	if ( isZero() )
		return false;

	if ( rh.isZero() )
		return true;

	if ( exponent == rh.exponent )
		return ( mantissa > rh.mantissa );

	return ( exponent > rh.exponent );
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::big( TBigFloat const& rh ) const
{
	if ( isSign() != rh.isSign() )
		return rh.isSign();

	if ( isSign() )
		return rh.bigWithoutSign(*this);

	return bigWithoutSign( rh );
}

//              1 + y                  1       2n
// ln(x) = ln( ------ ) = 2 y *SUM[ ------- * y   ]   
//              1 - y          n=0   2n + 1
// y = ( x - 1 ) / ( x + 1 )   x > 0
template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::ln()
{
	if ( isSign() )
	{
		return 1;
	}

	TBigFloat y = *this;

	add( 1.0 );

	y.sub( 1.0 );
	y.div( *this );

	// result = y;
	setValue( y );

	TBigFloat   yn = y;
	//y -> y^2
	y.mul( y );

	unsigned n = 1;
	TBigFloat An;

	TBigFloat temp;

	while(1)
	{
		yn.mul( y );
		An = yn;

		temp.setValue( 2 * n + 1 );
		An.div( temp );

		if ( !normalize( An ) )
			break;

		assert( isSign() == An.isSign() );
		doMantissaAdd( An );

		assert( n < n + 1 );
		++n;
	}

	return exponent.add( 1 );
}


template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::ln( TBigFloat const& val )
{
	assert( this != &val );

	if ( val.isSign() )
	{
		setNan();
		return 2;
	}

	unsigned c = 0;
	TBigFloat  temp;

	temp.setLn2();
	setValue( val.exponent );
	c += mul( temp );

	temp.stats    = val.stats;
	temp.mantissa = val.mantissa;
	temp.exponent.setZero();

	c+= temp.ln();
	c+= addDirty( temp );

	return (c)? 1 : 0;
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::rejectDecimal()
{
	if ( isZero() )
		return;

	if ( exponent >= int( MantTotalBits - 1) )
		return;

	if ( exponent.isSign() )
	{
		setZero();
		return;
	}

	int e;
	if ( !exponent.convertToInt( e ) )
	{
		assert( 0 );
	}
	assert( e >= 0 );

	unsigned val = MantTotalBits - e - 1;

	unsigned nW = val / BaseTypeBits;
	unsigned nB = val % BaseTypeBits;

	for( unsigned i = 0 ; i < nW ; ++i )
		mantissa.setElement( i , 0 );

	val = mantissa.getElement( nW );

	val >>= nB;
	val <<= nB;

	mantissa.setElement( nW , val );
}


//  x           n
// e   = SUM[  x /  n ! ]   
//       n=0
template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::expFraction( TBigFloat const& val )
{
	assert( val <= 1.0f && val >= -1.0f );

	TBigFloat fact;
	fact.setOne();

	TBigFloat xn   = val;
	TBigFloat An;

	*this = 1.0;
	add( val );

	unsigned n = 2;

	while(1)
	{
		fact.mul( n );
		xn.mul( val );

		An = xn;
		An.div( fact );

		if ( !normalize( An ) )
			break;

		if ( isSign() == An.isSign() )
			doMantissaAdd( An );
		else
			doMantissaSub( An );

		assert( n < n + 1 );
		++n;
	}
}


template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::convertToDouble( double& val )
{
	assert( sizeof( BaseType ) == 4 );
	DFloatFormat* fmt = (DFloatFormat*)&val;

	if ( exponent >  DFLOAT_EXP_BIAS ||
		 exponent < -DFLOAT_EXP_BIAS )
	{
		//TODO: Denormalize case
		return false;
	}

	int exp;
	if ( !exponent.convertToInt( exp ) )
	{
		assert( 0 );
	}

	fmt->sign = ( isSign() ) ? 1 : 0;
	fmt->exp = unsigned( exp + DFLOAT_EXP_BIAS );

	if ( sizeof( BaseType ) == 4 )
	{
		uint32 mh = mantissa.getElement( MantNum - 1 );
		uint32 ml = mantissa.getElement( MantNum - 2 );

		mh <<= 1;

		assert( BaseTypeBits == 32 );

		fmt->hmant = mh >> ( BaseTypeBits - 20 );
		fmt->lmant = ( mh << 20 ) | (ml >> ( BaseTypeBits - 20 - 1) );
	}
	else
	{
		assert(0);
	}

	

	return true;
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::convertToFloat( float& val )
{
	SFloatFormat* fmt = (SFloatFormat*)&val;

	if ( exponent >  SFLOAT_EXP_BIAS ||
		exponent < -SFLOAT_EXP_BIAS )
	{
		//TODO : Denormalize case
		return false;
	}
	int exp;
	if ( !exponent.convertToInt( exp ) )
	{
		assert( 0 );
	}

	fmt->sign = ( isSign() ) ? 1 : 0;
	fmt->exp = unsigned( exp + SFLOAT_EXP_BIAS );

	if ( sizeof( BaseType ) == 4 )
	{
		uint32 m = mantissa.getElement( MantNum - 1 );
		m <<= 1;
		m >>= BaseTypeBits - 23;
		fmt->mant = m;
	}
	else
	{
		assert(0);
	}
	return true;
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setValue( float val )
{
	SFloatFormat* fmt = (SFloatFormat*) &val;

	if ( fmt->exp == 0 )
	{
		if ( fmt->mant == 0 )
		{
			setZero();
			return;
		}
		//TODO: Denormalize case
	}
	else
	{
		int      exp  = fmt->exp;
		uint32   mant = BaseTypeHighestBit | ( fmt->mant << 8 );

		stats = 0;
		if ( fmt->sign )
		{
			stats |= STATS_SIGN;
		}

		if ( exp == SFLOAT_EXP_ERROR )
		{
			if ( mant != 0 )
				stats |= STATS_NAN;
			else
				stats |= STATS_INFINITY;

			return;
		}

		mantissa.setZero();
		if ( sizeof( BaseType ) == 4 )
		{
			mantissa.setElement( MantNum - 1 , mant );
		}
		else
		{
			assert(0);
		}
		
		exponent = exp - SFLOAT_EXP_BIAS;
	}
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setValue( double val )
{
	DFloatFormat* fmt = (DFloatFormat*) &val;

	if ( fmt->exp == 0 )
	{
		if ( fmt->lmant == 0 && fmt->hmant == 0 )
		{
			setZero();
			return;
		}
		//TODO: Denormalize case
	}
	else
	{
		int       exp   = fmt->exp;
		uint32    lmant = fmt->lmant << 11;
		uint32    hmant = BaseTypeHighestBit | ( fmt->hmant << 11 ) | ( fmt->lmant >> (32 - 11) ) ;

		stats = 0;

		if ( fmt->sign )
		{
			stats |= STATS_SIGN;
		}

		if ( exp == DFLOAT_EXP_ERROR )
		{
			if ( hmant != 0 || lmant != 0 )
				stats |= STATS_NAN;
			else
				stats |= STATS_INFINITY;
			return;
		}

		if ( sizeof( BaseType ) == 4 )
		{
			mantissa.setZero();
			mantissa.setElement( MantNum - 1 , hmant );

			if ( MantNum > 1 )
				mantissa.setElement( MantNum - 2 , lmant );
		}
		else
		{
			assert(0);
		}

		exponent = exp - DFLOAT_EXP_BIAS;
	}
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setValue( unsigned val )
{
	if ( !val )
	{
		setZero();
		return;
	}

	unsigned bit = MantType::findHighestNonZeroBit( val );
	assert( bit != 0 );

	val <<= BaseTypeBits - bit;

	mantissa.setZero();
	mantissa.setElement( MantNum - 1 , val );
	exponent.setValue( bit - 1 );
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setZero()
{
	stats = STATS_ZERO;
	mantissa.setZero();
	exponent.setZero();
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setValue( TBigFloat const& val )
{
	stats    = val.stats;
	exponent = val.exponent;
	mantissa = val.mantissa;
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::exp( TBigFloat const& val )
{
	TBigFloat& ncVal = const_cast< TBigFloat& >( val );

	int ne = 0;

	TBigUint< ExpNum > Nexp;
	bool ch = ncVal.exponent >= 0;

	if ( ch )
	{
		ncVal.exponent.getElements( Nexp );
		Nexp += 1u;

		ncVal.exponent = -1;
	}

	expFraction( ncVal );

	if ( ch )
	{
		ncVal.exponent = Nexp;
		ncVal.exponent -= 1;
		unsigned carry = pow2NDirty( Nexp );

		return carry;
	}
	return 0;
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setOne()
{
	stats = 0;
	exponent.setZero();
	mantissa.setZero();
	mantissa.setElement( MantNum - 1 , BaseTypeHighestBit );
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::mul( unsigned rh )
{
	if ( isNan() )
		return 2;

	if ( isZero() )
		return 0;

	unsigned carry = mantissa.mul( rh );

	if ( carry )
	{
		unsigned move = BigUintBase::findHighestNonZeroBit( carry );
		mantissa.shiftRightBitLessElementBit( move , carry );
		return exponent.add( move );
	}
	return 0;
}


template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::getIntegerPart( TBigFloat& rh ) const
{
	if ( isZero() )
		return;

	if ( exponent.isSign() )
	{
		rh.setZero();
		return;
	}


	if ( exponent > MantTotalBits - 1 )
	{
		rh = *this;
		return;
	}

	rh.stats    = stats;
	rh.exponent = exponent;

	int e;
	if ( !exponent.convertToInt( e ) )
	{
		assert( 0 );
	}
	assert( e >= 0 );

	unsigned val = e + 1;

	unsigned nW = val / BaseTypeBits;
	unsigned nB = val % BaseTypeBits;
	nB = BaseTypeBits - nB;

	int idx = MantNum - 1;
	int idxEnd = MantNum - 1 - nW;
	for( ; idx > idxEnd ; --idx )
		rh.mantissa.setElement( idx , mantissa.getElement( idx ) );

	val = mantissa.getElement( idx );

	val >>= nB;
	val <<= nB;

	rh.mantissa.setElement( idx , val );

	for( --idx ; idx >= 0 ; --idx )
		rh.mantissa.setElement( idx , 0 );
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::getString( std::string& str , unsigned base /*= 10 */ )
{
	TBigFloat new_exp;
	TBigFloat temp;
	temp.exponent = this->exponent;
	temp.exponent.sub( MantTotalBits - 1 );

	new_exp.setValue( temp.exponent );

	temp.setLn2();
	new_exp.mul( temp );
	temp.setLn10();
	new_exp.div( temp );

	temp.setOne();
	temp.exponent.sub( 2 );

	new_exp.add( temp );

	if( !new_exp.isSign() && !new_exp.isInteger())
	{
		temp.setOne();
		new_exp.add( temp );
	}

	new_exp.rejectDecimal();
	assert( new_exp.isInteger() );

	new_exp.convertMantissaToUInt();

	temp.setValue( 10u );
	temp.powDirty( new_exp.mantissa );

	if ( new_exp.isSign() )
		temp.inverse();

	TBigFloat new_mant;
	new_mant = *this;
	new_mant.div( temp );

	int ne;
	new_mant.exponent.convertToInt( ne );
	assert( MantTotalBits - 1 >= ne );

	unsigned mv = ( MantTotalBits - 1 ) - ne;

	if ( mv )
		new_mant.mantissa.shiftRightBitLessElementBit( mv , 0 );

	MantType::convertToString( new_mant.mantissa ,  str , 10 , 30  );
}




template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::convertToUInt( unsigned& val )
{
	if ( isNan() )
		return true;

	if ( isZero() )
	{
		val = 0;
		return true;
	}

	if ( !isInteger() )
		return false;

	int ne;
	if ( !exponent.convertToInt( ne ) )
		return false;

	if ( ne > sizeof( int ) * 4 )
		return false;

	val = mantissa.getElement( MantNum - 1 ) >> ( BaseTypeBits - ne - 1 ); 

	return true;
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::isInteger()
{
	if ( isNan() )
		return false;

	unsigned num = getMantissaVaildBitNum();

	if ( num == 0 ) //isZero()
		return true;

	if ( exponent < num - 1 )
		return false;

	return true;
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setFromString( char const* str , unsigned base )
{
	bool sF = false;
	setZero();
	char const* p = str;

	TBigFloat temp;

	ExpType exp_num;
	exp_num.setZero();
	while ( *p != '\0' )
	{
		char c = *p;
		if( c == '.' )
			sF = true;
		else if ( c == 'E')
			break;
		else
		{
			mul( base );
			BaseType n = MantType::convertCharToNum( *p );
			temp.setValue( n );
			add( temp );

			if ( sF )
				exp_num.add( 1 );
		}
		++p;
	}

	if ( *p == 'E' )
	{
		++p;
		ExpType exp_num2;
		exp_num2.setFromStr( p , base );
		exp_num.sub( exp_num2 );
	}

	temp.setValue( base );
	temp.powDirty( exp_num );
	div( temp );
}


template < int MantNum , int ExpNum >
template < int N >
unsigned TBigFloat<MantNum, ExpNum>::powDirty( TBigUint< N >& rh )
{
	if ( isNan() )
		return 2;

	if ( rh.isZero() )
	{
		if( isZero() )
		{
			setNan();
			return 2;
		}
		else
		{
			setOne();
			return 0;
		}
	}

	TBigFloat start  = *this;
	setOne();

	unsigned c = 0;
	while( !c )
	{
		if ( rh.mod2() )
			c += mul( start );

		rh.shiftRightBitLessElementBit( 1 , 0 );

		if ( rh.isZero()  )
			break;

		c += start.mul( start );
	}
	return ( c ) ? 1 : 0;
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::convertMantissaToUInt()
{
	assert( isInteger() );

	unsigned num = getMantissaVaildBitNum();

	if ( num == 0 )
	{
		mantissa.setZero();
		exponent.setZero();
		return true;
	}

	if ( exponent > MantTotalBits - 1 )
		return false;

	if ( exponent < num - 1 )
		return false;

	int ne;
	if ( !exponent.convertToInt( ne ) )
	{
		assert( 0 );
	}
	assert( ne > 0 );
	unsigned move = MantTotalBits - unsigned( ne + 1 );

	mantissa.shiftRightBit( move , 0 );

	return true;
}

#endif // BFloat_h__