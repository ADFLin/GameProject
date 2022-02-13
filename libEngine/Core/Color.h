#pragma once
#ifndef Color_H_1E8ED4FF_1757_42A2_A7E6_7284644A5164
#define Color_H_1E8ED4FF_1757_42A2_A7E6_7284644A5164

#include "IntegerType.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

class FColor
{
public:
	static uint32 ToARGB(float r, float g, float b, float a = 1.0)
	{
		return ToARGB(uint8(255 * r), uint8(255 * g), uint8(255 * b), uint8(255 * a));
	}
	static uint32 ToRGBA(float r, float g, float b, float a = 1.0)
	{
		return ToRGBA(uint8(255 * r), uint8(255 * g), uint8(255 * b), uint8(255 * a));
	}
	static uint32 ToXRGB(float r, float g, float b)
	{
		return ToXRGB(uint8(255 * r), uint8(255 * g), uint8(255 * b));
	}

	static uint32 ToARGB(uint8 r, uint8 g, uint8 b, uint8 a)
	{
		return (a << 24) | (r << 16) | (g << 8) | b;
	}
	static uint32 ToXRGB(uint8 r, uint8 g, uint8 b)
	{
		return (r << 16) | (g << 8) | b;
	}

	static uint32 ToXBGR(uint8 r, uint8 g, uint8 b)
	{
		return (b << 16) | (g << 8) | r;
	}

	static uint32 ToRGBA(uint8 r, uint8 g, uint8 b, uint8 a)
	{
		return (r << 24) | (g << 16) | (b << 8) | a;
	}
};


template < class T >
struct TColorElementTraits
{

};

template <>
struct TColorElementTraits< float >
{
	static float Min() { return 0.0f; }
	static float Max() { return 1.0f; }
	static float Normalize(uint8 value)
	{
		return float(value) / 255.0f;
	}
	static float Normalize(float value)
	{
		return value;
	}

};
template <>
struct TColorElementTraits< uint8 >
{
	static uint8 Min() { return 0x00; }
	static uint8 Max() { return 0xff; }
	static uint8 Normalize(uint8 value)
	{
		return value;
	}
	static uint8 Normalize(float value)
	{
		return uint8(255 * value);
	}
};


template< class T >
class TColor3
{
public:
	using CET = TColorElementTraits< T >;

	T  r, g, b;

	TColor3() = default;
	TColor3(T cr, T cg, T cb)
		:r(cr), g(cg), b(cb)
	{
	}
	TColor3(T const* v)
		:r(v[0]), g(v[1]), b(v[2])
	{
	}

	TColor3& operator = (TColor3 const& rhs) = default;

	template< class Q >
	TColor3(TColor3<Q> const& c)
		: r(CET::Normalize(c.r))
		, g(CET::Normalize(c.g))
		, b(CET::Normalize(c.b))
	{
	}

	template< class Q >
	TColor3& operator = (TColor3< Q > const& rhs)
	{
		r = CET::Normalize(rhs.r);
		g = CET::Normalize(rhs.g);
		b = CET::Normalize(rhs.b);
		return *this;
	}

	operator T const*() const { return &r; }
	operator T*      () { return &r; }

	uint32 toXRGB() const { return FColor::ToXRGB(r, g, b); }
	uint32 toXBGR() const { return FColor::ToXBGR(r, g, b); }
};

template< class T >
class TColor4 : public TColor3< T >
{
public:
	using TColor3<T>::r;
	using TColor3<T>::g;
	using TColor3<T>::b;
	T a;

	using CET = typename TColor3<T>::CET;

	TColor4() = default;
	TColor4(T cr, T cg, T cb, T ca = CET::Max()) :TColor3<T>(cr, cg, cb), a(ca) {}
	TColor4(TColor3<T> const& rh , T ca = CET::Max()) :TColor3<T>(rh), a(ca) {}

	//template< class Q >
	//TColor4(TColor3<Q> const& rh, Q ca = TColorElementTraits<Q>::Max()) :TColor3<T>(rh), a(ca) {}

	TColor4(T const* v) :TColor3<T>(v), a(v[3]) {}

	TColor4& operator = (TColor3< T > const& rhs)
	{
		r = rhs.r;
		g = rhs.g;
		b = rhs.b;
		a = CET::Max();
		return *this;
	}

	template< class Q >
	TColor4(TColor4<Q> const& c)
		: TColor3<T>(CET::Normalize(c.r), CET::Normalize(c.g), CET::Normalize(c.b))
		, a(CET::Normalize(c.a))
	{
	}

	template< class Q >
	TColor4& operator = (TColor4< Q > const& rhs)
	{
		r = CET::Normalize(rhs.r);
		g = CET::Normalize(rhs.g);
		b = CET::Normalize(rhs.b);
		a = CET::Normalize(rhs.a);
		return *this;
	}

	TColor4 bgra() const { return TColor4(b, g, r, a); }
	TColor3<T> rgb() const { return TColor3<T>(r, g, b); }
	operator T const*() const { return &r; }
	operator T*      () { return &r; }
	uint32 toARGB() const { return FColor::ToARGB(r, g, b, a); }
	uint32 toRGBA() const { return FColor::ToRGBA(r, g, b, a); }
};


template<class T , class CompType >
class ColorEnumT
{
	using CET = TColorElementTraits<CompType>;
public:
	static T Black() { return T( 0, 0, 0 ); }
	static T White() { return T( CET::Max(), CET::Max(), CET::Max()); }

};

class Color3ub : public TColor3< uint8 > 
	           , public ColorEnumT<Color3ub , uint8 >
{
public:
	using TColor3<uint8>::TColor3;

	Color3ub() = default;
	Color3ub(uint8 cr, uint8 cg, uint8 cb)
		:TColor3< uint8 >(cr, cg, cb) {}

};



class Color3f : public TColor3< float >
	          , public ColorEnumT<Color3f , float >
{
	using Vector3 = Math::Vector3;
public:
	using TColor3<float>::TColor3;

	Color3f() = default;
	Color3f(float cr, float cg, float cb)
		:TColor3< float >(cr, cg, cb) {}

	operator Vector3() const { return Vector3(r, g, b); }
};



class Color4ub : public TColor4< uint8 >
	           , public ColorEnumT<Color4ub, uint8>
{
public:
	using TColor4<uint8>::TColor4;

	Color4ub() = default;
	Color4ub(uint8 cr, uint8 cg, uint8 cb, uint8 ca = 0xff)
		:TColor4< uint8 >(cr, cg, cb, ca) {}
};

class Color4f : public TColor4< float >
	          , public ColorEnumT<Color4f , float>
{
	using Vector3 = Math::Vector3;
	using Vector4 = Math::Vector4;
	using CET = TColorElementTraits<float>;
public:
	using TColor4<float>::TColor4;
	using TColor4<float>::r;
	using TColor4<float>::g;
	using TColor4<float>::b;
	using TColor4<float>::a;

	Color4f() = default;

	Color4f(Vector3 const& v) :TColor4<float>(v.x, v.y, v.z, CET::Max()) {}
	Color4f(Vector4 const& v) :TColor4<float>(v.x, v.y, v.z, v.w) {}

	operator Vector4() const { return Vector4(r, g, b, a); }

	friend Color4f operator * (float f, Color4f const& c) { return Color4f(f * c.r, f * c.g, f * c.b, f * c.a); }
	friend Color4f operator - (Color4f const& lhs, Color4f const& rhs) { return Color4f(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a); }

};


using LinearColor = Color4f;


class FColorConv
{
public:
	using Vector3 = Math::Vector3;
	static Color3f HSVToRGB(Vector3 hsv);
	static Vector3 RGBToHSV(Color3f rgb);
};


#endif // Color_H_1E8ED4FF_1757_42A2_A7E6_7284644A5164