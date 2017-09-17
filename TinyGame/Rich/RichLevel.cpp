#include "RichPCH.h"
#include "RichLevel.h"
#include "RichArea.h"
#include "RichPlayer.h"

#include <algorithm>

namespace Rich
{

	struct MapInfo
	{
		int width;
		int height;
	};

	Level::Level() 
		:mWorld( this )
	{
		mIdxActive = -1;
	}

	void Level::init()
	{
		int mapWidth  = 32;
		int mapHeight = 32;

		mWorld.setupMap( mapWidth , mapHeight );
	}

	void Level::destroyPlayer( Player* player )
	{
		PlayerVec::iterator iter = std::find( mPlayerVec.begin() , mPlayerVec.end() , player );
		assert( iter != mPlayerVec.end() );
		delete player;
		mPlayerVec.erase( iter );
	}

	Player* Level::createPlayer()
	{
		ActorId id = ActorId( mPlayerVec.size() );
		Player* player = new Player( mWorld , id );
		mPlayerVec.push_back( player );
		mActors.push_back( player );
		return player;
	}

	void Level::tick()
	{
		for( auto actor : mActors )
		{
			actor->tick();
		}
		updateTurn();
	}

	void Level::restart()
	{
		mIdxActive = -1;
		if ( mIdxActive == -1 )
		{
			if ( !mPlayerVec.empty() )
				nextActivePlayer();
		}
	}

	void Level::nextActivePlayer()
	{
		++mIdxActive;
		if( mIdxActive == mPlayerVec.size() )
			mIdxActive = 0;
		Player* player = mPlayerVec[mIdxActive];
		mTurn.beginTurn(*player);

		for( ActorComp* actor : mActors )
		{
			actor->turnTick();
		}
	}

	void Level::runPlayerMove( int movePower )
	{
		mTurn.goMoveByPower( movePower );
	}

	void Level::updateTurn()
	{
		if ( mIdxActive == -1 )
			return;

		if ( !mTurn.update() )
		{
			mTurn.endTurn();
			nextActivePlayer();
		}
	}

}//namespace Rich