#include "DDGIProbeManager.h"

#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/GpuProfiler.h"
#include "Renderer/SceneView.h"

namespace Render
{
	class DDGIUpdateIrradianceShader : public GlobalShader
	{
		DECLARE_SHADER(DDGIUpdateIrradianceShader, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/DDGIUpdate"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, DDGIProbeCount);
			BIND_SHADER_PARAM(parameterMap, DDGIRayCountPerProbe);
			BIND_SHADER_PARAM(parameterMap, DDGIBlinkRate);
			BIND_SHADER_PARAM(parameterMap, DDGIRandomRotation);
			BIND_SHADER_PARAM(parameterMap, RayHitBuffer);
			BIND_SHADER_PARAM(parameterMap, OutIrradiance);
		}
		DEFINE_SHADER_PARAM(DDGIProbeCount);
		DEFINE_SHADER_PARAM(DDGIRayCountPerProbe);
		DEFINE_SHADER_PARAM(DDGIRandomRotation);
		DEFINE_SHADER_PARAM(DDGIBlinkRate);
		DEFINE_SHADER_PARAM(RayHitBuffer);
		DEFINE_SHADER_PARAM(OutIrradiance);
	};

	class DDGIUpdateDistanceShader : public GlobalShader
	{
		DECLARE_SHADER(DDGIUpdateDistanceShader, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/DDGIUpdate"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, DDGIProbeCount);
			BIND_SHADER_PARAM(parameterMap, DDGIRayCountPerProbe);
			BIND_SHADER_PARAM(parameterMap, DDGIBlinkRate);
			BIND_SHADER_PARAM(parameterMap, DDGIRandomRotation);
			BIND_SHADER_PARAM(parameterMap, RayHitBuffer);
			BIND_SHADER_PARAM(parameterMap, OutDistance);
		}
		DEFINE_SHADER_PARAM(DDGIProbeCount);
		DEFINE_SHADER_PARAM(DDGIRayCountPerProbe);
		DEFINE_SHADER_PARAM(DDGIBlinkRate);
		DEFINE_SHADER_PARAM(DDGIRandomRotation);
		DEFINE_SHADER_PARAM(RayHitBuffer);
		DEFINE_SHADER_PARAM(OutDistance);
	};

	class DDGIUpdateIrradianceBorderShader : public GlobalShader
	{
		DECLARE_SHADER(DDGIUpdateIrradianceBorderShader, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/DDGIUpdate"; }
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, DDGIProbeCount);
			BIND_SHADER_PARAM(parameterMap, OutIrradiance);
		}
		DEFINE_SHADER_PARAM(DDGIProbeCount);
		DEFINE_SHADER_PARAM(OutIrradiance);
	};

	class DDGIUpdateDistanceBorderShader : public GlobalShader
	{
		DECLARE_SHADER(DDGIUpdateDistanceBorderShader, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/DDGIUpdate"; }
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, DDGIProbeCount);
			BIND_SHADER_PARAM(parameterMap, OutDistance);
		}
		DEFINE_SHADER_PARAM(DDGIProbeCount);
		DEFINE_SHADER_PARAM(OutDistance);
	};

	IMPLEMENT_SHADER(DDGIUpdateIrradianceShader, EShader::Compute, SHADER_ENTRY(UpdateIrradianceCS));
	IMPLEMENT_SHADER(DDGIUpdateDistanceShader, EShader::Compute, SHADER_ENTRY(UpdateDistanceCS));
	IMPLEMENT_SHADER(DDGIUpdateIrradianceBorderShader, EShader::Compute, SHADER_ENTRY(UpdateIrradianceBorderCS));
	IMPLEMENT_SHADER(DDGIUpdateDistanceBorderShader, EShader::Compute, SHADER_ENTRY(UpdateDistanceBorderCS));


	struct DDGIProbeVisualizeParams
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(DDGIProbeVisualizeParamsBlock);
		Vector3 startPos;
		float   probeRadius;
		Vector3 xAxis;
		float   spacing;
		Vector3 yAxis;
		int     dummy1;
		IntVector3 gridNum;
		int     dummy2;
	};

	class DDGIProbeVisualizeProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(DDGIProbeVisualizeProgram, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/DDGIProbeVisualize"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, DDGIIrradianceTexture);
			BIND_SHADER_PARAM(parameterMap, DDGISampler);
		}

		DEFINE_SHADER_PARAM(DDGIIrradianceTexture);
		DEFINE_SHADER_PARAM(DDGISampler);
	};

	IMPLEMENT_SHADER_PROGRAM(DDGIProbeVisualizeProgram);

	DDGIProbeManager::DDGIProbeManager()
	{
		mGridCount = IntVector3(0, 0, 0);
		mProbeSpacing = 0.0f;
		mVolumeMin = Vector3(0, 0, 0);
	}
	DDGIProbeManager::~DDGIProbeManager()
	{
		cleanup();
	}

	void DDGIProbeManager::cleanup()
	{
		mIrradianceTexture.release();
		mDistanceTexture.release();
		mMetadataTexture.release();
		mRayHitBuffer.release();
		mParamBuffer.releaseResource();
	}


	bool DDGIProbeManager::initializeRHI(DDGIConfig const& config)
	{
		cleanup();

		mGridCount = config.gridCount;
		mProbeSpacing = config.probeSpacing;
		mRayCountPerProbe = config.rayCountPerProbe;

		int probeTotal = mGridCount.x * mGridCount.y * mGridCount.z;

		mIrradianceTexture = RHICreateTexture2D(ETexture::RGBA16F,
			mGridCount.x * 8,
			mGridCount.y * mGridCount.z * 8,
			1, 1, TCF_CreateSRV | TCF_CreateUAV);

		mDistanceTexture = RHICreateTexture2D(ETexture::RG16F,
			mGridCount.x * 8,
			mGridCount.y * mGridCount.z * 8,
			1, 1, TCF_CreateSRV | TCF_CreateUAV);

		mMetadataTexture = RHICreateTexture2D(ETexture::RGBA32F,
			mGridCount.x,
			mGridCount.y * mGridCount.z,
			1, 1, TCF_CreateSRV | TCF_CreateUAV);

		mRayHitBuffer = RHICreateBuffer(sizeof(float) * 4, probeTotal * mRayCountPerProbe, BCF_CreateUAV | BCF_CreateSRV | BCF_Structured);

		// Initialize Shaders
		mUpdateIrradianceShader = ShaderManager::Get().getGlobalShaderT<DDGIUpdateIrradianceShader>();
		mUpdateDistanceShader = ShaderManager::Get().getGlobalShaderT<DDGIUpdateDistanceShader>();
		mUpdateIrradianceBorderShader = ShaderManager::Get().getGlobalShaderT<DDGIUpdateIrradianceBorderShader>();
		mUpdateDistanceBorderShader = ShaderManager::Get().getGlobalShaderT<DDGIUpdateDistanceBorderShader>();
		mVisualizeProgram = ShaderManager::Get().getGlobalShaderT<DDGIProbeVisualizeProgram>();
		return mIrradianceTexture.isValid() && mDistanceTexture.isValid() && mMetadataTexture.isValid() && mParamBuffer.isValid();
	}


	void DDGIProbeManager::prepareForGpuUpdate(RHICommandList& commandList)
	{
		//updateParamBuffer(commandList);
		RHIResourceTransition(commandList, { mIrradianceTexture, mDistanceTexture, mMetadataTexture }, EResourceTransition::UAV);
	}

	void DDGIProbeManager::finalizeGpuUpdate(RHICommandList& commandList)
	{
		updateProbes(commandList);
		RHIResourceTransition(commandList, { mIrradianceTexture, mDistanceTexture, mMetadataTexture }, EResourceTransition::SRV);
	}

	void DDGIProbeManager::updateProbes(RHICommandList& commandList)
	{
		GPU_PROFILE("UpdateProbes");

		{
			GPU_PROFILE("Update Irradiance");
			RHISetComputeShader(commandList, mUpdateIrradianceShader->getRHI());
			SET_SHADER_PARAM(commandList, *mUpdateIrradianceShader, DDGIProbeCount, mGridCount);
			SET_SHADER_PARAM(commandList, *mUpdateIrradianceShader, DDGIRayCountPerProbe, mRayCountPerProbe);
			SET_SHADER_PARAM(commandList, *mUpdateIrradianceShader, DDGIBlinkRate, 0.02f);
			SET_SHADER_PARAM(commandList, *mUpdateIrradianceShader, DDGIRandomRotation, mRandomRotation);
			mUpdateIrradianceShader->setStorageBuffer(commandList, mUpdateIrradianceShader->mParamRayHitBuffer, *mRayHitBuffer, EAccessOp::ReadOnly);
			mUpdateIrradianceShader->setRWTexture(commandList, mUpdateIrradianceShader->mParamOutIrradiance, *mIrradianceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}

		{
			GPU_PROFILE("Copy Irradiance Borders");
			RHISetComputeShader(commandList, mUpdateIrradianceBorderShader->getRHI());
			SET_SHADER_PARAM(commandList, *mUpdateIrradianceBorderShader, DDGIProbeCount, mGridCount);
			mUpdateIrradianceBorderShader->setRWTexture(commandList, mUpdateIrradianceBorderShader->mParamOutIrradiance, *mIrradianceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}

		{
			GPU_PROFILE("Update Distance");
			RHISetComputeShader(commandList, mUpdateDistanceShader->getRHI());
			SET_SHADER_PARAM(commandList, *mUpdateDistanceShader, DDGIProbeCount, mGridCount);
			SET_SHADER_PARAM(commandList, *mUpdateDistanceShader, DDGIRayCountPerProbe, mRayCountPerProbe);
			SET_SHADER_PARAM(commandList, *mUpdateDistanceShader, DDGIBlinkRate, 0.02f);
			SET_SHADER_PARAM(commandList, *mUpdateDistanceShader, DDGIRandomRotation, mRandomRotation);
			mUpdateDistanceShader->setStorageBuffer(commandList, mUpdateDistanceShader->mParamRayHitBuffer, *mRayHitBuffer, EAccessOp::ReadOnly);
			mUpdateDistanceShader->setRWTexture(commandList, mUpdateDistanceShader->mParamOutDistance, *mDistanceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}

		{
			GPU_PROFILE("Copy Distance Borders");
			RHISetComputeShader(commandList, mUpdateDistanceBorderShader->getRHI());
			SET_SHADER_PARAM(commandList, *mUpdateDistanceBorderShader, DDGIProbeCount, mGridCount);
			mUpdateDistanceBorderShader->setRWTexture(commandList, mUpdateDistanceBorderShader->mParamOutDistance, *mDistanceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}
	}


#if 0
	

	void DDGIProbeManager::update(float deltaTime, const Math::Vector3& viewPos)
	{
		// 1. Volume Position Update (Dynamic Scrolling)
		Math::Vector3 center = viewPos;
		Math::Vector3 halfExtent = Math::Vector3((float)mGridCount.x, (float)mGridCount.y, (float)mGridCount.z) * (mProbeSpacing * 0.5f);
		Math::Vector3 newMin = center - halfExtent;

		// Snap to spacing for stable temporal filtering
		newMin.x = Math::Floor(newMin.x / mProbeSpacing) * mProbeSpacing;
		newMin.y = Math::Floor(newMin.y / mProbeSpacing) * mProbeSpacing;
		newMin.z = Math::Floor(newMin.z / mProbeSpacing) * mProbeSpacing;
		mVolumeMin = newMin;

		// 2. Random Rotation for Rays (Temporal Stability)
		static Math::Vector3 rotAxis(0, 1, 0); // Can be randomized further
		static float rotAngle = 0.0f;
		rotAngle += deltaTime * 0.5f; // Constant slow rotation or frame-random
		mRandomRotation = Math::Matrix4::Rotate(rotAxis, rotAngle);
	}

	void DDGIProbeManager::updateParamBuffer(RHICommandList& commandList)
	{
		DDGIGlobalParameters params;
		params.volumeMin = mVolumeMin;
		params.probeSpacing = mProbeSpacing;
		params.probeCount = mGridCount;
		params.rayCountPerProbe = mRayCountPerProbe;
		params.randomRotation = mRandomRotation;
		
		mParamBuffer.updateBuffer(params);
	}




	void DDGIProbeManager::updateProbes(RHICommandList& commandList)
	{
		// 1. Update Irradiance
		{
			RHISetComputeShader(commandList, mUpdateIrradianceShader->getRHI());
			setupShaderParameters(commandList, *mUpdateIrradianceShader);
			SET_SHADER_PARAM(commandList, *mUpdateIrradianceShader, DDGIBlinkRate, 0.02f);
			mUpdateIrradianceShader->setStorageBuffer(commandList, mUpdateIrradianceShader->mParamRayHitBuffer, *mRayHitBuffer, EAccessOp::ReadOnly);
			mUpdateIrradianceShader->setRWTexture(commandList, mUpdateIrradianceShader->mParamOutIrradiance, *mIrradianceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}

		// 2. Update Distance
		{
			RHISetComputeShader(commandList, mUpdateDistanceShader->getRHI());
			setupShaderParameters(commandList, *mUpdateDistanceShader);
			SET_SHADER_PARAM(commandList, *mUpdateDistanceShader, DDGIBlinkRate, 0.02f);
			mUpdateDistanceShader->setStorageBuffer(commandList, mUpdateDistanceShader->mParamRayHitBuffer, *mRayHitBuffer, EAccessOp::ReadOnly);
			mUpdateDistanceShader->setRWTexture(commandList, mUpdateDistanceShader->mParamOutDistance, *mDistanceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}

		RHIResourceTransition(commandList, { mIrradianceTexture, mDistanceTexture }, EResourceTransition::UAV);

		// 3. Update Borders
		{
			RHISetComputeShader(commandList, mUpdateIrradianceBorderShader->getRHI());
			setupShaderParameters(commandList, *mUpdateIrradianceBorderShader);
			mUpdateIrradianceBorderShader->setRWTexture(commandList, mUpdateIrradianceBorderShader->mParamOutIrradiance, *mIrradianceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);

			RHISetComputeShader(commandList, mUpdateDistanceBorderShader->getRHI());
			setupShaderParameters(commandList, *mUpdateDistanceBorderShader);
			mUpdateDistanceBorderShader->setRWTexture(commandList, mUpdateDistanceBorderShader->mParamOutDistance, *mDistanceTexture, 0, EAccessOp::ReadAndWrite);
			RHIDispatchCompute(commandList, mGridCount.x, mGridCount.y, mGridCount.z);
		}
	}



#endif

	void DDGIProbeManager::renderDebug(RHICommandList& commandList, ViewInfo& view, float probeRadius)
	{
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

		RHISetShaderProgram(commandList, mVisualizeProgram->getRHI());
		view.setupShader(commandList, *mVisualizeProgram);

		if (!mProbeVisBuffer.isValid())
		{
			mProbeVisBuffer.initializeResource(1, EStructuredBufferType::Const);
		}

		DDGIProbeVisualizeParams params;
		params.startPos = mVolumeMin;
		params.xAxis = Vector3(1, 0, 0);
		params.yAxis = Vector3(0, 1, 0);
		params.spacing = mProbeSpacing;
		params.probeRadius = probeRadius;
		params.gridNum = mGridCount;
		mProbeVisBuffer.updateBuffer(params);

		mVisualizeProgram->setStructuredUniformBufferT<DDGIProbeVisualizeParams>(commandList, *mProbeVisBuffer.getRHI());
		SET_SHADER_TEXTURE(commandList, *mVisualizeProgram, DDGIIrradianceTexture, *mIrradianceTexture);
		mVisualizeProgram->setSampler(commandList, mVisualizeProgram->mParamDDGISampler, TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());

		RHISetInputStream(commandList, nullptr, nullptr, 0);
		RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 6, mGridCount.x * mGridCount.y * mGridCount.z);
	}

} //namespace Render

