#ifndef TVector2_h__
#define TVector2_h__

#include "Math/Base.h"

#include "Meta/Select.h"
#include "Meta/MetaBase.h"

#include <algorithm>

template<class T>
class TVector2
{
	using ScalarInputType = typename TSelect< Meta::IsPrimary<T>::Value, const T, T const& >::Type;
public:
	using ScalarType = T;
	
	TVector2() = default;
	constexpr TVector2(TVector2 const& rhs) = default;

	constexpr TVector2( ScalarInputType x ,ScalarInputType y):x(x),y(y){}

	template< class Q >
	constexpr TVector2( TVector2<Q> const& v ){ setValue( T(v.x) , T(v.y) ); }

	template< class Q >
	constexpr TVector2& operator = (TVector2<Q> const& v){  x = T(v.x);  y = T(v.y);  return *this; }

	constexpr void  setValue( ScalarInputType vx,ScalarInputType vy){	x = vx; y = vy;  }
	T        dot  ( TVector2 const& v ) const { return x * v.x + y * v.y ; }
	T        cross( TVector2 const& v ) const { return x * v.y - y * v.x ; }
	T        length2()                  const { return x * x + y * y; }

	TVector2 mul( TVector2 const& v ) const { return TVector2( x * v.x , y * v.y );  }
	TVector2 div( TVector2 const& v ) const { return TVector2( x / v.x , y / v.y );  }
	TVector2 max( TVector2 const& v ) const { return TVector2(Math::Max(x , v.x), Math::Max(y, v.y)); }
	TVector2 min( TVector2 const& v ) const { return TVector2(Math::Min(x, v.x), Math::Min(y, v.y)); }

	TVector2 abs() const { return TVector2(Math::Abs(x), Math::Abs(y)); }

	void setMax(TVector2 const& rhs)
	{
		if (x < rhs.x) x = rhs.x;
		if (y < rhs.y) y = rhs.y;
	}

	void setMin(TVector2 const& rhs)
	{
		if (x > rhs.x) x = rhs.x;
		if (y > rhs.y) y = rhs.y;
	}

	TVector2& operator *= ( ScalarInputType s )  {  x *= s ; y *= s; return *this;  }
	TVector2& operator /= ( ScalarInputType s )  {  x /= s ; y /= s; return *this;  }
	TVector2& operator += ( TVector2 const& v ){  x += v.x;  y += v.y;  return *this;  }
	TVector2& operator -= ( TVector2 const& v ){  x -= v.x;  y -= v.y;  return *this;  }


	TVector2 const operator - ( void ) const { return  TVector2( -x , -y );  }

	operator T*(){ return &x; }
	operator T const*() const { return &x; }

	static TVector2 Zero(){ return TVector2(0,0); }
	static TVector2 Fill(ScalarInputType s) { return TVector2(s, s); }
	static TVector2 PositiveX(){ return TVector2(1,0); }
	static TVector2 PositiveY(){ return TVector2(0,1); }
	static TVector2 NegativeX(){ return TVector2(-1,0); }
	static TVector2 NegativeY(){ return TVector2(0,-1); }

	TVector2 const operator + (TVector2 const& v) const {	return TVector2(x + v.x,y + v.y);  }
	TVector2 const operator - (TVector2 const& v) const {	return TVector2(x - v.x,y - v.y);  }

	TVector2 const operator * ( ScalarInputType s ) const {	return TVector2( x * s, y * s );  }
	TVector2 const operator / ( ScalarInputType s ) const {	return TVector2( x / s, y / s );  }

	TVector2 operator * (TVector2 const& v1) const {  return mul(v1);  }
	TVector2 operator / (TVector2 const& v1) const {  return div(v1);  }


	constexpr bool operator == (TVector2 const& v) const {  return x == v.x  && y == v.y;  }
	constexpr bool operator != (TVector2 const& v) const {  return ! (*this == v ); }

	bool     operator < (TVector2 const& rhs) const { return x < rhs.x && y < rhs.y; }
	bool     operator > (TVector2 const& rhs) const { return rhs < *this ; }
	bool     operator <= (TVector2 const& rhs) const { return x <= rhs.x && y <= rhs.y; }
	bool     operator >= (TVector2 const& rhs) const { return rhs <= *this; }

	friend TVector2 const operator * (ScalarInputType s,TVector2 const& v){  return TVector2(s*v.x,s*v.y);	}
	friend TVector2 Abs(TVector2 const& v)
	{
		return TVector2(std::abs(v.x), std::abs(v.y));
	}
public:
	T x,y;

private:
	void operator + ( int ) const;
	void operator - ( int ) const;
	void operator +=( int ) const;
	void operator -=( int ) const;

};

template< class T >
FORCEINLINE TVector2<T>  Clamp(TVector2<T> const& v, TVector2<T> const& min, TVector2<T> const& max)
{
	return TVector2<T>(Math::Clamp(v.x, min.x, max.x), Math::Clamp(v.y, min.y, max.y));
}

template< class T >
FORCEINLINE T  TaxicabDistance(TVector2<T> const& v1 , TVector2<T> const& v2)
{
	TVector2<T> dAbs = Abs(v2 - v1);
	return dAbs.x + dAbs.y;
}

#endif // TVector2_h__

