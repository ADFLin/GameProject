#include "CometAction.h"

#include "CometParticle.h"

namespace Comet 
{
	MoveAct::MoveAct( Real multiplier /*= Real(1.0) */ ) 
		:mFactor( Real(0.0) )
		,mMultiplier( multiplier )
	{

	}

	void MoveAct::preUpdate( Emitter& e , TimeType time )
	{
		mFactor = mMultiplier * time;
	}

	void MoveAct::update( Emitter& e , Particle& p , TimeType time )
	{
		p.shiftPos( mFactor * p.getVelocity() );
	}

	void DeadAct::update( Emitter& e , Particle& p , TimeType time )
	{
		if ( p.getLifeTime() <= 0 )
		{
			p.setDead( true );
		}
	}


	AgeAct::AgeAct( Real multiplier /*= Real(1.0) */ ) 
		:mFactor( TimeType(0.0) )
		,mMultiplier( multiplier )
	{

	}

	void AgeAct::preUpdate( Emitter& e , TimeType time )
	{
		mFactor = TimeType( mMultiplier * time );
	}

	void AgeAct::update( Emitter& e , Particle& p , TimeType time )
	{
		TimeType dt = p.getLifeTime() - mFactor;
		if ( dt <= 0 )
		{
			dt = 0;
		}
		p.setLifeTime( dt );
	}


	DecayAct::DecayAct( Real multiplier /*= Real(1.0) */ ) 
		:mFactor( TimeType(0.0) )
		,mMultiplier( multiplier )
	{

	}

	void DecayAct::preUpdate( Emitter& e , TimeType time )
	{
		mFactor = TimeType( mMultiplier * time );
	}

	void DecayAct::update( Emitter& e , Particle& p , TimeType time )
	{
		TimeType dt = p.getLifeTime() - mFactor;
		if ( dt <= 0 )
		{
			p.setDead( true );
			dt = 0;
		}
		p.setLifeTime( dt );
	}


	void DumpAct::update( Emitter& e , Particle& p , TimeType time )
	{
		CoordType const& v = p.getVelocity();
		p.setVelocity( v * mDumpping );
	}

}//namespace Comet 
