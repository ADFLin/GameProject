#pragma once

#ifndef SceneLighting_H_1974DEA6_1F22_4485_A8C4_0FAAFA1E0EC3
#define SceneLighting_H_1974DEA6_1F22_4485_A8C4_0FAAFA1E0EC3

#include "RHI/GlobalShader.h"
#include "Renderer/BasePassRendering.h"
#include "ReflectionCollect.h"

namespace Render
{
	struct ViewInfo;

	enum class ELightType
	{
		Spot,
		Point,
		Directional,

		COUNT ,
	};

	REF_ENUM_BEGIN(ELightType)
		REF_ENUM(Spot)
		REF_ENUM(Point)
		REF_ENUM(Directional)
	REF_ENUM_END()

	struct LightInfo
	{
		ELightType type;
		Vector3   pos;
		Vector3   color;
		Vector3   dir;
		Vector3   spotAngle;
		float     intensity;
		float     radius;

		Vector3   upDir;
		bool      bCastShadow;

		void setupShaderGlobalParam(RHICommandList& commandList, ShaderProgram& shader) const;

		bool testVisible(ViewInfo const& view) const;

		REFLECT_STRUCT_BEGIN(LightInfo)
			REF_PROPERTY(type)
			REF_PROPERTY(pos)
			REF_PROPERTY(color)
			REF_PROPERTY(dir)
			REF_PROPERTY(spotAngle)
			REF_PROPERTY(intensity)
			REF_PROPERTY(radius)
			REF_PROPERTY(bCastShadow)
		REFLECT_STRUCT_END()
	};


	class DeferredLightingBaseProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamGBuffer.bindParameters(parameterMap, true);
		}

		void setParamters(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets)
		{
			mParamGBuffer.setParameters(commandList, *this, sceneRenderTargets);
		}

		void clearTextures(RHICommandList& commandList)
		{
			mParamGBuffer.clearTextures(commandList, *this);
		}

		GBufferShaderParameters mParamGBuffer;

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/DeferredLighting";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(LightingPassPS) },
			};
			return entries;
		}

	};


	class DeferredLightingProgram : public DeferredLightingBaseProgram
	{
		DECLARE_SHADER_PROGRAM(DeferredLightingProgram, Global)
		using BaseClass = DeferredLightingBaseProgram;

		SHADER_PERMUTATION_TYPE_INT_COUNT(UseLightType, SHADER_PARAM(DEFERRED_LIGHT_TYPE) , (int)ELightType::COUNT );
		SHADER_PERMUTATION_TYPE_BOOL(HaveBoundShape, SHADER_PARAM(DEFERRED_SHADING_USE_BOUND_SHAPE));

		using PermutationDomain = TShaderPermutationDomain<UseLightType, HaveBoundShape>;

		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			if (domain.get<HaveBoundShape>())
			{
				static ShaderEntryInfo const entriesUseBoundShape[] =
				{
					{ EShader::Vertex , SHADER_ENTRY(LightingPassVS) },
					{ EShader::Pixel  , SHADER_ENTRY(LightingPassPS) },
				};
				return entriesUseBoundShape;
			}

			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(LightingPassPS) },
			};
			return entries;
		}

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
		}
	};



	class LightingShowBoundProgram : public DeferredLightingBaseProgram
	{
		DECLARE_SHADER_PROGRAM(LightingShowBoundProgram, Global)
		using BaseClass = DeferredLightingBaseProgram;

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(DEFERRED_SHADING_USE_BOUND_SHAPE), true);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(LightingPassVS) },
				{ EShader::Pixel  , SHADER_ENTRY(ShowBoundPS) },
			};
			return entries;
		}
	};


}//namespace Render

#endif // SceneLighting_H_1974DEA6_1F22_4485_A8C4_0FAAFA1E0EC3
