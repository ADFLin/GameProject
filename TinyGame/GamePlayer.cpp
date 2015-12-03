#include "TinyGamePCH.h"
#include "GamePlayer.h"

IPlayerManager::Iterator::Iterator( IPlayerManager* _mgr )
{
	mgr      = _mgr;
	numCount = (int) _mgr->getPlayerNum();
	player   = NULL;
	goNext();
}

void IPlayerManager::Iterator::goNext()
{
	unsigned idxCount = ( player ) ? ( player->getId() + 1 ) : 0;
	while( numCount > 0 )
	{
		player = mgr->getPlayer( idxCount );
		if ( player )
			break;

		++idxCount;
	}
	--numCount;
}