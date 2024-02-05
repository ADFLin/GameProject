#pragma once
#ifndef FixNumber_H_68F5D9AD_4125_4F3E_92C2_584B8C055082
#define FixNumber_H_68F5D9AD_4125_4F3E_92C2_584B8C055082

template< class IntType , IntType SCALE >
class TFixedValue
{
public:

	double operator (){ return static_cast<double>( mValue ) / SCALE;  }

	TFixedValue operator + ( TFixedValue const& rhs ) const
	{
		return TFixedValue( mValue + rhs.mValue );
	}
	TFixedValue operator - ( TFixedValue const& rhs ) const
	{
		return TFixedValue( mValue - rhs.mValue );
	}
	TFixedValue operator * ( TFixedValue const& rhs ) const
	{
		return TFixedValue( ( mValue * rhs.mValue ) / SCALE );
	}
	TFixedValue operator / ( TFixedValue const& rhs ) const
	{
		return TFixedValue( ( mValue / rhs.mValue ) * SCALE );
	}
	IntType mValue;
};

#endif // FixNumber_H_68F5D9AD_4125_4F3E_92C2_584B8C055082