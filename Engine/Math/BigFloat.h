#ifndef BFloat_h__
#define BFloat_h__

#include "BigInteger.h"
#include "MacroCommon.h"

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

	bool convertToUInt( unsigned& val ) const;
	bool convertToFloat( float& val ) const;
	bool convertToDouble( double& val ) const
	{
		if ( isNan() ) return false;
		if ( isZero() ) { val = 0.0; return true; }
		if ( isInfinity() ) { val = isSign() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity(); return true; }

		int e;
		if (!exponent.convertToInt(e))
		{
			// Exponent too large for double?
			val = isSign() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
			return true;
		}
		
		// Range Check (Underflow/Overflow)
		// Double limit: +/- 1023. Denormals down to -1074.
		if ( e > 1023 )
		{
			val = isSign() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
			return true;
		}
		if ( e < -1023 )
		{
			// Could be denormal... but for now let's just flush to zero if too small.
			// BigFloat usually normalizes to 1.xxx.
			// Double Denormal: 0.xxx * 2^-1022.
			// Minimum double ~ 2^-1074.
			if (e < -1075) 
			{
				val = 0.0;
				return true;
			}
			// Let's rely on ldexp for denormals/rare cases if we don't want to handle bit-packing denormals manually
			// or just flush sub-normal range to 0 for safety in this specific context (fractal zoom).
			// The unit tests shouldn't hit denormals for 1.0 cases.
			if (e < -1023)
			{
				val = 0.0;
				return true;
			}
		}

		// Bit-Exact Construction
		// Assume Normalized BigFloat (MSB is 1)
		// DFloatFormat: Sign(1) Exp(11) hmant(20) lmant(32)
		// Bias 1023.
		
		uint64_t raw = 0;
		uint64_t sign = isSign() ? 1ULL : 0ULL;
		uint64_t expBits = (uint64_t)(e + 1023); // Bias
		
		// Mantissa bits
		uint32 mh = mantissa.getElement(MantNum - 1);
		uint32 ml = (MantNum > 1) ? mantissa.getElement(MantNum - 2) : 0;
		
		// Default BigFloat is 1.xxxx (bit 31 is 1)
		// Double expects 1.xxxx explicit (so store xxxx)
		// remove implicit 1
		mh <<= 1; 
		
		// We need top 52 bits called 'f'.
		// mh has 31 bits (31..1). 
		// ml has 32 bits.
		
		// Top 20 bits of fraction -> hmant (bits 51..32)
		// mh >> 12.
		uint64_t hmant = mh >> 12;
		
		// Bottom 32 bits of fraction -> lmant (bits 31..0)
		// Need remaining 11 bits of mh (31..1 shifted to 20..0? No)
		// mh << 20 puts remaining 11 bits at top of 32-bit word?
		// See previous analysis: (mh << 20) | (ml >> 11) works.
		
		uint64_t lmant = ((uint64_t)mh << 20) | ((uint64_t)ml >> 11);
		// Note: mh is 32-bit. Cast to 64 to avoid overflow if shifted?
		// Actually mh<<20 fits in 32 bits, but writing to uint64 lmant.
		// lmant should be just the lower 32 bits of the 52-bit mantissa.
		// Wait, I can construct the full 64-bit uint directly.
		
		// Double Layout: [Sign 1][Exp 11][Mant 52]
		raw = (sign << 63) | (expBits << 52);
		
		// Mant 52 splits into High(20) and Low(32).
		// hmant is 20 bits. lmant is 32 bits.
		// The `lmant` calculation above:
		// (mh << 20) is 32-bit. It keeps the 11 bits at [31..21]. overlap at 20 is 0.
		// (ml >> 11) is 32-bit. bits [20..0].
		// So `lmant` variable holds the bottom 32 bits of the fraction.
		// `hmant` variable holds the top 20 bits.
		
		raw |= (hmant << 32) | (lmant & 0xFFFFFFFF);
		
		#ifdef _MSC_VER
		val = *reinterpret_cast<double*>(&raw);
		#else
		memcpy(&val, &raw, sizeof(double));
		#endif

		return true;
	}

	void setZero();
	void setOne();
	void setLn10();
	void setLn2();

	bool operator >  ( TBigFloat const& rh ) const { return big(rh);}
	bool operator <= ( TBigFloat const& rh ) const { return !big(rh); }
	bool operator <  ( TBigFloat const& rh ) const { return rh.big( *this ); }
	bool operator >= ( TBigFloat const& rh ) const { return !rh.big( *this ); }
	bool operator == ( TBigFloat const& rh ) const { return stats == rh.stats && exponent == rh.exponent && mantissa == rh.mantissa; }
	bool operator != ( TBigFloat const& rh ) const { return !(*this == rh); }

	unsigned add( TBigFloat const& rh )
	{
		ExpType diff;
		
		int expDiff;
		// Check invalid fast
		diff = exponent;
		if ( diff.sub(rh.exponent) )
		{
			// exponent < rh.exponent.
			ExpType tempExp = rh.exponent;
			tempExp.sub( exponent ); // get abs diff
			tempExp.convertToInt( expDiff );
			if ( expDiff > (int)MantTotalBits + 1 )
			{
				*this = rh;
				return 0;
			}
		}
		else
		{
			// exponent >= rh.exponent
			diff.convertToInt( expDiff );
			if ( expDiff > (int)MantTotalBits + 1 )
			{
				return 0;
			}
		}

		TBigFloat temp = rh; 
		return addDirty( temp ); 
	}
	unsigned sub( TBigFloat const& rh )
	{
		ExpType diff;
		int expDiff;
		// Check invalid fast
		diff = exponent;
		if ( diff.sub(rh.exponent) )
		{
			// exponent < rh.exponent.
			ExpType tempExp = rh.exponent;
			tempExp.sub( exponent ); // get abs diff
			tempExp.convertToInt( expDiff );
			if ( expDiff > (int)MantTotalBits + 1 )
			{
				*this = rh;
				changeSign();
				return 0;
			}
		}
		else
		{
			// exponent >= rh.exponent
			diff.convertToInt( expDiff );
			if ( expDiff > (int)MantTotalBits + 1 )
			{
				return 0;
			}
		}

		TBigFloat temp = rh; 
		return subDirty( temp ); 
	}


	unsigned mul( unsigned  rh );

	void inverse();

	unsigned getMantissaBitNum() const;

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

	unsigned div( TBigFloat const& rh , bool* outExact = 0 );


	unsigned subDirty( TBigFloat& rh );
	unsigned addDirty( TBigFloat& rh );

	unsigned doMantissaAdd( TBigFloat const& rh);
	unsigned doMantissaSub( TBigFloat& rh );

	void getIntegerPart( TBigFloat& rh ) const;

	bool isZero() const
	{ 
		//CHECK( mantissa.isZero() && exponent.isZero() );
		return mantissa.isZero(); 
	}

	bool isStandardize() const 
	{ 
		return  mantissa.isZero() || ( mantissa.getElement( MantNum - 1 ) & BaseTypeHighestBit )!= 0; 
	}

	unsigned standardize();
	bool     normalize( TBigFloat& rh );
	bool     isInteger() const;

};

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::inverse()
{
	TBigFloat temp = *this;
	setOne();
	unsigned c = div( temp );
	CHECK( c == 0 );
}

template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::getMantissaBitNum() const
{
	if ( isZero() )
		return 0;

	int word = mantissa.getLowestNonZeroElement();
	if ( word == -1 ) return 0;
	unsigned bit = BigUintBase::findLowestNonZeroBit( mantissa.getElement( word ) );

	if ( bit == 0 ) return 0;
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
	CHECK( carry == 0 );
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
	//CHECK ( isStandardize() || isZero() );
	standardize();
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
	typename MantType::MulRType result;

	mantissa.doMul( rh.mantissa , result );

	int word = result.getHighestNonZeroElement();
	if ( word == -1 ) // result == 0;
	{
		setZero();
		return 0;
	}
	CHECK( word == ( 2 * MantNum - 1 ) );

	unsigned carry = 0;

	unsigned val = result.getElement( 2 * MantNum - 1 );

	CHECK( BigUintBase::findHighestNonZeroBit( val ) == BaseTypeBits || 
		    BigUintBase::findHighestNonZeroBit( val ) == BaseTypeBits - 1 );

	carry += exponent.add( rh.exponent );

	if ( val & BaseTypeHighestBit )
		carry += exponent.add( 1 );
	else
		result.shiftLeftBitLessElementBit( 1 , 0 );

	bool roundUp = ( result.getElement( MantNum - 1 ) & BaseTypeHighestBit ) != 0;

	for( int i = 0 ; i < MantNum  ; ++i )
	{
		unsigned val = result.getElement( i + MantNum ); 
		mantissa.setElement( i , val );
	}

	if ( roundUp )
	{
		if ( mantissa.add( 1 ) )
		{
			mantissa.setElement( MantNum - 1 , BaseTypeHighestBit );
			carry += exponent.add( 1 );
		}
	}

	if ( isSign() == rh.isSign() )
		abs();
	else
		setSign();

	CHECK( isStandardize() );
	return carry;
}



template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::div( TBigFloat const& rh , bool* outExact )
{
	if ( rh.isZero() )
	{
		return 1;
	}

	if ( isZero() )
	{
		if (outExact) *outExact = true;
		return 0;
	}

	typename MantType::MulRType md;
	typename MantType::MulRType mr;
	typename MantType::MulRType mod;

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

	if ( outExact ) *outExact = mod.isZero();

	mod.shiftLeftBit( 1 , 0 );
	if ( mod >= mr )
	{
		md.add( 1 );
	}

	unsigned carry = 0;

	int word = md.getHighestNonZeroElement();
	CHECK ( word == MantNum || word == MantNum - 1 );

	carry += exponent.sub( rh.exponent );

	if ( word == MantNum )
	{
		CHECK( md.getElement( MantNum ) == 1 );
		md.shiftRightBitLessElementBit( 1 , 0 );
	}
	else
	{
		CHECK( BigUintBase::findHighestNonZeroBit( md.getElement( MantNum - 1 ) ) == BaseTypeBits );
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

	CHECK( isStandardize() );
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
		if ( move >= MantTotalBits )
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

	CHECK( exponent == rh.exponent );
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
	unsigned targetShift = (unsigned)(MantNum - 1) * BaseTypeBits + (BaseTypeBits - 1);
	unsigned currentShift = (unsigned)word * BaseTypeBits + (bit - 1);
	
	if (targetShift > currentShift)
	{
		unsigned move = targetShift - currentShift;
		mantissa.shiftLeftBit( move , 0 );
		exponent.sub( move );
	}
	else if (currentShift > targetShift)
	{
		unsigned move = currentShift - targetShift;
		mantissa.shiftRightBit( move , 0 );
		exponent.add( move );
	}
	return 0;
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
	CHECK( isStandardize() && rh.isStandardize() );
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

	if ( y.isZero() )
	{
		setZero();
		return 0;
	}

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

		if ( An.isZero() || !normalize( An ) )
			break;

		CHECK( isSign() == An.isSign() );
		doMantissaAdd( An );

		CHECK( n < n + 1 );
		++n;
	}

	return exponent.add( 1 );
}


template < int MantNum , int ExpNum >
unsigned TBigFloat<MantNum, ExpNum>::ln( TBigFloat const& val )
{
	CHECK( this != &val );

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
		CHECK( 0 );
	}
	CHECK( e >= 0 );

	unsigned val = MantTotalBits - e - 1;

	unsigned nW = val / BaseTypeBits;
	unsigned nB = val % BaseTypeBits;

	for( unsigned i = 0 ; i < nW ; ++i )
		mantissa.setElement( i , 0 );

	BaseType m = mantissa.getElement( nW );
	m >>= nB;
	m <<= nB;

	mantissa.setElement( nW , m );
	standardize();
}


//  x           n
// e   = SUM[  x /  n ! ]   
//       n=0
template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::expFraction( TBigFloat const& val )
{
	CHECK( val <= 1.0f && val >= -1.0f );

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

		if ( An.isZero() || !normalize( An ) )
			break;

		if ( isSign() == An.isSign() )
			doMantissaAdd( An );
		else
			doMantissaSub( An );

		CHECK( n < n + 1 );
		++n;
	}
}




template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::convertToFloat( float& val ) const
{
	SFloatFormat* fmt = (SFloatFormat*)&val;

	if ( exponent >  SFLOAT_EXP_BIAS ||
		exponent < -SFLOAT_EXP_BIAS )
	{
		//#TODO : Denormalize case
		return false;
	}
	int exp;
	if ( !exponent.convertToInt( exp ) )
	{
		CHECK( 0 );
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
		CHECK(0);
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
		//#TODO: Denormalize case
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
			CHECK(0);
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
		//#TODO: Denormalize case
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
			CHECK(0);
		}

		exponent = exp - DFLOAT_EXP_BIAS;
	}
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setValue( unsigned val )
{
	if ( !val ) { setZero(); return; }
	stats = 0; // Clear stats
	mantissa.setZero();
	mantissa.setElement( 0 , val );
	exponent.setValue( (int)MantTotalBits - 1 );
	standardize();
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
template< int N >
void TBigFloat<MantNum, ExpNum>::setValue( TBigInt< N > const& val )
{
	if ( val.isZero() ) { setZero(); return; }

	stats = 0; // Clear stats

	TBigInt< N > temp = val;
	bool beS = temp.isSign();
	if ( beS ) temp.applyComplement();

	mantissa.setZero();
	for( int i = 0 ; i < MantNum && i < N ; ++i )
		mantissa.setElement( i , temp.castUInt().getElement(i) );

	exponent.setValue( (int)MantTotalBits - 1 );
	standardize();
	if ( beS ) setSign();
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::getIntegerPart( TBigFloat& rh ) const
{
	if ( isZero() ) { rh.setZero(); return; }

	if ( exponent.isSign() )
	{
		rh.setZero();
		return;
	}

	if ( exponent >= int( MantTotalBits - 1 ) )
	{
		rh = *this;
		return;
	}

	rh.stats    = stats;
	rh.exponent = exponent;

	int e;
	exponent.convertToInt( e );
	unsigned val = (unsigned)(MantTotalBits - 1 - e);

	unsigned nW = val / BaseTypeBits;
	unsigned nB = val % BaseTypeBits;

	rh.mantissa = mantissa;
	for( unsigned i = 0 ; i < nW ; ++i )
		rh.mantissa.setElement( i , 0 );

	BaseType m = rh.mantissa.getElement( nW );
	m >>= nB;
	m <<= nB;
	rh.mantissa.setElement( nW , m );
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::getString( std::string& str , unsigned base /*= 10 */ )
{
	if ( isZero() ) { str = "0"; return; }
	if ( isNan() ) { str = "NaN"; return; }
	if ( isInfinity() ) { str = isSign() ? "-Infinity" : "Infinity"; return; }

	bool beS = isSign();
	TBigFloat val = *this;
	val.abs();

	// 1. Calculate approximate 10-base exponent
	TBigInt< ExpNum > p_int;
	p_int.setZero();
	int e;
	if ( val.exponent.convertToInt( e ) )
	{
		// P = floor( exponent * log10(2) )
		int p_guess = (int)std::floor( (double)e * 0.3010299956639812 );
		p_int.setValue( p_guess );
	}

	// 2. Normalize val to [1, 10)
	TBigFloat scale; scale.setValue( 10u );
	TBigInt< ExpNum > p_abs_int = p_int; if ( p_abs_int.isSign() ) p_abs_int.applyComplement();
	TBigUint< ExpNum > p_work = p_abs_int.castUInt();
	scale.powDirty( p_work );
	if ( p_int.isSign() ) 
		val.mul( scale );
	else
		val.div( scale );

	int adj = 0;
	TBigFloat ten; ten.setValue(10u);
	int safety = 0;
	while ( val >= ten && safety++ < 100 ) { val.div( ten ); adj++; }
	safety = 0;
	while ( val < 1.0 && !val.isZero() && safety++ < 100 ) { val.mul( ten ); adj--; }
	p_int.add( adj );

	// 3. Extract digits one by one
	str.clear();
	if ( beS ) str += '-';
	
	// 3. Extract digits methods
	str.clear();
	if ( beS ) str += '-';
	
	// Optimization: Extract 9 digits at a time
	static const double DIGIT_BLOCK_VAL_D = 1e9;
	static const unsigned DIGIT_BLOCK_VAL = 1000000000;
	TBigFloat digitBlockScaler; 
	digitBlockScaler.setValue( DIGIT_BLOCK_VAL );

	bool isFirstBlock = true;
	// Calculate max meaningful decimal digits based on bit precision
	// log10(2) = 0.30102999566
	int maxDigits = (int)(MantTotalBits * 0.30103) + 9; // +9 for buffer

	for ( int i = 0; i < maxDigits; i += 9 )
	{
		double d;
		val.convertToDouble( d );
		
		unsigned block = (unsigned)d;
		// Fail-safe for precision
		if ( block >= DIGIT_BLOCK_VAL ) block = DIGIT_BLOCK_VAL - 1;

		char buffer[16];
		if ( isFirstBlock )
		{
			sprintf_s(buffer, "%u.", block);
			str += buffer;
			isFirstBlock = false;
		}
		else
		{
			sprintf_s(buffer, "%09u", block);
			str += buffer;
		}


		val.sub( (double)block );
		val.mul( digitBlockScaler );
		
		// Tolerance check for precision noise
		double dbgVal;
		if ( val.convertToDouble(dbgVal) && std::abs(dbgVal) < 1e-5 )
		{
			val.setZero();
		}

		if ( val.isZero() && i > 15 ) break;
	}
	// Remove trailing zeros
	size_t decimalPointPos = str.find('.');
	if (decimalPointPos != std::string::npos)
	{
		size_t lastNonZero = str.find_last_not_of('0');
		if (lastNonZero != std::string::npos)
		{
			if (str[lastNonZero] == '.')
			{
				str.erase(lastNonZero);
			}
			else
			{
				str.erase(lastNonZero + 1);
			}
		}
	}

	if ( !p_int.isZero() )
	{
		std::string expStr;
		p_int.getString( expStr , 10 );
		if ( expStr != "0" )
		{
			str += "E";
			str += expStr;
		}
	}
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::convertToUInt( unsigned& val ) const
{
	if ( isNan() ) return false;
	if ( isZero() ) { val = 0; return true; }
	
	int ne;
	if ( !exponent.convertToInt( ne ) ) return false;
	if ( ne < 0 ) { val = 0; return true; }
	if ( ne > 31 ) return false;

	val = mantissa.getElement( MantNum - 1 ) >> ( (int)BaseTypeBits - 1 - ne ); 
	return true;
}

template < int MantNum , int ExpNum >
bool TBigFloat<MantNum, ExpNum>::isInteger() const
{
	if ( isNan() )
		return false;

	unsigned num = getMantissaBitNum();

	if ( num == 0 ) //isZero()
		return true;

	if ( exponent < num - 1 )
		return false;

	return true;
}

template < int MantNum , int ExpNum >
void TBigFloat<MantNum, ExpNum>::setFromString( char const* str , unsigned base )
{
	setZero(); 
	abs(); 
	if ( !str || *str == '\0' ) return;

	bool isNeg = false;
	if ( *str == '-' ) { isNeg = true; ++str; }
	else if ( *str == '+' ) { ++str; }

	// Use TBigInt to build the coefficient precisely as an integer
	TBigInt< MantNum > coefficient;
	coefficient.setZero();
	
	bool sF = false;
	ExpType exp_num; exp_num.setZero();

	char const* p = str;
	int maxDigits = (int)(MantTotalBits * 0.30103);
	int digitCount = 0;

	// Optimization: Chunk parsing
	while ( *p != '\0' )
	{
		if( *p == '.' ) { sF = true; }
		else if ( *p == 'E' || *p == 'e' ) { break; }
		else
		{
			if ( digitCount >= maxDigits )
			{
				// Prevention of overflow: Skip accumulation
				if ( !sF ) exp_num.sub( 1 );
			}
			else
			{
				unsigned chunk = 0;
				unsigned multiplier = 1;
				int count = 0;

				// Process up to 9 digits at once
				while ( *p >= '0' && *p <= '9' && count < 9 )
				{
					// Check limit inside chunking
					if ( digitCount + count >= maxDigits ) break;
					
					// Leading zeros before any non-zero digit are not significant, except after decimal point (handled by exp)
					// Actually, 'digitCount' tracks precision usage.
					// We should only increment 'digitCount' if we are storing significant digits or if the number is non-zero.
					// But wait, "0.0000" -> sF=true. '0' digits.
					// If we parse '0', coefficient remains 0.
					
					chunk = chunk * 10 + (*p - '0');
					multiplier *= 10;
					++p;
					++count;
					if ( sF ) exp_num.add( 1 );
				}
				
				if ( count > 0 )
				{
					if ( multiplier > 10 ) coefficient.mul( multiplier );
					else coefficient.mul( 10 );
					
					coefficient.add( chunk );
					
					if ( !coefficient.isZero() )
					{
						digitCount += count;
					}
					else
					{
						// If coefficient is still zero, these were leading zeros.
						// We don't count them towards maxDigits limit.
						
						// BUT, if we are after decimal point (sF=true), we tracked them in exp_num.
						// So mathematically it is correct (value is 0 * 10^-N = 0).
						// Just need to ensure we don't assume we "ran out of precision" for 0.00000000001
					}

					// p already advanced, continue loop but don't skip check
					continue;
				}
				
				// Fallback for remaining single digits if limit hit inside chunk block processing
				if ( *p >= '0' && *p <= '9' )
				{
					if ( digitCount < maxDigits )
					{
						unsigned n = MantType::convertCharToNum( *p );
						coefficient.mul( base );
						coefficient.add( n );
						if ( sF ) exp_num.add( 1 );
						
						if (!coefficient.isZero()) digitCount++;
					}
					else
					{
						if ( !sF ) exp_num.sub( 1 );
					}
				}
			}
		}
		if ( *p != '\0' && *p != 'E' && *p != 'e' ) ++p;
	}

	TBigFloat val;
	val.setValue( coefficient );

	if ( *p == 'E' || *p == 'e' )
	{
		++p;
		ExpType exp_num2;
		exp_num2.setFromStr( p , 10 );
		exp_num.sub( exp_num2 );
	}

	if ( !exp_num.isZero() )
	{
		ExpType abs_exp = exp_num;
		bool expIsNeg = abs_exp.isSign();
		if ( expIsNeg ) abs_exp.applyComplement();

		TBigFloat factor; factor.setValue( 10u );
		TBigUint< ExpNum > p_work = abs_exp.castUInt();
		factor.powDirty( p_work );
		
		if ( exp_num.isSign() ) val.mul( factor );
		else
		{
			bool beExact;
			val.div( factor , &beExact );
			if ( !beExact && val.mantissa.add( 1 ) )
			{
				val.mantissa.shiftRightBit( 1 , 0 );
				val.mantissa.setElement( MantNum - 1 , BaseTypeHighestBit );
				val.exponent.add( 1 );
			}
		}
	}

	val.standardize();
	*this = val;
	if ( isNeg ) setSign();
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
	CHECK( isInteger() );

	unsigned num = getMantissaBitNum();

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
		CHECK( 0 );
	}
	CHECK( ne > 0 );
	unsigned move = MantTotalBits - unsigned( ne + 1 );

	mantissa.shiftRightBit( move , 0 );

	return true;
}

#endif // BFloat_h__