#include "Bloom.h"

namespace Render
{

	IMPLEMENT_SHADER_PROGRAM(DownsampleProgram);
	IMPLEMENT_SHADER_PROGRAM(BloomSetupProgram);
	IMPLEMENT_SHADER_PROGRAM(FliterProgram);
	IMPLEMENT_SHADER_PROGRAM(FliterAddProgram);
	IMPLEMENT_SHADER_PROGRAM(TonemapProgram);


	int generateGaussianlDisburtionWeightAndOffset(float kernelRadius, Vector2 outWeightAndOffset[128])
	{
		float fliterScale = 1.0f;
		float filterScale = 1.0f;
		float sigmaScale = 0.2;

		int   sampleRadius = Math::CeilToInt(fliterScale * kernelRadius);
		sampleRadius = Math::Clamp(sampleRadius, 1, MaxWeightNum - 1);

		float sigma = kernelRadius * sigmaScale;
		auto GetWeight = [=](float x)
		{
			return Math::Exp(-Math::Squre(x / sigma));
		};

		int numSamples = 0;
		float totalSampleWeight = 0;
		for (int sampleIndex = -sampleRadius; sampleIndex <= sampleRadius; sampleIndex += 2)
		{
			float weightA = GetWeight(sampleIndex);


			float weightB = 0;

			if (sampleIndex != sampleRadius)
			{
				weightB = GetWeight(sampleIndex + 1);
			}
			float weightTotal = weightA + weightB;
			float offset = Math::Lerp(sampleIndex, sampleIndex + 1, weightB / weightTotal);

			outWeightAndOffset[numSamples].x = weightTotal;
			outWeightAndOffset[numSamples].y = offset;
			totalSampleWeight += weightTotal;
			++numSamples;
		}


		float invTotalSampleWeight = 1.0 / totalSampleWeight;
		for (int i = 0; i < numSamples; ++i)
		{
			outWeightAndOffset[i].x *= invTotalSampleWeight;
		}
		return numSamples;
	}

}//namespace Render