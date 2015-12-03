#include "CometEmiter.h"

#include "CometParticle.h"

namespace Comet
{

	void  Emitter::run( TimeType time )
	{
		if ( !isEnable() )
			return;

		mNumRespawn = mClock->tick( time );

		mEventListener->preUpdate( time );

		ActionCollection::preUpdate( *this , time );

		mFactory->update( *this , *mStorage , time );

		ActionCollection::postUpdate( *this , time );
	}


	void Emitter::doUpdate( Particle& p , TimeType time )
	{
		if ( p.isDead() )
		{
			if ( mNumRespawn <= 0 )
				return;

			p.setDead( false );
			initialize( p );
			--mNumRespawn;
		}

		ActionCollection::update( *this , p , time );
		mEventListener->updateParticle( p , time );
	}



}//namespace ps