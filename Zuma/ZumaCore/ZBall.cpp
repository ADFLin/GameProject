#include "ZumaPCH.h"
#include "ZBall.h"

#include "ZPath.h"
#include "ZEvent.h"
#include "ZEventHandler.h"

namespace Zuma
{

	float const g_ZBallRaidus   = 16;
	float const g_ZBallConDist  = 2 * g_ZBallRaidus ;

	float const speedMergeMove  = 0.35;
	int   const timeStopDelay   = 600;
	int   const timeInsert      = 200;


#ifdef _DEBUG
	ZConBall* g_debugBall = NULL;
#endif
	ZConBall::ZConBall( ZColor color ) 
		:ZObject( color )
		,mToolProp( TOOL_NORMAL )
		,mLifeTime(0)
		,mTimerCon(0)
		,mFlag(0)
	{
		mPathPosOffset = rand() % 100;
	}

	void ZConBall::update( long time , UpdateInfo const& info  )
	{
		mLifeTime += time;

		switch ( getConState() )
		{
		case CS_DISCONNECT:
			if ( info.prevBall->getConState() != CS_INSERTING )
			{
				if ( getPathPos() - info.prevBall->getPathPos() <= g_ZBallConDist )
				{
					setConState( CS_NORMAL );
					info.handler->processEvent( EVT_BALL_CONNECT , this );
					updatePathPos( info.prevBall->getPathPos() + g_ZBallConDist , *info.path );
				}
				else if ( info.prevBall->getColor() == getColor() )
				{
					float maxMovePathPos =  getPathPos() - speedMergeMove * time;
					float finalPos = info.prevBall->getPathPos() + g_ZBallConDist;

					if ( finalPos > maxMovePathPos )
					{
						setConState( CS_NORMAL );
						info.handler->processEvent( EVT_BALL_CONNECT , this );
						updatePathPos( info.prevBall->getPathPos() + g_ZBallConDist , *info.path );
					}
					else
					{
						updatePathPos( maxMovePathPos , *info.path );
					}
				}
				else if ( mTimerCon > 0)
				{
					mTimerCon -= time;
					updatePathPos( getPathPos() + 0.5 * info.moveSpeed * float(mTimerCon )/ timeStopDelay , *info.path ) ;
				}
			}
			else if ( mTimerCon > 0)
			{
				mTimerCon -= time;
				updatePathPos( getPathPos() + 0.5 * info.moveSpeed * float(mTimerCon )/ timeStopDelay , *info.path ) ;
			}
			break;
		case CS_NORMAL:
			updatePathPos( info.prevBall->getPathPos() + g_ZBallConDist , *info.path );
			break;

		case CS_INSERTING:
			{
				float  finalPathPos;

				if ( checkFlag( CBF_INSERT_NEXT) && info.prevBall )
				{
					finalPathPos = info.prevBall->getPathPos() + g_ZBallConDist;
				}
				else if ( !checkFlag( CBF_INSERT_NEXT) && info.nextBall  )
				{
					finalPathPos = info.nextBall->getPathPos() - g_ZBallConDist;
				}
				else
				{
					finalPathPos = getPathPos();
				}

				Vector2  destPos = info.path->getLocation( finalPathPos );
				Vector2  offset  = destPos - getPos();

				if ( mTimerCon < time )
				{
					if ( checkFlag( CBF_INSERT_NEXT ) )
						setConState( CS_NORMAL );
					else
						setConState( CS_DISCONNECT );

					updatePathPos( finalPathPos , *info.path );
					info.handler->processEvent( EVT_INSERT_BALL_FINISH , this );
					mTimerCon = 0;
				}
				else
				{
					setPos( getPos() + (float( time ) / mTimerCon ) * offset );

					mTimerCon -= time;

					if ( finalPathPos != getPathPos() )
					{
						if ( checkFlag( CBF_INSERT_NEXT ) )
						{
							finalPathPos = info.prevBall->getPathPos() + g_ZBallConDist * float( timeInsert - mTimerCon ) / timeInsert;
						}
						else
						{
							finalPathPos = info.nextBall->getPathPos() - g_ZBallConDist * float( timeInsert - mTimerCon ) / timeInsert;
						}

						setPathPos( finalPathPos );
					}
				}
			}
			break;
		}
	}

	void ZConBall::updatePathPos( float pos , ZPath& path )
	{	
		Vector2 newPos = path.getLocation( pos );

		Vector2 newDir;
		float len;
		if ( pos - getPathPos() > 1e-2 )
		{
			newDir = newPos - getPos();

			len = sqrt( newDir.length2() );
			if ( pos - getPathPos() < 0 )
				len = -len;
		}
		else
		{
			newDir = newPos - path.getLocation( pos - 0.1f );

			len = sqrt( newDir.length2() );
		}

		setDir( ( 1.0f / len  ) * newDir  );
		enableFlag( CBF_MASK_BACK , path.haveMask( pos - g_ZBallRaidus ) );
		enableFlag( CBF_MASK_FRONT , path.haveMask( pos + g_ZBallRaidus ) );
		setPathPos( pos );
		setPos( newPos );
	}

	void ZConBall::disconnect()
	{
		if ( getConState() == CS_DISCONNECT )
			return;

		if ( checkFlag( CBF_DESTORY ) )
			return;

		setConState( CS_DISCONNECT );
		mTimerCon = timeStopDelay;
	}

	void ZConBall::insert()
	{
		setConState( CS_INSERTING );
		mTimerCon = timeInsert;
	}

	ZBallNode ZConBall::getLinkNode()
	{
#if ZUSE_LINKABLE
		return this;
#else
		return mNodeIter;
#endif
	}

	ZObject::ZObject( ZColor color /*= zRed */ ) 
		:mColor( color )
		,mDir( 1.0f , 0 )
	{

	}

	ZMoveBall::ZMoveBall( ZColor color )
		:ZObject( color )
	{

	}

	void ZMoveBall::update( unsigned time )
	{
		mPos += time * getSpeed() * getDir();
	}

}//namespace Zuma