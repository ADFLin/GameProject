#include "BMPCH.h"
#include "BMAction.h"

#include "BMLevel.h"

namespace BomberMan
{

	void evalPlayerAction( Player& player , ActionTrigger& trigger )
	{
		switch( player.getState() )
		{
		case Player::STATE_NORMAL:
		case Player::STATE_INVICIBLE:
			if ( trigger.detect( ACT_BM_MOVE_LEFT ) )
			{
				player.move( DIR_WEST );
			}
			else if ( trigger.detect( ACT_BM_MOVE_RIGHT ) )
			{
				player.move( DIR_EAST );
			}
			else if ( trigger.detect( ACT_BM_MOVE_TOP ) )
			{
				player.move( DIR_NORTH );
			}
			else if ( trigger.detect( ACT_BM_MOVE_DOWN ) )
			{
				player.move( DIR_SOUTH );
			}
			break;
		case Player::STATE_GHOST:
			switch( player.getFaceDir() )
			{
			case DIR_EAST:
				if ( trigger.detect( ACT_BM_MOVE_TOP ) ||
					trigger.detect( ACT_BM_MOVE_LEFT ) )
				{
					player.addActionKey( Player::ACT_MOVE );
				}
				else if ( trigger.detect( ACT_BM_MOVE_DOWN ) ||
					trigger.detect( ACT_BM_MOVE_RIGHT ) )
				{
					player.addActionKey( Player::ACT_MOVE_INV );
				}
				break;
			case DIR_WEST:
				if ( trigger.detect( ACT_BM_MOVE_DOWN ) ||
					trigger.detect( ACT_BM_MOVE_LEFT ) )
				{
					player.addActionKey( Player::ACT_MOVE );
				}
				else if ( trigger.detect( ACT_BM_MOVE_TOP ) ||
					trigger.detect( ACT_BM_MOVE_RIGHT ) )
				{
					player.addActionKey( Player::ACT_MOVE_INV );
				}
				break;
			case DIR_SOUTH:
				if ( trigger.detect( ACT_BM_MOVE_RIGHT ) ||
					trigger.detect( ACT_BM_MOVE_TOP ) )
				{
					player.addActionKey( Player::ACT_MOVE );
				}
				else if ( trigger.detect( ACT_BM_MOVE_LEFT ) ||
					trigger.detect( ACT_BM_MOVE_DOWN ) )
				{
					player.addActionKey( Player::ACT_MOVE_INV );
				}
				break;
			case DIR_NORTH:
				if ( trigger.detect( ACT_BM_MOVE_LEFT ) ||
					trigger.detect( ACT_BM_MOVE_TOP ) )
				{
					player.addActionKey( Player::ACT_MOVE );
				}
				else if ( trigger.detect( ACT_BM_MOVE_RIGHT ) ||
					trigger.detect( ACT_BM_MOVE_DOWN ) )
				{
					player.addActionKey( Player::ACT_MOVE_INV );
				}
				break;
			}
			break;
		}

		if ( trigger.detect( ACT_BM_BOMB ) )
		{
			player.addActionKey( Player::ACT_BOMB );
		}
		if ( trigger.detect( ACT_BM_FUNCTION ) )
		{
			player.addActionKey( Player::ACT_FUN );
		}
	}
	CFrameActionTemplate::CFrameActionTemplate( World& level ) 
		:KeyFrameActionTemplateT< KeyFrameData >( gMaxPlayerNum )
		,mLevel( level )
	{

	}

	void CFrameActionTemplate::firePortAction( ActionTrigger& trigger )
	{
		Player* player = mLevel.getPlayer( trigger.getPort() );
		if ( player )
		{
			evalPlayerAction( *player , trigger );
		}
	}

}//namespace BomberMan