#include "ZumaPCH.h"
#include "ZLevel.h"

#include "ZPath.h"
#include "ZBall.h"
#include "ZBallConGroup.h"
#include "IRenderSystem.h"
#include "ZEvent.h"
#include "ZLevelManager.h"
#include "ResID.h"

namespace Zuma
{


	long const timeMinComboKeep = 1000;
	float const BombRadius = 100;

	struct ShotThinkData : public ThinkData
	{
		bool      haveShot;
		unsigned  timeStart;

		void release(){ delete this; }
	};

	ZFrog::ZFrog() 
		:mIsShow( true )
		,mColorNext( zNull )
		,mBallOffset( 0.0 )
		,mFrameEye( 0 )
		,mFireBallSpeed( 1.0f )
	{

	}

	bool ZFrog::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		return true;
	}

	void ZFrog::shotBallThink( ThinkData* ptr )
	{
		ShotThinkData* data = static_cast< ShotThinkData* >( ptr );

		unsigned time = g_GameTime.curTime - data->timeStart;

		unsigned const timeCycle = 250;
		float const speed = 4.0f;

		if ( time < timeCycle / 2 )
			mBallOffset -= speed;
		else
		{
			if ( !data->haveShot )
			{
				ZMoveBall* ball = new ZMoveBall( getColor() );

				ball->setPos( getPos() );
				ball->setDir( getDir() );
				ball->setSpeed( mFireBallSpeed );

				setColor( getNextColor() );

				processEvent( EVT_FIRE_BALL , ball );
				data->haveShot = true;
			}
			mBallOffset += speed;
		}

		unsigned timeDif = abs( (float)time - timeCycle / 2 );
		if ( timeDif < 2.0 * timeCycle / 16.0 )
			mFrameEye = 2;
		else if ( timeDif < 5.0 * timeCycle / 16.0 )
			mFrameEye = 1;
		else
			mFrameEye = 0;

		if ( time < timeCycle )
			setNextThinkTime( g_GameTime.nextTime );
		else
		{
			mBallOffset = 0;
			mFrameEye   = 0;
		}
	}

	void ZFrog::shotBall()
	{
		ShotThinkData* data = new ShotThinkData ;
		data->haveShot  = false;
		data->timeStart = g_GameTime.curTime;

		mBallOffset = 0;
		mFrameEye   = 0;

		setThink( &ZFrog::shotBallThink , data );
		setNextThinkTime( g_GameTime.nextTime );
	}

	void ZFrog::swapColor()
	{
		std::swap( mColorNext , mColor );
		processEvent( EVT_FROG_SWAP_BALL , this );
	}

	void ZFrog::update( unsigned time )
	{
		processThink( g_GameTime.curTime );
	}

	class BallToolTask : public LifeTimeTask
	{
	public:
		BallToolTask( ZConBall* _ball , ToolProp  _prop )
			:LifeTimeTask( 11 * 1000),ball(_ball),prop(_prop)
		{
		}

		void release(){ delete this; }
		void onStart()
		{  
			ball->setToolProp( prop ); 
		}

		void onEnd( bool beComplete )
		{
			ball->setToolProp( TOOL_NORMAL );
		}
		ToolProp  prop;
		TRefCountPtr< ZConBall >  ball;
	};

	ZLevel::ZLevel( ZLevelInfo& info )
		:mLvInfo( info )
	{
		createConGroup();

		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			getBallGroup(i)->setParentHandler( this );
		}

		getFrog().setParentHandler( this );
		getFrog().setPos( info.frogPos );
	}

	ZLevel::~ZLevel()
	{
		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			delete getBallGroup(i)->getFollowPath();
			delete getBallGroup(i);
		}
	}

	bool ZLevel::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{

		ZConBall* ball;
		switch ( evtID )
		{
		case EVT_GROUP_COMBO:
			{
				ZComboInfo* info = data.getPtr<ZComboInfo>();
				if ( mTimePrvCombo + timeMinComboKeep >= g_GameTime.curTime )
					++mNumSeqCombo;
				else 
					mNumSeqCombo = 0;

				mTimePrvCombo = g_GameTime.curTime;

				info->score = 100 * mNumSeqCombo + 10 * info->numBall;
				info->numComBoKeep = mNumSeqCombo;

				processEvent( EVT_ADD_SCORE , info->score );
			}
			break;
		case EVT_FIRE_BALL:
			{
				ZMoveBall* ball = data.getPtr<ZMoveBall>();
				mShootBallList.push_back( ball );
				getFrog().setNextColor( porduceRandomUsableColor() );
			}
			break;
		case EVT_BALL_CON_DESTROY:
			{
				ball = data.getPtr< ZConBall >();

				++mCurComboGauge;
				++mNumDestroyBall;

				switch( ball->getToolProp() )
				{
				case  TOOL_BOMB:
					applyBomb( ball->getPos() );
					break;
				case  TOOL_BACKWARDS:
					mTimeBackward = g_GameTime.curTime + 4000;
					break;
				case  TOOL_SLOW:
					mTimeSlow     = g_GameTime.curTime + 5000;
					break;
				case  TOOL_ACCURACY:
					mTimeAccuracy = g_GameTime.curTime + 5000;
					break;
				}
			}
			break;
		case EVT_UPDATE_CON_BALL:
			{
				static ToolProp const tools[] =
				{
					TOOL_BOMB ,
					TOOL_BACKWARDS ,
					TOOL_SLOW ,
					TOOL_ACCURACY ,
				};

				ball = data.getPtr< ZConBall >();
				mNextColorBit |= 1 << ball->getColor();

				if ( mTimeNextTool < g_GameTime.curTime && 
					ball->getPathPos() > 0 )
				{
					if ( rand() % 100 == 0 )
					{
						mTimeNextTool += 3000 + rand() % 1000;
						processEvent( EVT_ADD_TASK , new BallToolTask( ball , tools[ rand() % 4 ] ) );
					}
				}
			}
			break;
		case EVT_INSERT_BALL_FINISH:
			break;
		}
		return true;
	}

	void ZLevel::applyBomb( Vec2f const& pos )
	{
		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			getBallGroup(i)->applyBomb( pos , BombRadius );
		}
	}

	void ZLevel::restart()
	{
		clearMoveBallList();
		mLvInfo.numStartBall = 10;
		mLvInfo.numColor  = 4;
		mLvInfo.repetBall = 50;
		mLvInfo.repetDec  = 5;

		mCurComboGauge = 0;
		mMaxComboGauge = 5;

		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			ZConBallGroup* group = getBallGroup(i);

			group->reset();
			generateBall( *group , 
				mLvInfo.numStartBall , mLvInfo.numColor , 
				mLvInfo.repetBall , mLvInfo.repetDec );
		}
		setMoveSpeed( 0.5 );

		getFrog().show( true );
		getFrog().setColor( zNull );
		getFrog().setNextColor( zNull );

		//getFrog().setColor( porduceRandomUsableColor() );

		mTimeBackward = g_GameTime.curTime - 60;
		mTimeSlow     = g_GameTime.curTime - 60;
		mTimeAccuracy = g_GameTime.curTime - 60;
		mTimeNextTool = g_GameTime.curTime + 5000;
		mTimePrvCombo = g_GameTime.curTime - 60;

		mNumSeqCombo = 0;
		mNumDestroyBall = 0;

	}

	void ZLevel::update( long time )
	{
		mColorBit     = mNextColorBit;
		mNextColorBit = 0;

		if ( !( ( 1 << getFrog().getColor() )& mColorBit ) )
			getFrog().setColor( porduceRandomUsableColor() );
		if ( !( ( 1 << getFrog().getNextColor()) & mColorBit ) )
			getFrog().setNextColor( porduceRandomUsableColor() );

		getFrog().update( time );
		processThink( g_GameTime.curTime );
		updateMoveBall( time );

		unsigned flag = 0;
		if ( mTimeBackward > g_GameTime.curTime )
			flag |= UG_BACKWARDS;
		if ( mTimeSlow  > g_GameTime.curTime )
			flag |= UG_SLOW;

		if ( mTimeAccuracy > g_GameTime.curTime )
		{
			getFrog().setFireBallSpeed( 1.5f );
		}
		else
		{
			getFrog().setFireBallSpeed( 1.0f );
		}

		bool isLvFinish = true;
		bool isLvFail   = false;

		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			ZConBallGroup* group = getBallGroup(i);

			group->update( time ,flag ); 

			if ( group->getBallNum() != 0 )
				isLvFinish = false;

			ZConBall* ball = group->getLastBall();
			if ( ball && ball->getPathPos() > group->getFollowPath()->getPathLength() )
				isLvFail = true;
		}


		if ( isLvFinish )
			processEvent( EVT_LEVEL_GOAL , this );
		else if ( isLvFail )
			processEvent( EVT_LEVEL_FAIL , this );
	}

	void ZLevel::updateMoveBall( long time )
	{
		for( MoveBallList::iterator iter = mShootBallList.begin();
			iter != mShootBallList.end();  )
		{
			ZMoveBall* ball = *iter;

			ball->update( time );

			processEvent( EVT_UPDATE_MOVE_BALL , ball );

			float minR2 = 1e8f;
			ZConBall* findBall = NULL;
			ZConBallGroup* findGroup = NULL;
			for ( int i = 0 ; i < getBallGroupNum(); ++i )
			{
				float r2;
				ZConBall* tBall = getBallGroup(i)->findNearestBall( ball->getPos() , r2 , true );

				if ( r2 < minR2 )
				{
					findBall = tBall;
					findGroup = getBallGroup( i );
					minR2 = r2;
				}
			}

			if ( minR2 < g_ZBallConDist * g_ZBallConDist )
			{
				Vec2f offset = ball->getPos() - findBall->getPos();

				bool beNext = offset.dot( findBall->getDir() ) > 0;


				ZConBallGroup* ballGroup = findGroup;

				if ( !beNext  )
				{
					ZConBall* cBall = ballGroup->getPrevConBall(*findBall);

					if ( findBall->getConState() == CS_NORMAL )
					{
						findBall = cBall;
						beNext = true;
					}
				}

				ZConBall* conBall = ballGroup->insertBall( *findBall , ball->getColor() , beNext );

				conBall->setPos( ball->getPos() );
				conBall->setPathPos( findBall->getPathPos() );

				processEvent( EVT_INSERT_BALL , conBall );

				delete ball;
				iter = mShootBallList.erase( iter );
			}
			else
			{ 
				Vec2f const& pos = ball->getPos();
				if ( !isInScreenRange( pos ) )
				{
					delete ball;
					iter = mShootBallList.erase( iter );
				}
				else ++iter;
			}
		}

	}

	ZConBall* ZLevel::calcFrogAimBall( Vec2f& aimPos )
	{
		float tMin = 1e8;
		ZConBall* result = NULL;

		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			float t;
			ZConBall* ball = getBallGroup(i)->rayTest( getFrog().getPos() , getFrog().getDir() , t );

			if ( t < tMin )
			{
				result = ball;
				tMin = t;
			}
		}

		aimPos = getFrog().getPos() + tMin * getFrog().getDir();

		return result;
	}

	ZColor ZLevel::porduceRandomUsableColor()
	{
		if( mColorBit == 0 )
			return zNull;

		unsigned color;
		do 
		{
			color = rand() % zColorNum;
		}
		while( ( mColorBit & ( 1 << color ) ) == 0 );

		return ZColor( color );
	}

	int ZLevel::getTotalConBallNum() const
	{
		int result = 0;

		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			result += getBallGroup(i)->getBallNum();
		}

		return result;
	}

	void ZLevel::setMoveSpeed( float val , bool beS  )
	{
		mMoveSpeed = val; 
		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			getBallGroup(i)->setForwardSpeed( val , beS );
		}
	}

	void ZLevel::generateBall( ZConBallGroup& group , int numBall ,int numColor , int repet , int repetDec )
	{
		ZConBall* ball = group.getFristBall();

		ZColor color;
		if ( ball )
			color = ball->getColor();
		else
		{
			color = ZColor( rand() % numColor );
			mCurRepet = repet;
		}

		while( numBall > 0 )
		{
			while( (rand()%100) < repet )
			{
				group.addBall( color );
				mCurRepet -= repetDec;
				--numBall;
			}
			color = ZColor( rand() % numColor );
			mCurRepet = repet;
		}
	}

	void ZLevel::clacFrogColor( ThinkData* ptr )
	{
		if ( getFrog().getColor() == zNull )
			getFrog().setColor( porduceRandomUsableColor() );

		if ( getFrog().getNextColor() == zNull )
			getFrog().setNextColor( porduceRandomUsableColor() );
	}

	void ZLevel::clearMoveBallList()
	{
		for( MoveBallList::iterator iter = mShootBallList.begin();
			iter != mShootBallList.end() ; ++iter )
		{
			delete *iter;
		}
		mShootBallList.clear();
	}

	bool ZLevel::isInScreenRange( Vec2f const& pos )
	{
		return pos.x + g_ZBallRaidus > 0 && 
			pos.y + g_ZBallRaidus > 0 &&
			pos.x - g_ZBallRaidus < g_ScreenWidth &&
			pos.y - g_ZBallRaidus < g_ScreenHeight;
	}

	void ZLevel::createConGroup()
	{
		CVDataVec posVec;
		for ( int i = 0 ; i < mLvInfo.numCurve ; ++i )
		{
			posVec.clear();
			if ( !loadPathVertex( mLvInfo.pathCurve[i].c_str() ,posVec ) )
			{
				CVData data;
				data.flag = 0;
				posVec.reserve( 4 );
				data.pos = 0.8f * Vec2f(150,120); posVec.push_back( data );
				data.pos = 0.8f * Vec2f(290,50) ; posVec.push_back( data );
				data.pos = 0.8f * Vec2f(580,55) ; posVec.push_back( data );
				data.pos = 0.8f * Vec2f(725,175); posVec.push_back( data );
			}

			ZCurvePath* sPath = new ZCurvePath;

			sPath->buildSpline( posVec , 2 );
			//path  = new ZLinePath( Vec2D( 200 ,100) , Vec2D( 600 , 100 ) );
			mBallGroup[i] = new ZConBallGroup( sPath ) ;
		}
	}

	int ZLevel::getBallGroupNum() const
	{
		return mLvInfo.numCurve;
	}

	void ZLevel::checkImportBall()
	{
		for ( int i = 0 ; i < getBallGroupNum(); ++i )
		{
			ZConBallGroup* group = getBallGroup(i);
			if ( mCurComboGauge >= mMaxComboGauge )
				continue;

			ZConBall* ball = group->getFristBall();
			if ( ball == NULL || ball->getPathPos() > -10 )
			{
				generateBall( *group , 
					2 , mLvInfo.numColor , 
					mLvInfo.repetBall , mLvInfo.repetDec );
			}
		}
	}

}