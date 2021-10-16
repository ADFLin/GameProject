#ifndef TVector3_h__
#define TVector3_h__

template< class T >
class TVector3
{
public:
	using ScalarInputType = T;
	using ScaleType = T;

	TVector3() = default;
	TVector3(TVector3 const&) = default;

	constexpr explicit TVector3(T const v[])
		:x(v[0]), y(v[1]), z(v[2])
	{

	}
	constexpr TVector3( ScalarInputType sx,ScalarInputType sy, ScalarInputType sz)
		:x(sx), y(sy), z(sz)
	{

	}

	template< class Q >
	constexpr TVector3( TVector3< Q > const& rhs )
		:x( rhs.x ),y(rhs.y),z(rhs.z){}

	constexpr void setValue( ScalarInputType sx,ScalarInputType sy,ScalarInputType sz)
	{ x=sx; y=sy; z=sz;}

	T length2() const { return dot( *this ); }

	operator T*(){ return &x; }
	operator T const*() const { return &x; }

	static TVector3 Zero() { return TVector3(0, 0, 0); }
	static TVector3 Fill(ScalarInputType s) { return TVector3(s, s, s); }
	static TVector3 PositiveX() { return TVector3(1, 0, 0); }
	static TVector3 PositiveY() { return TVector3(0, 1, 0); }
	static TVector3 PositiveZ() { return TVector3(0, 0, 1); }
	static TVector3 NegativeX() { return TVector3(-1, 0, 0); }
	static TVector3 NegativeY() { return TVector3(0, -1, 0); }
	static TVector3 NegativeZ() { return TVector3(0, 0, -1); }

	TVector3& operator += ( TVector3 const& v);
	TVector3& operator -= ( TVector3 const& v);
	TVector3& operator *= ( ScalarInputType s );


	TVector3 mul( TVector3 const& v )
	{
		return TVector3( x * v.x , y * v.y , z * v.z );
	}

	T        dot(TVector3 const& b) const;
	TVector3 cross(TVector3 const& b) const;

	bool     operator == ( TVector3 const& v ) const { return x == v.x && y == v.y && z == v.z; }
	bool     operator != ( TVector3 const& v ) const { return !this->operator == ( v ); }

public:
	T x,y,z;

private:
	void operator + ( int ) const;
	void operator - ( int ) const;
	void operator +=( int ) const;
	void operator -=( int ) const;
};


template< class T >
FORCEINLINE T TVector3<T>::dot( TVector3 const& b ) const
{
	return x*b.x + y*b.y + z*b.z;
}

template< class T >
FORCEINLINE TVector3<T> TVector3<T>::cross( TVector3<T> const& b ) const
{
	return TVector3( y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x );
}

template< class T >
FORCEINLINE TVector3<T>& TVector3<T>::operator += (const TVector3& v)
{
	x += v.x; y += v.y; z += v.z;
	return *this;
}

template< class T >
FORCEINLINE TVector3<T>& TVector3<T>::operator -= (const TVector3& v)
{
	x -= v.x; y -= v.y; z -= v.z;
	return *this;
}

template< class T >
FORCEINLINE TVector3<T>& TVector3<T>::operator *= ( ScalarInputType s )
{
	x *= s; y *= s; z *= s;
	return *this;
}

template< class T >
FORCEINLINE TVector3<T> operator + (TVector3<T> const& a,TVector3<T> const& b )
{
	return TVector3< T >(a.x+b.x,a.y+b.y,a.z+b.z);
}

template< class T >
FORCEINLINE TVector3<T> operator- (TVector3<T> const& a,TVector3<T> const& b)
{
	return TVector3< T >(a.x-b.x,a.y-b.y,a.z-b.z);
}

template< class T >
FORCEINLINE TVector3<T> operator * ( typename TVector3<T>::ScalarInputType s , TVector3<T> const& v )
{
	return TVector3< T >( s*v.x , s*v.y , s*v.z );
}

template< class T >
FORCEINLINE TVector3<T> operator*(TVector3<T> const& v, typename TVector3<T>::ScalarInputType s)
{
	return TVector3<T>( s*v.x , s*v.y , s*v.z );
}

template< class T >
FORCEINLINE TVector3<T> operator/(TVector3<T> const& v, typename TVector3<T>::ScalarInputType s)
{
	return TVector3<T>( v.x / s , v.y / s , v.z / s );
}


template< class T >
FORCEINLINE TVector3<T> operator-(TVector3<T> const& a)
{
	return TVector3<T>(-a.x,-a.y,-a.z);
}

#endif // TVector3_h__