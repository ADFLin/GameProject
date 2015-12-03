#include "CometParticle.h"

namespace Comet
{

	Particle::Particle() 
		:mVelocity( CoordType::Zero() )
		,mPos( CoordType::Zero() )
		,mFlag( Particle::eDead )
	{

	}

	Particle::~Particle()
	{

	}


}//namespace Comet