#include "ZumaPCH.h"
#include "ZBallConGroup.h"

#include "ZBall.h"
#include "ZEvent.h"

#include "Math/PrimitiveTest2D.h"

#include <list>
#include <cassert>
#include <algorithm>

namespace Zuma
{
	int const g_MinComboNum = 3;
	float const speedBackward = -0.1f;
	float const accelerationPath  = 0.01f;

	ZConBallGroup::ZConBallGroup( ZPath* _path ) 
		:mFollowPath( _path )
		,mNumCheckBall(0)
	{
		mSpeedCur = 0.0f;
	}

	bool LineCircleTest( Vector2 const& rPos , Vector2 const& rDir , 
		Vector2 const& cPos , float r2 , float t[] )
	{
		Vector2 offset = cPos - rPos;
		float B = offset.dot( rDir );
		float C = offset.length2() - r2;

		float D = B * B - C;

		if ( D < 0 )
			return false;

		D = sqrt( D );

		t[0] = B - D;
		t[1] = B + D;

		return  true;
	}

	ZConBall* ZConBallGroup::rayTest( Vector2 const& rPos , Vector2 const& rDir , float& tMin )
	{

		ZConBall* result = NULL ;
		tMin = 1e8f;

		float r2 = g_ZBallRaidus * g_ZBallRaidus;
		for( ZBallList::iterator iter = mBallList.begin();
			iter != mBallList.end() ; ++iter )
		{
			ZConBall* ball = *iter;

			if ( ball->isMask() )
				continue;

			float t[2];

			if ( !Math2D::LineCircleTest( rPos , rDir , ball->getPos() , g_ZBallRaidus , t ) )
				continue;

			if ( t[0] > 0 )
			{
				if ( t[0] < tMin )
				{
					tMin = t[0];
					result = ball;
				}
			}
			else if ( t[1] > 0  )
			{
				if ( t[1] < tMin )
				{
					tMin = t[1];
					result = ball;
				}
			}
		}

		return result;
	}


	bool ZConBallGroup::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		switch ( evtID )
		{
		case EVT_DISCONNECT_BALL:
			break;
		case EVT_BALL_CONNECT:
			{
				ZConBall* ball = data.getPtr< ZConBall >();
				if ( ball->getColor() == getPrevConBall(*ball)->getColor() )
					addCheckBall( ball );
			}
			break;
		case EVT_INSERT_BALL_FINISH:
			addCheckBall( data.getPtr< ZConBall >() );
			break;
		case EVT_BALL_CON_DESTROY :
			if ( mBallList.size() == mDestroyList.size() )
			{
				ZConBall* ball = data.getPtr<ZConBall>();
				mPathPosFinish = ball->getPathPos();
				processEvent( EVT_GROUP_EMPTY , this );
			}
			break;
		}
		return true;
	}


	ZConBall* ZConBallGroup::getPrevConBall( ZConBall& ball ) const
	{
#if ZUSE_LINKABLE
		return ball.getPrevtLink();
#else
		if ( ball.mNodeIter == mBallList.begin() )
			return NULL;

		ZBallNode node = ball.mNodeIter;

		return *(--node);
#endif
	}

	ZConBall* ZConBallGroup::getNextConBall( ZConBall& ball ) const
	{
#if ZUSE_LINKABLE
		return ball.getNextLink();
#else
		ZBallNode node = ball.mNodeIter;

		++node;
		if ( node == mBallList.end() )
			return NULL;

		return *node;
#endif
	}



	ZConBall* ZConBallGroup::getLastBall() const
	{
		return ( mBallList.empty() ) ? NULL : mBallList.back();
	}


	ZConBall* ZConBallGroup::getFristBall() const
	{
		return ( mBallList.empty() ) ? NULL : mBallList.front();
	}

	ZConBall* ZConBallGroup::destroyBall( ZBallNode start , ZBallNode end )
	{
		++end;
		for( ;start != end ;++start )
		{
			ZConBall* ball = *start;
			destroyBall( ball );
		}
		if ( end == mBallList.end() )
			return NULL;

		return *end;
	}

	void ZConBallGroup::destroyBall( ZConBall* ball )
	{
		if ( ball->checkFlag( CBF_DESTORY ) )
			return;

		ball->addFlag( CBF_DESTORY );
		mDestroyList.push_front( ball );

		processEvent( EVT_BALL_CON_DESTROY , ball );
	}

	void ZConBallGroup::applyBomb( Vector2 const& pos , float radius )
	{

		float r2 = radius  * radius ;
		for( ZBallList::iterator iter = mBallList.begin();
			iter != mBallList.end() ; ++iter )
		{
			ZConBall* ball = *iter;

			if ( ( ball->getPos() - pos ).length2() < r2 )
			{
				ball->addFlag( CBF_BOMB );
				destroyBall( ball );
			}
		}
	}

	int ZConBallGroup::calcComboNum( ZConBall& ball , ZBallNode& start , ZBallNode& end )
	{
		ZColor color = ball.getColor();

		start = ball.getLinkNode();
		end   = ball.getLinkNode();

		ZConBall* testBall;

		int result = 1;

		testBall = ( ball.getConState() == CS_NORMAL ) ? getPrevConBall( ball ) : NULL;

		while( testBall && testBall->getColor() == color )
		{
			++result;
			start = testBall->getLinkNode();

			if ( testBall->getConState() != CS_NORMAL  )
				break;

			testBall = getPrevConBall( *testBall );
		}

		testBall = getNextConBall( ball );
		while( testBall && testBall->getColor() == color )
		{
			if ( testBall->getConState() != CS_NORMAL )
				break;

			++result;

			end = testBall->getLinkNode();
			testBall = getNextConBall( *testBall );
		}

		return result;
	}

	ZConBall* ZConBallGroup::addBall( ZColor color )
	{
		ZConBall* newBall = doInsertBall( mBallList.begin() , color );

		ZConBall* nextBall = getNextConBall(*newBall);

		if ( nextBall )
		{
			newBall->setPathPos( nextBall->getPathPos() - g_ZBallConDist );
			nextBall->setConState( CS_NORMAL );
		}
		else
			newBall->setPathPos( -g_ZBallRaidus );

		newBall->setConState( CS_DISCONNECT );

		return newBall;
	}

	ZConBall* ZConBallGroup::insertBall( ZConBall& ball , ZColor color , bool beNext )
	{
		ZBallNode node = ball.getLinkNode();

		if ( beNext ) 
			++node;

		ZConBall* newBall = doInsertBall( node , color );

		newBall->insert();

		if ( beNext )
		{
			newBall->addFlag( CBF_INSERT_NEXT );
			newBall->setPathPos( getPrevConBall(*newBall)->getPathPos() );
		}
		else
		{
			newBall->setPathPos( getNextConBall(*newBall)->getPathPos() );
		}

		return newBall;
	}

	ZConBall* ZConBallGroup::doInsertBall( ZBallNode where , ZColor color )
	{
		ZConBall* ball = new ZConBall( color );
#if ZUSE_LINKABLE
		if ( where )
		{
			where->insertBack( ball );
		}
		else
		{



		}
#else

		mBallList.insert( where , ball );

		ball->mNodeIter = --where;
		assert( ball == *ball->mNodeIter );

		return ball;
#endif
	}


	void ZConBallGroup::updateBallGroup( long time , unsigned flag )
	{
		if ( mBallList.empty() )
			return;

		float speed = 0.1f;

		if ( flag & UG_BACKWARDS )
		{
			speed = speedBackward;
		}
		else 
		{
			speed = getForwardSpeed();
			if ( flag & UG_SLOW )
				speed *= 0.1f;
		}

		if ( speed > mSpeedCur )
			mSpeedCur = std::min( speed , mSpeedCur + accelerationPath * time );
		else
			mSpeedCur = std::max( speed , mSpeedCur - accelerationPath * time );


		if ( flag & UG_BACKWARDS )
		{
			ZConBall* ball = getLastBall();

			while( ball )
			{
				if ( ball->getConState() != CS_NORMAL )
					break;

				ZConBall* prev = getPrevConBall( *ball );

				if ( prev == NULL )
					break;

				ball = prev;
			}

			ZConBall* prev = getPrevConBall( *ball );
			ZConBall* next = getNextConBall( *ball );

			ball->updatePathPos( ball->getPathPos() + mSpeedCur * time , *getFollowPath() );
		}


		bool frist = true;

		ZConBall* cur  = mBallList.front();
		ZConBall::UpdateInfo info;
		info.prevBall  = NULL;
		info.nextBall  = getNextConBall( *cur );
		info.moveSpeed = getForwardSpeed();
		info.handler   = this;
		info.path      = getFollowPath();

		while ( cur )
		{
			if ( frist && cur->getConState() != CS_INSERTING )
			{
				cur->setConState( CS_DISCONNECT );

				if ( ( flag & UG_BACKWARDS ) == 0 )
				{
					float pathPos = cur->getPathPos() + mSpeedCur * time;
					cur->updatePathPos( pathPos , *getFollowPath() );
				}
				frist = false;
			}
			else 
			{
				cur->update( time , info );
			}

			updateBall( cur );

			if ( info.nextBall == NULL )
				break;

			info.prevBall = cur;
			cur  = info.nextBall;
			info.nextBall = getNextConBall( *cur );
		}
	}

	void ZConBallGroup::update( long time , unsigned flag )
	{
		updateBallGroup( time , flag );
		updateCheckBall();
		updateDestroyBall();
	}


	void ZConBallGroup::updateDestroyBall()
	{
		for( ZBallList::iterator iter = mDestroyList.begin();
			iter != mDestroyList.end() ; ++iter )
		{
			ZConBall* ball = *iter;
			mBallList.erase( ball->mNodeIter );
		}
		mDestroyList.clear();
	}


	void ZConBallGroup::updateCheckBall()
	{

		for( int i = 0 ; i < mNumCheckBall ; ++i )
		{
			ZConBall* ball = mCheckBall[i];

			if ( ball->checkFlag( CBF_DESTORY ) )
				continue;

			ZBallNode start , end;

			int num = calcComboNum( *ball , start , end );

			if (  num >= g_MinComboNum )
			{

				ZConBall* nextBall = destroyBall( start , end );

				ZComboInfo info;
				info.pos     = ball->getPos();
				info.color   = ball->getColor();
				info.numBall = num;
				info.numTool = 0;

				++end;

				while( start != end )
				{
					ZConBall* tBall = *start;
					if ( tBall->getToolProp() != TOOL_NORMAL )
						info.tool[ info.numTool++ ] = tBall->getToolProp();

					if ( info.numTool > MAX_NUM_COMBOINFO_TOOL )
					{
						DevMsg( 0, "MAX_NUM_COMBOINFO_TOOL" );
						break;
					}
					++start;
				}

				processEvent( EVT_GROUP_COMBO , &info );

				if ( nextBall && nextBall->getConState() == CS_NORMAL )
					nextBall->disconnect();

			}

		}

		mNumCheckBall = 0;
	}

	void ZConBallGroup::setForwardSpeed( float val , bool beS )
	{
		mSpeedForward = val;
		if ( beS )
			mSpeedCur = val;
	}

	void ZConBallGroup::updateBall( ZConBall* cur )
	{
		processEvent( EVT_UPDATE_CON_BALL , cur );
	}

	ZConBall* ZConBallGroup::findNearestBall( Vector2 const& pos , float& minR2 , bool checkMask /*= false */ )
	{
		minR2 = 1e8f;
		ZConBall* result = NULL;

		for( ZBallList::iterator iter = mBallList.begin();
			iter != mBallList.end() ; ++iter )
		{
			ZConBall* ball = *iter;

			if ( checkMask && checkBallMask( *ball , pos ) )
				continue;

			float r2 = ( pos - ball->getPos() ).length2();

			if ( r2 < minR2 )
			{
				result = ball;
				minR2 = r2;
			}
		}

		return result;
	}

	bool ZConBallGroup::checkBallMask( ZConBall& ball , Vector2 const pos )
	{
		return ball.isMask();
	}

	void ZConBallGroup::addCheckBall( ZConBall* ball )
	{
		assert( mNumCheckBall < MaxNumCheckBall );
		mCheckBall[ mNumCheckBall++ ] = ball;
	}

	void ZConBallGroup::reset()
	{
		mPathPosFinish = 0;
		mNumCheckBall = 0;
		mDestroyList.clear();
		mBallList.clear();
	}

}//namespace Zuma
