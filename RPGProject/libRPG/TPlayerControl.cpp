#include "ProjectPCH.h"
#include "TPlayerControl.h"

#include "ui/CGameUI.h"
#include "SpatialComponent.h"

#include "PhysicsSystem.h"
#include "CSceneLevel.h"
#include "TNavigator.h"

#include "EventType.h"
#include "UtilityGlobal.h"
#include "UtilityMath.h"

#include "CHero.h"
PlayerControl* g_PlayerControl = NULL;


PlayerControl::PlayerControl( CameraView* cam  , CGameUI* gameUI , CHero* player ) 
	:m_navigator( new TNavigator )
	,mGameUI( gameUI )
	,mCamControl( cam )
{

	g_PlayerControl = this;

	setPlayer( player );
	setCamera( cam );
	setDefaultSetting();

	m_navigator->setupOuter( player );

	controlMode = CTM_NORMAL;
	m_state     = CAMS_AUTO_MOVE;
	maxRotateAngle = player->getMaxRotateAngle();
	autoMoveTime   = 2.5;
	idleTime   = 0;
	turnAngle  = Math::Deg2Rad( 3 );
	controlBit = 0;
}

void PlayerControl::updateFrame()
{

	controlBit = InputControl::scanInput();

	if (  m_navigator->getNavStats() != NAV_MOVE_FINISH )
	{
		if ( controlBit & CT_MOVE_BIT )
			m_navigator->cancelRoute();
		else
		{
			m_navigator->performMovement();
				m_state = CAMS_AUTO_MOVE;
			updateCamera();
			return;
		}
	}

	if ( controlMode == CTM_TAKE_OBJ )
	{
		//m_takeObj->setPosition( m_player->getPosition() + m_player->getFaceDir() * 50 ) ;
	}

	if ( controlBit & CON_BIT( CT_USE ) )
	{
		if ( controlMode == CTM_TAKE_OBJ )
		{
			takeOffObj();
		}
		else if ( m_curChest )
		{
			//TPhyEntity* pe = m_curChest;
			//TChestTrigger* chest = UPCAST_PTR( TChestTrigger , pe );
			//m_gameUI->chestPanel->setItemStorage( &chest->getItems() );
			//m_gameUI->chestPanel->show( true );
		}
		else if ( mPlayer->getActivityType() == ACT_LADDER )
		{
			mPlayer->setActivity( ACT_JUMP );
		}
		else if ( mPlayer->getStateBit() & AS_CATCH_OBJ )
		{
			mPlayer->removeStateBit( AS_CATCH_OBJ );
		}
		//else if ( takeonObj() )
		//{

		//}
		else
		{
			mPlayer->evalUse();
		}

	}
	else if ( controlBit & CON_BIT( CT_DEFENSE ) )
	{
		mPlayer->defense( true );
	}
	else
	{
		if ( mPlayer->getActivityType() == CT_DEFENSE )
			mPlayer->defense( false );

		if ( controlBit & CON_BIT( CT_ATTACK ) )
		{
			mPlayer->attack();
		}
		else if ( mPlayer->getActivityType() != ACT_JUMP &&
				  mPlayer->getActivityType() != ACT_ATTACK )
		{
			//if ( m_player->getMovementType() == MT_LADDER )
			//{
			//	updateLadderMove();
			//}
			//else 
			{
				updateMove();
			}
		}
	}

	updateCamera();
	controlBit = 0;
}

void PlayerControl::updateLadderMove()
{
	m_state = CAMS_AUTO_MOVE;

	if ( controlBit & CON_BIT(CT_MOVE_FORWARD) )
	{
		moveDir = Vec3D( 0,0,1);
		mPlayer->moveSpace( moveDir * 100 );
	}
	else if ( controlBit & CON_BIT(CT_MOVE_BACK ) )
	{
		moveDir = Vec3D( 0 , 0 , -1 );
		mPlayer->moveSpace( moveDir * 100 );
	}
	else if ( controlBit & CON_BIT(CT_JUMP ) )
	{
		if ( controlBit & CON_BIT( CT_MOVE_LEFT ))
		{
			Vec3D dir = Vec3D(0,0,1).cross( mPlayer->getFaceDir() );
			mPlayer->jump( dir * 200 );
			//m_player->setMovementType( MT_NORMAL_MOVE );
		}
		else if ( controlBit & CON_BIT(CT_MOVE_RIGHT) )
		{
			Vec3D dir = - Vec3D(0,0,1).cross( mPlayer->getFaceDir() );
			mPlayer->jump( dir * 200 );
			//m_player->setMovementType( MT_NORMAL_MOVE );
		}
	}
}

Vec3D dbgMoveDir;

void PlayerControl::updateMove()
{
	moveDir.setValue( 0,0,0 );
	moveOffset = mPlayer->getMaxMoveSpeed();


	if ( ( controlBit & CON_BIT( CT_TURN_RIGHT )) &&
		!( controlBit & CON_BIT( CT_TURN_LEFT )) )
	{
		//m_camera->trunRight( turnAngle );
		mPlayer->turnRight( turnAngle );
		m_state = CAMS_AUTO_MOVE;
	}
	else if ( ( controlBit & CON_BIT( CT_TURN_LEFT ) ) &&
		     !( controlBit & CON_BIT( CT_TURN_RIGHT ) ) )	      
	{
		//m_camera->trunRight( -turnAngle );
		mPlayer->turnRight( -turnAngle );
		m_state = CAMS_AUTO_MOVE;
	}

	unsigned moveBit = controlBit & CT_MOVE_BIT;

	if ( moveBit & CON_BIT( CT_MOVE_FORWARD ) )
	{
		Vec3D v1 = mCamera->getViewDir();
		Vec3D v2 = mPlayer->getFaceDir();

		moveDir += mCamera->getViewDir();
	}
	else if ( moveBit & CON_BIT( CT_MOVE_BACK ) )
	{
		moveDir -= mCamera->getViewDir();
	}

	if ( moveBit & CON_BIT( CT_MOVE_RIGHT ) )
	{
		moveDir -= mCamera->transLocalDir( gLocalCamRightDir );
	}
	else if ( moveBit & CON_BIT( CT_MOVE_LEFT ) )
	{
		moveDir += mCamera->transLocalDir( gLocalCamRightDir );
	}

	if ( controlBit & CON_BIT( CT_JUMP ) )
	{
		moveDir[2] = 0;
		if ( moveDir.length2() != 0 )
			moveDir.normalize();
		mPlayer->jump( moveDir * moveOffset );

	}
	else if ( moveBit )
	{
		idleTime = 0;
		moveDir[2] = 0;
		moveDir.normalize();

		dbgMoveDir = moveDir;

		Vec3D faceDir = mPlayer->getFaceDir();

		float angle = UM_Angle( moveDir , faceDir );

		float turnAngle = TMin( maxRotateAngle , angle );

		if ( !( controlBit & CT_TURN_BIT ) )
		{
			m_state = CAMS_LOOK_ACTOR;
			if ( moveDir.cross( faceDir ).z > 0 )
			{
				mPlayer->turnRight( -turnAngle );
			}
			else
			{
				mPlayer->turnRight( turnAngle );
			}
		}

		if ( angle < Math::Deg2Rad(60) )
			mPlayer->moveFront( moveOffset );
	}
	else
	{
		mPlayer->setActivity( ACT_IDLE );

		idleTime += g_GlobalVal.frameTime;

		if ( idleTime >= autoMoveTime )
		{
			m_state = CAMS_AUTO_MOVE;
		}
	}
}



void PlayerControl::updateCamera()
{

	Vec3D playerPos = mSpatialComp->getPosition() + mPlayer->getBodyHalfHeigh() * Vec3D(0,0,1);

	if ( controlMode == CTM_FREE_VIEW )
	{
		Vec3D dir = mPlayer->getFaceDir();
		Vec3D goalCamPos = playerPos - 50 * dir + 
			               mPlayer->getBodyHalfHeigh() * Vec3D(0,0,1);

		if ( DistanceSqure( mCamera->getPosition() , goalCamPos ) > 1 )
			mCamControl.move( playerPos , goalCamPos , dir );
		return;
	}


	Vec3D goalCamPos;
	Vec3D ObjLookDir;

	if ( m_state == CAMS_LOOK_ACTOR )
	{

		Vec3D distDir = mCamera->getPosition() - playerPos;
		distDir[2] = 0;
		float dist = distDir.length();
		float offset = dist - mCamControl.getCamToObjDist();

		goalCamPos = playerPos;
		goalCamPos += mCamControl.getCamToObjDist() / dist * distDir;
		goalCamPos += mCamControl.getCamHeight() * Vec3D( 0,0,1 );

		ObjLookDir = mPlayer->getFaceDir();

		mCamControl.setTurnSpeed( Math::Deg2Rad(90) );
	}
	else if ( m_state == CAMS_AUTO_MOVE )
	{
		goalCamPos =  computeMovePos();
		ObjLookDir = mPlayer->getFaceDir();//actor->getPosition() - goalCamPos ;
		ObjLookDir[2] = 0;
		ObjLookDir.normalize();

		mCamControl.setTurnSpeed( Math::Deg2Rad(90) );
	}

	//m_camera->move( actor->getPosition() , goalCamPos ,ObjLookDir );


	RayTraceResult  result;
	if ( mSceneLevel->getPhysicsWorld()->testRayCollision( 
		 playerPos , goalCamPos , COF_TERRAIN | COF_WATER , 
		 mPlayer->getEntity()->getComponentT< PhyCollision >( COMP_PHYSICAL ) , result ) )
	{
		goalCamPos = Math::Lerp( playerPos , goalCamPos , result.hitFraction );
	}

	Vec3D distDir = goalCamPos - playerPos;
	distDir[2] = 0;

	float camToObjLen2 = Vec3D( 0, mCamControl.getCamToObjDist() , mCamControl.getCamHeight() ).length2();
	float height = sqrt( camToObjLen2 - distDir.length2() );

	Vec3D tempPos = goalCamPos + ( height - mCamControl.getCamHeight() ) * Vec3D( 0,0,1 );

	//if ( !RayLineTest(m_actor->getPosition() , tempPos , COF_TERRAIN | COF_WATER , m_actor , &callback) )
	//{
	//	
	//}

	goalCamPos = tempPos;
	mCamControl.move( playerPos , goalCamPos , ObjLookDir );
}

void PlayerControl::setCamera( CameraView* camera )
{
	assert( camera );
	mCamera = camera;
	mCamControl.changeCamera( camera );

	mCamControl.setObjLookDist( 0 );
	mCamControl.setCamToObjDist( 300 );
	mCamControl.setMoveAnglurSpeed( Math::Deg2Rad( 90 ) );
	mCamControl.setObjLookDist( 60 );
}

Vec3D PlayerControl::computeMovePos()
{
	Vec3D pos = mSpatialComp->getPosition() + 0.5 * mPlayer->getBodyHalfHeigh() * Vec3D(0,0,1);

	pos -=  mCamControl.getCamToObjDist() * mPlayer->getFaceDir();
	pos +=  mCamControl.getCamHeight() * Vec3D( 0,0,1 );

	return pos;
}

bool PlayerControl::onMessage( MouseMsg& msg )
{
	if ( msg.onLeftDown() )
	{
		controlBit |= CON_BIT( CT_ATTACK );
	}
	else if ( msg.onRightDown() )
	{
		controlMode = CTM_FREE_VIEW;
		oldX = msg.x();
		oldY = msg.y();
	}
	else if ( msg.onRightUp() )
	{
		controlMode = CTM_NORMAL;
	}
	else if ( msg.onWheelFront() )
	{
		mCamControl.setCamToObjDist(
			mCamControl.getCamToObjDist() - 10 );
	}
	else if ( msg.onWheelBack() )
	{
		mCamControl.setCamToObjDist(
			mCamControl.getCamToObjDist() + 10 );
	}
	else if ( msg.onLeftDClick() )
	{
		//TPhyEntity* pe = choiceEntity( msg.getPos() );
	}

	//if ( msg.isRightDown() && msg.onDraging() )
	//{
	//	m_camera->trunViewByMouse( oldX,oldY , msg.x() , msg.y() );

	//	oldX = msg.x(); 
	//	oldY = msg.y();
	//}


	return false;
}

void PlayerControl::onHotKey( ControlKey key )
{
	TEvent event( EVT_HOT_KEY , key , nullptr , nullptr );
	UG_SendEvent( event );
}

TObject* PlayerControl::findObj()
{
	//TEntityManager& manager = TEntityManager::instance();

	//TEntityManager::iterator iter = manager.getIter();

	//float r2 = 100 * 100;
	//Vec3D const& pos = mSpatialComp->getPosition();

	//while ( TEntityBase* entity = manager.findEntityInSphere( iter , pos , r2 ) )
	//{
	//	if ( TObject* obj = TObject::downCast( entity ) )
	//	{
	//		return obj;
	//	}
	//}

	return NULL;
}

bool PlayerControl::takeOnObj()
{
	//m_takeObj = findObj();
	//if ( m_takeObj )
	//{
	//	m_takeObj->setKimematic( true );
	//	m_angularFactor = m_takeObj->getPhyBody()->getAngularFactor();
	//	m_takeObj->getPhyBody()->setAngularFactor( 0 );
	//	m_takeObj->getPhyBody()->setAngularVelocity( Vec3D(0,0,0) );
	//	m_takeObj->getPhyBody()->setGravity( Vec3D(0,0, 0 ) );

	//	
	//	Vec3D to = 	 mSpatialComp->getPosition() + m_player->getFaceDir() * 100 ;
	//	Vec3D from = m_takeObj->getPosition();
	//	from[2] = to.z();

	//	TraceCallback callback;
	//	RayLineTest( from  , to , COF_ACTOR , m_takeObj , &callback );
	//	if ( callback.hasHit() )
	//	{
	//		m_takePos = callback.getHitPos();
	//	}

	//	m_takePos = to - mSpatialComp->getPosition();
	//	
	//	controlMode = CTM_TAKING_OBJ;
	//	m_moveStep = 0;

	//	int index = addThink( (fnThink) &RPGPlayerControl::moveObjThink  );
	//	setNextThink( index , g_GlobalVal.nextTime );
	//	return true;
	//}
	return false;
}

void PlayerControl::takeOffObj()
{
	controlMode = CTM_NORMAL;

	//m_takeObj->setKimematic( false );
	//m_takeObj->getPhyBody()->setAngularFactor( m_angularFactor );
	//m_takeObj->getPhyBody()->activate();
	//m_takeObj->getPhyBody()->setGravity( Vec3D(0,0, - g_GlobalVal.gravity ) );
	//m_takeObj = NULL;
}

void PlayerControl::moveObjThink()
{
	if ( m_moveStep == 0 )
	{
		controlMode = CTM_TAKE_OBJ;
	}
	else
	{

	}
}

void PlayerControl::setRoutePos( Vec3D const& pos )
{
	m_navigator->setGoalPos( pos , 20 );
}

void PlayerControl::changeLevel( CSceneLevel* level )
{
	mSceneLevel = level;
	if ( mGameUI )
		mGameUI->changeLevel( level );
	m_navigator->setNavMesh( level->getNavMesh() );
}

//bool PlayerControl::changeChest( TChestTrigger* chest )
//{
//	if ( m_curChest )
//		return false;
//
//	//TEventSystem::getInstance().disconnectEvent( this , EVT_ENTITY_DESTORY );
//	m_curChest = chest;
//	//TEventSystem::getInstance().connectEvent( EVT_ENTITY_DESTORY , chest->getRefID() , this , &RPGPlayerControl::onChestDestory );
//	return true;
//}

//bool PlayerControl::removeChest( TChestTrigger* chest )
//{
//	if ( m_curChest != chest )
//		return false;
//
//	m_curChest = NULL;
//	m_gameUI->chestPanel->show( false );
//
//	return true;
//}

//void PlayerControl::onChestDestory()
//{
//	m_curChest = NULL;
//	m_gameUI->chestPanel->show( false );
//}

ILevelObject* PlayerControl::choiceEntity( Vec2i const& pos , int sw , int sh )
{
	Vec3D dir = mCamera->calcScreenRayDir( pos.x , pos.y , sw , sh );
	return NULL;
}

void PlayerControl::setDefaultSetting()
{
	registerKey( KEY_UP    , CT_MOVE_FORWARD , true );
	registerKey( KEY_LEFT  , CT_MOVE_LEFT    , true );
	registerKey( KEY_DOWN  , CT_MOVE_BACK    , true );
	registerKey( KEY_RIGHT , CT_MOVE_RIGHT   , true );
	registerKey( KEY_W     , CT_MOVE_FORWARD , true );
	registerKey( KEY_A     , CT_MOVE_LEFT    , true );
	registerKey( KEY_S     , CT_MOVE_BACK    , true );
	registerKey( KEY_D     , CT_MOVE_RIGHT   , true );
	registerKey( KEY_Q     , CT_TURN_LEFT    , true );
	registerKey( KEY_E     , CT_TURN_RIGHT   , true );
	registerKey( KEY_R     , CT_USE          , false );
	registerKey( KEY_G     , CT_ATTACK       , false );
	registerKey( KEY_F     , CT_DEFENSE      , true );
	registerKey( KEY_SPACE , CT_JUMP         , false );

	registerKey( KEY_1     , CT_FAST_PLAY_1  , false );
	registerKey( KEY_2     , CT_FAST_PLAY_2  , false );
	registerKey( KEY_3     , CT_FAST_PLAY_3  , false );
	registerKey( KEY_4     , CT_FAST_PLAY_4  , false );
	registerKey( KEY_5     , CT_FAST_PLAY_5  , false );
	registerKey( KEY_6     , CT_FAST_PLAY_6  , false );
	registerKey( KEY_7     , CT_FAST_PLAY_7  , false );
	registerKey( KEY_8     , CT_FAST_PLAY_8  , false );
	registerKey( KEY_9     , CT_FAST_PLAY_9  , false );
	registerKey( KEY_0     , CT_FAST_PLAY_0  , false );

	registerKey( KEY_X     , CT_SHOW_EQUIP_PANEL  , false );
	registerKey( KEY_C     , CT_SHOW_ITEM_BAG_PANEL, false );
	registerKey( KEY_Z     , CT_SHOW_SKILL_PANEL, false );
}

void PlayerControl::setPlayer( CHero* player )
{
	mPlayer = player; 
	mSpatialComp = player->getSpatialComponent();
}

unsigned InputControl::scanInput()
{
	unsigned controlBit = 0;

	for( std::list<KeyInfo>::iterator iter = ConList.begin();
		iter != ConList.end() ; ++iter )
	{
		int vkey = iter->vKey;
		if ( ::GetAsyncKeyState( vkey ) ) 
		{
			if ( state[ vkey ] )
			{
				if ( iter->conkey < CT_CONTROL_KEY )
				{
					controlBit |= CON_BIT( iter->conkey );
				}
				else
				{
					onHotKey( iter->conkey );
				}
				state[ vkey ] = iter->keep;
			}
		}
		else
		{
			state[vkey] = true;
		}
	}

	return controlBit;
}

void InputControl::registerKey( int vKey , ControlKey conKey , bool keep )
{
	KeyInfo info;
	info.conkey = conKey;
	info.vKey   = vKey;
	info.keep   = keep;
	ConList.push_back( info );
}
