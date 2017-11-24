#include "ZumaPCH.h"
#include "ZLevelStage.h"

#include "ZEvent.h"
#include "ZLevelManager.h"
#include "ZBallConGroup.h"
#include "AudioPlayer.h"

#include "ZTask.h"

namespace Zuma
{
	LevelStage::LevelStage( ZLevelInfo& info ) 
		:ZStage( STAGE_LEVEL )
		,mLevel( info )
		,mLvSkipTask( NULL )
		,mPreLvInfo( NULL )
	{
		mRenderParam.aimBall = NULL;
		mRenderParam.aimPos  = mLevel.getFrog().getPos();

		for( int i = 0 ; i < mLevel.getBallGroupNum() ; ++i )
		{
			ZPath* path = mLevel.getBallGroup(i)->getFollowPath();
			float ePos = path->getPathLength();
			Vector2 dir  = path->getLocation( ePos ) - path->getLocation( ePos - 5.0f );
			mRenderParam.holeDir[i] = dir / sqrt( dir.length2() );
		}

		mLevel.setParentHandler( this );
		mPauseGame = false;
	}


	void LevelStage::onStart()
	{
		ZTexButton* button;
		button = new ZTexButton( 
			UI_MENU_BUTTON , IMAGE_MENU_BUTTON , Vector2(540,5) , NULL );

		getUISystem()->addWidget( button );

		restartLevel();

#if 0
		changeState( mPreLvInfo ? LVS_START_QUAKE : LVS_START_TITLE );
#else
		changeState( LVS_START_TITLE );
#endif
		numLive = 3;
	}


	void LevelStage::onUpdate( long time )
	{
		if ( mPauseGame )
			return;

		if ( showSorce < levelScore )
		{
			int ds = levelScore - showSorce;
			showSorce += ( ds > 100 ) ? int( ds * 0.001 ) : 1;
		}

		switch( lvState )
		{
		case LVS_START_TITLE:
		case LVS_START_QUAKE:
		case LVS_PASS_LEVEL:
		case LVS_FINISH:
			break;
		case LVS_IMPORT_BALL:
		case LVS_FAIL:
		case LVS_PLAYING:
			if ( lvState == LVS_PLAYING )
				mLevel.checkImportBall();
			mLevel.update( time );
			break;
		}

		mRenderParam.aimBall = mLevel.calcFrogAimBall( mRenderParam.aimPos );
	}

	bool LevelStage::onMouse( MouseMsg const& msg )
	{
		if ( mPauseGame )
			return true;

		Vector2 dir = Vector2( msg.getPos() ) - mLevel.getFrog().getPos();
		dir *= 1.0 / sqrt( dir.length2() );
		mLevel.getFrog().setDir( dir );

		switch( lvState )
		{
		case LVS_START_TITLE:
			if ( msg.onLeftDown() && mLvSkipTask )
			{
				mLvSkipTask->skipAction();
				mLvSkipTask = NULL;
			}
			break;
		case LVS_PLAYING:
			if ( msg.onLeftDown() || msg.onLeftDClick() )
			{
				mLevel.getFrog().shotBall();
				return false;
			}
			else if ( msg.onRightDown() )
			{
				mLevel.getFrog().swapColor();
				return false;
			}
			break;
		}

		return true;
	}

	bool LevelStage::onUIEvent( int evtID , int uiID , ZWidget* ui )
	{
		switch ( uiID )
		{
		case UI_MENU_BUTTON: 
			if ( getUISystem()->findUI( UI_MENU_PANEL ) == NULL )
				getUISystem()->addWidget( new ZMenuPanel( Vec2i(0,0) , NULL ) );
			break;
		}

		return true;
	}

	void LevelStage::onKey( unsigned key , bool isDown )
	{
		if ( !isDown )
			return;

		switch ( key )
		{
		case 'O':
			sendEvent( EVT_CHANGE_DEV_STAGE , &mLevel.getInfo() );
			break;
		case 'N':
			sendEvent( EVT_LEVEL_CHANGE_NEXT , &mLevel.getInfo() );
			break;
		case 'P':
			mPauseGame = !mPauseGame;
			break;
		}
	}

	bool LevelStage::onEvent( int evtID , ZEventData const& data , ZEventHandler* sender )
	{
		switch( evtID )
		{
		case EVT_LEVEL_FAIL:
			changeState( LVS_FAIL ); break;
		case EVT_LEVEL_GOAL:
			changeState( LVS_FINISH ); break;
		case EVT_ADD_SCORE:
			levelScore += data.getInt();
			break;
		}
		return BaseClass::onEvent( evtID , data , sender);
	}

	void LevelStage::onLevelTaskEnd( LevelStageTask* task )
	{
		switch( lvState )
		{
		case LVS_START_QUAKE:
			changeState( LVS_START_TITLE ); 
			break;
		case LVS_START_TITLE:
			changeState( LVS_IMPORT_BALL ); 
			break;
		case LVS_IMPORT_BALL:
			changeState( LVS_PLAYING );
			break;
		case LVS_FINISH:
			processEvent( EVT_LEVEL_END , this ); 
			break;
		case LVS_PASS_LEVEL :
			processEvent( EVT_LEVEL_CHANGE_NEXT , this );
			break;
		case LVS_FAIL:
			restartLevel();
			changeState( LVS_START_TITLE );
			break;
		}
	}

	void LevelStage::changeState( LvState state )
	{
		lvState = state;

		switch( state )
		{
		case LVS_START_QUAKE:
			mLvSkipTask = new LevelQuakeTask( this , g_QuakeMap[ rand() % 2 ] , *mPreLvInfo );
			break;
		case LVS_START_TITLE:
			mLvSkipTask = new LevelTitleTask( this );
			break;
		case LVS_IMPORT_BALL:
			mLvSkipTask = new LevelImportBallTask( this , 0.05f ) ;
			break;
		case LVS_FINISH:
			mLvSkipTask = new LevelFinishTask( this  );
			break;
		case LVS_FAIL:
			mLvSkipTask = new LevelFailTask( this );
			break;
		case LVS_PASS_LEVEL:
			mLvSkipTask = NULL;
			break;
		default:
			mLvSkipTask = NULL;
		}

		if ( mLvSkipTask )
			processEvent( EVT_ADD_TASK , mLvSkipTask );

	}

	void LevelStage::restartLevel()
	{
		mLevel.restart();

		mPauseGame = false;
		showSorce  = 0;
		levelScore = 0;
	}




	LevelStageTask::LevelStageTask( LevelStage* stage )
		:mLevelStage( stage )
	{
	}

	void LevelStageTask::onEnd( bool beComplete )
	{
		mLevelStage->onLevelTaskEnd( this );
	}

	bool LevelStageTask::onUpdate( long time )
	{
		if ( !mLevelStage->isPauseGame() )
		{
			timerSound += time;
			return updateAction( time );
		}
		return true;
	}

	bool LevelStageTask::checkSoundTimer( unsigned time )
	{
		if ( timerSound > time )
		{
			timerSound -= time;
			return true;
		}
		return false;
	}

	void LevelStageTask::skipAction()
	{
		doSkipAction();
		skip();
	}

	void LevelTitleTask::onStart()
	{
		pathPos = 0.0f;
		idxCurPath = 0;

		isFinish = false;

		ZLevel& level = mLevelStage->getLevel();
		curProgess = eShowLevelTitle;
		curPath = level.getBallGroup( idxCurPath )->getFollowPath();
		level.setMoveSpeed( 0.0f , true );

		setOrder( LRO_TEXT );
		Global::getRenderSystem().addRenderable( this );

		scaleFontB = 0;
		scaleFontT = 0;

		TaskBase* task = new ValueModifyTask< float >( scaleFontT , 0 , 1 , 800 );
		task->setNextTask( new ValueModifyTask< float >( scaleFontB , 0 , 1 , 800 ) );
		getHandler()->addTask( task , this , TASK_FONT_FADE_IN );

		fontFadeOut = false;
		playSound( SOUND_GAME_START );
	}


	bool LevelTitleTask::updateAction( long time )
	{
		if ( isFinish )
			return false;

		float const moveSpeed = 0.8f;

		ZLevel& level = mLevelStage->getLevel();

		switch( curProgess )
		{
		case eShowLevelTitle:
			break;
		case eShowCurveLine:
			pathPos += moveSpeed * time;
			if ( checkSoundTimer( 250 ) )
			{
				float factor = 1.0f - 0.5 * pathPos / curPath->getPathLength();
				playSound( SOUND_TRAIL_LIGHT , 1.0f , false , factor );
			}

			if ( !fontFadeOut && pathPos > curPath->getPathLength() )
			{
				++idxCurPath;
				if ( idxCurPath < level.getBallGroupNum() )
				{
					curPath = level.getBallGroup( idxCurPath )->getFollowPath();
					pathPos = 0;
				}
				else
				{
					TaskBase* task = new ValueModifyTask< float >( scaleFontT , 1 , 0 , 300 );
					getHandler()->addTask( task , this , TASK_FONT_FADE_OUT );
					task = new ValueModifyTask< float >( scaleFontB , 1 , 0 , 300 );
					getHandler()->addTask( task );
					fontFadeOut = true;
				}
			}
			break;
		}

		return true;
	}

	void LevelTitleTask::onTaskMessage( TaskBase* task , TaskMsg const& msg )
	{
		if ( msg.onEnd() && !msg.checkFlag( TF_HAVE_NEXT_TASK ) )
		{
			switch ( msg.getGroup() )
			{
			case TASK_FONT_FADE_IN:
				timerSound = 250;
				curProgess = eShowCurveLine;
				break;
			case TASK_FONT_FADE_OUT:
				Global::getRenderSystem().removeObj( this );
				playSound( SOUND_TRAIL_LIGHT_END );
				isFinish = true;
				break;
			}
		}
	}

	void LevelTitleTask::onEnd( bool beComplete )
	{
		if ( !beComplete )
		{
			Global::getRenderSystem().removeObj( this );
			getHandler()->removeTask( TASK_FONT_FADE_IN );
			getHandler()->removeTask( TASK_FONT_FADE_OUT );
		}

		LevelStageTask::onEnd( beComplete );
	}

	void LevelImportBallTask::onStart()
	{
		ZLevel& level = mLevelStage->getLevel();
		level.setMoveSpeed( 1 , true );
		level.setThink( &ZLevel::clacFrogColor );
		level.setNextThinkTime( 1000 );

		timerSound = 250;

		importLen = level.getInfo().numStartBall * g_ZBallConDist;
	}

	bool LevelImportBallTask::updateAction( long time )
	{
		ZLevel& level = mLevelStage->getLevel();

		ZConBall* ball = level.getBallGroup(0)->getLastBall();

		assert( ball );

		if ( ball->getPathPos() > importLen )
		{
			level.setMoveSpeed( saveSpeed );
			return false;
		}
		else if ( checkSoundTimer( 250 ) )
		{
			playSound( SOUND_ROLLING  );
		}

		return true;
	}

	void LevelFinishTask::onStart()
	{
		ZLevel& level = mLevelStage->getLevel();
		curPath = level.getBallGroup(0)->getFollowPath();
		idxCurPath = 0;
		pathPos = level.getBallGroup( idxCurPath )->getFinishPathPos();
		PSPos   = 0;

		timerSound = 250;
	}

	bool LevelFinishTask::updateAction( long time )
	{
		float const moveSpeed = 0.5;
		pathPos += moveSpeed * time;

		ZLevel& level = mLevelStage->getLevel();

		if ( pathPos > PSPos + 100 )
		{
			ShowTextTask* task = new ShowTextTask( "+200" , FONT_FLOAT , 1000 );

			mLevelStage->addScore( 200 );

			task->speed       = Vector2(0,-20.0f / 1000.0f);
			task->pos         = curPath->getLocation( pathPos );
			task->color.value = 0xff00ffff;
			task->setOrder( LRO_TEXT );

			getHandler()->addTask( task );
			playSound( SOUND_TRAIL_LIGHT , 1.0f , false  );

			PSPos = pathPos;
		}

		if (  pathPos > curPath->getPathLength() )
		{
			++idxCurPath;
			if ( idxCurPath < level.getBallGroupNum() )
			{
				curPath = level.getBallGroup( idxCurPath )->getFollowPath();
				pathPos = level.getBallGroup( idxCurPath )->getFinishPathPos();
				PSPos   = 0;
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	LevelQuakeTask::LevelQuakeTask( LevelStage* level , ZQuakeMap& QMap ,  ZLevelInfo& preLvInfo )
		:LevelStageTask( level )
		,mPreLvInfo( preLvInfo )
		,mQMap(QMap)
	{

	}

	void LevelQuakeTask::onStart()
	{
		mQMap.loadTexture() ;

		setOrder( LRO_QUAKE_MAP );
		Global::getRenderSystem().addRenderable( this );

		for ( int i = 0 ; i < mQMap.numBGAlapha ; ++i )
		{
			bgAlphaPos[i] = mQMap.bgAlpha[i].pos;
		}

		curProgess = eQuakeShake;
	}

	bool LevelQuakeTask::updateAction( long time )
	{
		switch ( curProgess )
		{
		case eQuakeMove:
			for ( int i = 0 ; i < mQMap.numBGAlapha ; ++i )
			{
				bgAlphaPos[i] += mQMap.bgAlpha[i].vel * time / 20.0;
			}
			break;
		case eQuakeShake:
			for ( int i = 0 ; i < mQMap.numBGAlapha ; ++i )
			{
				bgAlphaPos[i] = mQMap.bgAlpha[i].pos + 10 * mQMap.bgAlpha[i].vel * sin( 0.05 * g_GameTime.curTime );
			}
			if ( checkSoundTimer( 10000 ) )
			{
				playSound( SOUND_EARTHQUAKE );
			}
			break;
		}

		return true;
	}

	void LevelQuakeTask::onEnd( bool beComplete )
	{
		Global::getRenderSystem().removeObj( this );
		BaseClass::onEnd( beComplete );
	}

	void LevelFailTask::onStart()
	{
		ZLevel& level = mLevelStage->getLevel();

		curProgess = eRemoveBall;
		timerSound = 250;

		Vector2 const& dir = level.getFrog().getDir();
		angle = atan2( dir.y , dir.x );

		level.setMoveSpeed( 1.0 );


		//ZStarPS* ps = new ZStarPS( mLevelStage->getRenderSystem() );
		//ps->getEmitter().setPos( level.getFrog().getPos() );

		//psTask = new PSTask( mLevelStage->getRenderSystem() , ps , 1000 );
		//TaskBase* task = new DelayTask( 500 );
		//task->setNextTask( psTask );
		//getHandler()->addTask( psTask );
	}

	bool LevelFailTask::updateAction( long time )
	{
		float const rotateSpeed = 0.01f;
		//psTask->setLiftTime( time + 1 );

		ZLevel& level = mLevelStage->getLevel();

		switch ( curProgess )
		{
		case  eRemoveBall:
			{
				angle += rotateSpeed * time;
				level.getFrog().setDir( Vector2( cos( angle),sin(angle) ) );

				bool finish = true;
				for ( int i = 0 ; i < level.getBallGroupNum() ; ++i )
				{
					ZConBallGroup* group = level.getBallGroup(i);
					ZConBall* ball = group->getFristBall();
					if ( ball && ball->getPathPos() < group->getFollowPath()->getPathLength() )
						finish = false;
				}

				if ( finish )
				{
					level.getFrog().show( false );
					curProgess = eShowLifeNum;
				}
				else if ( checkSoundTimer( 500 ) )
				{
					playSound( SOUND_ROLLING );
				}
			}
			break;
		case eShowLifeNum :
			return false;
			//setLiftTime( 0 );
			break;
		}

		return true;
	}

}