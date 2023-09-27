#pragma once
#ifndef FFT_H_9ABF3973_71C0_4E29_9578_E20AF2269494
#define FFT_H_9ABF3973_71C0_4E29_9578_E20AF2269494

#include "Core/IntegerType.h"
#include "DataStructure/Array.h"
#include "Math/SIMD.h"

#define FFT_METHOD 1
#define FFT_USE_SIMD 0

struct Complex 
#if FFT_USE_SIMD
	: SIMD::SCompolex
#endif
{
#if FFT_USE_SIMD
	using SIMD::SCompolex::SCompolex;
#else
	Complex() = default;
	FORCEINLINE explicit Complex(float inR, float inI = 0) :r(inR), i(inI) {}

	float r;
	float i;
	FORCEINLINE float length() const { return Math::Sqrt(r * r + i * i); }

	FORCEINLINE Complex operator + (Complex const& rhs) const
	{
		return Complex{ r + rhs.r , i + rhs.i };
	}
	FORCEINLINE Complex operator - (Complex const& rhs) const
	{
		return Complex{ r - rhs.r , i - rhs.i };
	}
	FORCEINLINE Complex operator * (Complex const& rhs) const
	{
		return Complex{ r * rhs.r - i * rhs.i , r * rhs.i + i * rhs.r };
	}
	FORCEINLINE Complex& operator += (Complex const& rhs)
	{
		r += rhs.r;
		i += rhs.i;
		return *this;
	}
	FORCEINLINE friend Complex operator * (float s, Complex const& rhs)
	{
		return Complex(s * rhs.r, s * rhs.i);
	}

#endif
	FORCEINLINE static Complex Expi(float value)
	{
		float c, s;
		Math::SinCos(value, s, c);
		return Complex{ c , s };
	}
};


class FFT
{
public:


	struct Context
	{
		TArray< Complex > twiddles;
#if FFT_METHOD == 1
		TArray< uint32 >  reverseIndices;
#endif
		int sampleSize = 0;

		void set(int inSampleSize);

		Context(int inSampleSize) { set(inSampleSize); }
		Context() {}
	};

	//      N-1
	// Xk = sun[ xn exp( -i 2pi kn / N ) ]
	//      n=0 
	static void Transform(float data[], int numData, Complex outData[]);
	static void Transform(Context& context, float data[], int numData, Complex outData[]);
	static void TransformKiss(float data[], int numData, Complex outData[]);

	static void TransformIter(Context& context, Complex outData[]);
	static void Transform2_R(Context& context, Complex data[], int numData, Complex outData[], int stride);
	static void Transform_G(Context& context, float data[], Complex outData[]);
	static bool IsPowOf2(int num);

	static void PrintIndex(int numData);
	static void PrintIndex(int numTotalData, int step);
	static void PrintIndex2(int numTotalData, int indexData, int step, int showStep);
};


#endif // FFT_H_9ABF3973_71C0_4E29_9578_E20AF2269494
