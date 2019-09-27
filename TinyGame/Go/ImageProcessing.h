#pragma once
#ifndef ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59
#define ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59


#include "Core/Color.h"

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


template< class T >
struct TColorTraits
{
	static int GetR(T&);
	static int GetG(T&);
	static int GetB(T&);
	static int GetA(T&);
};


template< class T >
struct TColorTraits< TColor4<T> >
{
	template< class Q >
	static float GetR(TColor4<T> const& c) { return  TColorElementTraits<Q>::Normalize(c.r); }
	template< class Q >
	static float GetG(TColor4<T> const& c) { return  TColorElementTraits<Q>::Normalize(c.g); }
	template< class Q >
	static float GetB(TColor4<T> const& c) { return  TColorElementTraits<Q>::Normalize(c.b); }
	template< class Q >
	static float GetA(TColor4<T> const& c) { return  TColorElementTraits<Q>::Normalize(c.a); }
};




template< class T , class Q >
void GrayScale(TImageView< T > const& input, TImageView< Q >& output)
{
	for( int y = 0; y < input.getHeight(); ++y )
	{
		for( int x = 0; x < input.getWidth(); ++x )
		{
			T const& c = input(x, y);
			float r = TColorTraits<T>::GetR<float>(c);
			float g = TColorTraits<T>::GetG<float>(c);
			float b = TColorTraits<T>::GetB<float>(c);

			output(x, y) = 0.2126 * r + 0.7152 * g + 0.0722 * b;
		}
	}
}

void Downsample(TImageView< float > const& input, std::vector< float >& outData, TImageView<float>& outView)
{
	outData.resize((input.getWidth() / 2) * (input.getHeight() / 2));
	TImageView< float > downsampleView(outData.data(), input.getWidth() / 2, input.getHeight() / 2);

	for( int y = 0; y < downsampleView.getHeight(); ++y )
	{
		for( int x = 0; x < downsampleView.getWidth(); ++x )
		{
			downsampleView(x, y) = (input(2 * x, 2 * y) + input(2 * x + 1, 2 * y) + input(2 * x, 2 * y + 1) + input(2 * x + 1, 2 * y + 1)) / 4;
		}
	}
	outView = downsampleView;
}


float Dot3(float const* a, float const* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
float Conv2D(float const* v, int stride, float const* fliter)
{
	float result = Dot3(v, fliter);
	result += Dot3(v + stride, fliter + 3);
	result += Dot3(v + 2 * stride, fliter + 6);
	return result;
}


void Sobel(TImageView< float > const& input, TImageView< float >& output)
{
	float const Gx[] = { 1 , 0 , -1 , 2 , 0 , -2 ,  1, 0 , -1 };
	float const Gy[] = { 1 , 2 ,  1 , 0 , 0 , 0 ,  -1, -2 , -1 };
	for( int y = 0; y < input.getHeight() - 2; ++y )
	{
		for( int x = 0; x < input.getWidth() - 2; ++x )
		{
			float const* data = &input(x, y);
			float sx = Conv2D(data, input.getWidth(), Gx);
			float sy = Conv2D(data, input.getWidth(), Gy);

			float v = Math::Sqrt(sx * sx + sy * sy);

			if( v > 0.7 )
			{
				output(x + 1, y + 1) = v;
			}
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

void Normalize(TImageView< float >& input)
{
	float maxValue = 0;
	for( int y = 0; y < input.getHeight(); ++y )
	{
		for( int x = 0; x < input.getWidth(); ++x )
		{
			float value = input(x, y);
			if( value > maxValue )
				maxValue = value;
		}
	}

	for( int y = 0; y < input.getHeight(); ++y )
	{
		for( int x = 0; x < input.getWidth(); ++x )
		{
			input(x, y) /= maxValue;
		}
	}
}

void HoughLines(HoughSetting const& setting, TImageView< float > const& input , std::vector< float >& outData , TImageView<float>& outView , std::vector< HoughLine >& outLines )
{

#define USE_FAST_MATH 1

	int const HoughAngleNum = 90;
	float const HoughMaxAngle = 180;
	float const HoughAngleDelta = HoughMaxAngle / HoughAngleNum;
	float const HoughDistDelta = 1;
#if USE_FAST_MATH
	struct FFastMath
	{
		FFastMath()
		{
			for( int i = 0; i < HoughAngleNum; ++i )
			{
				float c, s;
				Math::SinCos(Math::Deg2Rad(HoughAngleDelta*i), s, c);
				mCosTable[i] = c;
				mSinTable[i] = s;
			}
		}

		float mCosTable[HoughAngleNum];
		float mSinTable[HoughAngleNum];
	};

#endif

	int const HX = HoughAngleNum;
	int HY = Math::CeilToInt( std::max( input.getWidth(), input.getHeight() ) / HoughDistDelta );
	outData.resize(HX * HY, 0);
	TImageView< float > houghView(outData.data(), HX, HY);
	float halfWidth = 0.5 * input.getWidth();
	float halfHeight = 0.5 * input.getHeight();
	int HeightSizeHalf = HY / 2;
#if USE_FAST_MATH
	static FFastMath sMath;
#endif
	for( int y = 0; y < input.getHeight(); ++y )
	{
		for( int x = 0; x < input.getWidth(); ++x )
		{
			float v = input(x, y);
			if( v > 0.2 )
			{
				for( int index = 0; index < HoughAngleNum; ++index )
				{
#if USE_FAST_MATH
					float c = sMath.mCosTable[index];
					float s = sMath.mSinTable[index];
#else
					float c, s;
					Math::SinCos(Math::Deg2Rad(index * HoughAngleDelta), s, c);
#endif
					float r = ( x - halfWidth ) * c + ( y - halfHeight ) * s;
					int ri = (int)Math::Round(r / HoughDistDelta);
					if( -HeightSizeHalf <= ri && ri < HeightSizeHalf )
					{
						houghView(index, ri + HeightSizeHalf) += 0.01;
					}
				}
			}
		}
	}

	Normalize(houghView);

	for( int y = 0; y < houghView.getHeight(); ++y )
	{
		for( int x = 0; x < houghView.getWidth(); ++x )
		{
			float v = houghView(x, y);
			if( v > setting.threshold )
			{
				struct PosFinder
				{
					PosFinder(TImageView< float >& inView)
						:houghView( inView ){ }

					float threshold = 0;
					float maxValue = 0;
					int yMax;
					int xMax;
					void operator ()(int x, int y)
					{
						float value = houghView(x, y);
						houghView(x, y) = 0;
						if ( value > maxValue )
						{
							maxValue = value;
							xMax = x;
							yMax = y;
						}

						int xT, yT;

						xT = x + 1;
						yT = y;
						if( houghView.isValidRange(xT, yT) && houghView(xT, yT) > threshold )
						{
							(*this)(xT, yT);
						}
						xT = x - 1;
						yT = y;
						if( houghView.isValidRange(xT, yT) && houghView(xT, yT) > threshold )
						{
							(*this)(xT, yT);
						}
						xT = x;
						yT = y + 1;
						if( houghView.isValidRange(xT, yT) && houghView(xT, yT) > threshold )
						{
							(*this)(xT, yT);
						}
					};
					TImageView< float >& houghView;
				};

				PosFinder finder(houghView);
				finder.threshold = setting.threshold;
				finder(x, y);

				HoughLine line;
				line.dist = (finder.yMax - HeightSizeHalf ) * HoughDistDelta;
				line.theta = HoughAngleDelta * finder.xMax;
				outLines.push_back(line);
			}
		}
	}

	outView = houghView;
}

template< class T >
void Conv(TImageView< T > const& input, TImageView< T > const& fliter, TImageView< T >& output)
{




}

#endif // ImageProcessing_H_560E20F5_C146_4AA4_B6D2_487C811E7C59
