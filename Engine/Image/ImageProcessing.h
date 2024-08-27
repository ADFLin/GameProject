#pragma once
#ifndef ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59
#define ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59

#include "Core/Color.h"
#include "Math/TVector2.h"
#include "DataStructure/Array.h"

template< class T >
class TImageView
{
public:
	TImageView()
		:mSize(TVector2<int>::Zero())
	{
		mStride = mSize.x;
		mData = nullptr;
	}
	TImageView(T* data, int w, int h)
	{
		mData = data;
		mStride = w;
		mSize.x = w;
		mSize.y = h;
	}

	TImageView(TImageView&& rhs)
		:mData(rhs.mData)
		,mSize(rhs.mSize)
		,mStride(rhs.mStride)
	{
		rhs.mData = nullptr;
		rhs.mSize = TVector2<int>::Zero();
		rhs.mStride = 0;
	}
	TImageView(TImageView const& rhs) = default;
	TImageView& operator = (TImageView const& rhs) = default;

	bool isValidRange(int x, int y) const
	{
		return 0 <= x && x < mSize.x && 0 <= y && y < mSize.y;
	}
	T*  getData() { return mData; }
	T&  operator()( int x, int y )
	{
		assert(isValidRange(x, y));
		return mData[y * mStride + x];
	}
	T const&  operator()(int x, int y) const
	{
		assert(isValidRange(x, y));
		return mData[y * mStride + x];
	}

	int getWidth() const { return mSize.x; }
	int getHeight() const { return mSize.y; }
	TVector2<int> getSize() const { return mSize; }

	TVector2<float> getPixelCenterToUV( TVector2<int> const& pos) const 
	{ 
		TVector2< float > temp = TVector2<float>(pos) + TVector2<float>(0.5f, 0.5f);
		return temp.div( mSize );
	}

	TVector2<int>   getPixelPos(TVector2<float> const& uv) const
	{
		TVector2< float > temp = uv.mul( mSize );
		TVector2<int> result;
		result.x = Math::FloorToInt(temp.x);
		result.y = Math::FloorToInt(temp.y);
		return  result;
	}
	TVector2<int> mSize;
	int mStride;
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

			output(x, y) = TColorElementTraits<Q>::Normalize(float(0.2126 * r + 0.7152 * g + 0.0722 * b));
		}
	}
}


void Downsample(TImageView< float > const& input, TArray< float >& outData, TImageView<float>& outView);


FORCEINLINE float Dot3(float const* RESTRICT a, float const* RESTRICT b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

FORCEINLINE float Conv2D(float const* RESTRICT v, int stride, float const* RESTRICT fliter)
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
	for (int y = 0; y < inoutput.getHeight(); ++y)
	{
		for (int x = 0; x < inoutput.getWidth(); ++x)
		{
			T& value = inoutput(x, y);
			func(value);
		}
	}
}
template< class T , class T2  >
void Fill(TImageView<T>& inoutput, TVector2<int> const& pos, TVector2<int> const& size , T2 const& value)
{
	TVector2<int> end = pos + size;
	for (int y = pos.y; y < end.y; ++y)
	{
		for (int x = pos.x; x < end.x; ++x)
		{
			inoutput(x, y) = value;
		}
	}
}
template< class T >
T Max(TImageView<T> const& input)
{
	T result = 0;
	for (int y = 0; y < input.getHeight(); ++y)
	{
		for (int x = 0; x < input.getWidth(); ++x)
		{
			T value = input(x, y);
			if (value > result)
				result = value;
		}
	}
	return result;
}

struct HoughLine
{
	float dist;
	float theta;
	int   removeCount;
	float maxValue;
};

struct HoughSetting 
{
	float threshold = 0.60f;
};



void Normalize(TImageView< float >& input);

void HoughLines(HoughSetting const& setting, TImageView< float > const& input , TArray< float >& outData , TImageView<float>& outView , TArray< HoughLine >& outLines , TArray<float>* outDebugData = nullptr );

template< class T >
void Conv(TImageView< T > const& input, TImageView< T > const& fliter, TImageView< T >& output)
{




}

#endif // ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59
