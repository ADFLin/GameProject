#pragma once
#ifndef PerlinNoise_H_204A661F_E041_467A_A923_5E706259A855
#define PerlinNoise_H_204A661F_E041_467A_A923_5E706259A855

#include "Core/IntegerType.h"

uint8 const DefaultPerm[256] =
{
	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
	140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
	247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
	57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
	74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
	60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
	65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,
	196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
	52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
	207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
	119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
	129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
	218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
	81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
	184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
	222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

template< int bRepeat = false >
class TPerlinNoise
{
public:

	TPerlinNoise()
	{
		for (int i = 0; i < 256; ++i)
		{
			mValue[i] = mValue[256 + i] = DefaultPerm[i];

			float s, c;
			Math::SinCos(2 * Math::PI * i / 256, s, c);
			mDir[i] = Vector2(c, s);
		}
	}

	template< class T >
	T getValue(T x)
	{
		auto& p = mValue;

		int xi = Math::FloorToInt(x);
		x -= xi; xi &= 0xff;
		if (bRepeat && repeat)
		{
			xi %= repeat;
		}

		T u = Fade(x);
		int a, b;
		a = p[xi];
		b = p[inc(xi)];
		return 	Lerp(u, Grad<T>(a, x),
			Grad<T>(b, x - 1));
	}

	template< class T >
	float getValue(T x, T y)
	{
		auto& p = mValue;

		int xi = Math::FloorToInt(x);
		int yi = Math::FloorToInt(y);

		x -= xi; xi &= 0xff;
		y -= yi; yi &= 0xff;

		if (bRepeat && repeat)
		{
			xi %= repeat;
			yi %= repeat;
		}

		T u = Fade(x);
		T v = Fade(y);

		int aa, ab, ba, bb;
		aa = p[p[xi] + yi];
		ba = p[p[inc(xi)] + yi];
		ab = p[p[xi] + inc(yi)];
		bb = p[p[inc(xi)] + inc(yi)];

		return Lerp(
			v,
			Lerp(u, Grad<T>(aa, x, y),
				Grad<T>(ba, x - 1, y)),
			Lerp(u, Grad<T>(ab, x, y - 1),
				Grad<T>(bb, x - 1, y - 1))
		);
	}

	template< class T >
	T getValue(T x, T y, T z)
	{
		auto& p = mValue;
		int xi = Math::FloorToInt(x);
		int yi = Math::FloorToInt(y);
		int zi = Math::FloorToInt(z);

		x -= xi; xi &= 0xff;
		y -= yi; yi &= 0xff;
		z -= zi; zi &= 0xff;


		if (bRepeat && repeat)
		{
			xi %= repeat;
			yi %= repeat;
			zi %= repeat;
		}

		T u = Fade(x);
		T v = Fade(y);
		T w = Fade(z);

		int aaa, aba, aab, abb, baa, bba, bab, bbb;
		aaa = p[p[p[xi] + yi] + zi];
		aba = p[p[p[xi] + inc(yi)] + zi];
		aab = p[p[p[xi] + yi] + inc(zi)];
		abb = p[p[p[xi] + inc(yi)] + inc(zi)];
		baa = p[p[p[inc(xi)] + yi] + zi];
		bba = p[p[p[inc(xi)] + inc(yi)] + zi];
		bab = p[p[p[inc(xi)] + yi] + inc(zi)];
		bbb = p[p[p[inc(xi)] + inc(yi)] + inc(zi)];

		return Lerp(
			w,
			Lerp(v, Lerp(u, Grad(aaa, x, y, z),
				Grad(baa, x - 1, y, z)),
				Lerp(u, Grad(aba, x, y - 1, z),
					Grad(bba, x - 1, y - 1, z))),
			Lerp(v, Lerp(u, Grad(aab, x, y, z - 1),
				Grad(bab, x - 1, y, z - 1)),
				Lerp(u, Grad(abb, x, y - 1, z - 1),
					Grad(bbb, x - 1, y - 1, z - 1)))
		);
	}

	int   repeat = 0;
	int   inc(int x) const
	{
		++x;
		if (bRepeat && repeat)
			x %= repeat;
		return x;
	}

	template < class T >
	T Grad(int hash, T x)
	{
		return Grad<T>(hash, x, 0);
	}

	template < class T >
	T Grad(int hash, T x, T y)
	{
#if 0
		auto const& dir = mDir[hash];
		return dir.dot(Vector2(x, y));
#else
		return Grad<T>(hash, x, y, 0);
#endif
	}

	template < class T >
	static T Grad(int hash, T x, T y, T z)
	{
#if 0
		int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
		T u = h < 8 ? x : y;              // INTO 12 GRADIENT DIRECTIONS.
		T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
#else
		switch (hash & 0xF)
		{
		case 0x0: return  x + y;
		case 0x1: return -x + y;
		case 0x2: return  x - y;
		case 0x3: return -x - y;
		case 0x4: return  x + z;
		case 0x5: return -x + z;
		case 0x6: return  x - z;
		case 0x7: return -x - z;
		case 0x8: return  y + z;
		case 0x9: return -y + z;
		case 0xA: return  y - z;
		case 0xB: return -y - z;
		case 0xC: return  y + x;
		case 0xD: return -y + z;
		case 0xE: return  y - x;
		case 0xF: return -y - z;
		}
		return 0;
#endif

	}
	template < class T >
	static T Fade(T x)
	{
		T x2 = x * x;
		return x2 * x * (6.0 * x2 - 15.0 * x + 10.0);
	}
	template < class T >
	static T Lerp(T t, T a, T b)
	{
		return a + t * (b - a);
	}

	uint32  mValue[512];
	Vector2 mDir[256];

};

#endif // PerlinNoise_H_204A661F_E041_467A_A923_5E706259A855