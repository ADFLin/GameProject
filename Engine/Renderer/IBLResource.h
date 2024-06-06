#pragma once
#ifndef IBLResource_H_CEA71573_90FB_426B_9BF1_1547F01E4274
#define IBLResource_H_CEA71573_90FB_426B_9BF1_1547F01E4274

#include "RHI/RHICommon.h"
#include "RHI/ShaderCore.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHICommand.h"

class DataCacheInterface;

namespace Render
{
	struct ImageBaseLightingData;

	struct IBLBuildSetting
	{
		int envSize;

		int irradianceSize;
		int irradianceSampleCount[2];
		int perfilteredSize;
		int prefilterSampleCount;
		int BRDFSampleCount;

		IBLBuildSetting()
		{
			envSize = 1024;
			irradianceSize = 256;
			perfilteredSize = 256;
			irradianceSampleCount[0] = 128;
			irradianceSampleCount[1] = 64;
			prefilterSampleCount = 2048;
			BRDFSampleCount = 2048;
		}
	};

	struct IBLResource
	{
		static int const NumPerFilteredLevel = 5;
		RHITextureCubeRef texture;
		RHITextureCubeRef irradianceTexture;
		RHITextureCubeRef perfilteredTexture;
		static TGlobalRenderResource< RHITexture2D > SharedBRDFTexture;

		void releaseRHI()
		{
			texture.release();
			irradianceTexture.release();
			perfilteredTexture.release();
			
			SharedBRDFTexture.releaseRHI();
		}

		static bool InitializeBRDFTexture(void* data);
		static void FillSharedBRDFData(TArray< uint8 >& outData)
		{
			return RHIReadTexture(*IBLResource::SharedBRDFTexture, ETexture::FloatRGBA, 0, outData);
		}
		bool initializeRHI(ImageBaseLightingData& IBLData);
		bool initializeRHI(IBLBuildSetting const& setting);
		void fillData(ImageBaseLightingData& outData);

		static void GetCubeMapData(TArray< uint8 >& data, ETexture::Format format, int size, int level, void* outData[]);
	};

	struct ImageBaseLightingData
	{
		int envMapSize;
		TArray< uint8 > envMap;
		int irradianceSize;
		TArray< uint8 > irradiance;
		int perFilteredSize;
		TArray< uint8 > perFiltered[IBLResource::NumPerFilteredLevel];

		template< class OP >
		void serialize(OP& op)
		{
			op & envMapSize & irradianceSize & perFilteredSize;
			op & envMap;
			op & irradiance;
			op & perFiltered;
		}
	};


	class IBLShaderParameters
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, IrradianceTexture);
			BIND_TEXTURE_PARAM(parameterMap, PrefilteredTexture);
			BIND_TEXTURE_PARAM(parameterMap, PreIntegratedBRDFTexture);
		}

		void setParameters(RHICommandList& commandList, ShaderProgram& shader, IBLResource& resource)
		{
			if (mParamIrradianceTexture.isBound())
			{
				shader.setTexture(commandList, mParamIrradianceTexture, resource.irradianceTexture);
			}

			shader.setTexture(commandList, mParamPrefilteredTexture, resource.perfilteredTexture, mParamPrefilteredTextureSampler,
				TStaticSamplerState< ESampler::Trilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp > ::GetRHI());

			if (resource.SharedBRDFTexture.isValid())
			{
				shader.setTexture(commandList, mParamPreIntegratedBRDFTexture, *resource.SharedBRDFTexture, mParamPreIntegratedBRDFTextureSampler,
					TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp > ::GetRHI());
			}
		}

		DEFINE_SHADER_PARAM(IrradianceTexture);
		DEFINE_TEXTURE_PARAM(PrefilteredTexture);
		DEFINE_TEXTURE_PARAM(PreIntegratedBRDFTexture);
	};

	class IBLResourceBuilder
	{
	public:
		bool loadOrBuildResource(DataCacheInterface& dataCache, char const* path, RHITexture2D& HDRImage, IBLResource& resource, IBLBuildSetting const& setting = IBLBuildSetting());
		bool buildIBLResource(RHITexture2D& envTexture, IBLResource& resource, IBLBuildSetting const& setting);
		void releaseRHI();


		bool initializeShader();

		class ScreenVS* mScreenVS = nullptr;
		class EquirectangularToCubePS* mEquirectangularToCubePS = nullptr;
		class IrradianceGenPS* mIrradianceGenPS = nullptr;
		class PrefilteredGenPS* mPrefilteredGenPS = nullptr;
		class PreIntegrateBRDFGenPS* mPreIntegrateBRDFGenPS = nullptr;

		class EquirectangularToCubeProgram* mProgEquirectangularToCube = nullptr;
		class IrradianceGenProgram* mProgIrradianceGen = nullptr;
		class PrefilteredGenProgram* mProgPrefilteredGen = nullptr;
		class PreIntegrateBRDFGenProgram* mProgPreIntegrateBRDFGen = nullptr;
	};


}
#endif // IBLResource_H_CEA71573_90FB_426B_9BF1_1547F01E4274