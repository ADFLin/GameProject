#pragma once
#ifndef DDGIProbeManager_H
#define DDGIProbeManager_H

#include "Math/Vector3.h"
#include "Math/TVector3.h"
#include "Math/Matrix4.h"

#include "RHI/RHICommand.h"
#include "RHI/GlobalShader.h"

namespace Render
{
	struct ViewInfo;
	class RHICommandList;


	enum class EProbeState : uint32
	{
		Active,     // Full update every frame
		Stable,     // Converged, low frequency update
		Relocating, // Repositioning due to occlusion
		Sleeping,   // Far from camera or invisible
		Invalid,    // Out of volume
	};

	struct DDGIGlobalParameters
	{
		Vector3      volumeMin;
		float        probeSpacing;   
		IntVector3   probeCount;
		int          rayCountPerProbe; 
		Matrix4      randomRotation;   
	};

	struct DDGIConfig
	{
		IntVector3 gridCount = IntVector3(16, 16, 16);
		float      probeSpacing = 2.0f;
		int        rayCountPerProbe = 128;
		float      hysteresis = 0.98f;
		float      maxDistance = 1000.0f;
	};

	struct DDGIProbeVisualizeParams;

	class DDGIProbeManager
	{
	public:
		DDGIProbeManager();
		~DDGIProbeManager();

		bool initializeRHI(DDGIConfig const& config);

		void cleanup();

		void update(float deltaTime, const Vector3& viewPos);

		void prepareForGpuUpdate(RHICommandList& commandList);
		void finalizeGpuUpdate(RHICommandList& commandList);

		void updateProbes(RHICommandList& commandList);

		void renderDebug(RHICommandList& commandList, ViewInfo& view, float probeRadius);

		template< typename TShader >
		void setupShaderParameters(RHICommandList& commandList, TShader& shader)
		{
			if (mParamBuffer.isValid())
				shader.setUniformBuffer(commandList, SHADER_PARAM(DDGIParamBuffer), *mParamBuffer.getRHI());

			if (mIrradianceTexture.isValid())
				shader.setTexture(commandList, SHADER_PARAM(DDGIIrradianceTexture), *mIrradianceTexture);

			if (mMetadataTexture.isValid())
				shader.setTexture(commandList, SHADER_PARAM(DDGIMetadataTexture), *mMetadataTexture);

			shader.setSampler(commandList, SHADER_PARAM(DDGISampler), TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());
		}

		// Accessors
		RHITexture2D* getIrradianceAtlas() const { return mIrradianceTexture; }
		RHITexture2D* getDistanceAtlas()   const { return mDistanceTexture; }
		RHITexture2D* getMetadataBuffer()  const { return mMetadataTexture; }
		RHIBuffer*    getRayHitBuffer()    const { return mRayHitBuffer; }

		Vector3     getVolumeMin()  const { return mVolumeMin; }
		IntVector3  getProbeCount() const { return mGridCount; }
		int         getRayCountPerProbe() const { return mRayCountPerProbe; }
		float       getProbeSpacing() const { return mProbeSpacing; }

		void        setVolumeMin(Vector3 const& min) { mVolumeMin = min; }
		void        setRandomRotation(Matrix4 const& rotation) { mRandomRotation = rotation; }

		int        mRayCountPerProbe = 64;

	private:
		void updateParamBuffer(RHICommandList& commandList);

		// RHI Resources
		RHITexture2DRef mIrradianceTexture;
		RHITexture2DRef mDistanceTexture;
		RHITexture2DRef mMetadataTexture; 
		RHIBufferRef    mRayHitBuffer;
		TStructuredBuffer<DDGIProbeVisualizeParams> mProbeVisBuffer;
		TStructuredBuffer<DDGIGlobalParameters> mParamBuffer;   // Constant Buffer
		
		// Shaders
		class DDGIUpdateIrradianceShader* mUpdateIrradianceShader;
		class DDGIUpdateDistanceShader* mUpdateDistanceShader;
		class DDGIUpdateIrradianceBorderShader* mUpdateIrradianceBorderShader;
		class DDGIUpdateDistanceBorderShader* mUpdateDistanceBorderShader;
		class DDGIProbeVisualizeProgram* mVisualizeProgram;

		// Volume State
		IntVector3 mGridCount;
		float      mProbeSpacing;
		Vector3    mVolumeMin;

		Matrix4    mRandomRotation;

		bool mRelocationEnabled = true;
	};

} //namespace Render

#endif //DDGIProbeManager_H
