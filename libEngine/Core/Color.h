#pragma once
#ifndef Color_H_1E8ED4FF_1757_42A2_A7E6_7284644A5164
#define Color_H_1E8ED4FF_1757_42A2_A7E6_7284644A5164

#include "IntegerType.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

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
struct TColorTypeTraits
{

};

template <>
struct TColorTypeTraits< float >
{
	static float Min() { return 0.0f; }
	static float Max() { return 1.0f; }
	static float Normlize(uint8 value)
	{
		return float(value) / 255.0f;
	}

};
template <>
struct TColorTypeTraits< uint8 >
{
	static uint8 Min() { return 0x00; }
	static uint8 Max() { return 0xff; }
	static uint8 Normalize(float value)
	{
		return uint8(255 * value);
	}
};


template< class T >
class TColor3
{
public:
	using Vector3 = Math::Vector3;
	
	typedef TColorTypeTraits< T > ColorTypeTraints;

	T  r, g, b;

	TColor3() {}
	TColor3(T cr, T cg, T cb)
		:r(cr), g(cg), b(cb)
	{
	}
	TColor3(T const* v)
		:r(v[0]), g(v[1]), b(v[2])
	{
	}
	TColor3(Vector3 const& v)
		:r(v.x), g(v.y), b(v.z)
	{
	}

	TColor3& operator = (TColor3 const& rhs) = default;

	template< class Q >
	TColor3(TColor3<Q> const& c)
		: r(ColorTypeTraints::Normlize(c.r))
		: g(ColorTypeTraints::Normlize(c.g))
		: b(ColorTypeTraints::Normlize(c.b))
	{
	}

	template< class Q >
	TColor3& operator = (TColor3< Q > const& rhs)
	{
		r = ColorTypeTraints::Normlize(rhs.r);
		g = ColorTypeTraints::Normlize(rhs.g);
		b = ColorTypeTraints::Normlize(rhs.b);
		return *this;
	}

	template< class T >
	operator T const*() const { return &r; }
	operator T*      () { return &r; }

	uint32 toXRGB() const { return FColor::ToXRGB(r, g, b); }
	uint32 toXBGR() const { return FColor::ToXBGR(r, g, b); }
};

template< class T >
class TColor4 : public TColor3< T >
{
public:
	using Vector4 = Math::Vector4;

	T a;

	TColor4() {}
	TColor4(T cr, T cg, T cb, T ca = ColorTypeTraints::Max()) :TColor3<T>(cr, cg, cb), a(ca) {}
	TColor4(TColor3<T> const& rh) :TColor3<T>(rh), a(ColorTypeTraints::Max()) {}
	TColor4(T const* v) :TColor3<T>(v), a(v[3]) {}
	TColor4(Vector3 const& v) :TColor3<T>(v) , a(ColorTypeTraints::Max()){}
	TColor4(Vector4 const& v) :TColor3<T>(v), a(v.w) {}

	TColor4& operator = (TColor3< T > const& rhs)
	{
		r = rhs.r;
		g = rhs.g;
		b = rhs.b;
		a = ColorTypeTraints::Max();
		return *this;
	}

	operator T const*() const { return &r; }
	operator T*      () { return &r; }
	uint32 toARGB() const { return FColor::ToARGB(r, g, b, a); }
	uint32 toRGBA() const { return FColor::ToRGBA(r, g, b, a); }	
};

#if 0

class Color3ub : public TColor3< uint8 >
{
public:
	Color3ub() {}
	Color3ub(uint8 cr, uint8 cg, uint8 cb)
		:Color3T< uint8 >(cr, cg, cb) {}

};

class Color3f : public TColor3< float >
{
public:
	Color3f() {}
	Color3f(float cr, float cg, float cb)
		:Color3T< float >(cr, cg, cb) {}
};



class Color4ub : public TColor4< uint8 >
{
public:
	Color4ub() {}
	Color4ub(uint8 cr, uint8 cg, uint8 cb, uint8 ca = 0xff)
		:TColor4< uint8 >(cr, cg, cb, ca) {}
};

class Color4f : public TColor4< float >
{
public:
	Color4f() {}
	Color4f(float cr, float cg, float cb, float ca = 1.0f)
		:TColor4< float >(cr, cg, cb, ca) {}
};
#else

typedef TColor4< uint8 > Color4ub;
typedef TColor3< uint8 > Color3ub;
typedef TColor4< float > Color4f;
typedef TColor3< float > Color3f;


#endif

typedef Color4f   LinearColor;


class FColorConv
{
public:
	using Vector3 = Math::Vector3;

	static Vector3 HSVToRGB(Vector3 hsv);
};


#endif // Color_H_1E8ED4FF_1757_42A2_A7E6_7284644A5164