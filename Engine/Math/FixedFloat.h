

namespace IEEE
{
	struct  SingleFloatFormat
	{
		uint32 frac:23;
		uint32 exp:8;
		uint32 s:1;

		static uint32 const FracBitNum = 23;
		static uint32 const ExpBias = 127;
		static uint32 const ExpError = 255;
	};
}
namespace Math
{

	template < class IntType >
	struct IntTypeBit { };

	template<> struct IntTypeBit< int16 >{ enum { Result = 16 } };
	template<> struct IntTypeBit< int32 >{ enum { Result = 32 } };
	template<> struct IntTypeBit< int64 >{ enum { Result = 64 } };

	template< class IntType , uint8 Offset >
	class TFixedValue
	{
		enum
		{
			MaxExp = IntTypeBit< IntType >::Result - Offset;
		};

		TFixedValue( float value )
		{
			if ( !fromFloat(value) )
				mValue = 0;
		}		

		float operator() const
		{
			return toFloat();
		}
		double operator() const
		{
			return double( mValue ) / ( 1 << Offset );
		}
		TFixedValue operator + ( TFixedValue const& other ) const
		{
			return TFixedValue( PrivateConstruct() , mValue + other.mValue );
		}
		TFixedValue operator - ( TFixedValue const& other ) const
		{
			return TFixedValue( PrivateConstruct() , mValue - other.mValue );
		}
		TFixedValue operator * ( TFixedValue const& other ) const
		{
			return TFixedValue( PrivateConstruct() , ( mValue * other.mValue ) >> Offset );
		}
		TFixedValue operator / ( TFixedValue const& other ) const
		{
			return TFixedValue( PrivateConstruct() , ( mValue << Offset ) / other.mValue );
		}
	private:

		std::string toString()
		{
			if ( IntTypeBit< IntType >::Result <= 32 )
			{
				double value = mValue;
				value /= double( 1 << Offset );
				InlineString< 512 > str;
				str.format( "%lf" , value );
				return std::string( str.c_str() );
			}
			else
			{


			}
		}

		float toFloat() const
		{
			using namespace IEEE;
			typedef SingleFloatFormat FloatFormat;
			float result;
			FloatFormat* fv = (FloatFormat*)&result;

			IntType value = mValue;
			if ( value < 0 )
			{
				value = -value;
				fv->s = 1;
			}
			else
			{
				fv->s = 0;
			}
			unsigned hBit;
			//fv->frac =  ;


		}
		bool fromFloat( float value )
		{
			using namespace IEEE;
			typedef SingleFloatFormat FloatFormat;
			FloatFormat* fv = (FloatFormat*)&value;
			if ( fv->exp == 0)
			{
				if ( fv->frac == 0 )
				{
					mValue = 0;
				}
				else 
				{
					//denormliaze format
					if ( Offset > FloatFormat::ExpBias - 1 )
					{

					}
					else
					{
						mValue = 0;
					}
				}
			}
			else
			{
				//normalize format
				if ( fv->exp == FloatFormat::ExpError )
				{
					return false;
				}
				int exp = int(fv->exp) - int(FloatFormat::ExpBias);
				if ( exp > MaxExp || exp < (int)-Offset)
				{
					return false;
				}

				mValue = uint32( 1 << FloatFormat::FracBitNum ) | fv->frac;
				mValue >>= ( exp - FloatFormat::FracBitNum ) - Offset;
			}
			return true;
		}
		struct PrivateConstruct {};
		TFixedValue( PrivateConstruct , IntType const& value ):mValue(value){ }
		IntType mValue;
	};



}