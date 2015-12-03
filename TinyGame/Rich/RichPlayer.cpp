#include "RichPCH.h"
#include "RichPlayer.h"

#include "RichWorld.h"

#include "Singleton.h"

namespace Rich
{

	Player::Player( World& world , ActorId id )
		:Actor( id )
		,mWorld( &world )
	{
		mState = eIDLE;
		mMovePower = 1;
	}


	bool Player::changePos( MapCoord const& pos )
	{
		Tile* tile = getWorld().getTile( pos );
		if ( !tile )
			return false;

		if ( tileHook.isLinked() )
			tileHook.unlink();

		tile->actors.push_back( *this );
		mPos = pos;
		return true;
	}

	bool Player::isActive()
	{
		if ( mState != eIDLE )
			return false;
		return true;
	}

	int Player::getMovePower()
	{
		return mMovePower;
	}




}//namespace Rich