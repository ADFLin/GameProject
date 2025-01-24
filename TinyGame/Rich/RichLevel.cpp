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
		:mWorld( this , nullptr)
	{
		mIdxActive = -1;
	}

	void Level::init()
	{
		int mapWidth  = 32;
		int mapHeight = 32;

		mWorld.setupMap( mapWidth , mapHeight );
	}

	void Level::reset()
	{
		for (auto iter = mWorld.createTileIterator(); iter.haveMore(); iter.advance())
		{
			iter.getTile()->reset();
		}

		int numArea = 0;
		for (auto iter = mWorld.createAreaIterator(); iter.haveMore(); iter.advance())
		{
			iter.getArea()->reset();
			++numArea;
		}

		for (auto player : mPlayerList)
		{
			player->reset();
		}
	}

	void Level::destroyPlayer(Player* player)
	{
		PlayerVec::iterator iter = std::find( mPlayerList.begin() , mPlayerList.end() , player );
		assert( iter != mPlayerList.end() );
		delete player;
		mPlayerList.erase( iter );
	}

	Player* Level::createPlayer()
	{
		ActorId id = ActorId( mPlayerList.size() );
		Player* player = new Player( mWorld , id );
		mPlayerList.push_back( player );
		mActors.push_back( player );
		return player;
	}

	void Level::tick()
	{
		for( auto actor : mActors )
		{
			actor->tick();
		}
		mTurn.tick();
	}

	void Level::start()
	{
		mIdxActive = -1;
		Coroutines::Stop(mRunHandle);
		mRunHandle = Coroutines::Start([this]
		{
			runLogicAsync();
		});
	}

	void Level::runLogicAsync()
	{
		while (true)
		{
			Player*  player = nextTurnPlayer();
			if ( player == nullptr)
				break;
			runPlayerTurn(*player);
		}
	}

	void Level::runPlayerTurn(Player& player)
	{
		mTurn.startTurn(player);
		for (ActorComp* actor : mActors)
		{
			actor->turnTick();
		}
		mTurn.runTurnAsync();
		mTurn.endTurn();
	}


	Player*  Level::nextTurnPlayer()
	{
		int prevActive = mIdxActive;
		++mIdxActive;
		if (mIdxActive == mPlayerList.size())
			mIdxActive = 0;

		while (prevActive != mIdxActive)
		{
			Player* player = mPlayerList[mIdxActive];
			if (player->mState != Player::eGAME_OVER)
			{
				return player;
			}
			++mIdxActive;
			if (mIdxActive == mPlayerList.size())
				mIdxActive = 0;
		}
		return nullptr;
	}

	void Level::runPlayerMove(int movePower)
	{
		mTurn.goMoveByPower( movePower );
	}


}//namespace Rich