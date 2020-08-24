#pragma once

#ifndef Bloom_H_6668CE28_79ED_42A6_8C31_233D67B019C2
#define Bloom_H_6668CE28_79ED_42A6_8C31_233D67B019C2

#include "RHI/GlobalShader.h"

namespace Render
{

	class DownsampleProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(DownsampleProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(DownsamplePS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, Texture);
			BIND_SHADER_PARAM(parameterMap, ExtentInverse);
		}

		DEFINE_SHADER_PARAM(ExtentInverse);
		DEFINE_TEXTURE_PARAM(Texture);
	};


	class BloomSetupProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(BloomSetupProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BloomSetupPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, Texture);
			BIND_SHADER_PARAM(parameterMap, BloomThreshold);
			//BIND_SHADER_PARAM(parameterMap, BloomIntensity);
		}

		DEFINE_TEXTURE_PARAM(Texture);
		DEFINE_SHADER_PARAM(BloomThreshold);
		//DEFINE_SHADER_PARAM(BloomIntensity);
	};

	class FliterProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(FliterProgram, Global)
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(FliterPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, FliterTexture);
		}

		DEFINE_TEXTURE_PARAM(FliterTexture);
	};

	class FliterAddProgram : public FliterProgram
	{
		using BaseClass = FliterProgram;
	public:

		DECLARE_SHADER_PROGRAM(FliterAddProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(APPLY_ADD_TEXTURE), true);
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_TEXTURE_PARAM(parameterMap, AddTexture);
		}

		DEFINE_TEXTURE_PARAM(AddTexture);
	};

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
			BIND_TEXTURE_PARAM(parameterMap, TextureInput0);
			BIND_TEXTURE_PARAM(parameterMap, BloomTexture);
		}

		DEFINE_TEXTURE_PARAM(TextureInput0);
		DEFINE_TEXTURE_PARAM(BloomTexture);
	};


	constexpr int MaxWeightNum = 64;

	int generateGaussianlDisburtionWeightAndOffset(float kernelRadius, Vector2 outWeightAndOffset[128]);


}//namespace Render


#endif // Bloom_H_6668CE28_79ED_42A6_8C31_233D67B019C2
