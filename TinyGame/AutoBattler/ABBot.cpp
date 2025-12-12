#include "ABPCH.h"
#include "ABBot.h"
#include "ABPlayer.h"
#include "ABWorld.h"

namespace AutoBattler
{
	BotController::BotController(Player& player, World& world)
		: mPlayer(player), mWorld(world)
	{
		mThinkTimer = 0.0f;
	}

	void BotController::update(float dt)
	{
		if (mWorld.getPhase() != BattlePhase::Preparation)
			return;

		mThinkTimer -= dt;
		if (mThinkTimer > 0)
			return;

		mThinkTimer = 0.5f; // Think every 0.5s

		// 1. Buy Logic (Greedy)
		if (mPlayer.getGold() >= 1) 
		{
			for(int i=0; i<mPlayer.mShopList.size(); ++i)
			{
				if (mPlayer.mShopList[i] != 0)
				{
					if (mPlayer.getGold() >= 1) 
					{
						if (mPlayer.buyUnit(i))
                            break; // One action per tick
					}
				}
			}
		}

		// 2. Deploy Logic
		// Move Bench units to Board if space
		auto& bench = mPlayer.mBench;
		for(int i=0; i<bench.size(); ++i)
		{
			Unit* unit = bench[i];
			if (unit)
			{
				// Find valid spot
				for(int y=PlayerBoard::MapSize.y/2; y<PlayerBoard::MapSize.y; ++y)
				{
					for(int x=0; x<PlayerBoard::MapSize.x; ++x)
					{
						if (!mPlayer.getBoard().isOccupied(x,y))
						{
							if (mPlayer.deployUnit(unit, Vec2i(x,y)))
                                return;
						}
					}
				}
			}
		}

		// 3. Reroll?
		if (mPlayer.getGold() > 4)
		{
			// mPlayer.rerollShop(); // Maybe not reroll too aggressively
		}
	}

}//namespace AutoBattler
