#include "RichPCH.h"
#include "RichPlayer.h"

#include "RichWorld.h"

#include "Singleton.h"

namespace Rich
{

	Player::Player( World& world , ActorId id )
		:ActorComp( id )
		,mOwnedWorld( &world )
	{
		mState = eIDLE;
		mMovePower = 1;
	}


	void Player::initPos(MapCoord const& pos, MapCoord const& posPrev)
	{
		mPosPrev = posPrev;
		mPos = pos;
		updateTile(pos);
	}

	bool Player::updateTile(MapCoord const& pos)
	{
		Tile* tile = getWorld().getTile(pos);
		if( tile == nullptr )
			return false;

		if( tileHook.isLinked() )
			tileHook.unlink();
		tile->actors.push_back(*this);

		return true;
	}

	bool Player::changePos(MapCoord const& pos)
	{
		Tile* tile = getWorld().getTile( pos );

		if( !updateTile(pos) )
			return false;

		mPosPrev = pos;
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