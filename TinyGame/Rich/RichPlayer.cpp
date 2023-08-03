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

	Tile* Player::updateTile(MapCoord const& pos)
	{
		Tile* tile = getWorld().getTile(pos);
		if( tile == nullptr )
			return tile;

		updateTile(*tile);
		return tile;
	}

	void Player::updateTile(Tile& tile)
	{
		if (tileHook.isLinked())
			tileHook.unlink();

		tile.actors.push_back(*this);
	}

	Tile* Player::changePos(MapCoord const& pos)
	{
		Tile* tile = getWorld().getTile( pos );
		if(tile == nullptr)
			return tile;

		updateTile(*tile);
		mPosPrev = pos;
		mPos = pos;
		return tile;
	}

	bool Player::isActive() const
	{
		if ( mState != eIDLE )
			return false;
		return true;
	}

	int Player::getMovePower()
	{
		return mMovePower;
	}

	void Player::modifyMoney(int delta)
	{
		mMoney += delta;
		WorldMsg msg;
		msg.id = MSG_PLAYER_MONEY_MODIFIED;
		msg.addParam(this);
		msg.addParam(delta);
		getWorld().dispatchMessage(msg);
	}

}//namespace Rich