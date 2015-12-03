#include "CometInitializer.h"

#include "CometParticle.h"
#include "CometRandom.h"

namespace Comet 
{
	ZoneInitializer::ZoneInitializer( Zone* zone ) 
		:mZone( zone )
	{

	}

	void Position::initialize( Particle& p )
	{
		p.setPos( mZone->calcPos() );
	}

	void Velocity::initialize( Particle& p )
	{
		p.setVelocity( mZone->calcPos() );
	}

	static UniformRandom sDefaultLifeRandom;

	Life::Life( TimeType max , TimeType min , IRandom* random /*= nullptr */ ) 
		:mRandom( random )
		,mMaxValue( max )
		,mMinValue( min )
	{
		if ( random == nullptr )
		{
			mRandom = &sDefaultLifeRandom;
		}
	}

	void Life::initialize( Particle& p )
	{
		p.initLifeTime( mRandom->getRangeReal( mMinValue , mMaxValue ) );
	}

}//namespace Comet 
