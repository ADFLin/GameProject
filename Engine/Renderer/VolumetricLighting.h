#pragma once
#ifndef VolumetricLighting_H_
#define VolumetricLighting_H_

#include "RHI/RHICommon.h"
#include "RHI/ShaderCore.h"
#include "Renderer/SceneLighting.h"

namespace Render
{
	struct GPU_ALIGN TiledLightInfo
	{
		DECLARE_BUFFER_STRUCT(TiledLightList);

		Vector3 pos;
		int32   type; 
		Vector3 color;
		float   intensity;
		Vector3 dir;
		float   radius;
		Vector4 param; // x y: spotAngle  , z : shadowFactor
		Matrix4 worldToShadow;

		void setValue(LightInfo const& light);
	};

	struct VolumetricLightingResources
	{
		static int constexpr MaxTiledLightNum = 1024;
		bool prepare(IntVector2 const& screenSize, int sizeFactor, int depthSlices);

		void releaseRHI();

		RHITexture3DRef mVolumeBufferA;
		RHITexture3DRef mVolumeBufferB;
		RHITexture3DRef mScatteringBuffer;    // Output of Integration (Accumulated Fog)
		RHITexture3DRef mScatteringRawBuffer; // Output of LightScattering (Raw Radiance)
		RHITexture3DRef mHistoryBuffer;
		RHITexture2DRef mShadowMapAtlas;
		TStructuredBuffer<TiledLightInfo> mTiledLightBuffer;
	};

	class ClearBufferProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(ClearBufferProgram, Global);

		static int constexpr SizeX = 8;
		static int constexpr SizeY = 8;
		static int constexpr SizeZ = 8;


		void bindParameters(ShaderParameterMap const& parameterMap) override;

		void setParameters(RHICommandList& commandList, RHITexture3D& Buffer , Vector4 const& clearValue);
		
	public:
		void clearTexture(RHICommandList& commandList, RHITexture3D& buffer , Vector4 const& clearValue);

		static void SetupShaderCompileOption(ShaderCompileOption& option);
		static char const* GetShaderFileName();
		static TArrayView< ShaderEntryInfo const > GetShaderEntries();

		ShaderParameter mParamBufferRW;
		ShaderParameter mParamClearValue;
	};

	class LightScatteringProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(LightScatteringProgram, Global);

		static int constexpr GroupSizeX = 8;
		static int constexpr GroupSizeY = 8;

		void bindParameters(ShaderParameterMap const& parameterMap) override;

	public:
		void setParameters(RHICommandList& commandList, ViewInfo& view , VolumetricLightingResources& resources, int numLights);
		void unbindParameters(RHICommandList& commandList, VolumetricLightingResources& resources);

		static void SetupShaderCompileOption(ShaderCompileOption& option);
		static char const* GetShaderFileName();
		static TArrayView< ShaderEntryInfo const > GetShaderEntries();

		ShaderParameter mParamVolumeBufferA;
		ShaderParameter mParamVolumeBufferB;
		ShaderParameter mParamScatteringRWBuffer;
		ShaderParameter mParamHistoryBuffer;
		ShaderParameter mParamHistoryBufferSampler;
		ShaderParameter mParamTiledLightList;
		ShaderParameter mParamTiledLightNum;
	};

	class VolumetricIntegrationProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(VolumetricIntegrationProgram, Global);

		static int constexpr GroupSizeX = 8;
		static int constexpr GroupSizeY = 8;

		void bindParameters(ShaderParameterMap const& parameterMap) override;

	public:
		void setParameters(RHICommandList& commandList, VolumetricLightingResources& resources);

		static void SetupShaderCompileOption(ShaderCompileOption& option);
		static char const* GetShaderFileName();
		static TArrayView< ShaderEntryInfo const > GetShaderEntries();
	public:
		DEFINE_SHADER_PARAM(ScatteringRWBuffer);
		DEFINE_TEXTURE_PARAM(VolumeBufferA);
	};

	class VolumetricVisualizationProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(VolumetricVisualizationProgram, Global);

		void bindParameters(ShaderParameterMap const& parameterMap) override;

	public:
		void setParameters(RHICommandList& commandList, ViewInfo& view, VolumetricLightingResources& resources, RHITexture2D& sceneDepth);

		static void SetupShaderCompileOption(ShaderCompileOption& option);
		static char const* GetShaderFileName();
		static TArrayView< ShaderEntryInfo const > GetShaderEntries();

		ShaderParameter mParamScatteringBuffer;
		ShaderParameter mParamScatteringBufferSampler;
		ShaderParameter mParamVolumeBufferA;
		ShaderParameter mParamSceneDepth;
	};

}//namespace Render

#endif // VolumetricLighting_H_
