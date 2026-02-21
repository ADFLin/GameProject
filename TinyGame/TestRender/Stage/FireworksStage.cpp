#include "TestRenderPCH.h"
#include "FireworksStage.h"
#include "RHI/DrawUtility.h"
#include "Widget/WidgetUtility.h"
#include <mutex>
#include "RHI/RHIGraphics2D.h"
#include "Renderer/RenderTargetPool.h"
#include "DrawEngine.h"

namespace Render
{
	REGISTER_STAGE_ENTRY("Fireworks Test", FireworksStage, EExecGroup::GraphicsTest, "Graphics");
	IMPLEMENT_SHADER_PROGRAM(FireworksProgram);
	IMPLEMENT_SHADER_PROGRAM(NightSkyProgram);



	// Add HSVToRGB helper
	static Vector3 HSVToRGB(float h, float s, float v)
	{
		int i = int(h * 6);
		float f = h * 6 - i;
		float p = v * (1 - s);
		float q = v * (1 - f * s);
		float t = v * (1 - (1 - f) * s);
		switch (i % 6)
		{
		case 0: return Vector3(v, t, p);
		case 1: return Vector3(q, v, p);
		case 2: return Vector3(p, v, t);
		case 3: return Vector3(p, q, v);
		case 4: return Vector3(t, p, v);
		case 5: return Vector3(v, p, q);
		}
		return Vector3(1, 1, 1);
	}

	static float RandomFloat()
	{
		return float(Global::Random()) / float(RAND_MAX);
	}

	static float RandomRange(float min, float max)
	{
		return min + (max - min) * RandomFloat();
	}

	static Vector3 RandomUnitVector()
	{
		Vector3 dir;
		do {
			dir = Vector3(RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f));
		} while (dir.length2() > 1.0f || dir.length2() < 0.001f);
		dir.normalize();
		return dir;
	}

	// --- ParticleGroup Implementation ---

	void ParticleGroup::addParticle(Vector3 const& pos, Vector3 const& vel, Vector3 const& color, float life)
	{
		Particle p;
		p.pos = pos;
		p.vel = vel;
		p.color = color;
		p.life = life;
		p.maxLife = life;
		mParticles.push_back(p);
	}

	void ParticleGroup::flushPending()
	{
		mParticles.append(mPendingParticles);
		mPendingParticles.clear();
		// Pending explosions are handled by System/Stage externally
		mPendingExplosions.clear();
	}


	// --- FireworksSystem Implementation ---
	
	void FireworksSystem::registerArchetype(int id, FireworkArchetype const& archetype)
	{
		FireworkArchetype archCopy = archetype;
		archCopy.id = id;

		// Ensure vector assumes id as index
		if (id >= mGroups.size())
		{
			mGroups.resize(id + 1);
		}
		mGroups[id] = std::make_unique<ParticleGroup>(archCopy);
	}

	ParticleGroup* FireworksSystem::getGroup(int id)
	{
		if (id < 0 || id >= mGroups.size()) return nullptr;
		return mGroups[id].get();
	}

	void FireworksSystem::createParticle(int archetypeId, Vector3 const& pos, Vector3 const& vel, Vector3 const& color)
	{
		if (ParticleGroup* group = getGroup(archetypeId))
		{
			const FireworkArchetype& arch = group->getArchetype();
			float life = RandomRange(arch.lifeMin, arch.lifeMax);
			group->addParticle(pos, vel, color, life);
		}
	}


	// --- Explosion Strategies ---

	class SphereBurst : public IExplosionStrategy
	{
	public:
		int countMin = 200;
		int countMax = 400;
		float speedMin = 20.0f;
		float speedMax = 50.0f;
		int childArchetypeId = -1;
		bool bSparkle = false;
		Vector3 colorOverride = Vector3(-1, -1, -1);

		void explode(const Particle& parent, FireworksSystem& system, Vector3 const& pos, Vector3 const& vel, Vector3 const& color) const override
		{
			if (childArchetypeId < 0) return;
			int count = int(RandomRange((float)countMin, (float)countMax));
			Vector3 baseColor = (colorOverride.x >= 0) ? colorOverride : color * 4.5f;

			for (int i = 0; i < count; ++i)
			{
				Vector3 dir = RandomUnitVector();
				float speed = RandomRange(speedMin, speedMax);
				
				Vector3 pColor = baseColor;
				if (bSparkle && RandomFloat() < 0.2f) pColor = Vector3(5, 5, 5);

				Vector3 pVel = vel * 0.2f + dir * speed;
				system.createParticle(childArchetypeId, pos, pVel, pColor);
			}
		}
	};

	class RingBurst : public IExplosionStrategy
	{
	public:
		int count = 60;
		float speed = 12.0f;
		int childArchetypeId = -1;

		void explode(const Particle& parent, FireworksSystem& system, Vector3 const& pos, Vector3 const& vel, Vector3 const& color) const override
		{
			if (childArchetypeId < 0) return;
			Vector3 axis = RandomUnitVector();
			Vector3 t, b;
			if (abs(axis.z) < 0.9) t = Vector3(0, 0, 1); else t = Vector3(1, 0, 0);
			b = axis.cross(t); b.normalize();
			t = b.cross(axis); t.normalize();

			Vector3 baseColor = color * 4.5f;

			for (int i = 0; i < count; ++i)
			{
				float angle = (float(i) / count) * 6.283185f;
				Vector3 dir = t * cos(angle) + b * sin(angle);
				Vector3 pVel = vel * 0.1f + dir * RandomRange(speed * 0.8f, speed * 1.2f);
				system.createParticle(childArchetypeId, pos, pVel, baseColor);
			}
		}
	};

	class ShapeBurst : public IExplosionStrategy
	{
	public:
		enum Type { Cross, Palm, Spiral, Spider, Willow };
		Type type = Cross;
		int childArchetypeId = -1;

		void explode(const Particle& parent, FireworksSystem& system, Vector3 const& pos, Vector3 const& vel, Vector3 const& color) const override
		{
			if (childArchetypeId < 0) return;
			Vector3 baseColor = color * 4.5f;
			if (type == Palm) baseColor = Vector3(1.0f, 0.6f, 0.2f) * 5.0f; 
			if (type == Willow) baseColor = Vector3(8.0f, 5.5f, 1.5f); 

			auto Spawn = [&](Vector3 dir, float speed)
			{
				Vector3 pVel = vel * 0.1f + dir * speed;
				system.createParticle(childArchetypeId, pos, pVel, baseColor);
			};

			if (type == Cross)
			{
				int arms = 6;
				int perArm = 15;
				Vector3 armDirRef = RandomUnitVector();
				// Create orthonormal basis for consistent cross
				// Correction: Cross should be 3D or 2D? "Cross" usually 3D axes or 6 directions.
				// Let's do 3 axes (6 directions).
				Vector3 axes[] = { Vector3(1,0,0), Vector3(0,1,0), Vector3(0,0,1) };
				// Rotate axes randomly?
				// Simple random rotation:
				// Just spawn lines.
				for (int a = 0; a < arms; ++a)
				{
					Vector3 armDir = RandomUnitVector(); 
					// Ideally we want structured shape. 
					// Let's assume RandomUnitVector is fine for "Star/SeaUrchin" shape actually.
					// If strict cross, we need fixed axes.
					// Let's use random arms for now as per previous logic.
					for (int i = 0; i < perArm; ++i)
					{
						float speed = 5.0f + (float(i) / perArm) * 15.0f;
						Spawn(armDir, speed);
					}
				}
			}
			else if (type == Palm)
			{
				int branches = 6;
				for (int i = 0; i < branches; ++i)
				{
					float angle = (float(i) / branches) * 6.28f;
					Vector3 dir(cos(angle), sin(angle), 0.5f);
					dir.normalize();
					for (int j = 0; j < 20; ++j)
					{
						Spawn(dir, float(j) * 1.5f + 5.0f);
					}
				}
			}
			else if (type == Spiral)
			{
				int strands = 4;
				int perStrand = 40;
				for (int s = 0; s < strands; ++s)
				{
					Vector3 axis = RandomUnitVector();
					Vector3 t, b;
					if (abs(axis.z) < 0.9) t = Vector3(0, 0, 1); else t = Vector3(1, 0, 0);
					b = axis.cross(t); b.normalize(); t = b.cross(axis); t.normalize();

					for (int i = 0; i < perStrand; ++i)
					{
						float angle = (float(i) / perStrand) * 12.0f + (float(s) / strands) * 6.28f;
						float dist = (float(i) / perStrand) * 20.0f;
						Vector3 dir = (t * cos(angle) + b * sin(angle) + axis * 0.5f);
						dir.normalize();
						Spawn(dir, 5.0f + dist);
					}
				}
			}
			else if (type == Spider)
			{
				for (int i = 0; i < 40; ++i) Spawn(RandomUnitVector(), RandomRange(30.0f, 45.0f));
			}
			else if (type == Willow)
			{
				for (int i = 0; i < 200; ++i) 
				{
					Vector3 dir(RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), RandomRange(0.0f, 1.0f));
					dir.normalize();
					Spawn(dir, RandomRange(5.0f, 12.0f));
				}
			}
		}
	};

	class ClusterBurst : public IExplosionStrategy
	{
	public:
		int clusters = 30;
		int childArchetypeId = -1;

		void explode(const Particle& parent, FireworksSystem& system, Vector3 const& pos, Vector3 const& vel, Vector3 const& color) const override
		{
			if (childArchetypeId < 0) return;

			for (int i = 0; i < clusters; ++i)
			{
				Vector3 dir = RandomUnitVector();
				Vector3 clusterPos = pos + dir * RandomRange(10.0f, 25.0f);
				Vector3 clusterColor = HSVToRGB(RandomFloat(), 0.8f, 1.0f) * 5.0f;

				// Instead of recursion or complex logic, we spawn particles at clusterPos.
				// Since we need to spawn 50 particles sharing same cluster pos/color,
				// loop createParticle.
				for (int j = 0; j < 50; ++j) 
				{
					Vector3 pDir = RandomUnitVector();
					Vector3 pVel = pDir * RandomRange(6.0f, 15.0f) + dir * 2.0f;
					system.createParticle(childArchetypeId, clusterPos, pVel, clusterColor);
				}
			}
		}
	};

	class CompositeBurst : public IExplosionStrategy
	{
	public:
		std::vector<std::shared_ptr<IExplosionStrategy>> strategies;

		void add(std::shared_ptr<IExplosionStrategy> strategy)
		{
			strategies.push_back(strategy);
		}

		void explode(const Particle& parent, FireworksSystem& system, Vector3 const& pos, Vector3 const& vel, Vector3 const& color) const override
		{
			for (auto& s : strategies)
			{
				s->explode(parent, system, pos, vel, color);
			}
		}
	};


	// --- Archetype ID Constants ---
	enum EArchetypeID
	{
		Arch_Rocket_Base = 0,
		Arch_Star_Simple,
		Arch_Star_Ring,
		Arch_Star_Heavy,
		Arch_Star_Willlow,
		Arch_Star_Spider,
		Arch_Star_Fish,
		Arch_Star_Strobe,
		Arch_Star_Palm,
		Arch_Star_Cluster,
		
		Arch_Trail_Simple,
		Arch_Trail_Long,
		
		Arch_Max
	};


	bool FireworksStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();

		mCamera.lookAt(Vector3(0, -150, 150), Vector3(0, 0, 80), Vector3(0, 0, 1));
		
		mUpdateThreadPool = std::make_unique<QueueThreadPool>();
		mUpdateThreadPool->init(SystemPlatform::GetProcessorNumber() - 1);


		// --- Register Archetypes ---

		// 1. Trails
		{
			FireworkArchetype arch;
			arch.gravity = Vector3(0, 0, 0); 
			arch.drag = 0.2f; // Keep drag for trails
			arch.lifeMin = 0.4f; arch.lifeMax = 0.4f;
			arch.size = 0.08f;
			mFireworksSystem.registerArchetype(Arch_Trail_Simple, arch);

			arch.lifeMin = 1.2f; arch.lifeMax = 1.2f; 
			mFireworksSystem.registerArchetype(Arch_Trail_Long, arch);
		}

		// 2. Stars
		{
			FireworkArchetype arch;
			// Simple Star
			arch.gravity = Vector3(0, 0, -4.0f);
			arch.drag = 0.4f;
			arch.lifeMin = 1.0f; arch.lifeMax = 2.5f;
			arch.trailChance = 0.1f; arch.trailArchetypeId = Arch_Trail_Simple;
			mFireworksSystem.registerArchetype(Arch_Star_Simple, arch);

			// Willow Star
			arch.drag = 0.55f; 
			arch.gravity = Vector3(0, 0, -3.0f);
			arch.lifeMin = 2.0f; arch.lifeMax = 3.5f;
			arch.trailChance = 0.5f; arch.trailArchetypeId = Arch_Trail_Long; 
			mFireworksSystem.registerArchetype(Arch_Star_Willlow, arch);
			
			// Fish Star
			arch.drag = 0.4f;
			arch.noiseStrength = 25.0f; 
			arch.lifeMin = 2.0f; arch.lifeMax = 3.0f;
			arch.trailChance = 0.8f;
			mFireworksSystem.registerArchetype(Arch_Star_Fish, arch);

			// Spider Star (Fast + Noise)
			FireworkArchetype spiderArch = arch; 
			spiderArch.noiseStrength = 40.0f; 
			spiderArch.drag = 0.5f;
			spiderArch.trailArchetypeId = Arch_Trail_Simple;
			spiderArch.trailChance = 0.1f;
			mFireworksSystem.registerArchetype(Arch_Star_Spider, spiderArch);

			// Strobe
			FireworkArchetype strobeArch = arch;
			strobeArch.noiseStrength = 0;
			strobeArch.trailChance = 0.1f;
			mFireworksSystem.registerArchetype(Arch_Star_Strobe, strobeArch);

			// Cluster Star (High Trail)
			FireworkArchetype clusterArch = arch;
			clusterArch.trailChance = 0.95f; 
			clusterArch.lifeMin = 1.0f; clusterArch.lifeMax = 1.5f;
			clusterArch.trailArchetypeId = Arch_Trail_Simple;
			mFireworksSystem.registerArchetype(Arch_Star_Cluster, clusterArch);
		}


		// 3. Rockets
		auto RegisterRocket = [&](int id, std::shared_ptr<IExplosionStrategy> strategy)
		{
			FireworkArchetype arch;
			arch.bIsRocket = true;
			arch.gravity = Vector3(0, 0, -8.0f);
			arch.drag = 1.0f; 
			arch.lifeMin = 6.0f; arch.lifeMax = 6.0f;
			arch.trailChance = 1.0f; 
			arch.trailArchetypeId = Arch_Trail_Simple;
			arch.explosionStrategy = strategy;
			arch.size = 0.45f;
			mFireworksSystem.registerArchetype(id, arch);
		};

		// Define Strategies
		auto sSphere = std::make_shared<SphereBurst>(); sSphere->childArchetypeId = Arch_Star_Simple; sSphere->countMin = 200; sSphere->countMax = 400;
		auto sRing = std::make_shared<RingBurst>(); sRing->childArchetypeId = Arch_Star_Simple;
		
		auto sDouble = std::make_shared<CompositeBurst>();
		auto sCore = std::make_shared<SphereBurst>(); sCore->countMin = 50; sCore->countMax = 50; sCore->speedMax = 6.0f; sCore->colorOverride = Vector3(6,6,6); sCore->childArchetypeId = Arch_Star_Simple;
		auto sShell = std::make_shared<SphereBurst>(); sShell->countMin = 150; sShell->countMax = 200; sShell->speedMin = 12.0f; sShell->childArchetypeId = Arch_Star_Simple;
		sDouble->add(sCore); sDouble->add(sShell);

		auto sWillow = std::make_shared<ShapeBurst>(); sWillow->type = ShapeBurst::Willow; sWillow->childArchetypeId = Arch_Star_Willlow;

		auto sFish = std::make_shared<SphereBurst>(); sFish->childArchetypeId = Arch_Star_Fish;
		
		auto sStrobe = std::make_shared<SphereBurst>(); sStrobe->childArchetypeId = Arch_Star_Strobe; sStrobe->colorOverride = Vector3(10,10,10);

		auto sCross = std::make_shared<ShapeBurst>(); sCross->type = ShapeBurst::Cross; sCross->childArchetypeId = Arch_Star_Simple;
		auto sPalm = std::make_shared<ShapeBurst>(); sPalm->type = ShapeBurst::Palm; sPalm->childArchetypeId = Arch_Star_Simple;
		auto sSpider = std::make_shared<ShapeBurst>(); sSpider->type = ShapeBurst::Spider; sSpider->childArchetypeId = Arch_Star_Spider; // Use specialized Spider arch
		auto sSpiral = std::make_shared<ShapeBurst>(); sSpiral->type = ShapeBurst::Spiral; sSpiral->childArchetypeId = Arch_Star_Simple;

		auto sCluster = std::make_shared<ClusterBurst>(); sCluster->childArchetypeId = Arch_Star_Cluster;

		// Register Rockets with different IDs
		RegisterRocket(100 + 0, sSphere);
		RegisterRocket(100 + 1, sRing);
		RegisterRocket(100 + 2, sDouble);
		RegisterRocket(100 + 3, sCross);
		RegisterRocket(100 + 4, sWillow);
		RegisterRocket(100 + 5, sSpider);
		RegisterRocket(100 + 6, sSpiral);
		RegisterRocket(100 + 7, sFish);
		RegisterRocket(100 + 8, sStrobe);
		RegisterRocket(100 + 9, sPalm);
		RegisterRocket(100 + 10, sCluster);
		
		::Global::GUI().cleanupWidget();
		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addCheckBox("Pause", bPaused);
		GSlider* slider = frame->addSlider("Spawn Scale");
		FWidgetProperty::Bind(slider, mSpawnScale, 0.1f, 10.0f);

		GSlider* bloomThresholdSlider = frame->addSlider("Bloom Threshold");
		FWidgetProperty::Bind(bloomThresholdSlider, mBloomThreshold, 0.0f, 2.0f);
		GSlider* bloomIntensitySlider = frame->addSlider("Bloom Intensity");
		FWidgetProperty::Bind(bloomIntensitySlider, mBloomIntensity, 0.0f, 5.0f);
		GSlider* blurRadiusSlider = frame->addSlider("Blur Radius");
		FWidgetProperty::Bind(blurRadiusSlider, blurRadiusScale, 0.1f, 10.0f);
		GSlider* skyIntensitySlider = frame->addSlider("Sky Intensity");
		FWidgetProperty::Bind(skyIntensitySlider, mSkyIntensity, 0.0f, 2.0f);


		spawnRocket();

		return true;
	}

	void FireworksStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 800;
		systemConfigs.bUseRenderThread = false;
	}

	bool FireworksStage::setupRenderResource(ERenderSystem systemName)
	{
		if (!BaseClass::setupRenderResource(systemName))
			return false;

		if (!SharedAssetData::loadCommonShader())
			return false;

		for (auto& mesh : mSimpleMeshs)
		{
			mesh.mInputLayoutDesc = InputLayoutDesc();
		}

		if (!SharedAssetData::createSimpleMesh())
			return false;

		if (!mInstancedInputLayout.isValid())
		{
			InputLayoutDesc desc;
			desc.addElement(0, EVertex::ATTRIBUTE_POSITION, EVertex::Float3);
			desc.addElement(1, EVertex::ATTRIBUTE1, EVertex::Float3, false, true, 1);
			desc.addElement(1, EVertex::ATTRIBUTE2, EVertex::Float1, false, true, 1);
			desc.addElement(1, EVertex::ATTRIBUTE3, EVertex::Float3, false, true, 1);
			mInstancedInputLayout = RHICreateInputLayout(desc);
		}

		VERIFY_RETURN_FALSE(mProgNightSky = ShaderManager::Get().getGlobalShaderT<NightSkyProgram>(true));
		VERIFY_RETURN_FALSE(mProgFireworks = ShaderManager::Get().getGlobalShaderT<FireworksProgram>(true));

		mSceneRenderTargets.initializeRHI();
		mBloomFrameBuffer = RHICreateFrameBuffer();
		VERIFY_RETURN_FALSE(mProgFliter = ShaderManager::Get().getGlobalShaderT< FliterProgram >());
		VERIFY_RETURN_FALSE(mProgFliterAdd = ShaderManager::Get().getGlobalShaderT< FliterAddProgram >());
		VERIFY_RETURN_FALSE(mProgDownsample = ShaderManager::Get().getGlobalShaderT< DownsampleProgram >());
		VERIFY_RETURN_FALSE(mProgBloomSetup = ShaderManager::Get().getGlobalShaderT< BloomSetupProgram >());
		VERIFY_RETURN_FALSE(mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >());


		return true;
	}


	void FireworksStage::preShutdownRenderSystem(bool bReInit)
	{
		BaseClass::preShutdownRenderSystem(bReInit);
		mSceneRenderTargets.releaseRHI();
		mBloomFrameBuffer.release();
		GRenderTargetPool.releaseRHI();

		mProgFireworks = nullptr;
		mProgNightSky = nullptr;
		mProgFliter = nullptr;
		mProgFliterAdd = nullptr;
		mProgDownsample = nullptr;
		mProgBloomSetup = nullptr;
		mProgTonemap = nullptr;
	}

	void FireworksStage::onUpdate(GameTimeSpan deltaTime)
	{
		struct UpdateThreadContext
		{
			struct NewParticle
			{
				int archetypeId;
				Vector3 pos;
				Vector3 vel;
				Vector3 color;
				float life;
			};
			struct PendingExplosion
			{
				Vector3 pos;
				Vector3 vel;
				Vector3 color;
				float   life; 
			};
			
			TArray<NewParticle> newParticles;
			TArray<PendingExplosion> explosions;
		};

		BaseClass::onUpdate(deltaTime);

		if (bPaused)
			return;

		mSpawnTimer += deltaTime * mSpawnScale;
		if (mSpawnTimer > 0.15f)
		{
			mSpawnTimer = 0.0f;
			spawnRocket();
		}

		// Calculate total tasks
		int chunkSize = 2048;
		struct TaskInfo
		{
			ParticleGroup* group;
			int startIndex;
			int count;
			int contextIndex;
		};
		std::vector<TaskInfo> tasks;
		
		for(auto& groupPtr : mFireworksSystem.mGroups)
		{
			if (!groupPtr || groupPtr->mParticles.empty()) continue;
			ParticleGroup& group = *groupPtr;
			int total = (int)group.mParticles.size();
			for(int i = 0; i < total; i += chunkSize)
			{
				TaskInfo task;
				task.group = &group;
				task.startIndex = i;
				task.count = std::min(chunkSize, total - i);
				task.contextIndex = (int)tasks.size();
				tasks.push_back(task);
			}
		}

		std::vector<UpdateThreadContext> contexts(tasks.size());

		if (!tasks.empty())
		{
			for (const auto& task : tasks)
			{
				mUpdateThreadPool->addFunctionWork([task, deltaTime, &contexts]()
				{
					UpdateThreadContext& ctx = contexts[task.contextIndex];
					ParticleGroup& group = *task.group;
					const FireworkArchetype& arch = group.mArchetype;
					
					// Optimization: Cache collision/trail params
					bool bHasTrail = (arch.trailArchetypeId >= 0);
					bool bIsRocket = arch.bIsRocket;
					float trailChance = arch.trailChance;
					
					Particle* pData = group.mParticles.data() + task.startIndex;
					
					for(int i = 0; i < task.count; ++i)
					{
						Particle& p = pData[i];
						p.pos += p.vel * deltaTime;
						p.vel += arch.gravity * deltaTime;
						p.vel *= pow(arch.drag, (float)deltaTime);
						
						if (arch.noiseStrength > 0.01f)
						{
							p.vel += Vector3(RandomRange(-1,1), RandomRange(-1,1), RandomRange(-1,1)) * arch.noiseStrength * deltaTime;
						}

						p.life -= deltaTime;
						
						if (bHasTrail)
						{
							if (RandomFloat() < trailChance)
							{
								// 0 life implies "use default from archetype"
								// Passing -1.0f as life to let system decide (or random range in system)
								// But wait, createParticle currently doesn't look up system inside here.
								// We just store request.
								ctx.newParticles.push_back({arch.trailArchetypeId, p.pos, Vector3(0,0,0), p.color * 0.5f, -1.0f});
							}
						}

						bool bDead = p.life <= 0;
						if (bIsRocket)
						{
							if (p.vel.z < 0 || bDead)
							{
								if (p.life >= -1.0f)
								{
									ctx.explosions.push_back({p.pos, p.vel, p.color, p.life});
									p.life = -5.0f;
								}
							}
						}
					}
				});
			}

			mUpdateThreadPool->waitAllWorkComplete(true);

			// Merge Results
			for (const auto& ctx : contexts)
			{
				for(const auto& req : ctx.newParticles) 
				{
					mFireworksSystem.createParticle(req.archetypeId, req.pos, req.vel, req.color);
				}
			}
			
			// Re-iterate tasks to identify correct explosion strategy
			for(int i=0; i < tasks.size(); ++i)
			{
				const auto& task = tasks[i];
				const auto& ctx = contexts[task.contextIndex];
				const FireworkArchetype& arch = task.group->mArchetype; // Retrieve archetype from task
				
				if (arch.explosionStrategy)
				{
					for(const auto& exp : ctx.explosions)
					{
						// Create a dummy parent particle for execute
						Particle p; 
						p.pos = exp.pos; p.vel = exp.vel; p.color = exp.color; p.life = exp.life;
						arch.explosionStrategy->explode(p, mFireworksSystem, exp.pos, exp.vel, exp.color);
					}
				}
			}

			// Cleanup Dead
			for (auto& groupPtr : mFireworksSystem.mGroups)
			{
				if (groupPtr)
					groupPtr->mParticles.removeAllSwapPred([](Particle const& p) { return p.life <= 0; });
			}
		}
	}

	void FireworksStage::spawnRocket()
	{
		int rocketId = 100 + (::Global::Random() % 11);
		
		Vector3 pos = Vector3(RandomRange(-15.0f, 15.0f), RandomRange(-15.0f, 15.0f), -10.0f);
		Vector3 vel = Vector3(RandomRange(-3.0f, 3.0f), RandomRange(-3.0f, 3.0f), RandomRange(25.0f, 35.0f));
		vel.z *= 1.2f;
		
		Vector3 color = HSVToRGB(RandomFloat(), 0.7f + 0.3f*RandomFloat(), 1.0f);
		color = color * 2.0f; 

		mFireworksSystem.createParticle(rocketId, pos, vel, color);
	}


	static int generateGaussianWeights(float kernelRadius, Vector2 outData[])
	{
		constexpr int MaxWeightNum = 64;

		int samples = int(kernelRadius * 2 + 1);
		if (samples % 2 == 0) samples++;
		samples = Math::Min(samples, MaxWeightNum);
		if (samples % 2 == 0) samples--;

		float sigma = kernelRadius / 2.0f;
		float sum = 0;
		int center = samples / 2;
		for (int i = 0; i < samples; ++i)
		{
			float offset = float(i - center);
			float w = exp(-(offset*offset) / (2 * sigma * sigma));
			outData[i] = Vector2(w, offset);
			sum += w;
		}
		for (int i = 0; i < samples; ++i)
		{
			outData[i].x /= sum;
		}
		return samples;
	}

	int FireworksStage::generateFliterData(int imageSize, Vector2 const& offsetDir, LinearColor const& bloomTint, float bloomRadius)
	{
		Vector2 weightAndOffset[128];
		int numSamples = generateGaussianWeights(bloomRadius, weightAndOffset);

		mWeightData.resize(128);
		mUVOffsetData.resize(128);

		for (int i = 0; i < numSamples; ++i)
		{
			mWeightData[i] = weightAndOffset[i].x * Vector4(bloomTint);
			mUVOffsetData[i] = (weightAndOffset[i].y / float(imageSize)) * offsetDir;
		}
		return numSamples;
	}

	void FireworksStage::onRender(float dFrame)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		GRenderTargetPool.freeAllUsedElements();

		initializeRenderState();

		mSceneRenderTargets.prepare(screenSize);
		mSceneRenderTargets.attachDepthTexture();

		{
			RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);

			// Draw Sky Background
			{
				GPU_PROFILE("NightSky");
				RHISetShaderProgram(commandList, mProgNightSky->getRHI());
				mView.setupShader(commandList, *mProgNightSky);
				SET_SHADER_PARAM(commandList, *mProgNightSky, MoonDir, Vector3(0.1, 0.2, 0.8).getNormal());
				SET_SHADER_PARAM(commandList, *mProgNightSky, SkyIntensity, mSkyIntensity);


				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				DrawUtility::ScreenRect(commandList);
			}

			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
			DrawUtility::AixsLine(commandList);

			RHISetBlendState(commandList, TStaticBlendState<Render::CWM_RGBA, Render::EBlend::SrcAlpha, Render::EBlend::One>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<false>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<Render::ECullMode::None>::GetRHI());

			RHISetShaderProgram(commandList, mProgFireworks->getRHI());
			mView.setupShader(commandList, *mProgFireworks);

			int totalParticles = 0;
			for (auto& group : mFireworksSystem.mGroups)
			{
				if(group) totalParticles += group->mParticles.size();
			}

			if (totalParticles > 0)
			{
				// Ensure buffer size
				if (!mInstanceBuffer.isValid() || mInstanceBuffer->getSize() < totalParticles * sizeof(InstanceData))
				{
					mInstanceBuffer = RHICreateVertexBuffer(sizeof(InstanceData), Math::Max((size_t)totalParticles + 4096, size_t(2048)), BCF_CpuAccessWrite);
				}

				// Map buffer once, fill sequentially
				InstanceData* pData = (InstanceData*)RHILockBuffer(mInstanceBuffer, ELockAccess::WriteDiscard);
				if (pData)
				{
					int bufferOffset = 0;
					
					// Draw each group
					for (auto& groupPtr : mFireworksSystem.mGroups)
					{
						if (!groupPtr || groupPtr->mParticles.empty()) continue;
						ParticleGroup& group = *groupPtr;
						const FireworkArchetype& arch = group.mArchetype;
						float baseRadius = arch.size;
						bool isRocket = arch.bIsRocket;
						bool isStrobe = (arch.id == Arch_Star_Strobe); 

						int count = group.mParticles.size();
						for (int i=0; i<count; ++i)
						{
							const Particle& p = group.mParticles[i];
							
							float alpha = (p.life / p.maxLife);
							if (isRocket) alpha = 1.0f; 
							alpha = alpha * alpha;

							if (isStrobe)
							{
								if (int(p.life * 15) % 2 == 0) alpha = 0;
							}

							pData[bufferOffset + i].pos = p.pos;
							pData[bufferOffset + i].radius = baseRadius;
							pData[bufferOffset + i].color = p.color * alpha;
						}
						
						// Issue Draw Call for this batch?
						// Wait, cannot draw while mapped if we use same buffer?
						// Usually: Map -> Fill All -> Unmap -> Draw.
						// But we want to draw PER GROUP if we had different shaders.
						// Currently same shader. We CAN batch all into one draw call if they use same geometry.
						// BUT they might need different Uniforms later?
						// For now, same shader. Let's just fill ALL then use `DrawInstanced(..., baseInstance)`?
						// RHI might not support baseInstance easily in high level wrapper.
						// Simple approach: Fill ALL, then Loop Draw with offsets.
						
						bufferOffset += count;
					}
					RHIUnlockBuffer(mInstanceBuffer);

					// Draw Loop
					int currentStart = 0;
					auto& mesh = mSimpleMeshs[SimpleMeshId::SpherePlane];
					
					InputStreamInfo inputStreams[2];
					inputStreams[0].buffer = mesh.mVertexBuffer;
					inputStreams[0].stride = mesh.mInputLayoutDesc.getVertexSize(0);
					inputStreams[0].offset = 0;
					inputStreams[1].buffer = mInstanceBuffer;
					inputStreams[1].stride = sizeof(InstanceData);

					for (auto& groupPtr : mFireworksSystem.mGroups)
					{
						if (!groupPtr || groupPtr->mParticles.empty()) continue;
						
						int count = groupPtr->mParticles.size();
						
						// Setup Uniforms per group if needed (e.g. texture)
						// Currently generic.
						
						// Set Stream Offset for Instance Buffer?
						// RHISetInputStream supports offset.
						inputStreams[1].offset = currentStart * sizeof(InstanceData);
						
						RHISetInputStream(commandList, mInstancedInputLayout, inputStreams, 2);
						RHISetIndexBuffer(commandList, mesh.mIndexBuffer);
						RHIDrawIndexedPrimitiveInstanced(commandList, mesh.mType, 0, mesh.mIndexBuffer->getNumElements(), count);
						
						currentStart += count;
					}
				}
			}
		}

		RHITexture2DRef postProcessRT;
		// Bloom Pass
		{
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			RHITexture2DRef downsampleTextures[6];

			{
				auto DownsamplePass = [this](RHICommandList& commandList, int index, RHITexture2D& sourceTexture) -> RHITexture2DRef
				{
					RenderTargetDesc desc;
					InlineString<128> str;
					str.format("Downsample(%d)", index);

					Vec2i size;
					size.x = (sourceTexture.getSizeX() + 1) / 2;
					size.y = (sourceTexture.getSizeY() + 1) / 2;
					desc.debugName = str;
					desc.size = size;
					desc.format = ETexture::FloatRGBA;
					desc.creationFlags = TCF_CreateSRV;
					RHITexture2DRef downsampleTexture = static_cast<RHITexture2D*>(GRenderTargetPool.fetchElement(desc)->texture.get());
					mBloomFrameBuffer->setTexture(0, *downsampleTexture);
					RHISetViewport(commandList, 0, 0, size.x, size.y);
					RHISetFrameBuffer(commandList, mBloomFrameBuffer);
					RHISetShaderProgram(commandList, mProgDownsample->getRHI());
					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgDownsample, Texture, sourceTexture, sampler);
					SET_SHADER_PARAM(commandList, *mProgDownsample, ExtentInverse, Vector2(1.0f / (size.x), 1.0f / (size.y)));

					DrawUtility::ScreenRect(commandList);
					return downsampleTexture;
				};

				RHITexture2DRef halfSceneTexture;
				{
					halfSceneTexture = DownsamplePass(commandList, 0, mSceneRenderTargets.getFrameTexture());
				}

				{
					RenderTargetDesc desc;
					Vec2i size = Vec2i(halfSceneTexture->getSizeX(), halfSceneTexture->getSizeY());
					desc.debugName = "BloomSetup";
					desc.size = size;
					desc.format = ETexture::FloatRGBA;
					desc.creationFlags = TCF_CreateSRV;

					PooledRenderTargetRef bloomSetupRT = GRenderTargetPool.fetchElement(desc);

					mBloomFrameBuffer->setTexture(0, *bloomSetupRT->texture);
					RHISetViewport(commandList, 0, 0, size.x, size.y);
					RHISetFrameBuffer(commandList, mBloomFrameBuffer);
					RHISetShaderProgram(commandList, mProgBloomSetup->getRHI());

					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
					SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgBloomSetup, Texture, *halfSceneTexture, sampler);
					SET_SHADER_PARAM(commandList, *mProgBloomSetup, BloomThreshold, mBloomThreshold);

					DrawUtility::ScreenRect(commandList);

					downsampleTextures[0] = static_cast<RHITexture2D*>(bloomSetupRT->texture.get());
				}

				{

					for (int i = 1; i < ARRAY_SIZE(downsampleTextures); ++i)
					{
						downsampleTextures[i] = DownsamplePass(commandList, i, *downsampleTextures[i - 1]);
					}
				}
			}

			{
				auto BlurPass = [this](RHICommandList& commandList, int index, RHITexture2D& fliterTexture, RHITexture2D& bloomTexture, LinearColor const& tint, float blurSize)
				{
					int numSamples = 0;
					float bloomRadius = blurSize * 0.01 * 0.5 * fliterTexture.getSizeX() * blurRadiusScale;

					auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();

					IntVector2 sizeH = IntVector2(fliterTexture.getSizeX(), fliterTexture.getSizeY()); ;

					InlineString<128> str;
					RenderTargetDesc desc;
					str.format("BlurH(%d)", index);
					desc.debugName = str;
					desc.size = sizeH;
					desc.format = ETexture::FloatRGBA;
					desc.creationFlags = TCF_CreateSRV;
					PooledRenderTargetRef blurXRT = GRenderTargetPool.fetchElement(desc);
					mBloomFrameBuffer->setTexture(0, *blurXRT->texture);
					{
						RHISetFrameBuffer(commandList, mBloomFrameBuffer);
						RHISetViewport(commandList, 0, 0, sizeH.x, sizeH.y);
						RHISetShaderProgram(commandList, mProgFliter->getRHI());
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgFliter, FliterTexture, fliterTexture, sampler);
						numSamples = generateFliterData(fliterTexture.getSizeX(), Vector2(1, 0), LinearColor(1, 1, 1, 1), bloomRadius);
						mProgFliter->setParam(commandList, mProgFliter->mParamWeights, mWeightData.data(), MaxWeightNum);
						mProgFliter->setParam(commandList, mProgFliter->mParamUVOffsets, (Vector4*)mUVOffsetData.data(), MaxWeightNum / 2);
						mProgFliter->setParam(commandList, mProgFliter->mParamWeightNum, numSamples);
						DrawUtility::ScreenRect(commandList);

					}

					IntVector2 sizeV = IntVector2(fliterTexture.getSizeX(), fliterTexture.getSizeY()); ;

					str.format("BlurV(%d)", index);
					desc.debugName = str;
					PooledRenderTargetRef blurYRT = GRenderTargetPool.fetchElement(desc);
					mBloomFrameBuffer->setTexture(0, *blurYRT->texture);
					{
						RHISetFrameBuffer(commandList, mBloomFrameBuffer);
						RHISetViewport(commandList, 0, 0, sizeV.x, sizeV.y);
						RHISetShaderProgram(commandList, mProgFliterAdd->getRHI());

						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgFliterAdd, FliterTexture, *blurXRT->texture, sampler);
						SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgFliterAdd, AddTexture, bloomTexture, sampler);
						numSamples = generateFliterData(fliterTexture.getSizeY(), Vector2(0, 1), tint, bloomRadius);

						mProgFliterAdd->setParam(commandList, mProgFliterAdd->mParamWeights, mWeightData.data(), MaxWeightNum);
						mProgFliterAdd->setParam(commandList, mProgFliterAdd->mParamUVOffsets, (Vector4*)mUVOffsetData.data(), MaxWeightNum / 2);
						mProgFliterAdd->setParam(commandList, mProgFliterAdd->mParamWeightNum, numSamples);

						DrawUtility::ScreenRect(commandList);
					}

					return static_cast<RHITexture2D*>(blurYRT->texture.get());
				};

				struct BlurInfo
				{
					LinearColor tint;
					float size;
				};

				BlurInfo blurInfos[] =
				{
					{ LinearColor(0.3465f, 0.3465f, 0.3465f), 0.3f },
					{ LinearColor(0.138f, 0.138f, 0.138f), 1.0f },
					{ LinearColor(0.1176f, 0.1176f, 0.1176f), 2.0f },
					{ LinearColor(0.066f, 0.066f, 0.066f), 10.0f },
					{ LinearColor(0.066f, 0.066f, 0.066f), 30.0f },
					{ LinearColor(0.061f, 0.061f, 0.061f), 64.0f },
				};
				RHITexture2DRef bloomTexture = GBlackTexture2D;
				{
					float tintScale = mBloomIntensity / float(ARRAY_SIZE(downsampleTextures));


					for (int i = 0; i < ARRAY_SIZE(downsampleTextures); ++i)
					{
						int index = ARRAY_SIZE(downsampleTextures) - 1 - i;
						RHITexture2DRef fliterTexture = downsampleTextures[index];
						bloomTexture = BlurPass(commandList, i, *fliterTexture, *bloomTexture, Vector4(blurInfos[index].tint) * tintScale, blurInfos[index].size);
					}
				}

				// Tonemap & Combine
				{
					mSceneRenderTargets.swapFrameTexture();

					RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
					RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
					RHISetShaderProgram(commandList, mProgTonemap->getRHI());

					PostProcessContext context;
					context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();
					mProgTonemap->setParameters(commandList, context);
					mProgTonemap->setBloomTexture(commandList, *bloomTexture);

					DrawUtility::ScreenRect(commandList);


				}

				postProcessRT = &mSceneRenderTargets.getFrameTexture();
			}
		}

		{
			RHISetFrameBuffer(commandList, nullptr);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());

			ShaderHelper::Get().copyTextureToBuffer(commandList, *postProcessRT);
		}

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		int totalParticles = 0;
		for(auto& group : mFireworksSystem.mGroups) { if(group) totalParticles += group->mParticles.size(); }
		InlineString< 128 > str;
		str.format("Particles : %d", totalParticles);
		g.drawText(Vector2(20, 20), str);
		g.endRender();
	}

}
