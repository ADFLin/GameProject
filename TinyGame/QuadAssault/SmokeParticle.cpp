#include "SmokeParticle.h"

#include "Level.h"

IMPL_OBJECT_CLASS( SmokeParticle , OT_PARTICLE , "Particle.Smoke" )

SmokeParticle::SmokeParticle( Vec2f const& pos ) 
	:BaseClass( pos )
{

}


void SmokeParticle::init()
{
	BaseClass::init();
	maxLife=45;
	life=maxLife;
}

void SmokeParticle::onSpawn( unsigned flag )
{
	BaseClass::onSpawn( flag );

	mPos.x += getLevel()->random(8,16)-8;
	mPos.y += getLevel()->random(8,16)-8;
}

void SmokeParticle::tick()
{
	BaseClass::tick();
}
