#ifndef Particle_h__
#define Particle_h__

#include "Object.h"

class Particle : public LevelObject
{
	DECLARE_OBJECT_CLASS( Particle , LevelObject )
public:
	Particle( Vec2f const& pos );

	virtual void init();
	virtual void tick();
	virtual void onSpawn( unsigned flag );

protected:
	float life;
	float maxLife;

	friend class SmokeRenderer;
	friend class DebrisParticleRenderer;

};


#endif // Particle_h__