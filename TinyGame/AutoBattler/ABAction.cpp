#include "ABPCH.h"
#include "ABAction.h"
#include "ABStage.h"
#include "ABWorld.h"
#include "ABDefine.h"

namespace AutoBattler
{
	void ABFrameActionTemplate::firePortAction(ActionPort port, ActionTrigger& trigger)
	{

	}

	void ABFrameActionTemplate::setupPlayer(IPlayerManager& playerManager)
	{

	}

	void ABFrameActionTemplate::executeActions(LevelStage& stage)
	{
		for (size_t i = 0; i < mNumPort; ++i)
		{
			ActionPort port = mFrameData[i].port;
			for (int k = 0; k < mFrameData[i].actions.size(); ++k)
			{
				stage.executeAction(port, mFrameData[i].actions[k]);
			}
		}
	}

	//========================================================================
	// FABAction - Static action execution functions
	//========================================================================

	bool FABAction::Execute(World& world, ActionPort port, ABActionItem const& item)
	{
		int playerIndex = (int)port;
		
		if (playerIndex < 0 || playerIndex >= AB_MAX_PLAYER_NUM)
			return false;
		
		Player& player = world.getPlayer(playerIndex);
		
		switch (item.type)
		{
		case ACT_BUY_UNIT:
			player.buyUnit(item.buy.slotIndex);
			return true;
			
		case ACT_SELL_UNIT:
			player.sellUnit(item.sell.slotIndex);
			return true;
			
		case ACT_REFRESH_SHOP:
			player.rerollShop();
			return true;
			
		case ACT_LEVEL_UP:
			player.buyExperience();
			return true;
			
		case ACT_DEPLOY_UNIT:
			{
				// Resolve Unit from Source
				Unit* unit = nullptr;
				Vec2i srcPos(-1, -1);
				
				if (item.deploy.srcType == 1) // Bench
				{
					if (player.mBench.isValidIndex(item.deploy.srcX))
						unit = player.mBench[item.deploy.srcX];
				}
				else // Board
				{
					int x = item.deploy.srcX;
					int y = item.deploy.srcY;
					if (player.getBoard().isValid(Vec2i(x, y)))
					{
						unit = player.getBoard().getUnit(x, y);
						srcPos = Vec2i(x, y);
					}
				}

				if (unit)
				{
					player.deployUnit(unit, Vec2i(item.deploy.destX, item.deploy.destY), srcPos);
					return true;
				}
			}
			return false;
			
		case ACT_RETRACT_UNIT:
			{
				Unit* unit = nullptr;
				if (item.retract.srcType == 0) // Board
				{
					int x = item.retract.srcX;
					int y = item.retract.srcY;
					if (player.getBoard().isValid(Vec2i(x, y)))
						unit = player.getBoard().getUnit(x, y);
				}
				else // Bench
				{
					if (player.mBench.isValidIndex(item.retract.srcX))
						unit = player.mBench[item.retract.srcX];
				}

				if (unit)
				{
					Vec2i srcPos(-1, -1);
					if (item.retract.srcType == 0) 
						srcPos = Vec2i(item.retract.srcX, item.retract.srcY);

					player.retractUnit(unit, item.retract.benchIndex, srcPos);
					return true;
				}
			}
			return false;
			
		case ACT_SYNC_DRAG:
			// Visual-only action - requires Stage, not handled here
			return false;
			
		default:
			return false;
		}
	}
}

