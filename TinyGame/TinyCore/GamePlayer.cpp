#include "TinyGamePCH.h"
#include "GamePlayer.h"

IPlayerManager::Iterator::Iterator( IPlayerManager* inManager )
{
	mManager      = inManager;
	mPlayerCount = (int) inManager->getPlayerNum();
	mPlayer = nullptr;
	goNext();
}

void IPlayerManager::Iterator::goNext()
{
	if ( mPlayerCount )
	{
		PlayerId idNext = (mPlayer) ? PlayerId(mPlayer->getId() + 1) : PlayerId(0);

		int count = 0;
		for(; count < MAX_PLAYER_NUM; ++count)
		{
			mPlayer = mManager->getPlayer(idNext);
			if( mPlayer )
				break;
			++idNext;
		}
		if( count == MAX_PLAYER_NUM )
		{
			mPlayerCount = 0;
		}
		--mPlayerCount;
	}
	else
	{
		mPlayer = nullptr;
	}
}