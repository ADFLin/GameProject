#include "FX/ParticleSystem.h"
#include "FX/ParticleModuleCommon.h"
#include "ProfileSystem.h"
#include <algorithm>

namespace Render
{
	int ParticleSystem::registerProfile(ParticleProfile const& profile)
	{
		int id = (int)mEmitters.size();
		ParticleProfile copy = profile;
		copy.id = id;
		mEmitters.push_back(std::make_unique<ParticleEmitter>(copy));
		return id;
	}

	std::unique_ptr<ParticleChunk> ParticleSystem::fetchFreeChunk()
	{
		if (mFreeChunks.empty())
		{
			return std::make_unique<ParticleChunk>();
		}
		auto chunk = std::move(mFreeChunks.back());
		mFreeChunks.pop_back();
		chunk->count = 0;
		return chunk;
	}

	void ParticleSystem::recycleChunk(std::unique_ptr<ParticleChunk> chunk)
	{
		if (chunk)
		{
			mFreeChunks.push_back(std::move(chunk));
		}
	}

	ParticleEmitter* ParticleSystem::getEmitter(int id)
	{
		if (id >= 0 && id < (int)mEmitters.size())
		{
			return mEmitters[id].get();
		}
		return nullptr;
	}

	void ParticleSystem::createParticles(int profileId, int count, Vector3 const& pos, Vector3 const& vel, Color4f const& color)
	{
		spawnParticles(profileId, count, [&](auto& p, int idx)
		{
			p.pos = pos;
			p.vel = vel;
			p.color = color;
			p.life = 1.0f;
			p.maxLife = 1.0f;
			p.radius = 1.0f;
		});
	}

	void ParticleSystem::processSpawnRequests()
	{
		PROFILE_ENTRY("ParticleSystem.processSpawnRequests");
		ParticleInitContext ctx;
		for (auto& req : mPendingRequests)
		{
			if (auto* emitter = getEmitter(req.profileId))
			{
				for (auto& chunk : req.chunks)
				{
					for (auto& mod : emitter->mProfile.initModules)
					{
#if !PS_USE_SOA
						mod->initialize(ctx, chunk->data, chunk->count);
#else
						mod->initialize(ctx, *chunk);
#endif
					}
					emitter->mChunks.push_back(std::move(chunk));
				}
				emitter->mLastRequestIndex = -1;
			}
		}
		mPendingRequests.clear();
	}

	void ParticleSystem::prepareUpdate(float dt)
	{
		// 呼叫所有 Emitter 的 Spawn 模組來產生新的請求
		for (auto& emitter : mEmitters)
		{
			for (auto& spawnMod : emitter->spawnModules)
			{
				spawnMod->update(*emitter, *this, dt);
			}
		}

		processSpawnRequests();
	}

	void ParticleSystem::tick(float dt, QueueThreadPool* threadPool, RenderContext* renderContext)
	{
		PROFILE_ENTRY("ParticleSystem.tick");
		struct TaskInfo { ParticleEmitter* emitter; ParticleChunk* chunk; };
		TArray<TaskInfo> tasks;
		for (auto& e : mEmitters)
		{
			for (auto& c : e->mChunks)
			{
				if (c->count > 0)
				{
					tasks.push_back({ e.get(), c.get() });
				}
			}
		}

		TArray<ParticleUpdateContext> contexts(tasks.size());
		for (auto& ctx : contexts)
		{
			ctx.dt = dt;
		}

		if (!tasks.empty())
		{
			const int batchSize = 4;
			for (int i = 0; i < (int)tasks.size(); i += batchSize)
			{
				int endStart = std::min(i + batchSize, (int)tasks.size());
				threadPool->addFunctionWork([i, endStart, &tasks, &contexts, renderContext]()
				{
					PROFILE_ENTRY("ParticleSystem.ParallelUpdate");
					for (int tIdx = i; tIdx < endStart; ++tIdx)
					{
						auto& task = tasks[tIdx];
						auto& ctx = contexts[tIdx];
						auto* chunk = task.chunk;

						for (auto& mod : task.emitter->mProfile.updateModules)
						{
#if !PS_USE_SOA
							mod->update(ctx, chunk->data, chunk->count);
#else
							mod->update(ctx, *chunk);
#endif
						}

						int L = 0, R = chunk->count - 1;
#if !PS_USE_SOA
						while (L <= R)
						{
							if (chunk->data[L].life <= 0.0f)
							{
								while (R > L && chunk->data[R].life <= 0.0f)
								{
									R--;
								}
								if (L == R)
								{
									break;
								}
								chunk->data[L] = chunk->data[R];
								R--;
							}
							L++;
						}
#else
						for (int j = 0; j < chunk->count; )
						{
							if (chunk->data[j].template get<4>() <= 0.0f) // index 4: life
							{
								if (j < chunk->count - 1)
								{
									chunk->data.copyFrom(j, chunk->data, chunk->count - 1);
								}
								chunk->count--;
							}
							else
							{
								j++;
							}
						}
						L = chunk->count;
#endif
						chunk->count = L;

						if (renderContext && L > 0)
						{
							int offset = renderContext->pCounter->fetch_add(L);
							if (offset + L <= renderContext->maxInstances)
							{
								char* pOutBase = (char*)renderContext->pOutBase + (size_t)offset * renderContext->instanceStride;
								for (int idx = 0; idx < L; ++idx)
								{
									char* pOut = pOutBase + (size_t)idx * renderContext->instanceStride;
#if !PS_USE_SOA
									std::memcpy(pOut, &chunk->data[idx], 32);
#else
									auto p = chunk->data[idx];
									struct Out { Vector3 pos; float rad; Color4f col; };
									Out& out = *(Out*)pOut;
									out.pos = p.template get<0>();
									out.rad = p.template get<1>();
									out.col = p.template get<2>();
#endif
								}
							}
						}
					}
				});
			}
			threadPool->waitAllWorkComplete(true);

			for (int i = 0; i < (int)tasks.size(); ++i)
			{
				auto& ctx = contexts[i];
				auto& task = tasks[i];

				{
					PROFILE_ENTRY("ParticleSystem.NewParticles");
					// Process New Particles
					for (auto& np : ctx.newParticles)
					{
						createParticles(np.profileId, 1, np.pos, np.vel, np.color);
					}
				}

				{
					PROFILE_ENTRY("ParticleSystem.DeathEvents");
					// Process Death Events
					if (task.emitter->mProfile.deathEvent && !ctx.deathEvents.empty())
					{
						for (const auto& deathP : ctx.deathEvents)
						{
							task.emitter->mProfile.deathEvent->execute(deathP, *this);
						}
					}
				}
			}

			// Chunk Packing
			{
				PROFILE_ENTRY("ParticleSystem.ChunkPacking");
				for (auto& e : mEmitters)
				{
					int dst = 0, src = (int)e->mChunks.size() - 1;
					while (dst < src)
					{
						auto* dC = e->mChunks[dst].get();
						auto* sC = e->mChunks[src].get();
						int space = ParticleChunk::MaxParticles - dC->count;
						if (space <= 0)
						{
							dst++;
							continue;
						}
						if (sC->count <= 0)
						{
							src--;
							continue;
						}
						int move = std::min(space, sC->count);
#if !PS_USE_SOA
						FMemory::Copy(&dC->data[dC->count], &sC->data[sC->count - move], move * sizeof(ParticleLayout));
#else
						for (int k = 0; k < move; ++k)
						{
							dC->data.copyFrom(dC->count + k, sC->data, sC->count - move + k);
						}
#endif
						dC->count += move;
						sC->count -= move;
					}

					while (!e->mChunks.empty() && e->mChunks.back()->count == 0)
					{
						recycleChunk(std::move(e->mChunks.back()));
						e->mChunks.pop_back();
					}
				}
			}
		}
	}

	void BurstSpawnModule::update(ParticleEmitter& emitter, ParticleSystem& system, float dt)
	{
		if (interval <= 0.0f)
		{
			if (timer == 0.0f)
			{
				if (customSpawnFunc)
				{
					customSpawnFunc(emitter, system, count);
				}
				else
				{
					system.createParticles(emitter.mProfile.id, count, emitter.localPosition, Vector3(0, 0, 0), Color4f(1, 1, 1));
				}
				timer = 1.0f;
			}
		}
		else
		{
			timer -= dt;
			if (timer <= 0.0f)
			{
				if (customSpawnFunc)
				{
					customSpawnFunc(emitter, system, count);
				}
				else
				{
					system.createParticles(emitter.mProfile.id, count, emitter.localPosition, Vector3(0, 0, 0), Color4f(1, 1, 1));
				}
				timer = interval;
			}
		}
	}

	void RateSpawnModule::update(ParticleEmitter& emitter, ParticleSystem& system, float dt)
	{
		accumulator += dt * rate;
		int sc = (int)accumulator;
		if (sc > 0)
		{
			accumulator -= (float)sc;
			if (customSpawnFunc)
			{
				customSpawnFunc(emitter, system, sc);
			}
			else
			{
				system.createParticles(emitter.mProfile.id, sc, emitter.localPosition, Vector3(0, 0, 0), Color4f(1, 1, 1));
			}
		}
	}
}
