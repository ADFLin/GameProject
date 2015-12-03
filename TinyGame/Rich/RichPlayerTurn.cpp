#include "RichPCH.h"
#include "RichPlayerTurn.h"

#include "RichPlayer.h"
#include "RichCell.h"

namespace Rich
{

	void PlayerTurn::beginTurn( Player& player )
	{
		mPlayer = &player;
		mReqInfo.id = REQ_NONE;
		
		mStep = STEP_WAIT;

		mCurMoveStep = 0;
		mTotalMoveStep = 0;

		WorldMsg msg;
		msg.id   = MSG_TURN_START;
		msg.turn = this;
		mPlayer->getWorld().sendMsg( msg );

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

		World& world = mPlayer->getWorld();

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
						step[i] = mPlayer->getWorld().getRandom().getInt() % 6 + 1;
						mTotalMoveStep += step[i];
					}
				}
				
				WorldMsg msg;
				msg.id = MSG_THROW_DICE;
				msg.numDice    = mNumDice;
				msg.diceValue  = step;
				mPlayer->getWorld().sendMsg( msg );

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


				DirType dir[ MAX_DIR_NUM ];
				int numDir = world.getMoveDir( mPlayer->getPos() , mPlayer->getPrevPos() , dir );

				mMoveDir = dir[0];
				if ( numDir > 1 )
				{
#if 0
					if ( mCurMoveStep == 0 )
					{
						ReqData data;
						data.numDir = numDir;
						data.dir    = dir;
						queryAction( REQ_MOVE_DIR , data );
						mActState = STEP_MOVE_DIR;
						return;
					}
					else
#endif
					{
						int idx = mPlayer->getWorld().getRandom().getInt() % numDir;
						mMoveDir = dir[ idx ];
					}
				}

			}
		case STEP_MOVE_DIR:
			{
				bool isFinish = false;
				if ( mMoveDir == NoDir )
				{
					mTurnState = TURN_END;
					return;
				}

				++mCurMoveStep;

				bool beStay = mCurMoveStep == mTotalMoveStep;
				MapCoord posMove = world.getConPos( mPlayer->getPos() , mMoveDir );

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
				msg.player = mPlayer;

				if ( !sendTurnMsg( msg ) )
				{
					mStep = STEP_PROCESS_CELL;
					return;
				}
			}
		case STEP_PROCESS_CELL:
			{
				bool beStay = mCurMoveStep == mTotalMoveStep;

				Cell* cell = world.getCell( mTile->cell );
				if ( cell )
				{
					if ( beStay )
						cell->onPlayerStay( *this );
					else
						cell->onPlayerPass( *this );

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
		mPlayer->getWorld().sendMsg( msg );
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
		mPlayer->getWorld().sendMsg( msg );
	}

	bool PlayerTurn::sendTurnMsg( WorldMsg const& event )
	{
		mPlayer->getWorld().sendMsg( event );
		return checkKeepRun();
	}

	bool PlayerTurn::checkKeepRun()
	{
		if ( !mControl->needRunTurn() || waitQuery() )
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

	void PlayerTurn::queryAction( RequestID id, ReqData const& data )
	{
		mReqInfo.id = id;

		switch( id )
		{
		case REQ_BUY_LAND:
			mReqInfo.callback = stdex::bind( &PlayerTurn::replyBuyLand , this , stdex::placeholders::_1 , data.land , data.money );
			break;
		case REQ_UPGRADE_LAND:
			mReqInfo.callback = stdex::bind( &PlayerTurn::replyUpgradeLevel , this , stdex::placeholders::_1 , data.land , data.money );
			break;
		case REQ_ROMATE_DICE_VALUE:
			mReqInfo.callback = stdex::bind( &PlayerTurn::replyRomateDiceValue , this , stdex::placeholders::_1 );
			break;
		}
		mPlayer->getController().queryAction( id , *this , data );
	}

	void PlayerTurn::replyAction( ReplyData const& answer )
	{
		assert( mReqInfo.id != REQ_NONE );
		mReqInfo.callback( answer );
		mLastAnswerReq = mReqInfo.id;
		mReqInfo.id = REQ_NONE;
	}

	void PlayerTurn::replyUpgradeLevel( ReplyData const& answer , LandCell* land , int cost )
	{
		if ( answer.intVal == 0 )
			return;
		mPlayer->modifyMoney( -cost );
		land->upgradeLevel();
	}

	void PlayerTurn::replyBuyLand( ReplyData const& answer , LandCell* land , int piece )
	{
		if ( answer.intVal == 0 )
			return;
		mPlayer->modifyMoney( -piece );
		land->setOwner( *mPlayer );

		WorldMsg msg;
		msg.id   = MSG_BUY_LAND;
		msg.land = land;
		mPlayer->getWorld().sendMsg( msg );
	}

	void PlayerTurn::replyRomateDiceValue( ReplyData const& answer )
	{
		assert( 0 < answer.intVal && answer.intVal <= 6 );
		goMoveByStep( answer.intVal );
	}

}//namespace Rich