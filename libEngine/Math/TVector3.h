#ifndef TVector3_h__
#define TVector3_h__

template< class T >
class TVector3
{
public:
	using InputType = T;
	using ScaleType = T;

	TVector3() = default;
	TVector3(TVector3 const&) = default;

	constexpr explicit TVector3(T const v[])
		:x(v[0]), y(v[1]), z(v[2])
	{

	}
	constexpr TVector3( InputType sx,InputType sy, InputType sz)
		:x(sx), y(sy), z(sz)
	{

	}

	template< class Q >
	constexpr TVector3( TVector3< Q > const& rhs )
		:x( rhs.x ),y(rhs.y),z(rhs.z){}

	void setValue( InputType sx,InputType sy,InputType sz)
	{ x=sx; y=sy; z=sz;}

	T length2() const { return dot( *this ); }

	operator T*(){ return &x; }
	operator T const*() const { return &x; }

	TVector3& operator += ( TVector3 const& v);
	TVector3& operator -= ( TVector3 const& v);
	TVector3& operator *= ( InputType s );


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
FORCEINLINE TVector3<T>& TVector3<T>::operator *= ( InputType s )
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
FORCEINLINE TVector3<T> operator * ( typename TVector3<T>::InputType s , TVector3<T> const& v )
{
	return TVector3< T >( s*v.x , s*v.y , s*v.z );
}

template< class T >
FORCEINLINE TVector3<T> operator*(TVector3<T> const& v, typename TVector3<T>::InputType s)
{
	return TVector3<T>( s*v.x , s*v.y , s*v.z );
}

template< class T >
FORCEINLINE TVector3<T> operator/(TVector3<T> const& v, typename TVector3<T>::InputType s)
{
	return TVector3<T>( v.x / s , v.y / s , v.z / s );
}


template< class T >
FORCEINLINE TVector3<T> operator-(TVector3<T> const& a)
{
	return TVector3<T>(-a.x,-a.y,-a.z);
}

#endif // TVector3_h__