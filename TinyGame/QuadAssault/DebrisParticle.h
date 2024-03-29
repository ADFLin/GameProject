#ifndef DebrisParticle_h__
#define DebrisParticle_h__

#include "Particle.h"

class DebrisParticle : public Particle
{
	DECLARE_OBJECT_CLASS( DebrisParticle , Particle )
public:
	DebrisParticle( Vec2f const& pos );
	virtual void init();
	virtual void tick();
	virtual void onSpawn( unsigned flag );

	virtual IObjectRenderer* getRenderer();

};

#endif // DebrisParticle_h__