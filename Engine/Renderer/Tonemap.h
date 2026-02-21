#pragma once

#ifndef Tonemap_H_D4A95FB3_7E3D_4113_8393_AD185D2EDB89
#define Tonemap_H_D4A95FB3_7E3D_4113_8393_AD185D2EDB89

#include "PostProcess.h"

namespace Render
{
	class TonemapProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(TonemapProgram, Global);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_BLOOM), true);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Tonemap";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
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
		}
		void setParameters(RHICommandList& commandList, PostProcessContext const& context)
		{
			mParamPostProcess.setParameters(commandList, *this, context);
		}

		void setBloomTexture(RHICommandList& commandList, RHITexture2D& bloomTexture)
		{
			setTexture(commandList, mParamBloomTexture, bloomTexture);
		}

		PostProcessParameters mParamPostProcess;
		ShaderParameter       mParamBloomTexture;
	};

}

#endif // Tonemap_H_D4A95FB3_7E3D_4113_8393_AD185D2EDB89
