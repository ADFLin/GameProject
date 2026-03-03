#include "TestRenderPCH.h"
#include "FireworksStage.h"
#include "RHI/DrawUtility.h"
#include "Widget/WidgetUtility.h"
#include "Renderer/RenderThread.h"
#include "RenderUtility.h"

#include "RHI/RHIGraphics2D.h"
#include "Renderer/RenderTargetPool.h"
#include "DrawEngine.h"
#include "ProfileSystem.h"
#include "Renderer/MeshBuild.h"
#include "Meta/Unroll.h"

// Engine FX System
#include "FX/ParticleModuleCommon.h"

#include <mutex>
#include <atomic>
#include <cstring>

namespace Render
{
	REGISTER_STAGE_ENTRY("Fireworks Test", FireworksStage, EExecGroup::GraphicsTest, "Graphics");
	IMPLEMENT_SHADER_PROGRAM(NightSkyProgram);

	// --- Helpers ---
	Vector3 HSVToRGB(float h, float s, float v)
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

	// --- Demo Specific Modules ---

	struct KillAtApexUpdate 
	{
		PS_UPDATE(ctx, p)
		{
			if (p.vel.z < 0.0f) { p.life = 0.0f; }
		}
	};
	struct StrobeAlphaUpdate 
	{
		PS_UPDATE(ctx, p)
		{ 
			float alpha = p.life / p.maxLife;
			alpha = (alpha < 0) ? 0 : alpha * alpha;
			if (int(p.life * 15) % 2 == 0) alpha = 0;
			p.color.a = alpha;
		}
	};
	struct RocketAlphaUpdate 
	{
		PS_UPDATE(ctx, p) { p.color.a = 1.0f; }
	};
	struct GhostAlphaUpdate
	{
		PS_UPDATE(ctx, p)
		{
			float ratio = p.life / p.maxLife;
			if (ratio > 0.7f) p.color.a = 0.0f;
			else p.color.a = (ratio / 0.7f) * (ratio / 0.7f);
		}
	};

	// --- Fireworks Strategies ---

	class SphereBurst : public IParticleEvent
	{
	public:
		SphereBurst(int childProfileId, int countMin = 200, int countMax = 400, float speedMin = 20.0f, float speedMax = 50.0f, Vector3 colorOverride = Vector3(-1, -1, -1), bool bSparkle = false)
			: childProfileId(childProfileId), countMin(countMin), countMax(countMax), speedMin(speedMin), speedMax(speedMax), colorOverride(colorOverride), bSparkle(bSparkle) {}

		int countMin, countMax;
		float speedMin, speedMax;
		int childProfileId;
		bool bSparkle;
		Vector3 colorOverride;

		void execute(const Particle& parent, ParticleSystem& system) const override
		{
			if (childProfileId < 0) return;
			int count = int(RandomRange((float)countMin, (float)countMax));
			Vector3 color = Vector3(parent.color.r, parent.color.g, parent.color.b);
			Vector3 baseColor = (colorOverride.x >= 0) ? colorOverride : color * 4.5f;

			system.spawnParticles(childProfileId, count, [&](ParticleRef p, int index)
			{
				Vector3 dir = RandomUnitVector();
				float speed = RandomRange(speedMin, speedMax);
				p.pos = parent.pos;
				p.color = baseColor;
				if (bSparkle && RandomFloat() < 0.2f)
				{
					p.color = Vector3(5, 5, 5);
				}
				p.vel = parent.vel * 0.2f + dir * speed;
			});
		}
	};

	class RingBurst : public IParticleEvent
	{
	public:
		RingBurst(int childProfileId, int count = 60, float speed = 12.0f)
			: childProfileId(childProfileId), count(count), speed(speed) {}
		int count;
		float speed;
		int childProfileId;
		void execute(const Particle& parent, ParticleSystem& system) const override
		{
			if (childProfileId < 0) 
				return;

			Vector3 axis = RandomUnitVector();
			Vector3 t, b;
			if (abs(axis.z) < 0.9) t = Vector3(0, 0, 1); else t = Vector3(1, 0, 0);
			b = axis.cross(t); b.normalize(); t = b.cross(axis); t.normalize();
			Vector3 color = Vector3(parent.color.r, parent.color.g, parent.color.b);
			Vector3 baseColor = color * 4.5f;
			system.spawnParticles(childProfileId, count, [&](ParticleRef p, int index)
			{
				float angle = (float(index) / count) * 6.283185f;
				Vector3 dir = t * cos(angle) + b * sin(angle);
				p.pos = parent.pos;
				p.vel = parent.vel * 0.1f + dir * RandomRange(speed * 0.8f, speed * 1.2f);
				p.color = baseColor;
			});
		}
	};

	class ShapeBurst : public IParticleEvent
	{
	public:
		enum Type 
		{ 
			Cross, 
			Palm, 
			Spiral, 
			Spider, 
			Willow, 
			Heart 
		};
		ShapeBurst(Type type, int childProfileId) : type(type), childProfileId(childProfileId) {}
		Type type;
		int childProfileId;
		void execute(const Particle& parent, ParticleSystem& system) const override
		{
			if (childProfileId < 0) return;
			Vector3 color = Vector3(parent.color.r, parent.color.g, parent.color.b);
			Vector3 baseColor = color * 4.5f;
			if (type == Palm) baseColor = Vector3(1.0f, 0.6f, 0.2f) * 5.0f; 
			if (type == Willow) baseColor = Vector3(8.0f, 5.5f, 1.5f); 
			auto Spawn = [&](Vector3 dir, float speed) 
			{
				Vector3 pVel = parent.vel * 0.1f + dir * speed;
				system.createParticle(childProfileId, parent.pos, pVel, baseColor);
			};
			if (type == Cross) 
			{
				int arms = 6; int perArm = 15;
				for (int a = 0; a < arms; ++a)
				{
					Vector3 armDir = RandomUnitVector(); 
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
					for (int j = 0; j < 20; ++j) { Spawn(dir, float(j) * 1.5f + 5.0f); }
				}
			} else if (type == Spiral) 
			{
				int strands = 4; int perStrand = 40;
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
				system.spawnParticles(childProfileId, 40, [&](ParticleRef p, int index)
				{
					p.pos = parent.pos;
					p.vel = parent.vel * 0.1f + RandomUnitVector() * RandomRange(30.0f, 45.0f);
					p.color = baseColor;
				});
			} 
			else if (type == Willow) 
			{
				system.spawnParticles(childProfileId, 200, [&](ParticleRef p, int index)
				{
					Vector3 dir(RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), RandomRange(0.0f, 1.0f));
					dir.normalize();
					p.pos = parent.pos;
					p.vel = parent.vel * 0.1f + dir * RandomRange(5.0f, 12.0f);
					p.color = baseColor;
				});
			} 
			else if (type == Heart) 
			{
				int count = 180;
				Vector3 axis = RandomUnitVector();
				Vector3 t, b;
				if (abs(axis.z) < 0.9) t = Vector3(0, 0, 1); else t = Vector3(1, 0, 0);
				b = axis.cross(t); b.normalize(); t = b.cross(axis); t.normalize();
				for (int i = 0; i < count; ++i) 
				{
					float angle = (float(i) / count) * 6.283185f;
					float x = 16 * pow(sin(angle), 3);
					float y = 13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle);
					Vector3 dir = t * (x / 16.0f) + b * (y / 16.0f);
					Spawn(dir, 15.0f);
				}
			}
		}
	};

	class ClusterBurst : public IParticleEvent
	{
	public:
		ClusterBurst(int childProfileId, int clusters = 30) : childProfileId(childProfileId), clusters(clusters) {}
		int clusters; int childProfileId;
		void execute(const Particle& parent, ParticleSystem& system) const override
		{
			if (childProfileId < 0) return;
			for (int i = 0; i < clusters; ++i) {
				Vector3 dir = RandomUnitVector();
				Vector3 clusterPos = parent.pos + dir * RandomRange(10.0f, 25.0f);
				Vector3 clusterColor = HSVToRGB(RandomFloat(), 0.8f, 1.0f) * 5.0f;
				system.spawnParticles(childProfileId, 50, [&](ParticleRef p, int index)
				{
					Vector3 pDir = RandomUnitVector();
					p.pos = clusterPos;
					p.vel = pDir * RandomRange(6.0f, 15.0f) + dir * 2.0f;
					p.color = clusterColor;
				});
			}
		}
	};

	class CompositeBurst : public IParticleEvent
	{
	public:
		TArray<std::shared_ptr<IParticleEvent>> strategies;
		void add(std::shared_ptr<IParticleEvent> strategy) { strategies.push_back(strategy); }
		void execute(const Particle& parent, ParticleSystem& system) const override
		{
			for (auto& s : strategies) { s->execute(parent, system); }
		}
	};

	// --- Archetype IDs ---
	enum EArchetypeID
	{
		Arch_Rocket_Base = 0,
		Arch_Star_Simple, Arch_Star_Willlow, Arch_Star_Spider, Arch_Star_Fish, Arch_Star_Strobe, Arch_Star_Palm, Arch_Star_Cluster, Arch_Star_Brocade, Arch_Star_Ghost,
		Arch_Trail_Simple, Arch_Trail_Long,
		Arch_Max
	};

	bool FireworksStage::onInit()
	{
		if (!BaseClass::onInit())
		{
			return false;
		}
		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame(Vec2i(200, 250));
		frame->addCheckBox("Pause", bPaused);
		FWidgetProperty::Bind(frame->addSlider("Spawn Scale"), mSpawnScale, 0.1f, 10.0f);
		FWidgetProperty::Bind(frame->addSlider("Sky Intensity"), mSkyIntensity, 0.0f, 2.0f);
		FWidgetProperty::Bind(frame->addSlider("Bloom Threshold"), mBloomThreshold, 0.0f, 1.0f);
		FWidgetProperty::Bind(frame->addSlider("Bloom Intensity"), mBloomIntensity, 0.0f, 20.0f);
		FWidgetProperty::Bind(frame->addSlider("Blur Scale"), blurRadiusScale, 1.0f, 10.0f);

		mCamera.lookAt(Vector3(0, -150, 150), Vector3(0, 0, 80), Vector3(0, 0, 1));
		mUpdateThreadPool = std::make_unique<QueueThreadPool>();
		mUpdateThreadPool->init(8);

		int archIds[Arch_Max];
		// Register Archetypes
		{
			ParticleProfile arch;
			arch.addInitModuleSequence(LifeInit{0.4f, 0.4f}, SizeInit{0.08f});
			arch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, 0)}, DragUpdate{0.2f}, KinematicsUpdate{}, AlphaUpdate{});
			archIds[Arch_Trail_Simple] = mParticleSystem.registerProfile(arch);

			arch.addInitModuleSequence(LifeInit{1.2f, 1.2f}, SizeInit{0.08f});
			arch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, 0)}, DragUpdate{0.2f}, KinematicsUpdate{}, AlphaUpdate{});
			archIds[Arch_Trail_Long] = mParticleSystem.registerProfile(arch);
		}
		{
			ParticleProfile arch;
			arch.addInitModuleSequence(LifeInit{1.0f, 2.5f}, SizeInit{0.12f});
			arch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -4.0f)}, DragUpdate{0.4f}, KinematicsUpdate{}, AlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 0.1f});
			archIds[Arch_Star_Simple] = mParticleSystem.registerProfile(arch);

			ParticleProfile willowArch;
			willowArch.addInitModuleSequence(LifeInit{2.0f, 3.5f}, SizeInit{0.12f});
			willowArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -3.0f)}, DragUpdate{0.55f}, KinematicsUpdate{}, AlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Long], 0.5f});
			archIds[Arch_Star_Willlow] = mParticleSystem.registerProfile(willowArch);
			
			ParticleProfile fishArch;
			fishArch.addInitModuleSequence(LifeInit{2.0f, 3.0f}, SizeInit{0.12f});
			fishArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -4.0f)}, DragUpdate{0.4f}, NoiseUpdate{25.0f}, KinematicsUpdate{}, AlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 0.8f});
			archIds[Arch_Star_Fish] = mParticleSystem.registerProfile(fishArch);

			ParticleProfile spiderArch; 
			spiderArch.addInitModuleSequence(LifeInit{1.0f, 2.5f}, SizeInit{0.12f});
			spiderArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -4.0f)}, DragUpdate{0.5f}, NoiseUpdate{40.0f}, KinematicsUpdate{}, AlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 0.1f});
			archIds[Arch_Star_Spider] = mParticleSystem.registerProfile(spiderArch);

			ParticleProfile strobeArch;
			strobeArch.addInitModuleSequence(LifeInit{1.0f, 2.5f}, SizeInit{0.12f});
			strobeArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -4.0f)}, DragUpdate{0.4f}, KinematicsUpdate{}, StrobeAlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 0.1f});
			archIds[Arch_Star_Strobe] = mParticleSystem.registerProfile(strobeArch);

			ParticleProfile clusterArch;
			clusterArch.addInitModuleSequence(LifeInit{1.0f, 1.5f}, SizeInit{0.12f});
			clusterArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -4.0f)}, DragUpdate{0.4f}, KinematicsUpdate{}, AlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 0.95f}, DeathUpdate{true});
			archIds[Arch_Star_Cluster] = mParticleSystem.registerProfile(clusterArch);

			ParticleProfile brocadeArch;
			brocadeArch.addInitModuleSequence(LifeInit{3.0f, 4.5f}, SizeInit{0.15f});
			brocadeArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -3.5f)}, DragUpdate{0.65f}, KinematicsUpdate{}, AlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Long], 0.8f});
			archIds[Arch_Star_Brocade] = mParticleSystem.registerProfile(brocadeArch);

			ParticleProfile ghostArch;
			ghostArch.addInitModuleSequence(LifeInit{2.5f, 3.5f}, SizeInit{0.12f});
			ghostArch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -2.5f)}, DragUpdate{0.45f}, KinematicsUpdate{}, GhostAlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 0.2f});
			archIds[Arch_Star_Ghost] = mParticleSystem.registerProfile(ghostArch);
		}

		auto RegisterRocket = [&](std::shared_ptr<IParticleEvent> strategy) {
			ParticleProfile arch;
			arch.addInitModuleSequence(LifeInit{6.0f, 6.0f}, SizeInit{0.45f});
			arch.addUpdateModuleSequence(GravityUpdate{Vector3(0, 0, -8.0f)}, DragUpdate{1.0f}, KinematicsUpdate{}, KillAtApexUpdate{}, RocketAlphaUpdate{}, TrailUpdate{archIds[Arch_Trail_Simple], 1.0f}, DeathUpdate{true});
			arch.deathEvent = strategy;
			mRocketProfileIds.push_back(mParticleSystem.registerProfile(arch));
		};

		auto sSphere = std::make_shared<SphereBurst>(archIds[Arch_Star_Simple], 200, 400);
		auto sRing   = std::make_shared<RingBurst>(archIds[Arch_Star_Simple]);
		auto sDouble = std::make_shared<CompositeBurst>();
		auto sCore   = std::make_shared<SphereBurst>(archIds[Arch_Star_Simple], 50, 50, 20.0f, 6.0f, Vector3(6,6,6));
		auto sShell  = std::make_shared<SphereBurst>(archIds[Arch_Star_Simple], 150, 200, 12.0f);
		sDouble->add(sCore); sDouble->add(sShell);
		auto sWillow = std::make_shared<ShapeBurst>(ShapeBurst::Willow, archIds[Arch_Star_Willlow]);
		auto sFish   = std::make_shared<SphereBurst>(archIds[Arch_Star_Fish]);
		auto sStrobe = std::make_shared<SphereBurst>(archIds[Arch_Star_Strobe], 200, 400, 20.0f, 50.0f, Vector3(10,10,10));
		auto sCross  = std::make_shared<ShapeBurst>(ShapeBurst::Cross, archIds[Arch_Star_Simple]);
		auto sPalm   = std::make_shared<ShapeBurst>(ShapeBurst::Palm, archIds[Arch_Star_Simple]);
		auto sSpider = std::make_shared<ShapeBurst>(ShapeBurst::Spider, archIds[Arch_Star_Spider]);
		auto sSpiral = std::make_shared<ShapeBurst>(ShapeBurst::Spiral, archIds[Arch_Star_Simple]);
		auto sCluster = std::make_shared<ClusterBurst>(archIds[Arch_Star_Cluster]);
		auto sHeart  = std::make_shared<ShapeBurst>(ShapeBurst::Heart, archIds[Arch_Star_Simple]);
		auto sBrocade = std::make_shared<SphereBurst>(archIds[Arch_Star_Brocade], 300, 500, 15.0f, 35.0f);
		auto sGhost   = std::make_shared<SphereBurst>(archIds[Arch_Star_Ghost], 250, 400); sGhost->colorOverride = Vector3(2, 6, 8);

		auto sBrocadeStrobe = std::make_shared<CompositeBurst>();
		sBrocadeStrobe->add(sBrocade);
		sBrocadeStrobe->add(std::make_shared<SphereBurst>(archIds[Arch_Star_Strobe], 50, 50, 2.0f, 8.0f));

		auto sGhostRain = std::make_shared<CompositeBurst>();
		sGhostRain->add(std::make_shared<ShapeBurst>(ShapeBurst::Willow, archIds[Arch_Star_Ghost]));
		sGhostRain->add(sGhost);

		mRocketProfileIds.clear();
		RegisterRocket(sSphere); RegisterRocket(sRing); RegisterRocket(sDouble); RegisterRocket(sCross); RegisterRocket(sWillow);
		RegisterRocket(sSpider); RegisterRocket(sSpiral); RegisterRocket(sFish); RegisterRocket(sStrobe); RegisterRocket(sPalm);
		RegisterRocket(sCluster); RegisterRocket(sHeart); RegisterRocket(sBrocadeStrobe); RegisterRocket(sGhostRain);

		ParticleProfile spawnerProfile;
		int archSpawnerId = mParticleSystem.registerProfile(spawnerProfile);
		ParticleEmitter* spawner = mParticleSystem.getEmitter(archSpawnerId);
		auto RocketSpawnFunc = [this](ParticleEmitter& e, ParticleSystem& s, int count)
		{
			for (int i=0; i<count; ++i) 
			{
				int rocketId = mRocketProfileIds[::Global::Random() % mRocketProfileIds.size()];
				Vector3 pos = Vector3(RandomRange(-15.0f, 15.0f), RandomRange(-15.0f, 15.0f), -10.0f);
				Vector3 vel = Vector3(RandomRange(-3.0f, 3.0f), RandomRange(-3.0f, 3.0f), RandomRange(25.0f, 35.0f));
				vel.z *= 1.2f;
				Vector3 color = HSVToRGB(RandomFloat(), 0.7f + 0.3f*RandomFloat(), 1.0f) * 2.0f; 
				s.createParticle(rocketId, pos, vel, color);
			}
		};
		mRocketSpawnerRate = new RateSpawnModule(1.0f / 0.15f, RocketSpawnFunc);
		spawner->addSpawnModule(std::unique_ptr<ISpawnModule>(mRocketSpawnerRate));
		mRocketSpawnerRate->accumulator = 1.0f; 
		return true;
	}

	void FireworksStage::onUpdate(GameTimeSpan deltaTime)
	{
		PROFILE_ENTRY("FireworksStage.onUpdate");
		BaseClass::onUpdate(deltaTime);
		if (bPaused)
		{
			return;
		}
		if (mRocketSpawnerRate) { mRocketSpawnerRate->rate = (1.0f / 0.15f) * mSpawnScale; }
		mParticleSystem.prepareUpdate(deltaTime);
		int maxParticles = 0;
		for (auto& emitterPtr : mParticleSystem.mEmitters) 
		{ 
			if (emitterPtr) maxParticles += (int)emitterPtr->mChunks.size() * ParticleChunk::MaxParticles; 
		}
		if (maxParticles > 0) 
		{
			auto& storage = mInstanceDataStorage[mStorageIdx % 2];
			if (storage.size() != maxParticles) storage.resize(maxParticles);
			mRenderCount = 0;
			ParticleSystem::RenderContext ctx;
			ctx.pOutBase = storage.data(); ctx.pCounter = &mRenderCount; ctx.maxInstances = maxParticles;
			ctx.instanceStride = sizeof(ParticleSpriteRenderer::InstanceData);
			mParticleSystem.tick(deltaTime, mUpdateThreadPool.get(), &ctx);
		} 
		else 
		{ 
			mParticleSystem.tick(deltaTime, mUpdateThreadPool.get()); 
		}
		mStorageIdx++;
	}

	void FireworksStage::onRender(float dFrame)
	{
		PROFILE_ENTRY("FireworksStage.onRender");
		Vec2i screenSize = ::Global::GetScreenSize();
		int totalParticles = mRenderCount.load();
		int frameIdx = (mStorageIdx - 1) % 2;
		ViewInfo viewSnapshot = mView;
		RenderThread::AddCommand("RenderFireworks", [this, totalParticles, screenSize, viewSnapshot, frameIdx]() mutable
		{
			PROFILE_ENTRY("Fireworks.RenderCommand");
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			const ParticleSpriteRenderer::InstanceData* pInstances = mInstanceDataStorage[frameIdx].data();
			GRenderTargetPool.freeAllUsedElements();
			initializeRenderState();
			mSceneRenderTargets.prepare(screenSize);
			mSceneRenderTargets.attachDepthTexture();
			{
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
				RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 1), 1);
				{
					GPU_PROFILE("NightSky");
					RHISetShaderProgram(commandList, mProgNightSky->getRHI());
					viewSnapshot.setupShader(commandList, *mProgNightSky);
					SET_SHADER_PARAM(commandList, *mProgNightSky, MoonDir, Vector3(0.1, 0.2, 0.8).getNormal());
					SET_SHADER_PARAM(commandList, *mProgNightSky, SkyIntensity, mSkyIntensity);
					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
					DrawUtility::ScreenRect(commandList);
				}
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetFixedShaderPipelineState(commandList, viewSnapshot.worldToClip);
				DrawUtility::AixsLine(commandList);
				if (totalParticles > 0) mParticleRenderer.render(commandList, viewSnapshot, pInstances, totalParticles);
			}
			RHITexture2DRef postProcessRT;
			{
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				BloomConfig config; config.threshold = mBloomThreshold; config.intensity = mBloomIntensity; config.blurRadiusScale = blurRadiusScale;
				RHITexture2DRef bloomTexture = FBloom::Render(commandList, mSceneRenderTargets.getFrameTexture(), *mBloomFrameBuffer, config);
				FTonemap::Render(commandList, mSceneRenderTargets, bloomTexture);
				postProcessRT = &mSceneRenderTargets.getFrameTexture();
			}
			{
				RHISetFrameBuffer(commandList, nullptr);
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None >::GetRHI());
				ShaderHelper::Get().copyTextureToBuffer(commandList, *postProcessRT);
			}
		});
		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		InlineString< 128 > str; str.format("Particles : %d", totalParticles);
		g.drawText(Vector2(20, 20), str);
		g.endRender();
	}

	void FireworksStage::configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
	{
		systemConfigs.screenWidth = 1280; systemConfigs.screenHeight = 800; systemConfigs.bUseRenderThread = true;
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
		if (!mParticleRenderer.init()) 
			return false;
		VERIFY_RETURN_FALSE(mProgNightSky = ShaderManager::Get().getGlobalShaderT<NightSkyProgram>(true));
		mSceneRenderTargets.initializeRHI();
		mBloomFrameBuffer = RHICreateFrameBuffer();
		return true;
	}

	void FireworksStage::preShutdownRenderSystem(bool bReInit)
	{
		BaseClass::preShutdownRenderSystem(bReInit);
		mSceneRenderTargets.releaseRHI();
		mBloomFrameBuffer.release();
		GRenderTargetPool.releaseRHI();
		mParticleRenderer.release();
		mProgNightSky = nullptr;
	}
}
