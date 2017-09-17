#include "RichPCH.h"
#include "RichArea.h"

#include "RichPlayer.h"
#include "RichPlayerTurn.h"

namespace Rich
{

	void LandArea::onPlayerStay( PlayerTurn& turn )
	{
		Player* player = turn.getPlayer();
		if ( mOwner )
		{
			if ( mOwner == player )
			{
				int cost = calcUpdateCost( *player );
				if ( cost > player->getTotalMoney() )
				{
					ActionReqestData data;
					data.land  = this;
					data.money = cost;
					turn.queryAction( REQ_UPGRADE_LAND , data );
				}
			}
			else
			{
				int toll = calcToll( *player );

				if ( toll < player->getTotalMoney() )
				{
					player->modifyMoney( -toll );
					mOwner->modifyMoney( toll );

					WorldMsg msg;
					msg.id     = MSG_PAY_PASS_TOLL;
					msg.addParam(toll);
					player->getWorld().dispatchMessage( msg );
				}
				else
				{


				}
			}
		}
		else
		{
			int price = calcPrice( *player );
			if ( price > player->getTotalMoney() )
			{
				ActionReqestData data;
				data.land  = this;
				data.money = price;
				turn.queryAction( REQ_BUY_LAND , data );
			}
		}
	}


	void StartArea::giveMoney( Player& player )
	{
		player.modifyMoney( money );
	}

	void StartArea::onPlayerPass( PlayerTurn& turn )
	{
		giveMoney( *turn.getPlayer() );
	}

	void StartArea::onPlayerStay( PlayerTurn& turn )
	{
		giveMoney( *turn.getPlayer() );
	}


	void StoreArea::onPlayerStay( PlayerTurn& turn )
	{
		WorldMsg msg;
		msg.id = MSG_ENTER_STORE;
		msg.addParam((int)mType);
		turn.sendTurnMsg(msg);
	}

}//namespace Rich