#include "ImageProcessing.h"



void Downsample(TImageView< float > const& input, std::vector< float >& outData, TImageView<float>& outView)
{
	outData.resize((input.getWidth() / 2) * (input.getHeight() / 2));
	TImageView< float > downsampleView(outData.data(), input.getWidth() / 2, input.getHeight() / 2);

	for (int y = 0; y < downsampleView.getHeight(); ++y)
	{
		for (int x = 0; x < downsampleView.getWidth(); ++x)
		{
			downsampleView(x, y) = (input(2 * x, 2 * y) + input(2 * x + 1, 2 * y) + input(2 * x, 2 * y + 1) + input(2 * x + 1, 2 * y + 1)) / 4;
		}
	}
	outView = downsampleView;
}

void Sobel(TImageView< float > const& input, TImageView< float >& output)
{
	float const Gx[] = { 1 , 0 , -1 , 2 , 0 , -2 ,  1, 0 , -1 };
	float const Gy[] = { 1 , 2 ,  1 , 0 , 0 , 0 ,  -1, -2 , -1 };
	for (int y = 0; y < input.getHeight() - 2; ++y)
	{
		for (int x = 0; x < input.getWidth() - 2; ++x)
		{
			float const* data = &input(x, y);
			float sx = Conv2D(data, input.getWidth(), Gx);
			float sy = Conv2D(data, input.getWidth(), Gy);

			float vSqure = sx * sx + sy * sy;

			if (vSqure > 0.7 * 0.7)
			{
				output(x + 1, y + 1) = Math::Sqrt(vSqure);
			}
		}
	}
}

void Normalize(TImageView< float >& input)
{
	float maxValue = 0;
	for (int y = 0; y < input.getHeight(); ++y)
	{
		for (int x = 0; x < input.getWidth(); ++x)
		{
			float value = input(x, y);
			if (value > maxValue)
				maxValue = value;
		}
	}

	for (int y = 0; y < input.getHeight(); ++y)
	{
		for (int x = 0; x < input.getWidth(); ++x)
		{
			input(x, y) /= maxValue;
		}
	}
}

#define USE_FAST_MATH 1
#if USE_FAST_MATH
int const HoughAngleNum = 90;
float const HoughMaxAngle = 180;
float const HoughAngleDelta = HoughMaxAngle / HoughAngleNum;
float const HoughDistDelta = 1;
struct FFastMath
{
	FFastMath()
	{
		for (int i = 0; i < HoughAngleNum; ++i)
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

void HoughLines(HoughSetting const& setting, TImageView< float > const& input, std::vector< float >& outData, TImageView<float>& outView, std::vector< HoughLine >& outLines, std::vector<float>* outDebugData)
{

	int const HX = HoughAngleNum;
	int HY = Math::CeilToInt(Math::Max(input.getWidth(), input.getHeight()) / HoughDistDelta);
	outData.resize(HX * HY, 0);
	TImageView< float > houghView(outData.data(), HX, HY);
	float halfWidth = 0.5 * input.getWidth();
	float halfHeight = 0.5 * input.getHeight();
	int HeightSizeHalf = HY / 2;
#if USE_FAST_MATH
	static FFastMath sMath;
#endif
	float const offsetX = 0.5f - halfWidth;
	float const offsetY = 0.5f - halfHeight;
	for (int y = 0; y < input.getHeight(); ++y)
	{
		for (int x = 0; x < input.getWidth(); ++x)
		{
			float v = input(x, y);
			if (v > 0.2)
			{
				for (int index = 0; index < HoughAngleNum; ++index)
				{
#if USE_FAST_MATH
					float c = sMath.mCosTable[index];
					float s = sMath.mSinTable[index];
#else
					float c, s;
					Math::SinCos(Math::Deg2Rad(index * HoughAngleDelta), s, c);
#endif
					float r = (x + offsetX) * c + (y + offsetY) * s;
					int ri = (int)Math::Round(r / HoughDistDelta);
					if (-HeightSizeHalf <= ri && ri < HeightSizeHalf)
					{
						houghView(index, ri + HeightSizeHalf) += 0.01;
					}
				}
			}
		}
	}

	Normalize(houghView);

	if (outDebugData)
	{
		*outDebugData = outData;
	}

	for (int y = 0; y < houghView.getHeight(); ++y)
	{
		for (int x = 0; x < houghView.getWidth(); ++x)
		{
			float v = houghView(x, y);
			if (v > setting.threshold)
			{
				struct PosFinder
				{
					PosFinder(TImageView< float >& inView)
						:houghView(inView) { }

					float threshold = 0;
					float maxValue = 0;
					int   removeCount = 0;
					int yMax;
					int xMax;
					void operator ()(int x, int y)
					{
						float value = houghView(x, y);
						++removeCount;
						houghView(x, y) = 0;
						if (value > maxValue)
						{
							maxValue = value;
							xMax = x;
							yMax = y;
						}

						int xT, yT;

						xT = x + 1;
						yT = y;
						if (houghView.isValidRange(xT, yT) && houghView(xT, yT) > threshold)
						{
							(*this)(xT, yT);
						}
						xT = x - 1;
						yT = y;
						if (houghView.isValidRange(xT, yT) && houghView(xT, yT) > threshold)
						{
							(*this)(xT, yT);
						}
						xT = x;
						yT = y + 1;
						if (houghView.isValidRange(xT, yT) && houghView(xT, yT) > threshold)
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
				line.dist = (finder.yMax - HeightSizeHalf) * HoughDistDelta;
				line.theta = HoughAngleDelta * finder.xMax;
				line.removeCount = finder.removeCount;
				line.maxValue = finder.maxValue;
				outLines.push_back(line);
			}
		}
	}

	outView = houghView;
}
