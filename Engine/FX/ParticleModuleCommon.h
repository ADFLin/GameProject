#pragma once

#include "FX/ParticleSystem.h"
#include <cmath>

namespace Render
{
	extern float RandomFloat();

	FORCEINLINE float RandomRange(float min, float max)
	{
		return min + (max - min) * RandomFloat();
	}

	FORCEINLINE Vector3 RandomUnitVector()
	{
		Vector3 dir;
		do
		{
			dir = Vector3(RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f));
		} while (dir.length2() > 1.0f || dir.length2() < 0.001f);
		dir.normalize();
		return dir;
	}

#define PS_INIT(CONTEXT, PARTICLE)\
	template< typename P >\
	FORCEINLINE void operator()(ParticleInitContext& CONTEXT, P&& PARTICLE) const
#define PS_UPDATE(CONTEXT, PARTICLE)\
	template< typename P >\
	FORCEINLINE void operator()(ParticleUpdateContext& CONTEXT, P&& PARTICLE) const

	struct LifeInit
	{
		float min, max;
		PS_INIT(context, p)
		{
			float life = RandomRange(min, max);
			p.life = life;
			p.maxLife = life;
		}
	};

	struct SizeInit
	{
		float size;
		PS_INIT(context, p)
		{
			p.radius = size;
		}
	};

	struct KinematicsUpdate
	{
		PS_UPDATE(ctx, p)
		{
			p.pos += p.vel * ctx.dt;
			p.life -= ctx.dt;
		}
	};

	struct GravityUpdate
	{
		Vector3 gravity;
		PS_UPDATE(ctx, p)
		{
			p.vel += gravity * ctx.dt;
		}
	};

	struct DragUpdate
	{
		float drag;
		PS_UPDATE(ctx, p)
		{
			p.vel *= std::pow(drag, ctx.dt);
		}
	};

	struct NoiseUpdate
	{
		float strength;
		PS_UPDATE(ctx, p)
		{
			p.vel += Vector3(RandomRange(-1, 1), RandomRange(-1, 1), RandomRange(-1, 1)) * strength * ctx.dt;
		}
	};

	struct AlphaUpdate
	{
		PS_UPDATE(ctx, p)
		{
			float alpha = p.life / p.maxLife;
			p.color.a = (alpha < 0) ? 0 : alpha * alpha;
		}
	};

	struct TrailUpdate
	{
		int   id;
		float chance;
		PS_UPDATE(ctx, p)
		{
			if (id >= 0 && p.life > 0.0f && RandomFloat() < chance)
			{
				ctx.newParticles.push_back({ id, p.pos, Vector3(0,0,0), p.color * 0.5f, -1.0f });
			}
		}
	};

	struct DeathUpdate
	{
		bool bEvent;
		PS_UPDATE(ctx, p)
		{
			if (p.life <= 0.0f && bEvent && p.life > -1.0f)
			{
				ctx.deathEvents.push_back(p);
				p.life = -5.0f;
			}
		}
	};

}
