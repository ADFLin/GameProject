#pragma once

#ifndef SceneLighting_H_1974DEA6_1F22_4485_A8C4_0FAAFA1E0EC3
#define SceneLighting_H_1974DEA6_1F22_4485_A8C4_0FAAFA1E0EC3

#include "RHI/GlobalShader.h"
#include "Renderer/BasePassRendering.h"
#include "ReflectionCollect.h"

namespace Render
{
	struct ViewInfo;

	enum class LightType
	{
		Spot,
		Point,
		Directional,
	};

	struct LightInfo
	{
		LightType type;
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

		REFLECT_OBJECT_BEGIN(LightInfo)
			REF_PROPERTY(type)
			REF_PROPERTY(pos)
			REF_PROPERTY(color)
			REF_PROPERTY(dir)
			REF_PROPERTY(spotAngle)
			REF_PROPERTY(intensity)
			REF_PROPERTY(radius)
			REF_PROPERTY(bCastShadow)
		REFLECT_OBJECT_END()
	};

	class DeferredLightingProgram : public GlobalShaderProgram
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

	template< LightType LIGHT_TYPE, bool bUseBoundShape = false >
	class TDeferredLightingProgram : public DeferredLightingProgram
	{
		DECLARE_SHADER_PROGRAM(TDeferredLightingProgram, Global)
		using BaseClass = DeferredLightingProgram;

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			if (bUseBoundShape)
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

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(DEFERRED_LIGHT_TYPE), (int)LIGHT_TYPE);
			option.addDefine(SHADER_PARAM(DEFERRED_SHADING_USE_BOUND_SHAPE), bUseBoundShape);
		}
	};



	class LightingShowBoundProgram : public DeferredLightingProgram
	{
		DECLARE_SHADER_PROGRAM(LightingShowBoundProgram, Global)
		using BaseClass = DeferredLightingProgram;

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

#define DECLARE_DEFERRED_SHADER( NAME )\
	typedef TDeferredLightingProgram< LightType::NAME , true > DeferredLightingProgram##NAME;\

	DECLARE_DEFERRED_SHADER(Spot);
	DECLARE_DEFERRED_SHADER(Point);
	DECLARE_DEFERRED_SHADER(Directional);

}//namespace Render

#endif // SceneLighting_H_1974DEA6_1F22_4485_A8C4_0FAAFA1E0EC3
