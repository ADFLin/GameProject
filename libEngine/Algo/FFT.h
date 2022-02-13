#pragma once
#ifndef FFT_H_9ABF3973_71C0_4E29_9578_E20AF2269494
#define FFT_H_9ABF3973_71C0_4E29_9578_E20AF2269494

#include "Core/IntegerType.h"
#include "Math/SIMD.h"

#if USE_MATH_SIMD
struct Complex : SIMD::SCompolex
{
	using SIMD::SCompolex::SCompolex;
public:

#else
struct Complex
{
	Complex() {}
	explicit Complex(float inR, float inI = 0) :r(inR), i(inI) {}

	float r;
	float i;
	float  length() const { return Math::Sqrt(r * r + i * i); }

	Complex operator + (Complex const& rhs) const
	{
		return Complex{ r + rhs.r , i + rhs.i };
	}
	Complex operator - (Complex const& rhs) const
	{
		return Complex{ r - rhs.r , i - rhs.i };
	}
	Complex operator * (Complex const& rhs) const
	{
		return Complex{ r * rhs.r - i * rhs.i , r * rhs.i + i * rhs.r };
	}
#endif
	static Complex Expi(float value)
	{
		float c, s;
		Math::SinCos(value, s, c);
		return Complex{ c , s };
	}
};


#define FFT_METHOD 1

class FFT
{
public:

	static bool IsPowOf2(int num)
	{
		return (num & (num - 1)) == 0;
	}


	struct Context
	{
		std::vector< Complex > factors;
#if FFT_METHOD == 1
		std::vector< uint32 >  reverseIndices;
#endif
		int sampleSize;
	};

	//      N-1
	// Xk = sun[ xn exp( -i 2pi kn / N ) ]
	//      n=0 
	static void Transform(float data[], int numData, Complex outData[]);

	static void _fft(Complex buf[], Complex out[], int n, int step)
	{
		if( step < n )
		{
			_fft(out, buf, n, step * 2);
			_fft(out + step, buf + step, n, step * 2);

			for( int i = 0; i < n; i += 2 * step )
			{
				Complex t = Complex::Expi(-Math::PI * i / n) * out[i + step];
				buf[i / 2] = out[i] + t;
				buf[(i + n) / 2] = out[i] - t;
			}
		}
	}

	static void Transform2_RA(Context& context, int numData, Complex outData[], uint stride);
	static void Transform2_R(Context& context, Complex data[], int numData, Complex outData[], uint stride);
	static void Transform_G(float data[], int numData, Complex outData[]);
};


#endif // FFT_H_9ABF3973_71C0_4E29_9578_E20AF2269494
