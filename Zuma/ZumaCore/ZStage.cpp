#include "ZumaPCH.h"
#include "ZStage.h"

#include "ZBallConGroup.h"
#include "ZEvent.h"
#include "ZResManager.h"
#include "ZLevelManager.h"
#include "ResID.h"
#include "ZTask.h"

#include "PlatformThread.h"
#include "SystemMessage.h"
#include "SystemPlatform.h"
#include <fstream>

static int64 gLoadingTime = 0;

namespace Zuma
{

	bool savePathVertex( char const* path , CurveVertexVector& vtxVec )
	{
		std::ofstream stream( path , std::ios::binary );

		if ( !stream )
			return false;

		unsigned num = vtxVec.size();

		stream.write( (char*) &num , sizeof( num ) );

		for( unsigned i = 0 ; i < num  ;++i )
		{
			CurveVertex const& data = vtxVec[i];
			stream.write( (char*) &data , sizeof( data ) );
		}
		return true;
	}

	bool loadPathVertex( char const* path , CurveVertexVector& vtxVec )
	{
		std::ifstream stream( path , std::ios::binary );

		if ( !stream )
			return false;

		unsigned num;
		stream.read( (char*) &num , sizeof( num ) );

		vtxVec.reserve( num );

		for( unsigned i = 0 ; i < num  ;++i )
		{
			CurveVertex data;
			stream.read( (char*) &data , sizeof( data ) );
			vtxVec.push_back( data );
		}	

		return true;
	}

	ZDevStage::ZDevStage( ZLevelInfo& info )
		:ZStage( STAGE_DEV )
		,lvInfo( info )
	{
		buildCurve( 0 );
	}

	void ZDevStage::onKey( unsigned key , bool isDown )
	{
		switch( key )
		{
		case 'S': 
			savePathVertex( lvInfo.pathCurve[curCurveIndex].c_str() , vtxVec ); 
			break;
		case 'A': addPoint( mousePos ); break;
		case 'C': clearPoint(); break;
		case 'M': togglePointMask( mousePos ); break;
		case 'L': sendEvent( EVT_EXIT_DEV_STAGE , &lvInfo );
		case 'P': 
			buildCurve( (curCurveIndex + 1) % lvInfo.numCurve );
			break;
		}
	}

	bool ZDevStage::onMouse( MouseMsg const& msg  )
	{
		mousePos = Vector2( msg.getPos() );

		if ( msg.onLeftDown() )
		{
			mvIdx = getPoint( mousePos );
			return false;
		}
		else if ( msg.isLeftDown() && msg.isDraging() )
		{
			if ( mvIdx != -1 )
			{
				movePoint( mvIdx , mousePos );
				return false;
			}
		}

		return true;
	}

	void ZDevStage::clearPoint()
	{
		vtxVec.erase( vtxVec.begin() + 2 , vtxVec.end() );
	}

	void ZDevStage::movePoint( int idx , Vector2 const& dest )
	{
		assert( 0 <= idx && idx < vtxVec.size() );

		vtxVec[idx].pos = dest;

		for( int i = idx + 3 ; i >= idx - 3 ; --i )
		{
			if ( i >= vtxVec.size() )
				continue;

			if ( i < 3 )
				break;

			CRSpline2D& spline = splineVec[i-3];
			buildSpline( spline , i );
		}
	}

	void ZDevStage::addPoint( Vector2 const& pos )
	{

		CurveVertex data;

		data.pos  = pos;
		data.flag = 0;
		vtxVec.push_back( data );

		if ( vtxVec.size() < 4 )
			return;

		int idx = vtxVec.size() - 1;

		splineVec.push_back( CRSpline2D() );
		buildSpline( splineVec.back() , idx );
	}

	int ZDevStage::getPoint( Vector2 const& pos )
	{
		for( int i = 0 ; i < vtxVec.size() ; ++i )
		{
			CurveVertex& vtx = vtxVec[i];
			if ( (pos - vtx.pos ).length2() < 100 )
				return i;
		}
		return -1;
	}

	void ZDevStage::togglePointMask( Vector2 const& pos )
	{
		int idx = getPoint( pos );
		if ( idx != -1 )
		{
			if ( vtxVec[idx].flag & CurveVertex::eMask )
				vtxVec[idx].flag &= ~CurveVertex::eMask;
			else
				vtxVec[idx].flag |= CurveVertex::eMask;
		}
	}

	void ZDevStage::buildSpline( CRSpline2D& spline , int idxVtx )
	{
		spline.create(
			vtxVec[idxVtx-3].pos , vtxVec[idxVtx-2].pos , 
			vtxVec[idxVtx-1].pos , vtxVec[idxVtx].pos 
			);
	}

	void ZDevStage::buildCurve( int idx )
	{
		curCurveIndex = idx;
		mvIdx = -1;

		splineVec.clear();
		vtxVec.clear();

		if ( !loadPathVertex( lvInfo.pathCurve[idx].c_str() , vtxVec ))
		{
			vtxVec.reserve( 16 );
			addPoint( 0.8f * Vector2(150,120) );
			addPoint( 0.8f * Vector2(290,50) );
			addPoint( 0.8f * Vector2(580,55) );
			addPoint( 0.8f * Vector2(725,175) );
		}
		else
		{
			for( int i = 3 ; i < vtxVec.size() ; ++i )
			{			
				splineVec.push_back( CRSpline2D() );
				buildSpline( splineVec.back() , i );
			}
		}
	}

	LoadingStage::LoadingStage( char const* resID ) 
		:ZStage( STAGE_LOADING )
		,loadID( resID )
		,mThreadLoading( LoadingFun( this , &LoadingStage::loadResFun ) )
	{

	}

	void LoadingStage::onStart()
	{
		numTotal = Global::getResManager().countResNum( loadID );
		numCur   = numTotal;
		mThreadLoading.start();
	}

	void LoadingStage::loadResFun()
	{
		ScopeTickCount scope(gLoadingTime);
		Global::getRenderSystem().prevLoadResource();
		Global::getResManager().load( loadID );
		Global::getRenderSystem().postLoadResource();
	}

	void LoadingStage::onUpdate( long time )
	{
		if ( Global::getResManager().isLoading() )
			numCur = Global::getResManager().getUnloadedResNum();

		if ( !mThreadLoading.isRunning() )
		{
			sendEvent( EVT_LOADING_FINISH , this );
		}
	}


	ZStageController::~ZStageController()
	{
		delete mCurStage;
		delete mNextStage;
	}

	ZStageController::ZStageController()
	{
		mCurStage = new EmptyStage;
		mCurStage->mController = this;
		mNextStage = NULL;
	}

	void ZStageController::update( unsigned time )
	{
		if ( mNextStage )
		{
			mCurStage->onEnd();
			onStageEnd( mCurStage , m_chFlag );

			delete mCurStage;

			mNextStage->onStart();
			mCurStage = mNextStage;

			mNextStage = NULL;
		}
		mCurStage->onUpdate( time );
	}

	void ZStageController::changeStage( ZStage* stage , unsigned flag )
	{
		if ( mNextStage )
			delete mNextStage;

		m_chFlag    = flag;
		mNextStage = stage;
		mNextStage->mController = this;
		mNextStage->setParentHandler( this );
	}

	ZMainMenuStage::ZMainMenuStage()
		:MenuStage( STAGE_MAIN_MENU )
	{

	}

	void ZMainMenuStage::onStart()
	{
		MenuStage::onStart();

		//starPS = new ZStarPS( getRenderSystem() );
		//starPS->getEmitter().setPos( Vec2D(100,100) );

		angleSunGlow = 0;

		//getResManager().load( "MainMenu" );
		ZTexButton* button;
		getUISystem()->addWidget( new ZTexButton( UI_ADVENTLIRE , IMAGE_MM_ARCADE ,  Vec2i( 452 , 63) , NULL  ) );
		getUISystem()->addWidget( new ZTexButton( UI_GAUNTLET  , IMAGE_MM_GAUNTLET ,  Vec2i( 435 , 153 ) , NULL  ) );
		getUISystem()->addWidget( new ZTexButton( UI_OPTIONS_BUTTON , IMAGE_MM_OPTIONS , Vec2i( 423 , 237 ) , NULL  ) );
		getUISystem()->addWidget( new ZTexButton( UI_MORE_GAMES , IMAGE_MM_MOREGAMES , Vec2i( 395 , 306 ) , NULL  ) );
		getUISystem()->addWidget( new ZTexButton( UI_QUIT_GAME  , IMAGE_MM_QUIT , Vec2i(500 , 313 ) , NULL  ) );
	}

	void ZMainMenuStage::onUpdate( long time )
	{
		BaseClass::onUpdate( time );
		//starPS->update( time );
		angleSunGlow += 0.0001f * time;
	}



	AdvMenuStage::AdvMenuStage( ZUserData& data ) 
		:MenuStage( STAGE_ADV_MENU )
		,userData(data)
	{

	}

	void AdvMenuStage::onStart()
	{
		Global::getResManager().load( "AdventureScreen" );
		BaseClass::onStart();

		ZAdvButton::selectStageID = userData.unlockStage;

		finishTemp = ( userData.unlockStage - 1 )/ 3;

		ZAdvButton* button;
		if( createAdvButton( 1 , IMAGE_ADVDOOR1A , Vec2i(226,300) ) &&
			createAdvButton( 2 , IMAGE_ADVDOOR1B , Vec2i(297,269) ) &&
			createAdvButton( 3 , IMAGE_ADVDOOR1C , Vec2i(366,300) ) )
			//button = createAdvButton( 4 , IMAGE_ADVDOOR2A , TVec2D(226,300) );
			//button = createAdvButton( 5 , IMAGE_ADVDOOR2B , TVec2D(226,300) );
			//button = createAdvButton( 6 , IMAGE_ADVDOOR2C , TVec2D(226,300) );
			//button = createAdvButton( 7 , IMAGE_ADVDOOR3A , TVec2D(226,300) );
			//button = createAdvButton( 8 , IMAGE_ADVDOOR3B , TVec2D(226,300) );
			//button = createAdvButton( 9 , IMAGE_ADVDOOR3C , TVec2D(226,300) );
		{

		}

		ZTexButton* texButton;
		texButton = new ZTexButton( 
			UI_PLAY_STAGE , IMAGE_ADVPLAYBUTTON , Vec2i( 543 ,441 ) , NULL );
		getUISystem()->addWidget( texButton );
		texButton = new ZTexButton( 
			UI_MAIN_MENU , IMAGE_ADVMAINMENUBUTTON , Vec2i( 0 ,441 ) , NULL );
		getUISystem()->addWidget( texButton );
	}

	void AdvMenuStage::onUpdate( long time )
	{
		BaseClass::onUpdate( time );
	}

	ZAdvButton* AdvMenuStage::createAdvButton( unsigned stageID , ResID texID , Vec2i const& pos )
	{
		if ( ( stageID - 1 )/3 > finishTemp )
			return NULL;
		ZAdvButton* button;
		button = new ZAdvButton( texID , pos , NULL );
		button->stageID = stageID;
		button->enable( stageID <= userData.unlockStage );
		getUISystem()->addWidget( button );

		return button;
	}

}//namespace Zuma

