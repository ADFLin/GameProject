#pragma once

#ifndef PS_USE_SOA
#define PS_USE_SOA 1
#endif

#include "CoreShare.h"
#include "Math/Vector3.h"
#include "Core/Color.h"
#include "DataStructure/Array.h"
#include "DataStructure/SoALayout.h"
#include "Async/AsyncWork.h"

#include <memory>
#include <atomic>
#include <tuple>
#include <functional>

namespace Render
{
	using Math::Vector3;

	struct ParticleUpdateContext;
	class ParticleSystem;

	// 1. Definition of fields
#define PARTICLE_FIELDS(f) \
	f(Vector3, pos)        \
	f(float,   radius)     \
	f(Color4f, color)      \
	f(Vector3, vel)        \
	f(float,   life)       \
	f(float,   maxLife)

	DECLARE_SOA_LAYOUT(ParticleLayout, PARTICLE_FIELDS)

#if PS_USE_SOA
	using Particle = ParticleLayout;
	using ParticleRef = ParticleLayoutRef;
#else
	using Particle = ParticleLayout;
	using ParticleRef = Particle&;
#endif

	struct ParticleInitContext {};

	struct ParticleUpdateContext
	{
		struct NewParticle
		{
			int profileId;
			Vector3 pos;
			Vector3 vel;
			Color4f color;
			float life;
		};
		TArray<NewParticle> newParticles;
		TArray<ParticleLayout> deathEvents;
		float dt;
	};

	struct ParticleChunk;

	template<typename T, typename = void>
	struct THasParticlePreUpdate : std::false_type {};

	template<typename T>
	struct THasParticlePreUpdate<T, std::void_t<decltype(std::declval<T&>().preUpdate(std::declval<ParticleUpdateContext&>()))>> : std::true_type {};

	template<typename T>
	FORCEINLINE void ParticlePreUpdate(T& op, ParticleUpdateContext& ctx)
	{
		if constexpr (THasParticlePreUpdate<T>::value)
		{
			op.preUpdate(ctx);
		}
	}

	template<typename... Ts>
	struct TParticleUpdateModuleSequence
	{
		std::tuple<Ts...> ops;

		TParticleUpdateModuleSequence(Ts const&... inOps)
			: ops(inOps...)
		{
		}

		void preUpdate(ParticleUpdateContext& ctx)
		{
			std::apply([&ctx](auto&... op)
			{
				(ParticlePreUpdate(op, ctx), ...);
			}, ops);
		}

		template<typename P>
		FORCEINLINE void operator()(ParticleUpdateContext& ctx, P&& p)
		{
			std::apply([&ctx, &p](auto&... op)
			{
				(op(ctx, p), ...);
			}, ops);
		}
	};

	// 2. Module Interfaces
	class IParticleInitModule
	{
	public:
		virtual ~IParticleInitModule() = default;
#if !PS_USE_SOA
		virtual void initialize(ParticleInitContext& ctx, Particle pData[], int count) const = 0;
#else
		virtual void initialize(ParticleInitContext& ctx, ParticleChunk& chunk) const = 0;
#endif
	};

	class IParticleUpdateModule
	{
	public:
		virtual ~IParticleUpdateModule() = default;
#if !PS_USE_SOA
		virtual void update(ParticleUpdateContext& ctx, Particle pData[], int count) const = 0;
#else
		virtual void update(ParticleUpdateContext& ctx, ParticleChunk& chunk) const = 0;
#endif
	};

	class IParticleEvent
	{
	public:
		virtual ~IParticleEvent() = default;
		virtual void execute(const ParticleLayout& parent, ParticleSystem& system) const = 0;
	};

	class ParticleEmitter;
	class ISpawnModule
	{
	public:
		virtual ~ISpawnModule() = default;
		virtual void update(ParticleEmitter& emitter, ParticleSystem& system, float dt) = 0;
	};

	// 3. Concrete Spawn Modules
	class BurstSpawnModule : public ISpawnModule
	{
	public:
		int count;
		float interval;
		float timer;
		std::function<void(ParticleEmitter&, ParticleSystem&, int)> customSpawnFunc;

		BurstSpawnModule(int count, float interval = 0.0f, std::function<void(ParticleEmitter&, ParticleSystem&, int)> spawnFunc = nullptr)
			: count(count), interval(interval), timer(0.0f), customSpawnFunc(spawnFunc) {}

		void update(ParticleEmitter& emitter, ParticleSystem& system, float dt) override;
	};

	class RateSpawnModule : public ISpawnModule
	{
	public:
		float rate;
		float accumulator;
		std::function<void(ParticleEmitter&, ParticleSystem&, int)> customSpawnFunc;

		RateSpawnModule(float rate, std::function<void(ParticleEmitter&, ParticleSystem&, int)> spawnFunc = nullptr)
			: rate(rate), accumulator(0.0f), customSpawnFunc(spawnFunc) {}

		void update(ParticleEmitter& emitter, ParticleSystem& system, float dt) override;
	};

	// 4. Data Storage
	struct ParticleChunk
	{
		static const int MaxParticles = 4096;
		int count = 0;
#if !PS_USE_SOA
		Particle data[MaxParticles];
#else
		ParticleLayoutArray<TFixedAllocator<MaxParticles>> data;
		ParticleChunk()
		{
			data.resize(MaxParticles);
		}
#endif
	};

#if PS_USE_SOA
	struct ParticleSoAView
	{
		Vector3* pos;
		float* radius;
		Color4f* color;
		Vector3* vel;
		float* life;
		float* maxLife;
	};

	FORCEINLINE ParticleSoAView MakeParticleSoAView(ParticleChunk& chunk)
	{
		return {
			chunk.data.template getColumn<0>(),
			chunk.data.template getColumn<1>(),
			chunk.data.template getColumn<2>(),
			chunk.data.template getColumn<3>(),
			chunk.data.template getColumn<4>(),
			chunk.data.template getColumn<5>(),
		};
	}

	struct ParticleSoAProxy
	{
		Vector3& pos;
		float& radius;
		Color4f& color;
		Vector3& vel;
		float& life;
		float& maxLife;

		ParticleSoAProxy(ParticleSoAView& view, int index)
			: pos(view.pos[index])
			, radius(view.radius[index])
			, color(view.color[index])
			, vel(view.vel[index])
			, life(view.life[index])
			, maxLife(view.maxLife[index])
		{
		}

		operator ParticleLayout() const
		{
			return { pos, radius, color, vel, life, maxLife };
		}
	};
#endif

	// 5. Template Module Implementations
	template<typename PerParticleInitType>
	class TPerParticleInitModule : public IParticleInitModule
	{
	public:
		TPerParticleInitModule(PerParticleInitType const& inOp) : op(inOp) {}
		PerParticleInitType op;

#if !PS_USE_SOA
		void initialize(ParticleInitContext& ctx, Particle pData[], int count) const override
		{
			for (int i = 0; i < count; ++i)
			{
				op(ctx, pData[i]);
			}
		}
#else
		void initialize(ParticleInitContext& ctx, ParticleChunk& chunk) const override
		{
			for (int i = 0; i < chunk.count; ++i)
			{
				auto p = chunk.data[i].template as<ParticleRef>();
				op(ctx, p);
			}
		}
#endif
	};

	template<typename PerParticleUpdateType>
	class TPerParticleUpdateModule : public IParticleUpdateModule
	{
	public:
		TPerParticleUpdateModule(PerParticleUpdateType const& inOp) : op(inOp) {}
		PerParticleUpdateType op;

#if !PS_USE_SOA
		void update(ParticleUpdateContext& ctx, Particle pData[], int count) const override
		{
			auto localOp = op;
			ParticlePreUpdate(localOp, ctx);
			for (int i = 0; i < count; ++i)
			{
				localOp(ctx, pData[i]);
			}
		}
#else
		void update(ParticleUpdateContext& ctx, ParticleChunk& chunk) const override
		{
			auto localOp = op;
			ParticlePreUpdate(localOp, ctx);
			auto view = MakeParticleSoAView(chunk);
			for (int i = 0; i < chunk.count; ++i)
			{
				ParticleSoAProxy p(view, i);
				localOp(ctx, p);
			}
		}
#endif
	};

	// 6. Profile Configuration
	struct ParticleProfile
	{
		int id = -1;
		TArray<std::shared_ptr<IParticleInitModule>> initModules;
		TArray<std::shared_ptr<IParticleUpdateModule>> updateModules;
		std::shared_ptr<IParticleEvent> deathEvent;

		void addInitModule(std::shared_ptr<IParticleInitModule> mod)
		{
			initModules.push_back(mod);
		}

		void addUpdateModule(std::shared_ptr<IParticleUpdateModule> mod)
		{
			updateModules.push_back(mod);
		}

		template<typename TOp>
		void addInitModuleByOp(TOp&& op)
		{
			addInitModule(std::make_shared<TPerParticleInitModule<std::decay_t<TOp>>>(std::forward<TOp>(op)));
		}

		template<typename TOp>
		void addUpdateModuleByOp(TOp&& op)
		{
			addUpdateModule(std::make_shared<TPerParticleUpdateModule<std::decay_t<TOp>>>(std::forward<TOp>(op)));
		}

		template<typename... Ts>
		void addInitModuleSequence(Ts const&... ops)
		{
			auto fun = [=](ParticleInitContext& ctx, auto& p)
			{
				(ops(ctx, p), ...);
			};
			addInitModuleByOp(fun);
		}

		template<typename... Ts>
		void addUpdateModuleSequence(Ts const&... ops)
		{
			addUpdateModuleByOp(TParticleUpdateModuleSequence<Ts...>(ops...));
		}
	};

	class ParticleEmitter
	{
	public:
		ParticleEmitter(const ParticleProfile& profile) : mProfile(profile) {}
		const ParticleProfile& getProfile() const { return mProfile; }

		void addSpawnModule(std::unique_ptr<ISpawnModule> mod)
		{
			spawnModules.push_back(std::move(mod));
		}

		TArray<std::unique_ptr<ParticleChunk>> mChunks;
		int mLastRequestIndex = -1;
		ParticleProfile mProfile;
		TArray<std::unique_ptr<ISpawnModule>> spawnModules;
		Vector3 localPosition = Vector3(0, 0, 0);
	};

	class ParticleSystem
	{
	public:
		struct SpawnRequest
		{
			int profileId;
			TArray<std::unique_ptr<ParticleChunk>> chunks;
		};

		struct UpdateStats
		{
			std::atomic<uint64> workerUpdateTicks;
			uint64 gameThreadUpdateTicks;

			void reset()
			{
				workerUpdateTicks = 0;
				gameThreadUpdateTicks = 0;
			}

			uint64 getTotalUpdateTicks() const
			{
				return workerUpdateTicks.load() + gameThreadUpdateTicks;
			}
		};

		int registerProfile(ParticleProfile const& profile);
		ParticleEmitter* getEmitter(int id);
		void createParticles(int profileId, int count, Vector3 const& pos, Vector3 const& vel, Color4f const& color);
		void createParticle(int profileId, Vector3 const& pos, Vector3 const& vel, Color4f const& color)
		{
			createParticles(profileId, 1, pos, vel, color);
		}

		std::unique_ptr<ParticleChunk> fetchFreeChunk();
		void recycleChunk(std::unique_ptr<ParticleChunk> chunk);
		void processSpawnRequests();
		void prepareUpdate(float dt);
		void tick(float dt, QueueThreadPool* threadPool, UpdateStats* updateStats = nullptr);

		template<typename TInit>
		void spawnParticles(int profileId, int count, TInit&& initFunc)
		{
			ParticleEmitter* emitter = getEmitter(profileId);
			if (!emitter || count <= 0)
			{
				return;
			}
			if (emitter->mLastRequestIndex == -1)
			{
				emitter->mLastRequestIndex = (int)mPendingRequests.size();
				mPendingRequests.push_back({ profileId });
			}
			SpawnRequest& req = mPendingRequests[emitter->mLastRequestIndex];
			int remaining = count;
			int globalIdx = 0;
			while (remaining > 0)
			{
				if (req.chunks.empty() || req.chunks.back()->count >= ParticleChunk::MaxParticles)
				{
					req.chunks.push_back(fetchFreeChunk());
				}
				ParticleChunk* chunk = req.chunks.back().get();
				int toFill = std::min(remaining, ParticleChunk::MaxParticles - chunk->count);
				for (int i = 0; i < toFill; ++i)
				{
#if !PS_USE_SOA
					initFunc(chunk->data[chunk->count + i], globalIdx++);
#else
					auto p = chunk->data[chunk->count + i].template as<ParticleRef>();
					initFunc(p, globalIdx++);
#endif
				}
				chunk->count += toFill;
				remaining -= toFill;
			}
		}

		TArray<SpawnRequest> mPendingRequests;
		TArray<std::unique_ptr<ParticleChunk>> mFreeChunks;
		TArray<std::unique_ptr<ParticleEmitter>> mEmitters;
	};
}
