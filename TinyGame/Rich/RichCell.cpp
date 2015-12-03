#include "RichPCH.h"
#include "RichCell.h"

#include "RichPlayer.h"
#include "RichPlayerTurn.h"

namespace Rich
{

	void LandCell::onPlayerStay( PlayerTurn& turn )
	{
		Player* player = turn.getPlayer();
		if ( mOwner )
		{
			if ( mOwner == player )
			{
				int cost = calcUpdateCost( *player );
				if ( cost > player->getMoney() )
				{
					ReqData data;
					data.land  = this;
					data.money = cost;
					turn.queryAction( REQ_UPGRADE_LAND , data );
				}
			}
			else
			{
				int toll = calcToll( *player );

				if ( toll < player->getMoney() )
				{
					player->modifyMoney( -toll );
					mOwner->modifyMoney( toll );

					WorldMsg event;
					event.id     = MSG_PAY_PASS_TOLL;
					event.intVal = toll;
					player->getWorld().sendMsg( event );
				}
				else
				{


				}
			}
		}
		else
		{
			int price = calcPrice( *player );
			if ( price > player->getMoney() )
			{
				ReqData data;
				data.land  = this;
				data.money = price;
				turn.queryAction( REQ_BUY_LAND , data );
			}
		}
	}


	void StartCell::giveMoney( Player& player )
	{
		player.modifyMoney( money );
	}

	void StartCell::onPlayerPass( PlayerTurn& turn )
	{
		giveMoney( *turn.getPlayer() );
	}

	void StartCell::onPlayerStay( PlayerTurn& turn )
	{
		giveMoney( *turn.getPlayer() );
	}


	void StoreCell::onPlayerStay( PlayerTurn& turn )
	{
		WorldMsg event;
		event.id = MSG_ENTER_STORE;
		event.intVal = mType;
		turn.getPlayer()->getWorld().sendMsg( event );
	}

}//namespace Rich