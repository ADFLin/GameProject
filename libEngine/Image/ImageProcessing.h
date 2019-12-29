#pragma once
#ifndef ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59
#define ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59

#include "Core/Color.h"

#include <vector>

template< class T >
class TImageView
{
public:
	TImageView()
	{
		mWidth = 0;
		mHeight = 0;
		mData = nullptr;
	}
	TImageView(T* data, int w, int h)
	{
		mData = data;
		mWidth = w;
		mHeight = h;
	}

	bool isValidRange(int x, int y)
	{
		return 0 <= x && x < mWidth && 0 <= y && y < mHeight;
	}
	T*  getData() { return mData; }
	T&  operator()( int x, int y )
	{
		return mData[y * mWidth + x];
	}
	T const&  operator()(int x, int y) const
	{
		return mData[y * mWidth + x];
	}
	int getWidth() const { return mWidth; }
	int getHeight() const { return mHeight; }

	int mWidth;
	int mHeight;
	T*  mData;
};



struct FColorTraits
{

	template< class Q, class T >
	static auto GetA(TColor4<T> const& c) { return TColorElementTraits<Q>::Normalize(c.a); }

	template< class Q, class T >
	static auto GetR(TColor3<T> const& c) { return TColorElementTraits<Q>::Normalize(c.r); }
	template< class Q, class T >
	static auto GetG(TColor3<T> const& c) { return TColorElementTraits<Q>::Normalize(c.g); }
	template< class Q, class T >
	static auto GetB(TColor3<T> const& c) { return TColorElementTraits<Q>::Normalize(c.b); }

};

template< class T , class Q >
void GrayScale(TImageView< T > const& input, TImageView< Q >& output)
{
	for( int y = 0; y < input.getHeight(); ++y )
	{
		for( int x = 0; x < input.getWidth(); ++x )
		{
			T const& c = input(x, y);
			float r = FColorTraits::GetR<float>(c);
			float g = FColorTraits::GetG<float>(c);
			float b = FColorTraits::GetB<float>(c);

			output(x, y) = 0.2126 * r + 0.7152 * g + 0.0722 * b;
		}
	}
}

void Downsample(TImageView< float > const& input, std::vector< float >& outData, TImageView<float>& outView);


FORCEINLINE float Dot3(float const* a, float const* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

FORCEINLINE float Conv2D(float const* v, int stride, float const* fliter)
{
	float result = Dot3(v, fliter);
	result += Dot3(v + stride, fliter + 3);
	result += Dot3(v + 2 * stride, fliter + 6);
	return result;
}


void Sobel(TImageView< float > const& input, TImageView< float >& output);

template< class T , class TFunc >
void Transform(TImageView< T >& inoutput , TFunc&& func = TFunc() )
{
	for (int y = 0; y < input.getHeight(); ++y)
	{
		for (int x = 0; x < input.getWidth(); ++x)
		{
			T& value = inoutput(x, y);
			value = func(value);
		}
	}
}

struct HoughLine
{
	float dist;
	float theta;
};

struct HoughSetting 
{
	float threshold = 0.60;
};

void Normalize(TImageView< float >& input);

void HoughLines(HoughSetting const& setting, TImageView< float > const& input , std::vector< float >& outData , TImageView<float>& outView , std::vector< HoughLine >& outLines );

template< class T >
void Conv(TImageView< T > const& input, TImageView< T > const& fliter, TImageView< T >& output)
{




}

#endif // ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59
