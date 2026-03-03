#include "Bloom.h"

#include "RHI/ShaderManager.h"
#include "RHI/GpuProfiler.h"

namespace Render
{

	IMPLEMENT_SHADER_PROGRAM(DownsampleProgram);
	IMPLEMENT_SHADER_PROGRAM(BloomSetupProgram);
	IMPLEMENT_SHADER_PROGRAM(FliterProgram);
	IMPLEMENT_SHADER_PROGRAM(FliterAddProgram);


	int generateGaussianlDisburtionWeightAndOffset(float kernelRadius, Vector2 outWeightAndOffset[128])
	{
		float fliterScale = 1.0f;
		float sigmaScale = 0.2f;

		int   sampleRadius = Math::CeilToInt(fliterScale * kernelRadius);
		sampleRadius = Math::Clamp(sampleRadius, 1, MaxWeightNum - 1);

		float sigma = kernelRadius * sigmaScale;
		auto GetWeight = [=](float x)
		{
			return Math::Exp(-Math::Square(x / sigma));
		};

		int numSamples = 0;
		float totalSampleWeight = 0;
		for (int sampleIndex = -sampleRadius; sampleIndex <= sampleRadius; sampleIndex += 2)
		{
			float weightA = GetWeight(float(sampleIndex));
			float weightB = 0;

			if (sampleIndex != sampleRadius)
			{
				weightB = GetWeight(float(sampleIndex + 1));
			}
			float weightTotal = weightA + weightB;
			float offset = Math::Lerp(float(sampleIndex), float(sampleIndex + 1), weightB / weightTotal);

			outWeightAndOffset[numSamples].x = weightTotal;
			outWeightAndOffset[numSamples].y = offset;
			totalSampleWeight += weightTotal;
			++numSamples;
		}


		float invTotalSampleWeight = 1.0f / totalSampleWeight;
		for (int i = 0; i < numSamples; ++i)
		{
			outWeightAndOffset[i].x *= invTotalSampleWeight;
		}
		return numSamples;
	}

	RHITexture2DRef FBloom::Render(RHICommandList& commandList, RHITexture2D& sceneTexture, RHIFrameBuffer& bloomFrameBuffer, BloomConfig const& config)
	{
		GPU_PROFILE("Bloom");

		auto DownsampleProg = ShaderManager::Get().getGlobalShaderT<DownsampleProgram>();
		auto BloomSetupProg = ShaderManager::Get().getGlobalShaderT<BloomSetupProgram>();
		auto FliterProg = ShaderManager::Get().getGlobalShaderT<FliterProgram>();
		auto FliterAddProg = ShaderManager::Get().getGlobalShaderT<FliterAddProgram>();

		RHITexture2DRef downsampleTextures[6];

		auto DownsamplePass = [&](RHICommandList& commandList, int index, RHITexture2D& sourceTexture) -> RHITexture2DRef
		{
			RenderTargetDesc desc;
			InlineString<128> str;
			str.format("Downsample(%d)", index);

			IntVector2 size;
			size.x = (sourceTexture.getSizeX() + 1) / 2;
			size.y = (sourceTexture.getSizeY() + 1) / 2;
			desc.debugName = str;
			desc.size = size;
			desc.format = ETexture::FloatRGBA;
			desc.creationFlags = TCF_CreateSRV;
			RHITexture2DRef downsampleTexture = static_cast<RHITexture2D*>(GRenderTargetPool.fetchElement(desc)->texture.get());
			bloomFrameBuffer.setTexture(0, *downsampleTexture);
			RHISetViewport(commandList, 0, 0, size.x, size.y);
			RHISetFrameBuffer(commandList, &bloomFrameBuffer);
			RHISetShaderProgram(commandList, DownsampleProg->getRHI());
			auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *DownsampleProg, Texture, sourceTexture, sampler);
			SET_SHADER_PARAM(commandList, *DownsampleProg, ExtentInverse, Vector2(1.0f / float(size.x), 1.0f / float(size.y)));

			DrawUtility::ScreenRect(commandList);
			return downsampleTexture;
		};

		RHITexture2DRef halfSceneTexture = DownsamplePass(commandList, 0, sceneTexture);

		{
			RenderTargetDesc desc;
			IntVector2 size = IntVector2(halfSceneTexture->getSizeX(), halfSceneTexture->getSizeY());
			desc.debugName = "BloomSetup";
			desc.size = size;
			desc.format = ETexture::FloatRGBA;
			desc.creationFlags = TCF_CreateSRV;

			PooledRenderTargetRef bloomSetupRT = GRenderTargetPool.fetchElement(desc);

			bloomFrameBuffer.setTexture(0, *bloomSetupRT->texture);
			RHISetViewport(commandList, 0, 0, size.x, size.y);
			RHISetFrameBuffer(commandList, &bloomFrameBuffer);
			RHISetShaderProgram(commandList, BloomSetupProg->getRHI());

			auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *BloomSetupProg, Texture, *halfSceneTexture, sampler);
			SET_SHADER_PARAM(commandList, *BloomSetupProg, BloomThreshold, config.threshold);

			DrawUtility::ScreenRect(commandList);
			downsampleTextures[0] = static_cast<RHITexture2D*>(bloomSetupRT->texture.get());
		}

		for (int i = 1; i < ARRAY_SIZE(downsampleTextures); ++i)
		{
			downsampleTextures[i] = DownsamplePass(commandList, i, *downsampleTextures[i - 1]);
		}

		Vector4 weightData[128];
		Vector4 uvOffsetData[64];

		auto BlurPass = [&](RHICommandList& commandList, int index, RHITexture2D& fliterTexture, RHITexture2D& bloomTexture, LinearColor const& tint, float blurSize) -> RHITexture2DRef
		{
			float bloomRadius = blurSize * 0.01f * 0.5f * (float)fliterTexture.getSizeX() * config.blurRadiusScale;
			auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();

			auto GenerateFilterData = [&](int imageSize, Vector2 const& offsetDir, LinearColor const& bloomTint, float bloomRadius) -> int
			{
				Vector2 weightAndOffset[128];
				int numSamples = generateGaussianlDisburtionWeightAndOffset(bloomRadius, weightAndOffset);
				for (int i = 0; i < numSamples; ++i)
				{
					weightData[i] = weightAndOffset[i].x * Vector4(bloomTint);
					Vector2 offset = (weightAndOffset[i].y / float(imageSize)) * offsetDir;
					if (i % 2 == 0)
					{
						uvOffsetData[i / 2].x = offset.x;
						uvOffsetData[i / 2].y = offset.y;
					}
					else
					{
						uvOffsetData[i / 2].z = offset.x;
						uvOffsetData[i / 2].w = offset.y;
					}
				}
				return numSamples;
			};

			IntVector2 size = IntVector2(fliterTexture.getSizeX(), fliterTexture.getSizeY());
			RenderTargetDesc desc;
			desc.size = size;
			desc.format = ETexture::FloatRGBA;
			desc.creationFlags = TCF_CreateSRV;

			desc.debugName = "BlurH";
			PooledRenderTargetRef blurXRT = GRenderTargetPool.fetchElement(desc);
			bloomFrameBuffer.setTexture(0, *blurXRT->texture);
			{
				RHISetFrameBuffer(commandList, &bloomFrameBuffer);
				RHISetViewport(commandList, 0, 0, size.x, size.y);
				RHISetShaderProgram(commandList, FliterProg->getRHI());
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *FliterProg, FliterTexture, fliterTexture, sampler);
				int numSamples = GenerateFilterData(fliterTexture.getSizeX(), Vector2(1, 0), LinearColor(1, 1, 1, 1), bloomRadius);
				FliterProg->setParam(commandList, FliterProg->mParamWeights, weightData, MaxWeightNum);
				FliterProg->setParam(commandList, FliterProg->mParamUVOffsets, uvOffsetData, MaxWeightNum / 2);
				FliterProg->setParam(commandList, FliterProg->mParamWeightNum, numSamples);
				DrawUtility::ScreenRect(commandList);
			}

			desc.debugName = "BlurV";
			PooledRenderTargetRef blurYRT = GRenderTargetPool.fetchElement(desc);
			bloomFrameBuffer.setTexture(0, *blurYRT->texture);
			{
				RHISetFrameBuffer(commandList, &bloomFrameBuffer);
				RHISetViewport(commandList, 0, 0, size.x, size.y);
				RHISetShaderProgram(commandList, FliterAddProg->getRHI());
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *FliterAddProg, FliterTexture, *blurXRT->texture, sampler);
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *FliterAddProg, AddTexture, bloomTexture, sampler);
				int numSamples = GenerateFilterData(fliterTexture.getSizeY(), Vector2(0, 1), tint, bloomRadius);
				FliterAddProg->setParam(commandList, FliterAddProg->mParamWeights, weightData, MaxWeightNum);
				FliterAddProg->setParam(commandList, FliterAddProg->mParamUVOffsets, uvOffsetData, MaxWeightNum / 2);
				FliterAddProg->setParam(commandList, FliterAddProg->mParamWeightNum, numSamples);
				DrawUtility::ScreenRect(commandList);
			}
			return static_cast<RHITexture2D*>(blurYRT->texture.get());
		};

		struct BlurInfo
		{
			LinearColor tint;
			float size;
		};
		static const BlurInfo blurInfos[] =
		{
			{ LinearColor(0.3465f, 0.3465f, 0.3465f), 0.3f },
			{ LinearColor(0.138f, 0.138f, 0.138f), 1.0f },
			{ LinearColor(0.1176f, 0.1176f, 0.1176f), 2.0f },
			{ LinearColor(0.066f, 0.066f, 0.066f), 10.0f },
			{ LinearColor(0.066f, 0.066f, 0.066f), 30.0f },
			{ LinearColor(0.061f, 0.061f, 0.061f), 64.0f },
		};

		RHITexture2DRef bloomTexture = GBlackTexture2D;
		float tintScale = config.intensity / float(ARRAY_SIZE(downsampleTextures));
		for (int i = 0; i < ARRAY_SIZE(downsampleTextures); ++i)
		{
			int index = ARRAY_SIZE(downsampleTextures) - 1 - i;
			bloomTexture = BlurPass(commandList, i, *downsampleTextures[index], *bloomTexture, Vector4(blurInfos[index].tint) * tintScale, blurInfos[index].size);
		}

		return bloomTexture;
	}

	bool FBloom::InitailizeRHI() { return true; }
	void FBloom::ReleaseRHI() {}

}//namespace Render