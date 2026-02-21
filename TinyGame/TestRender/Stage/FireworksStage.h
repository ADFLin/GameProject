#define _USE_MATH_DEFINES
#include <math.h>
#include "Async/AsyncWork.h"
#include <mutex>
#include "TestRenderPCH.h"
#include "Stage/TestRenderStageBase.h"

// Use independent post-processing and bloom headers
#include "Renderer/Bloom.h"
#include "Renderer/Tonemap.h"


namespace Render
{


	class FireworksProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(FireworksProgram, Global);

	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
		}

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Fireworks";
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

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, ParticleOffset);
			BIND_SHADER_PARAM(parameterMap, ParticleRadius);
			BIND_SHADER_PARAM(parameterMap, ParticleColor);
		}

		void setParameters(RHICommandList& commandList, Vector3 const& pos, float radius, Vector3 const& color)
		{
			SET_SHADER_PARAM(commandList, *this, ParticleOffset, pos);
			SET_SHADER_PARAM(commandList, *this, ParticleRadius, radius);
			SET_SHADER_PARAM(commandList, *this, ParticleColor, color);
		}

		DEFINE_SHADER_PARAM(ParticleOffset);
		DEFINE_SHADER_PARAM(ParticleRadius);
		DEFINE_SHADER_PARAM(ParticleColor);
	};

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


	struct FireworkArchetype;
	class FireworksSystem;

	struct Particle
	{
		Vector3 pos;
		Vector3 vel;
		Vector3 color;
		float   life;
		float   maxLife;
		// Removed archetypeIndex - implied by container
	};

	class IExplosionStrategy
	{
	public:
		virtual ~IExplosionStrategy() = default;
		virtual void explode(const Particle& parent, FireworksSystem& system, Vector3 const& pos, Vector3 const& vel, Vector3 const& color) const = 0;
	};

	struct FireworkArchetype
	{
		int     id = -1; // Self ID
		// Physics
		Vector3 gravity = Vector3(0, 0, -4.0f);
		float   drag = 0.4f;
		float   noiseStrength = 0.0f;

		// Life & Visuals
		float   lifeMin = 1.0f;
		float   lifeMax = 2.0f;
		float   size = 0.12f;
		bool    bIsRocket = false;

		// Trail
		float   trailInterval = 0.0f; 
		float   trailChance = 0.0f;
		int     trailArchetypeId = -1;

		// Death
		std::shared_ptr<IExplosionStrategy> explosionStrategy;
	};

	class ParticleGroup
	{
	public:
		ParticleGroup(const FireworkArchetype& archetype) : mArchetype(archetype) {}

		void update(float dt); // To be implemented in cpp
		void addParticle(Vector3 const& pos, Vector3 const& vel, Vector3 const& color, float life);

		const FireworkArchetype& getArchetype() const { return mArchetype; }
		const TArray<Particle>& getParticles() const { return mParticles; }
		
		// Deferred events from update
		struct PendingExplosion
		{
			Vector3 pos;
			Vector3 vel;
			Vector3 color;
		};
		// Not thread safe directly, but we will handle threading in System or Stage
		TArray<Particle> mParticles;
		FireworkArchetype mArchetype; 

		// For double buffering or adding new particles safely:
		TArray<Particle> mPendingParticles;
		TArray<PendingExplosion> mPendingExplosions;
		
		void flushPending();
	};

	class FireworksSystem
	{
	public:
		void registerArchetype(int id, FireworkArchetype const& archetype);
		ParticleGroup* getGroup(int id);
		
		void createParticle(int archetypeId, Vector3 const& pos, Vector3 const& vel, Vector3 const& color);

		// Helper to ensure groups exist for all registered archetypes
		void initializeGroups();
		
		TArray<std::unique_ptr<ParticleGroup>> mGroups;
		// Map for fast lookup if IDs are sparse, but here IDs are dense 0..N
		// We use vector index = archetype ID for simplicity/speed
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
		
		void spawnRocket();
		
		float mSpawnTimer = 0.0f;
		
		bool  bPaused = false;
		float mSpawnScale = 1.0f;
		float mSkyIntensity = 1.0f;

		
		// mParticles removed - now inside System->Groups
		std::unique_ptr<QueueThreadPool> mUpdateThreadPool;

		FireworksSystem mFireworksSystem;

		FireworksProgram* mProgFireworks;
		NightSkyProgram*  mProgNightSky;


		// Bloom Resources
		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef mBloomFrameBuffer;

		FliterProgram* mProgFliter;
		FliterAddProgram* mProgFliterAdd;
		DownsampleProgram* mProgDownsample;
		BloomSetupProgram* mProgBloomSetup;
		TonemapProgram*    mProgTonemap;


		TArray< Vector4 > mWeightData;
		TArray< Vector2 > mUVOffsetData;
		float mBloomThreshold = 0.5f;
		float mBloomIntensity = 2.6f;
		float blurRadiusScale = 1.5f;

		int generateFliterData(int imageSize, Vector2 const& offsetDir, LinearColor const& bloomTint, float bloomRadius);

		struct InstanceData
		{
			Vector3 pos;
			float   radius;
			Vector3 color;
		};

		RHIBufferRef mInstanceBuffer;
		RHIInputLayoutRef mInstancedInputLayout;

	};
}
