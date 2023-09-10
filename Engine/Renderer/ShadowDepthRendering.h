#pragma once
#ifndef ShadowDepthRendering_H_D0420239_63F3_4485_AF53_6673ADCDF9D2
#define ShadowDepthRendering_H_D0420239_63F3_4485_AF53_6673ADCDF9D2

#include "RHI/MaterialShader.h"
#include "SceneLighting.h"

namespace Render
{
	class ShadowDepthProgram : public MaterialShaderProgram
	{
	public:
		using BaseClass = MaterialShaderProgram;
		DECLARE_EXPORTED_SHADER_PROGRAM(ShadowDepthProgram, Material, CORE_API);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/ShadowDepthRender";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};

	class ShadowDepthPositionOnlyProgram : public MaterialShaderProgram
	{
	public:
		using BaseClass = MaterialShaderProgram;
		DECLARE_EXPORTED_SHADER_PROGRAM(ShadowDepthPositionOnlyProgram, Material, CORE_API);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(SHADOW_DEPTH_POSITION_ONLY), true);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/ShadowDepthRender";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				//{ EShader::Pixel  , SHADER_ENTRY(NullPS) },
			};
			return entries;
		}
	};

	struct ShadowProjectParam
	{
		LightInfo const*   light;
		RHITextureBase* shadowTexture;
		Vector3         shadowOffset;
		Vector3         shadowParam;

		static int const MaxCascadeNum = 8;
		Matrix4      shadowMatrix[MaxCascadeNum];
		int          numCascade;
		float        cacadeDepth[MaxCascadeNum];
		Matrix4      shadowMatrixTest;

		void setupLight(LightInfo const& inLight);
		void setupShader(RHICommandList& commandList, ShaderProgram& program) const;
		void clearShaderResource(RHICommandList& commandList, ShaderProgram& program) const;
	};

	struct FShadowRendering
	{


	};

}//namespace Render

#endif // ShadowDepthRendering_H_D0420239_63F3_4485_AF53_6673ADCDF9D2