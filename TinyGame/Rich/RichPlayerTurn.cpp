#include "RichPlayerTurn.h"

#include "RichPlayer.h"
#include "RichArea.h"

#include <algorithm>

namespace Rich
{

	World& PlayerTurn::getWorld()
	{
		return mPlayer->getWorld();
	}

	void PlayerTurn::startTurn(Player& player)
	{
		mPlayer = &player;
		mReqInfo.id = REQ_NONE;

		mCurMoveStep = 0;
		mTotalMoveStep = 0;
		mUseRomateDice = false;

		WorldMsg msg;
		msg.id = MSG_TURN_START;
		msg.addParam(this);
		getWorld().dispatchMessage(msg);

		player.startTurn();
		mPlayer->getController().startTurn(*this);
	}

	void PlayerTurn::endTurn()
	{
		WorldMsg msg;
		msg.id = MSG_TURN_END;
		msg.addParam(this);
		getWorld().dispatchMessage(msg);

		mPlayer->getController().endTurn(*this);
		mPlayer = nullptr;
	}

	void PlayerTurn::runTurnAsync()
	{
		World& world = getWorld();

		if (!checkKeepRun())
		{
			return;
		}

		{
			int diceSteps[MAX_MOVE_POWER];
			if (mUseRomateDice)
			{
				int value = mTotalMoveStep / mNumDice;
				for (int i = 0; i < mNumDice; ++i)
				{
					diceSteps[i] = value;
				}
				//calcRomateDiceValue( mTotalMoveStep , mNumDice , diceSteps );
			}
			else
			{
				for (int i = 0; i < mNumDice; ++i)
				{
					diceSteps[i] = getWorld().getRandom().getInt() % 6 + 1;
					mTotalMoveStep += diceSteps[i];
				}
			}

			{
				WorldMsg msg;
				msg.id = MSG_THROW_DICE;
				msg.addParam(mNumDice);
				msg.addParam(diceSteps);
				getWorld().dispatchMessage(msg);
			}
		}

		for (int moveStep = 0; moveStep < mTotalMoveStep; ++moveStep)
		{
			LinkHandle links[MAX_DIR_NUM];
			int numLink = world.getMovableLinks(mPlayer->getPos(), mPlayer->getPrevPos(), links);
			mMoveLinkHandle = links[0];
			if (numLink > 1)
			{
#if 0
				if (mCurMoveStep == 0)
				{
					ReqData data;
					data.numLink = numLink;
					data.links = links;
					queryAction(REQ_MOVE_DIR, data);
					mActState = STEP_MOVE_DIR;
					return;
				}
				else
#endif
				{
					int idx = getWorld().getRandom().getInt() % numLink;
					mMoveLinkHandle = links[idx];
				}
			}


			bool isFinish = false;
			if (mMoveLinkHandle == EmptyLinkHandle)
			{
				break;
			}

			++mCurMoveStep;

			bool bStay = mCurMoveStep == mTotalMoveStep;
			MapCoord posMove = world.getLinkCoord(mPlayer->getPos(), mMoveLinkHandle);

			mTile = world.getTile(posMove);
			mPlayer->move(*mTile, bStay);

			{
				WorldMsg msg;
				msg.id = MSG_MOVE_STEP;
				msg.addParam(mPlayer);
				getWorld().dispatchMessage(msg);
			}

			if (!checkKeepRun())
			{
				break;
			}

			Area* area = world.getArea(mTile->areaId);
			if (area)
			{
				if (bStay)
					area->onPlayerStay(*this, *mPlayer);
				else
					area->onPlayerPass(*this);
			}

			{
				WorldMsg msg;
				msg.id = MSG_MOVE_END;
				getWorld().dispatchMessage(msg);
			}

			if (!checkKeepRun())
			{
				break;
			}
		}
	}

	void PlayerTurn::goMoveByPower(int movePower)
	{
		mCurMoveStep = 0;
		mTotalMoveStep = 0;
		mNumDice = movePower;

		WorldMsg msg;
		msg.id  = MSG_MOVE_START;
		getWorld().dispatchMessage( msg );
	}


	void PlayerTurn::goMoveByStep( int numStep )
	{
		mCurMoveStep = 0;
		mTotalMoveStep = numStep;
		mNumDice = 1;

		mUseRomateDice = true;

		WorldMsg msg;
		msg.id  = MSG_MOVE_START;
		getWorld().dispatchMessage( msg );
	}

	void PlayerTurn::moveDirectly(Player& player)
	{
		ActionReqestData data;
		mReqInfo.id = REQ_MOVE_DIRECTLY;
		mReqInfo.callback = [this, &player](ActionReplyData const& answer)
		{

		};
		queryResuest(player, data);
	}

	void PlayerTurn::moveTo(Player& player, Area& area, bool bTriggerArea /*= true*/)
	{
		Tile* tile = player.changePos(area.getPos());
		if (bTriggerArea)
		{
			area.onPlayerStay(*this, player);
		}
	}

	bool PlayerTurn::sendTurnMsg(WorldMsg const& msg)
	{
		getWorld().dispatchMessage( msg );
		return checkKeepRun();
	}

	bool PlayerTurn::checkKeepRun()
	{
		return mPlayer->isActive();
	}

	void PlayerTurn::doBankruptcy(Player& player)
	{
		player.mState = Player::eGAME_OVER;
		player.mMoney = 0;
	}

	bool PlayerTurn::processDebt(Player& player, int debt, std::function<void(int)> callback)
	{
		if (player.getTotalMoney() > debt)
		{
			player.modifyMoney(-debt);
			callback(debt);
			return true;
		}

		int totalAssetValue = player.getTotalAssetValue();
		if (debt > totalAssetValue)
		{
			for (auto asset : player.mAssets)
			{
				asset->changeOwner(nullptr);
			}

			callback(totalAssetValue);
			doBankruptcy(player);
			return true;
		}

		ActionReqestData data;
		data.debt = debt;
		mReqInfo.id = REQ_DISPOSE_ASSET;
		mReqInfo.callback = [this, &player, callback, debt](ActionReplyData const& answer)
		{
			int* sellAssetIndexList = answer.getParam<int*>(0);
			int numAssets = answer.getParam<int>(1);

			TArray<PlayerAsset*> assets;
			for (int i = 0; i < numAssets; ++i)
			{
				assets.push_back(player.mAssets[sellAssetIndexList[i]]);
			}

			int totalAssetValue = player.getTotalMoney();
			for (auto asset : assets)
			{
				totalAssetValue += asset->getAssetValue();
				asset->changeOwner(nullptr);
				asset->releaseAsset();
				player.mAssets.remove(asset);
			}

			WorldMsg msg;
			msg.id = MSG_DISPOSE_ASSET;
			msg.addParam(assets.data());
			msg.addParam(assets.size());
			mPlayer->getWorld().dispatchMessage(msg);

			CHECK(totalAssetValue >= debt);
			callback(debt);
			if (totalAssetValue > debt)
			{
				player.mMoney = totalAssetValue - debt;
			}
		};

		queryResuest(player, data);
		return false;
	}

	void PlayerTurn::queryResuest(Player& player, ActionReqestData const& data)
	{
		player.getController().queryAction(mReqInfo.id, *this, data);
	}

	void PlayerTurn::calcRomateDiceValue(int totalStep, int numDice, int value[])
	{
		assert( totalStep >= numDice && totalStep <= numDice * 6 );

		int const DiceMaxNumber = 6;

		IRandom& random = mPlayer->getWorld().getRandom();
		int frac = totalStep - numDice;
		for( int i = numDice ; i != 1 ; --i )
		{
			int minValue = std::max( frac - ( DiceMaxNumber - 1 ) * i , 0 );
			int temp = minValue + random.getInt() % ( DiceMaxNumber - minValue );
			frac -= temp;
			value[ numDice - i ] = 1 + temp;
		}

		value[ numDice - 1 ] = 1 + frac;
	}

	void PlayerTurn::queryAction( ActionRequestID id, ActionReqestData const& data )
	{
		mReqInfo.id = id;

		switch( id )
		{
		case REQ_BUY_LAND:
			mReqInfo.callback = [this, data](ActionReplyData const& answer)
			{
				if (answer.getParam<int>() == 0)
					return;

				if (mPlayer->getTotalMoney() > data.money)
				{
					mPlayer->modifyMoney(-data.money);
					data.land->setOwner(*mPlayer);
					mPlayer->mAssets.push_back(data.land);

					WorldMsg msg;
					msg.id = MSG_BUY_LAND;
					msg.addParam(data.land);
					mPlayer->getWorld().dispatchMessage(msg);
				}
			};
			break;
		case REQ_BUY_STATION:
			mReqInfo.callback = [this,data](ActionReplyData const& answer)
			{
				if( answer.getParam<int>() == 0 )
					return;

				if (mPlayer->getTotalMoney() > data.money)
				{
					mPlayer->modifyMoney(-data.money);
					data.station->setOwner(*mPlayer);
					mPlayer->mAssets.push_back(data.station);

					WorldMsg msg;
					msg.id = MSG_BUY_STATION;
					msg.addParam(data.station);
					mPlayer->getWorld().dispatchMessage(msg);
				}
			};
			break;
		case REQ_UPGRADE_LAND:
			mReqInfo.callback = [this, data](ActionReplyData const& answer)
			{
				if( answer.getParam<int>() == 0 )
					return;

				if (mPlayer->getTotalMoney() > data.money)
				{
					mPlayer->modifyMoney(-data.money);
					data.land->upgradeLevel();

					WorldMsg msg;
					msg.id = MSG_UPGRADE_LAND;
					msg.addParam(data.land);
					mPlayer->getWorld().dispatchMessage(msg);
				}
			};
			break;
		case REQ_ROMATE_DICE_VALUE:
			mReqInfo.callback = [this](ActionReplyData const& answer)
			{
				assert(0 < answer.getParam<int>() && answer.getParam<int>() <= 6);
				goMoveByStep(answer.getParam<int>());
			};
			break;
		}

		queryResuest(*mPlayer, data);
	}

	void PlayerTurn::replyAction( ActionReplyData const& answer )
	{
		assert( mReqInfo.id != REQ_NONE );
		mReqInfo.callback( answer );
		mLastAnswerReq = mReqInfo.id;
		mReqInfo.id = REQ_NONE;
	}

}//namespace Rich