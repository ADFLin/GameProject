#include "VolumetricLighting.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "Renderer/SceneView.h"

namespace Render
{
	void TiledLightInfo::setValue(LightInfo const& light)
	{
		pos = light.pos;
		type = (int32)light.type;
		color = light.color;
		intensity = light.intensity;
		dir = light.dir;
		radius = light.radius;
		float angleInner = Math::Min(light.spotAngle.x, light.spotAngle.y);
		param.x = Math::Cos(Math::DegToRad(Math::Min<float>(89.9, angleInner)));
		param.y = Math::Cos(Math::DegToRad(Math::Min<float>(89.9, light.spotAngle.y)));
	}

	bool VolumetricLightingResources::prepare(IntVector2 const& screenSize, int sizeFactor, int depthSlices)
	{
		int nx = (screenSize.x + sizeFactor - 1) / sizeFactor;
		int ny = (screenSize.y + sizeFactor - 1) / sizeFactor;


		if (mVolumeBufferA.isValid() &&
			mVolumeBufferA->getSizeX() == nx &&
			mVolumeBufferA->getSizeY() == ny &&
			mVolumeBufferA->getSizeZ() == depthSlices &&
			(mVolumeBufferA->getDesc().creationFlags & TCF_CreateUAV))
		{
			return true;
		}

		int nz = depthSlices;
		mVolumeBufferA = RHICreateTexture3D(ETexture::RGBA32F, nx, ny, nz, 1, 1, TCF_CreateUAV | TCF_CreateSRV);
		mVolumeBufferB = RHICreateTexture3D(ETexture::RGBA32F, nx, ny, nz, 1, 1, TCF_CreateUAV | TCF_CreateSRV);
		mScatteringBuffer = RHICreateTexture3D(ETexture::RGBA32F, nx, ny, nz, 1, 1, TCF_CreateUAV | TCF_CreateSRV);
		mHistoryBuffer = RHICreateTexture3D(ETexture::RGBA32F, nx, ny, nz, 1, 1, TCF_CreateUAV | TCF_CreateSRV);
		mScatteringRawBuffer = RHICreateTexture3D(ETexture::RGBA32F, nx, ny, nz, 1, 1, TCF_CreateUAV | TCF_CreateSRV);
		if (!mTiledLightBuffer.isValid())
		{
			mTiledLightBuffer.initializeResource(MaxTiledLightNum, EStructuredBufferType::Buffer);
		}

		return mVolumeBufferA.isValid() && mVolumeBufferB.isValid() && mScatteringBuffer.isValid() && mScatteringRawBuffer.isValid() && mTiledLightBuffer.isValid();
	}

	void VolumetricLightingResources::releaseRHI()
	{
		mVolumeBufferA.release();
		mVolumeBufferB.release();
		mScatteringBuffer.release();
		mHistoryBuffer.release();
		mScatteringRawBuffer.release();
		mTiledLightBuffer.releaseResource();
	}

	void ClearBufferProgram::bindParameters(ShaderParameterMap const& parameterMap)
	{
		mParamBufferRW.bind(parameterMap, SHADER_PARAM(TargetRWTexture));
		mParamClearValue.bind(parameterMap, SHADER_PARAM(ClearValue));
	}

	void ClearBufferProgram::setParameters(RHICommandList& commandList, RHITexture3D& Buffer , Vector4 const& clearValue)
	{
		setRWTexture(commandList, mParamBufferRW, Buffer, 0, EAccessOp::WriteOnly);
		setParam(commandList, mParamClearValue, clearValue);
	}
	
	void ClearBufferProgram::clearTexture(RHICommandList& commandList, RHITexture3D& buffer , Vector4 const& clearValue)
	{
		int nx = (buffer.getSizeX() + SizeX - 1) / SizeX;
		int ny = (buffer.getSizeY() + SizeY - 1) / SizeY;
		int nz = (buffer.getSizeZ() + SizeZ - 1) / SizeZ;
		RHISetShaderProgram(commandList, getRHI());
		setParameters(commandList, buffer, clearValue);
		RHIDispatchCompute(commandList, nx, ny, nz);
	}

	void ClearBufferProgram::SetupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addDefine(SHADER_PARAM(SIZE_X), SizeX);
		option.addDefine(SHADER_PARAM(SIZE_Y), SizeY);
		option.addDefine(SHADER_PARAM(SIZE_Z), SizeZ);
	}
	char const* ClearBufferProgram::GetShaderFileName()
	{
		return "Shader/BufferUtility";
	}
	TArrayView< ShaderEntryInfo const > ClearBufferProgram::GetShaderEntries()
	{
		static ShaderEntryInfo entries[] =
		{
			{ EShader::Compute , SHADER_ENTRY(BufferClearCS) },
		};
		return entries;
	}

	void LightScatteringProgram::bindParameters(ShaderParameterMap const& parameterMap)
	{
		mParamVolumeBufferA.bind(parameterMap, SHADER_PARAM(VolumeBufferA));
		mParamVolumeBufferB.bind(parameterMap, SHADER_PARAM(VolumeBufferB));
		mParamScatteringRWBuffer.bind(parameterMap, SHADER_PARAM(ScatteringRWBuffer));
		mParamHistoryBuffer.bind(parameterMap, SHADER_PARAM(HistoryBuffer));
		mParamHistoryBufferSampler.bind(parameterMap, SHADER_SAMPLER(HistoryBuffer));
		mParamTiledLightList.bind(parameterMap, SHADER_PARAM(TiledLightList));
		mParamTiledLightNum.bind(parameterMap, SHADER_PARAM(TiledLightNum));
	}

	void LightScatteringProgram::setParameters(RHICommandList& commandList, ViewInfo& view , VolumetricLightingResources& resources, int numLights)
	{
		if (mParamVolumeBufferA.isBound())
			setTexture(commandList, mParamVolumeBufferA, *resources.mVolumeBufferA);
		if (mParamVolumeBufferB.isBound())
			setTexture(commandList, mParamVolumeBufferB, *resources.mVolumeBufferB);
		if (mParamScatteringRWBuffer.isBound())
			setRWTexture(commandList, mParamScatteringRWBuffer, *resources.mScatteringRawBuffer, 0, EAccessOp::WriteOnly);
		if (mParamHistoryBuffer.isBound())
			setTexture(commandList, mParamHistoryBuffer, *resources.mHistoryBuffer, mParamHistoryBufferSampler, TStaticSamplerState<ESampler::Trilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());

		if (mParamTiledLightList.isBound())
			setStorageBuffer(commandList, mParamTiledLightList, *resources.mTiledLightBuffer.getRHI());
		else
			setStructuredStorageBufferT<TiledLightInfo>(commandList, *resources.mTiledLightBuffer.getRHI());

		view.setupShader(commandList, *this);
		if (mParamTiledLightNum.isBound())
			setParam(commandList, mParamTiledLightNum, numLights);
	}

	void LightScatteringProgram::unbindParameters(RHICommandList& commandList, VolumetricLightingResources& resources)
	{
		// D3D11 Hazard Workaround:
		// Bind mScatteringBuffer (Accumulation) to the UAV slot to evict mScatteringRawBuffer.
		// This ensures mScatteringRawBuffer is unbound from UAV before being bound as SRV in the next pass.
		if (mParamScatteringRWBuffer.isBound())
		{
			clearRWTexture(commandList, mParamScatteringRWBuffer);
		}
	}

	void LightScatteringProgram::SetupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addDefine(SHADER_PARAM(GROUP_SIZE_X), GroupSizeX);
		option.addDefine(SHADER_PARAM(GROUP_SIZE_Y), GroupSizeY);
	}
	char const* LightScatteringProgram::GetShaderFileName()
	{
		return "Shader/VolumetricLighting";
	}
	TArrayView< ShaderEntryInfo const > LightScatteringProgram::GetShaderEntries()
	{
		static ShaderEntryInfo entries[] =
		{
			{ EShader::Compute , SHADER_ENTRY(LightScatteringCS) },
		};
		return entries;
	}


	void VolumetricIntegrationProgram::bindParameters(ShaderParameterMap const& parameterMap)
	{
		mParamScatteringRWBuffer.bind(parameterMap, SHADER_PARAM(ScatteringRWBuffer));
		mParamVolumeBufferA.bind(parameterMap, SHADER_PARAM(VolumeBufferA)); // We will bind RawBuffer to this param name
	}

	void VolumetricIntegrationProgram::setParameters(RHICommandList& commandList, VolumetricLightingResources& resources)
	{
		if (mParamScatteringRWBuffer.isBound())
			setRWTexture(commandList, mParamScatteringRWBuffer, *resources.mScatteringBuffer, 0, EAccessOp::ReadAndWrite);
		if (mParamVolumeBufferA.isBound()) // New: Bind Input Source
		{
			setTexture(commandList, mParamVolumeBufferA, *resources.mScatteringRawBuffer);
		}
	}

	void VolumetricIntegrationProgram::SetupShaderCompileOption(ShaderCompileOption& option)
	{
		option.addDefine(SHADER_PARAM(GROUP_SIZE_X), GroupSizeX);
		option.addDefine(SHADER_PARAM(GROUP_SIZE_Y), GroupSizeY);
	}

	char const* VolumetricIntegrationProgram::GetShaderFileName()
	{
		return "Shader/VolumetricLighting";
	}

	TArrayView< ShaderEntryInfo const > VolumetricIntegrationProgram::GetShaderEntries()
	{
		static ShaderEntryInfo entries[] =
		{
			{ EShader::Compute , SHADER_ENTRY(VolumetricIntegrationCS) },
		};
		return entries;
	}


	void VolumetricVisualizationProgram::bindParameters(ShaderParameterMap const& parameterMap)
	{
		mParamScatteringBuffer.bind(parameterMap, SHADER_PARAM(ScatteringBuffer));
		mParamScatteringBufferSampler.bind(parameterMap, SHADER_SAMPLER(ScatteringBuffer));
		mParamVolumeBufferA.bind(parameterMap, SHADER_PARAM(VolumeBufferA));
		mParamSceneDepth.bind(parameterMap, SHADER_PARAM(SceneDepthTexture));
	}

	void VolumetricVisualizationProgram::setParameters(RHICommandList& commandList, ViewInfo& view, VolumetricLightingResources& resources, RHITexture2D& sceneDepth)
	{
		view.setupShader(commandList, *this);
		if (mParamScatteringBuffer.isBound())
		{
			// Explicitly bind TRILINEAR sampler with CLAMP to blend between depth slices smoothly
			setTexture(commandList, mParamScatteringBuffer, *resources.mScatteringBuffer, mParamScatteringBufferSampler, TStaticSamplerState<ESampler::Trilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());
		}
		if (mParamVolumeBufferA.isBound())
			setTexture(commandList, mParamVolumeBufferA, *resources.mVolumeBufferA);

		if (mParamSceneDepth.isBound())
			setTexture(commandList, mParamSceneDepth, sceneDepth);
	}

	void VolumetricVisualizationProgram::SetupShaderCompileOption(ShaderCompileOption& option)
	{
	}

	char const* VolumetricVisualizationProgram::GetShaderFileName()
	{
		return "Shader/VolumetricLighting";
	}

	TArrayView< ShaderEntryInfo const > VolumetricVisualizationProgram::GetShaderEntries()
	{
		static ShaderEntryInfo entries[] =
		{
			{ EShader::Vertex , SHADER_ENTRY(VolumetricVisualizationVS) },
			{ EShader::Pixel , SHADER_ENTRY(VolumetricVisualizationPS) },
		};
		return entries;
	}

	IMPLEMENT_SHADER_PROGRAM(ClearBufferProgram);
	IMPLEMENT_SHADER_PROGRAM(LightScatteringProgram);
	IMPLEMENT_SHADER_PROGRAM(VolumetricIntegrationProgram);
	IMPLEMENT_SHADER_PROGRAM(VolumetricVisualizationProgram);

}//namespace Render
