#ifndef FixNumber_h__
#define FixNumber_h__


template< class IntType , IntType SCALE >
class FixNumber
{
public:

	float operator (){ return static_cast< float >( mValue ) / SCALE;  }

	FixNumber operator + ( FixNumber const& rhs ) const
	{
		return FixNumber( mValue + rhs.mValue );
	}
	FixNumber operator - ( FixNumber const& rhs ) const
	{
		return FixNumber( mValue - rhs.mValue );
	}
	FixNumber operator * ( FixNumber const& rhs ) const
	{
		return FixNumber( ( mValue * rhs.mValue ) / SCALE );
	}
	FixNumber operator / ( FixNumber const& rhs ) const
	{
		return FixNumber( ( mValue / rhs.mValue ) * SCALE );
	}
	IntType mValue;
};
#endif // FixNumber_h__