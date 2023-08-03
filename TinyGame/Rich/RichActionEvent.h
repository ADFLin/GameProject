#include "RichPlayerTurn.h"
#include "RichPlayer.h"
#include "RichArea.h"

namespace Rich
{

	class ActionEvent_GoJail : public ActionEvent
	{
	public:
		void doAction(Player* player, PlayerTurn& turn) override
		{
			player->changeState(Player::eJAILED);
			Area* area = turn.getWorld().getAreaFromTag(EAreaTag::Jail);
			player->changePos(area->getPos());
		}

	};

	class ActionEvent_GetMoney : public ActionEvent
	{
	public:
		ActionEvent_GetMoney(int inMoney):money(inMoney){}

		void doAction(Player* player, PlayerTurn& turn) override
		{
			player->modifyMoney(money);
		}
		int money;
	};


	class ActionEvent_PayDebt : public ActionEvent
	{
	public:
		ActionEvent_PayDebt(int inMoney) :money(inMoney) {}

		void doAction(Player* player, PlayerTurn& turn) override
		{
			turn.processDebt(*player, money, [](int debt)
			{

			});
		}
		int money;
	};

	class ActionEvent_GoDirectly : public ActionEvent
	{
	public:
		void doAction(Player* player, PlayerTurn& turn) override
		{
			turn.moveDirectly(*player);
		}
	};

	class ActionEvent_GoTagArea : public ActionEvent
	{
	public:
		ActionEvent_GoTagArea(EAreaTag tag):tag(tag){}

		void doAction(Player* player, PlayerTurn& turn) override
		{
			Area* area =  turn.getWorld().getAreaFromTag(tag);
			if (area)
			{
				turn.moveTo(*player, *area, true);
			}
		}

		EAreaTag tag;
	};

	class ActionEvent_PayToEachPlayer : public ActionEvent
	{
	public:
		ActionEvent_PayToEachPlayer(int inMoney) :money(inMoney) {}

		void doAction(Player* player, PlayerTurn& turn) override
		{
			int totalMoney;
			int numPlayer = 0;
			for (auto otherPlayer : turn.getWorld().mObjectQuery->getPlayerList())
			{
				if (player == otherPlayer)
					continue;

				if (otherPlayer->mState == Player::eGAME_OVER)
					continue;

				totalMoney += money;
				++numPlayer;
			}

			turn.processDebt(*player, totalMoney, [player, numPlayer, &turn](int debt)
			{
				int giveMoney = debt / numPlayer;
				for (auto otherPlayer : turn.getWorld().mObjectQuery->getPlayerList())
				{
					if (player == otherPlayer)
						continue;

					if (otherPlayer->mState == Player::eGAME_OVER)
						continue;
					
					otherPlayer->modifyMoney(giveMoney);
				}
			});
		}
		int money;
	};

	class ActionEvent_PayLandTex : public ActionEvent
	{
	public:

		void doAction(Player* player, PlayerTurn& turn) override
		{
			class Visitor : public AreaVisitor
			{
			public:
				virtual void visit(LandArea& area)
				{
					if (area.getOwner() == player)
					{
						if (area.mLevel == area.getMaxLevel())
						{
							tex += hotelValue;
						}
						else
						{
							tex += area.mLevel * houseValue;
						}
					}
				}
				int tex = 0;
				int hotelValue;
				int houseValue;
				Player* player;
			} visitor;
			visitor.player = player;
			visitor.hotelValue = hotelValue;
			visitor.houseValue = houseValue;

			visitor.visitWorld(turn.getWorld());


			if (visitor.tex > 0)
			{
				turn.processDebt(*player, visitor.tex, [](int debt)
				{



				});
			}
		}

		int houseValue;
		int hotelValue;
	};

}//namespace Rich