#pragma once
#ifndef TVector4_H_B02B4708_650C_490F_B1C7_0401126349EF
#define TVector4_H_B02B4708_650C_490F_B1C7_0401126349EF


template< class T >
class TVector4
{
public:
	typedef T ScalarInputType;
	typedef T ScaleType;

	TVector4() = default;
	TVector4(TVector4 const&) = default;

	TVector4(T v[]) { setValue(v[0], v[1], v[2], v[3]); }
	TVector4(ScalarInputType sx, ScalarInputType sy, ScalarInputType sz, ScalarInputType sw);

	template< class Q >
	TVector4(TVector4< Q > const& rhs)
		:x(rhs.x), y(rhs.y), z(rhs.z), w(rhs.w)
	{
	}

	void setValue(ScalarInputType sx, ScalarInputType sy, ScalarInputType sz, ScalarInputType sw)
	{
		x = sx; y = sy; z = sz; w = sw;
	}

	T length2() const { return dot(*this); }

	operator T*() { return &x; }
	operator T const*() const { return &x; }

	static TVector4 Zero() { return TVector4(0, 0, 0, 0); }
	static TVector4 Fill(ScalarInputType s) { return TVector4(s, s, s, s); }

	TVector4& operator += (TVector4 const& v);
	TVector4& operator -= (TVector4 const& v);
	TVector4& operator *= (ScalarInputType s);


	TVector4 mul(TVector4 const& v)
	{
		return TVector4(x * v.x, y * v.y, z * v.z, w * v.w);
	}

	T        dot(TVector4 const& b) const;

	bool     operator == (TVector4 const& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
	bool     operator != (TVector4 const& v) const { return !this->operator == (v); }

public:
	T x, y, z, w;

private:
	void operator + (int) const;
	void operator - (int) const;
	void operator +=(int) const;
	void operator -=(int) const;
};



template< class T >
inline TVector4<T>::TVector4(ScalarInputType sx, ScalarInputType sy, ScalarInputType sz, ScalarInputType sw)
	:x(sx), y(sy), z(sz), w(sw)
{

}

template< class T >
inline T TVector4<T>::dot(TVector4 const& b) const
{
	return x*b.x + y*b.y + z*b.z + w*b.w;
}


template< class T >
inline TVector4<T>& TVector4<T>::operator += (const TVector4& v)
{
	x += v.x; y += v.y; z += v.z; w += v.w;
	return *this;
}

template< class T >
inline TVector4<T>& TVector4<T>::operator -= (const TVector4& v)
{
	x -= v.x; y -= v.y; z -= v.z; w -= v.w;
	return *this;
}

template< class T >
inline TVector4<T>& TVector4<T>::operator *= (ScalarInputType s)
{
	x *= s; y *= s; z *= s; w *= s;
	return *this;
}

template< class T >
inline TVector4<T> operator + (TVector4<T> const& a, TVector4<T> const& b)
{
	return TVector4< T >(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

template< class T >
inline TVector4<T> operator- (TVector4<T> const& a, TVector4<T> const& b)
{
	return TVector4< T >(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

template< class T >
inline TVector4<T> operator * (typename TVector4<T>::ScalarInputType s, TVector4<T> const& v)
{
	return TVector4< T >(s*v.x, s*v.y, s*v.z, s*v.w);
}

template< class T >
inline TVector4<T> operator*(TVector4<T> const& v, typename TVector4<T>::ScalarInputType s)
{
	return TVector4<T>(s*v.x, s*v.y, s*v.z, s*v.w);
}

template< class T >
inline TVector4<T> operator/(TVector4<T> const& v, typename TVector4<T>::ScalarInputType s)
{
	return TVector4<T>(v.x / s, v.y / s, v.z / s, v.w / s);
}


template< class T >
inline TVector4<T> operator-(TVector4<T> const& a)
{
	return TVector4<T>(-a.x, -a.y, -a.z, -a.w);
}

#endif // TVector4_H_B02B4708_650C_490F_B1C7_0401126349EF
