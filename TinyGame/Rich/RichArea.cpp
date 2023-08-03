#include "RichArea.h"

#include "RichPlayer.h"
#include "RichPlayerTurn.h"
#include "RichActionCard.h"

namespace Rich
{

	int LandArea::calcToll(Player const& player) const
	{
		int result = mInfo.tolls[mLevel];

		if (isGroupOwned(player.getWorld()))
			result *= 2;

		return result;
	}

	void LandArea::onPlayerStay(PlayerTurn& turn, Player& player)
	{
		if ( mOwner )
		{
			if ( mOwner == &player )
			{
				if (mLevel < getMaxLevel())
				{
					int cost = calcUpgradeCost(player);
					if (player.getTotalMoney() > cost)
					{
						ActionReqestData data;
						data.land = this;
						data.money = cost;
						turn.queryAction(REQ_UPGRADE_LAND, data);
					}
				}
			}
			else
			{
				int toll = calcToll( player );
				turn.processDebt( player, toll, [this, &player](int debt)
				{
					WorldMsg msg;
					msg.id = MSG_PAY_PASS_TOLL;
					msg.addParam(debt);
					player.getWorld().dispatchMessage(msg);

					mOwner->modifyMoney(debt);
				});
			}
		}
		else
		{
			int price = calcPrice( player );
			if ( player.getTotalMoney() > price )
			{
				ActionReqestData data;
				data.land  = this;
				data.money = price;
				turn.queryAction( REQ_BUY_LAND , data );
			}
		}
	}


	int LandArea::getAssetValue()
	{
		return mInfo.basePrice + mLevel * mInfo.upgradeCost;
	}

	void LandArea::changeOwner(Player* player)
	{
		mOwner = player;
	}

	bool LandArea::isGroupOwned(World& world) const
	{
		CHECK(mOwner);
		auto const& group = world.getAreaGroup(mInfo.group);
		for (auto area : group.areas)
		{
			if (static_cast<LandArea*>(area)->getOwner() == mOwner)
				return false;
		}
		return true;
	}

	void LandArea::install(World& world)
	{
		world.registerAreaGroup(this, mInfo.group);
	}

	void StationArea::onPlayerStay(PlayerTurn& turn, Player& player)
	{
		if (mOwner)
		{
			if (mOwner == &player)
			{

			}
			else
			{
				int toll = calcToll(player);
				turn.processDebt(player, toll, [this, &player](int debt)
				{
					WorldMsg msg;
					msg.id = MSG_PAY_PASS_TOLL;
					msg.addParam(debt);
					player.getWorld().dispatchMessage(msg);

					mOwner->modifyMoney(debt);
				});
			}
		}
		else
		{
			int price = calcPrice(player);
			if (player.getTotalMoney() > price)
			{
				ActionReqestData data;
				data.station = this;
				data.money = price;
				turn.queryAction(REQ_BUY_LAND, data);
			}
		}
	}

	void StationArea::install(World& world)
	{
		world.registerAreaGroup(this, STATION_GROUP);
	}

	int StationArea::calcToll(Player const& player) const
	{
		int num = getGroupNum(player.getWorld());
		int result = mInfo.tolls[num];
		return result;
	}

	int StationArea::getGroupNum(World& world) const
	{
		CHECK(mOwner);
		auto const& group = world.getAreaGroup(STATION_GROUP);
		int num = 0;
		for (auto area : group.areas)
		{
			if (static_cast<StationArea*>(area)->getOwner() == mOwner)
				++num;
		}

		return num;
	}

	int StationArea::getAssetValue()
	{
		return mInfo.basePrice;
	}

	void StationArea::changeOwner(Player* player)
	{
		mOwner = player;
	}


	void StartArea::onPlayerPass(PlayerTurn& turn)
	{
		int money = turn.getWorld().mObjectQuery->getGameOptions().passingGoAmount;
		turn.getPlayer()->modifyMoney(money);
	}

	void StartArea::onPlayerStay( PlayerTurn& turn, Player& player)
	{
		int money = turn.getWorld().mObjectQuery->getGameOptions().passingGoAmount;
		turn.getPlayer()->modifyMoney(money);
	}

	void StoreArea::onPlayerStay( PlayerTurn& turn, Player& player)
	{


		WorldMsg msg;
		msg.id = MSG_ENTER_STORE;
		msg.addParam((int)mType);
		turn.sendTurnMsg(msg);
	}

	void JailArea::install(World& world)
	{
		world.registerAreaTag(EAreaTag::Jail, this);
	}

	void CardArea::onPlayerStay(PlayerTurn& turn, Player& player)
	{
		switch (mGroup)
		{
		case CG_CHANCE:
			{
				int index = turn.getWorld().getRandom().getInt() % ICardSet::Get().getChanceCardNum();
				auto& actionCard = ICardSet::Get().getChanceCard(index);
				actionCard.action->doAction(turn.getPlayer(), turn);
			}
			break;
		case CG_COMMUNITY:
			{
				int index = turn.getWorld().getRandom().getInt() % ICardSet::Get().getCommunityCardNum();
				auto& actionCard = ICardSet::Get().getCommunityCard(index);
				actionCard.action->doAction(turn.getPlayer(), turn);
			}
			break;
		}
	}

	void AreaVisitor::visitWorld(World& world)
	{
		for (auto iter = world.createAreaIterator(); iter.haveMore(); iter.advance())
		{
			iter.getArea()->accept(*this);
		}
	}

}//namespace Rich