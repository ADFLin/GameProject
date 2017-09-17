#include "RichPCH.h"
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

	void PlayerTurn::beginTurn(Player& player)
	{
		mPlayer = &player;
		mReqInfo.id = REQ_NONE;
		
		mStep = STEP_WAIT;

		mCurMoveStep = 0;
		mTotalMoveStep = 0;

		WorldMsg msg;
		msg.id   = MSG_TURN_START;
		msg.addParam(this);
		getWorld().dispatchMessage( msg );

		mTurnState = TURN_KEEP;
		mUseRomateDice = false;
	}

	bool PlayerTurn::update()
	{
		do
		{
			updateTurnStep();
		}
		while ( mTurnState == TURN_KEEP );

		return mTurnState != TURN_END;
	}

	void PlayerTurn::updateTurnStep()
	{
		if ( !checkKeepRun() )
			return;

		World& world = getWorld();

		switch( mStep )
		{
		case STEP_WAIT:
			mTurnState = TURN_WAIT;
			return;
		case STEP_START:
			{
	
			}
		case STEP_THROW_DICE:
			{
				int step[ MAX_MOVE_POWER ];
				if ( mUseRomateDice )
				{
					int value = mTotalMoveStep / mNumDice;
					for( int i = 0 ; i < mNumDice; ++i )
					{
						step[i] = value;
					}
					//calcRomateDiceValue( mTotalMoveStep , mNumDice , step );
				}
				else
				{
					for( int i = 0 ; i < mNumDice; ++i )
					{
						step[i] = getWorld().getRandom().getInt() % 6 + 1;
						mTotalMoveStep += step[i];
					}
				}
				
				WorldMsg msg;
				msg.id = MSG_THROW_DICE;
				msg.addParam(mNumDice);
				msg.addParam(step);
				getWorld().dispatchMessage( msg );

				if ( !checkKeepRun() )
				{
					mStep = STEP_MOVE_START;
					return;
				}
			}
		case STEP_MOVE_START:
			{
				if ( mCurMoveStep == mTotalMoveStep )
				{
					mStep = STEP_END;
					WorldMsg msg;
					msg.id = MSG_MOVE_END;
					sendTurnMsg( msg );
					return;
				}


				LinkHandle links[ MAX_DIR_NUM ];
				int numLink = world.getMovableLinks( mPlayer->getPos() , mPlayer->getPrevPos() , links );

				mMoveLinkHandle = links[0];
				if ( numLink > 1 )
				{
#if 0
					if ( mCurMoveStep == 0 )
					{
						ReqData data;
						data.numLink = numLink;
						data.links   = links;
						queryAction( REQ_MOVE_DIR , data );
						mActState = STEP_MOVE_DIR;
						return;
					}
					else
#endif
					{
						int idx = getWorld().getRandom().getInt() % numLink;
						mMoveLinkHandle = links[ idx ];
					}
				}

			}
		case STEP_MOVE_DIR:
			{
				bool isFinish = false;
				if ( mMoveLinkHandle == EmptyLinkHandle )
				{
					mTurnState = TURN_END;
					return;
				}

				++mCurMoveStep;

				bool beStay = mCurMoveStep == mTotalMoveStep;
				MapCoord posMove = world.getLinkCoord( mPlayer->getPos() , mMoveLinkHandle );

				mTile = world.getTile( posMove );

				mPlayer->move( *mTile , beStay );
				if ( !checkKeepRun() )
				{
					mStep = STEP_MOVE_EVENT;
					return;
				}
			}

		case STEP_MOVE_EVENT:
			{
				WorldMsg msg;
				msg.id     = MSG_MOVE_STEP;
				msg.addParam(mPlayer);
				if ( !sendTurnMsg( msg ) )
				{
					mStep = STEP_PROCESS_AREA;
					return;
				}
			}
		case STEP_PROCESS_AREA:
			{
				bool beStay = mCurMoveStep == mTotalMoveStep;

				Area* area = world.getArea( mTile->areaId );
				if ( area )
				{
					if ( beStay )
						area->onPlayerStay( *this );
					else
						area->onPlayerPass( *this );

					if ( !checkKeepRun() )
					{
						mStep = STEP_MOVE_END;
						return;
					}
				}
			}
		case STEP_MOVE_END:
			mStep = STEP_MOVE_START;
			return;
		case STEP_END:
			mTurnState = TURN_END;
			return;
		}
	}

	void PlayerTurn::endTurn()
	{
		mPlayer = nullptr;
	}

	void PlayerTurn::goMoveByPower( int movePower )
	{
		mCurMoveStep = 0;
		mTotalMoveStep = 0;
		mNumDice = movePower;
		mStep = STEP_START;

		WorldMsg msg;
		msg.id  = MSG_MOVE_START;
		getWorld().dispatchMessage( msg );
	}


	void PlayerTurn::goMoveByStep( int numStep )
	{
		mCurMoveStep = 0;
		mTotalMoveStep = numStep;
		mNumDice = 1;
		mStep    = STEP_THROW_DICE;
		mUseRomateDice = true;

		WorldMsg msg;
		msg.id  = MSG_MOVE_START;
		getWorld().dispatchMessage( msg );
	}

	void PlayerTurn::obtainMoney(int money)
	{
		mPlayer->modifyMoney(money);
	}

	bool PlayerTurn::sendTurnMsg(WorldMsg const& msg)
	{
		getWorld().dispatchMessage( msg );
		return checkKeepRun();
	}

	bool PlayerTurn::checkKeepRun()
	{
		if ( !mControl->needRunTurn() || isWaitingQuery() )
		{
			mTurnState = TURN_WAIT;
			return false;
		}
		if ( !mPlayer->isActive() )
		{
			mTurnState = TURN_END;
			return false;
		}

		mTurnState = TURN_KEEP;
		return true;
	}

	void PlayerTurn::calcRomateDiceValue( int totalStep , int numDice , int value[] )
	{
		assert( totalStep >= numDice && totalStep <= numDice * 6 );

		int const DiceMaxNumber = 6;

		IRandom& random = mPlayer->getWorld().getRandom();
		int flac = totalStep - numDice;
		for( int i = numDice ; i != 1 ; --i )
		{
			int minValue = std::max( flac - ( DiceMaxNumber - 1 ) * i , 0 );
			int temp = minValue + random.getInt() % ( DiceMaxNumber - minValue );
			flac -= temp;
			value[ numDice - i ] = 1 + temp;
		}

		value[ numDice - 1 ] = 1 + flac;
	}

	void PlayerTurn::queryAction( ActionRequestID id, ActionReqestData const& data )
	{
		mReqInfo.id = id;

		switch( id )
		{
		case REQ_BUY_LAND:
			mReqInfo.callback = [this,data](ActionReplyData const& answer)
			{
				if( answer.getParam<int>() == 0 )
					return;
				mPlayer->modifyMoney(-data.money);
				data.land->setOwner(*mPlayer);

				WorldMsg msg;
				msg.id = MSG_BUY_LAND;
				msg.addParam(data.land);
				mPlayer->getWorld().dispatchMessage(msg);
			};
			break;
		case REQ_UPGRADE_LAND:
			mReqInfo.callback = [this, data](ActionReplyData const& answer)
			{
				if( answer.getParam<int>() == 0 )
					return;
				mPlayer->modifyMoney(-data.money);
				data.land->upgradeLevel();
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

		mPlayer->getController().queryAction( id , *this , data );
	}

	void PlayerTurn::replyAction( ActionReplyData const& answer )
	{
		assert( mReqInfo.id != REQ_NONE );
		mReqInfo.callback( answer );
		mLastAnswerReq = mReqInfo.id;
		mReqInfo.id = REQ_NONE;
	}

	void PlayerTurn::loseMoney(int money)
	{
		mPlayer->modifyMoney(-money);
	}

}//namespace Rich