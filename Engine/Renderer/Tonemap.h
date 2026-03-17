#pragma once

#ifndef Tonemap_H_D4A95FB3_7E3D_4113_8393_AD185D2EDB89
#define Tonemap_H_D4A95FB3_7E3D_4113_8393_AD185D2EDB89

#include "PostProcess.h"
#include "Renderer/BasePassRendering.h"
#include "RHI/DrawUtility.h"

namespace Render
{
	class TonemapProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(TonemapProgram, Global);

		SHADER_PERMUTATION_TYPE_BOOL(UseBloom, SHADER_PARAM(USE_BLOOM));
		SHADER_PERMUTATION_TYPE_BOOL(UseACES, SHADER_PARAM(USE_ACES));
		using PermutationDomain = TShaderPermutationDomain< UseBloom, UseACES >;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Tonemap";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamPostProcess.bindParameters(parameterMap);
			mParamBloomTexture.bind(parameterMap, SHADER_PARAM(BloomTexture));
			mParamExposure.bind(parameterMap, SHADER_PARAM(Exposure));
		}
		void setParameters(RHICommandList& commandList, PostProcessContext const& context)
		{
			mParamPostProcess.setParameters(commandList, *this, context);
		}

		void setBloomTexture(RHICommandList& commandList, RHITexture2D& bloomTexture)
		{
			setTexture(commandList, mParamBloomTexture, bloomTexture);
		}
		void setExposure(RHICommandList& commandList, float exposure)
		{
			setParam(commandList, mParamExposure, exposure);
		}

		PostProcessParameters mParamPostProcess;
		ShaderParameter       mParamBloomTexture;
		ShaderParameter       mParamExposure;
	};

	struct FTonemap
	{
		static void Render(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, RHITexture2D* bloomTexture = nullptr, float exposure = 1.0f);
	};

}

#endif // Tonemap_H_D4A95FB3_7E3D_4113_8393_AD185D2EDB89
