#define _USE_MATH_DEFINES
#include <math.h>
#include "Async/AsyncWork.h"
#include <mutex>
#include <atomic>
#include <memory>
#include <tuple>
#include <functional>
#include "TestRenderPCH.h"
#include "Stage/TestRenderStageBase.h"

// Use independent post-processing and bloom headers
#include "Renderer/Bloom.h"
#include "Renderer/Tonemap.h"

// Engine FX System
#include "FX/ParticleSystem.h"
#include "FX/ParticleRenderer.h"

namespace Render
{
	class NightSkyProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(NightSkyProgram, Global);
	public:
		static char const* GetShaderFileName() { return "Shader/Game/NightSky"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, MoonDir);
			BIND_SHADER_PARAM(parameterMap, SkyIntensity);
		}

		DEFINE_SHADER_PARAM(MoonDir);
		DEFINE_SHADER_PARAM(SkyIntensity);
	};

	class FireworksStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		bool onInit() override;
		void onUpdate(GameTimeSpan deltaTime) override;
		void onRender(float dFrame) override;
		bool setupRenderResource(ERenderSystem systemName) override;
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;
		void preShutdownRenderSystem(bool bReInit = false) override;
		
		RateSpawnModule* mRocketSpawnerRate = nullptr;
		bool  bPaused = false;
		float mSpawnScale = 1.0f;
		float mSkyIntensity = 1.0f;

		std::unique_ptr<QueueThreadPool> mUpdateThreadPool;
		uint32 mStorageIdx = 0;
		std::atomic<int> mRenderCount;
		ParticleSystem mParticleSystem;
		ParticleSpriteRenderer mParticleRenderer;
		NightSkyProgram*  mProgNightSky;
		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef mBloomFrameBuffer;
		TArray<ParticleSpriteRenderer::InstanceData> mInstanceDataStorage[2];

		float mBloomThreshold = 0.5f;
		float mBloomIntensity = 2.6f;
		float blurRadiusScale = 1.5f;

		TArray<int> mRocketProfileIds;
	};
}
