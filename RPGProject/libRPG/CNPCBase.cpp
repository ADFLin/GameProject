#include "ProjectPCH.h"
#include "CNPCBase.h"

#include "TNavigator.h"
#include "CSceneLevel.h"
#include "TRoleTemplate.h"

#include "SpatialComponent.h"

#include "ConsoleSystem.h"
#include "UtilityMath.h"

ConVar< float > npc_dead_body_save_time( 2 , "npc_dead_body_save_time" );
ConVar< float > npc_chest_vanish_time( 30 , "npc_chest_vanish_time" );
ConVar< float > npc_dead_body_vanish_time( 10 , "npc_dead_body_vanish_time" );
ConVar< bool >  npc_ai_enable( true , "npc_ai_enable" );

int g_useScheduleNum = 0;


DEFINE_AI_ACTION_SCHEDULE( CNPCBase , SCHE_NO_MOVE_IDLE )
	ADD_TASK( TASK_STOP_MOVE , 0 )
	ADD_TASK( TASK_WAIT      , 5.0f )
END_SCHDULE()

DEFINE_AI_ACTION_SCHEDULE( CNPCBase , SCHE_ATTACK  )
	ADD_TASK( TASK_STOP_MOVE        , 0 )
	ADD_TASK( TASK_SET_EMPTY_TRAGET , 0 )
	ADD_TASK( TASK_FACE_TARGET      , Math::Deg2Rad( 20.0f ) )
	ADD_TASK( TASK_ATTACK           , 0 )
	ADD_TASK( TASK_WAIT_ATTACK      , 0 )
END_SCHDULE()


DEFINE_AI_ACTION_SCHEDULE( CNPCBase , SCHE_MOVE_TO_TARGET )
	ADD_TASK( TASK_STOP_MOVE        , 0 )
	ADD_TASK( TASK_NAV_GOAL_TARGET  , 50.0f )
	ADD_TASK( TASK_WAIT_MOVEMENT    , 0 )
END_SCHDULE()


DEFINE_AI_ACTION_SCHEDULE( CNPCBase , SCHE_GUARD_POSION )
	ADD_TASK( TASK_SAVE_SELF_POS     , 0 )
	ADD_TASK( TASK_NAV_RAND_GOAL_POS , 300.0f )
	ADD_TASK( TASK_WAIT_MOVEMENT     , 1.0f )
	ADD_TASK( TASK_WAIT              , 2.0f )
	ADD_TASK( TASK_SCHEDULE_MAX_TIME , 30.0f )
	ADD_TASK( TASK_CHANGE_TASK       , 1 )
END_SCHDULE()

CLASS_AI_START( CNPCBase )

	ADD_SCHDULE( 
		SCHE_NO_MOVE_IDLE , 
		BIT( CDT_FIND_EMPTY ) |
		BIT( CDT_TAKE_DAMAGE )
	)

	ADD_SCHDULE( 
		SCHE_ATTACK , 
		BIT( CDT_EMPTY_DEAD ) |
		BIT( CDT_TAKE_DAMAGE ) 
	)

	ADD_SCHDULE( 
		SCHE_MOVE_TO_TARGET , 
		BIT( CDT_TAKE_DAMAGE ) 
	)

	ADD_SCHDULE( 
		SCHE_GUARD_POSION , 
		BIT( CDT_FIND_EMPTY ) |
		BIT( CDT_TAKE_DAMAGE ) 
	)

CLASS_AI_END()


CNPCBase::CNPCBase() 
{
	mNavigator.reset( new TNavigator );
}

bool CNPCBase::init( GameObject* gameObject , GameObjectInitHelper const& helper )
{
	if ( !BaseClass::init( gameObject , helper ) ) 
		return false;

	m_hEmpty = NULL;
	m_moveIntervalTime = 0;
	mWaitTime = 0;
	m_conditionBit = 0;

	mNavigator->setupOuter( this );
	setTeam( TEAM_MONSTER );
	setAIState( NPC_IDLE );

	return true;
}

CNPCBase::~CNPCBase()
{

}

void CNPCBase::destoryThink( ThinkContent& content )
{
	float dt = g_GlobalVal.curtime - ( m_deadTime + npc_dead_body_save_time );

	if ( dt < npc_dead_body_vanish_time )
	{
		float opacity = 1 - dt / npc_dead_body_vanish_time;

		float s = 0.97f;
		mModel->getCFActor()->scale( Vec3D(s,s,s) , CFly::CFTO_LOCAL );
		mModel->getCFActor()->setOpacity( opacity );
		content.nextTime = g_GlobalVal.nextTime;
	}
	else 
	{
		//getSceneLevel()->removeObject( this );
	}
}

int g_dbgNumChest = 0;
void CNPCBase::productChestThink( ThinkContent& content )
{
	++g_dbgNumChest;
	DevMsg( 5, "g_dbgNumChest = %d" , g_dbgNumChest );

	//TChestTrigger* trigger  = new TChestTrigger;
	//Vec3D pos = mSpatialComp->getPosition();

	//RayTraceResult result;
	//getSceneLevel()->getPhysicsWorld()->testRayCollision( 
	//	pos , pos + 200 * Vec3D(0,0,-1) , COF_TERRAIN , nullptr , result );

	//if ( result.haveHit )
	//{
	//	trigger->setPosition( result.hitPos );
	//}
	//else
	//{
	//	trigger->setPosition( pos );
	//}

	//int level = 10000;
	//int maxID = TItemManager::getInstance().getMaxItemID() + 1;

	//while ( level > 10 )
	//{
	//	trigger->addItem( TRandom() % maxID , 1 );
	//	level /= ( TRandom() % 20 ) + 1;
	//}

	//trigger->setVanishTime( npc_chest_vanish_time );
	//TEntityManager::instance().addEntity( trigger );
}

void CNPCBase::onDead()
{
	m_deadTime = g_GlobalVal.curtime;
	
	PhyCollision* comp = getEntity()->getComponentT< PhyCollision >( COMP_PHYSICAL );

	if ( comp )
	{

	}
	//TPhySystem::instance().removeEntity( this );
	//m_movement->enable( false );

	//for( int i = 0 ; i < getFlyActor().SkinNumber(); ++i )
	//{
	//	FnObject obj; obj.Object( getFlyActor().GetSkin(i) );

	//	MATERIALid matID[16];
	//	int num = obj.GetMaterials( matID );
	//	obj.UseMaterialOpacity( TRUE );
	//}
	{
		ThinkContent* tc = setupThink( this , &CNPCBase::destoryThink );
		tc->nextTime = g_GlobalVal.curtime + npc_dead_body_save_time;
	}
	{
		ThinkContent* tc = setupThink( this , &CNPCBase::productChestThink );
		tc->nextTime = g_GlobalVal.curtime;
	}
}

void CNPCBase::onSpawn()
{
	BaseClass::onSpawn();
	getNavigator()->setNavMesh( getSceneLevel()->getNavMesh() );
}

void CNPCBase::takeDamage( DamageInfo const& info )
{
	addCondition( CDT_TAKE_DAMAGE );
	BaseClass::takeDamage( info );
}


bool CNPCBase::haveCondition( ConditionType con )
{
	return ( m_conditionBit & BIT( con ) );
}

void CNPCBase::clearCondition( ConditionType con )
{
	m_conditionBit &= ~BIT(con);
}

void CNPCBase::addCondition( ConditionType con )
{
	m_conditionBit |=  BIT( con );
}

void CNPCBase::clearEmptyCondition( )
{
	clearCondition( CDT_EMPTY_DEAD );
	clearCondition( CDT_EMPTY_IN_ATTACK_RANGE );
	clearCondition( CDT_EMPTY_TOO_FAR );
	clearCondition( CDT_EMPTY_OUT_VIEW );
	clearCondition( CDT_EMPTY_IN_ATTACK_ANGLE );
}

void CNPCBase::getherEmptyCondition()
{
	clearEmptyCondition();

	if ( m_hEmpty ->isDead() )
	{
		m_hEmpty = NULL;
		addCondition( CDT_EMPTY_DEAD );
		return;
	}

	Vec3D p1 = getPosition();
	Vec3D p2 = m_hEmpty->getPosition();

	float dist = Math::Distance( p1 , p2 );
	if ( dist <= mAbilityProp->getPropValue( PROP_AT_RANGE ) + getNavigator()->getOuterOffset() )
	{
		addCondition( CDT_EMPTY_IN_ATTACK_RANGE );
	}
	else
	{
		addCondition( CDT_EMPTY_TOO_FAR );
	}

	if ( dist > getVisibleDistance() )
	{
		addCondition( CDT_EMPTY_OUT_VIEW );
	}

	float angle = UM_PlaneAngle( getFaceDir() ,getPosition() , m_hEmpty->getPosition() );

	if ( fabs( angle ) < Math::Deg2Rad( 45 ) )
	{
		addCondition( CDT_EMPTY_IN_ATTACK_ANGLE );
	}

	m_emptyConInfo.dist = dist;
	m_emptyConInfo.angle = angle;
}

void CNPCBase::gatherCondition()
{
	if ( m_hEmpty )
	{
		getherEmptyCondition();
	}
}

void CNPCBase::runAI()
{
	if ( ! npc_ai_enable )
		return;

	TPROFILE("NPCThink");
	if ( isDead() )
		return;

	sceneWorld();
	gatherCondition();
	maintainSchedule();

	clearCondition( CDT_TAKE_DAMAGE );
}

void CNPCBase::setAIState( NPCAIState state )
{
	m_AIState = state;
}

void CNPCBase::sceneWorld()
{
	if ( m_hEmpty == NULL )
	{
		clearCondition( CDT_FIND_EMPTY );
		CActor* actor = findEmpty( getVisibleDistance() );
		if ( actor != NULL )
		{
			m_hEmpty = actor;
			addCondition( CDT_FIND_EMPTY );
		}
	}
}


int CNPCBase::selectSchedule()
{
	switch ( getAIState() )
	{
	case NPC_IDLE:
		if ( haveCondition( CDT_FIND_EMPTY ) )
		{
			setAIState( NPC_COMBAT );
			m_targetPE = m_hEmpty;
			return SCHE_MOVE_TO_TARGET;
		}
		else 
		{
			int val  = TRandom() % 10;
			if ( val > 3 )
			{
				return SCHE_NO_MOVE_IDLE ;
			}
			else
			{
				return SCHE_GUARD_POSION;
			}
		}
		break;

	case NPC_COMBAT:
		{
			if (  haveCondition( CDT_EMPTY_DEAD )  || haveCondition( CDT_EMPTY_OUT_VIEW ) )
			{
				m_hEmpty = NULL;
				setAIState( NPC_IDLE );
				return SCHE_NO_MOVE_IDLE;
			}
			else if ( canDoAction() && haveCondition( CDT_EMPTY_IN_ATTACK_RANGE ) )
			{
				return SCHE_ATTACK;
			}
			else if ( haveCondition( CDT_EMPTY_TOO_FAR ))
			{
				m_targetPE = m_hEmpty;
				return SCHE_MOVE_TO_TARGET;
			}
		}
	case NPC_DEAD:
		break;
	}

	return SCHE_NO_SCHEDULE;
}

void CNPCBase::startTask( AITask const& task )
{
	switch ( task.type )
	{
	case TASK_WAIT :
		mModel->changeAction( ANIM_WAIT );
		mWaitTime = g_GlobalVal.curtime + task.FloatVal;
		break;

	case TASK_SAVE_SELF_POS :
		m_savePos = mSpatialComp->getPosition();
		taskFinish();
		break;

	case  TASK_STOP_MOVE:
		mNavigator->cancelRoute();
		taskFinish();
		break;

	case TASK_UPDATE_GOAL_EMPTY_POS:
		if ( m_hEmpty )
		{
			mNavigator->updateEntityPos( m_hEmpty->getPosition() );
			taskFinish();
		}
		else
		{
			taskFail( TF_NO_EMPTY );
		}
		break;

	case TASK_SET_EMPTY_TRAGET:
		if ( m_hEmpty )
		{
			m_targetPE = m_hEmpty;
			taskFinish();
		}
		else taskFail( TF_NO_EMPTY );
		break;
	case TASK_SCHEDULE_MAX_TIME:
		if ( g_GlobalVal.curtime - mSchdlestatus.startTime > task.FloatVal  )
		{
			taskFail( TF_UNKNOW );
		}
		else
		{
			taskFinish();
		}
		break;

	case TASK_NAV_GOAL_TARGET :
		if ( !m_targetPE )
		{
			taskFail( TF_NO_PATH );
			break;
		}
		
		getNavigator()->setGoalEntity( m_targetPE , task.FloatVal );

		if ( getNavigator()->getNavStats() != NAV_NO_FIND_PATH )
		{
			taskFinish();
		}
		else
		{
			taskFail( TF_NO_PATH );
		}
		break;
	case  TASK_NAV_GOAL_POS:
		getNavigator()->setGoalPos( m_savePos , task.FloatVal );
		if ( getNavigator()->getNavStats() != NAV_NO_FIND_PATH )
		{
			taskFinish( );
		}
		else
		{
			taskFail( TF_NO_PATH );
		}
		break;
	case  TASK_NAV_RAND_GOAL_POS:
		{
			float r = TRandom( 0 , task.FloatVal );
			float theta = TRandom( 0 , 2 * Math::PI );
			Vec3D dx( r * cos( theta ) , r * sin( theta ) , 0  );
			getNavigator()->setGoalPos( m_savePos + dx , 0  );
			if ( getNavigator()->getNavStats() != NAV_NO_FIND_PATH )
			{
				taskFinish();
			}
			else
			{
				taskFail( TF_NO_PATH );
			}
		}
		break;

	case  TASK_CHANGE_TASK :
		if ( task.IntVal >= getCurSchedule()->numTask ||
			 task.IntVal <  0 )
		{
			taskFail( TF_UNKNOW );
			break;
		}
		mSchdlestatus.curTaskIndex = task.IntVal - 1;
		taskFinish();
		break;

	case  TASK_SAVE_RAND_2D_POS :
		float theta = TRandom( 0 , 2 * Math::PI );
		m_savePos = mSpatialComp->getPosition() + 
			Vec3D( task.FloatVal * cos( theta ) , 
			       task.FloatVal * sin( theta ) , 0  );
		taskFinish();
		break;
	}
}

void CNPCBase::runTask( AITask const& task )
{
	switch ( task.type )
	{
	case TASK_WAIT :
		if ( mWaitTime < g_GlobalVal.curtime )
			taskFinish();
		break;

	case TASK_FACE_TARGET:
		if ( m_targetPE )
		{
			float angle = UM_PlaneAngle( getFaceDir() , getPosition() , m_targetPE->getPosition() );

			if ( fabs( angle ) < task.FloatVal )
			{
				taskFinish();
			}
			else
			{
				if ( angle < 0 )
					turnRight( TMax( angle , -getMaxRotateAngle() ) );
				else
					turnRight( TMin( angle ,  getMaxRotateAngle() ) );
			}			
		}
		else  taskFail( TF_NO_TARGET );
		break;

	case TASK_WAIT_CONDITION :
		if ( haveCondition( ConditionType( task.IntVal ) ) )
			taskFinish();
		break;

	case TASK_WAIT_ATTACK :
		if ( getActivityType() != ACT_ATTACK )
			taskFinish();
		break;

	case  TASK_WAIT_MOVEMENT:
		if ( getNavigator()->getNavStats() == NAV_MOVE_FINISH )
			taskFinish();
		break;
	case  TASK_ATTACK:
		if ( attack() )
			taskFinish();
		else
			taskFail( TF_CANT_ATTACK );
		break;

	}
}

void CNPCBase::taskFail( TaskFailCode code )
{
	mSchdlestatus.taskState = TKS_FAIL ;
	mSchdlestatus.failCode = code ;
	addCondition( CDT_TASK_FAIL );


}

void CNPCBase::taskFinish()
{
	mSchdlestatus.taskState = TKS_SUCCESS ;
	addCondition( CDT_TASK_SUCCESS );
}

AIActionSchedule* CNPCBase::getScheduleByType( int sche )
{
	static int const size = ARRAY_SIZE( GET_SCHEDULE_DATA( ThisClass ) );

	for ( int i = 0 ; i < size ; ++i )
	{
		if ( GET_SCHEDULE_DATA( ThisClass )[i].type == sche )
			return &GET_SCHEDULE_DATA( ThisClass )[i];
	}

	Msg("No Schedule Define %d" , sche );
	return NULL;
}

void CNPCBase::faceTarget( ILevelObject* target )
{
	Vec3D p1 = getPosition();
	p1[2] = 0;
	Vec3D p2 = target->getPosition();
	p2[2] = 0;
	setFaceDir( p2 - p1 );
}

bool CNPCBase::onAttackTest()
{
	if ( !BaseClass::onAttackTest() )
		return false;

	return true;
}

void CNPCBase::maintainSchedule()
{
	if ( isDead() )
		return;

	PROFILE_ENTRY( "maintainSchedule" );

	int const MAX_TASK_LOOP = 20;

	for( int loop = 0; loop < MAX_TASK_LOOP  ; ++loop )
	{
		if ( !checkScheduleValid() )
		{
			AIActionSchedule* newSche = getNewSchedule();
			changeSchedule( newSche );
			
			++g_useScheduleNum;
			DevMsg( 5 , "%d , change Schedule = %s" , 
				g_useScheduleNum ,  getCurSchedule()->name );
		}

		if ( !getCurSchedule() )
		{
			DevMsg( 5 , "No Schedule run" );
			break;
		}

		AITask const* curTask = getCurTask();

		if ( getCurTaskState() == TKS_NEW )
		{
			DevMsg( 5 , "new Task  = %s" , curTask->name );
			startTask( *curTask );

			if ( getCurTaskState() == TKS_NEW )
			{
				mSchdlestatus.taskState = TKS_RUNNING;
			}
		}

		if ( getCurTaskState() == TKS_RUNNING )
		{
			runTask( *curTask );
		}

		bool done = false;
		switch ( getCurTaskState() )
		{
		case TKS_SUCCESS: 
			changeNextTask();
			break;
		case TKS_FAIL:    
			DevMsg( 5 , "Task Fail : %s " , curTask->name );
			done = true;
			break;
		case TKS_RUNNING:
			done = true;
			break;
		}

		if ( done )
			break;

		if ( loop == MAX_TASK_LOOP )
		{
			Msg( "%Max Task Loop : %s" , getCurSchedule()->name );
		}
	}
}

AIActionSchedule* CNPCBase::getNewSchedule()
{
	TPROFILE( "getNewSchedule" );
	
	int scheType = selectSchedule();

	AIActionSchedule* result = getScheduleByType( scheType );
	if ( result == NULL )
	{
		result = getScheduleByType( SCHE_NO_MOVE_IDLE );
	}
	return result;
}

void CNPCBase::changeSchedule( AIActionSchedule* sche )
{
	clearCondition( CDT_TASK_FAIL );
	clearCondition( CDT_SCHEDULE_END );

	mSchdlestatus.data      = sche;
	mSchdlestatus.failCode  = TF_NO_FAIL;
	mSchdlestatus.taskState = TKS_NEW;
	mSchdlestatus.curTaskIndex = 0;
	mSchdlestatus.startTime = g_GlobalVal.curtime;

	onChangeSchedule();
}

AITask const* CNPCBase::getCurTask()
{
	if ( mSchdlestatus.data )
		return &mSchdlestatus.data->taskList[ mSchdlestatus.curTaskIndex ];

	return NULL;
}

bool CNPCBase::checkScheduleValid()
{
	if ( !getCurSchedule() )
		return false;

	if ( mSchdlestatus.curTaskIndex == getCurSchedule()->numTask )
		return false;

	if ( m_conditionBit & getCurSchedule()->interruptsCond )
		return false;

	if ( haveCondition( CDT_SCHEDULE_END ) ||
		 haveCondition( CDT_TASK_FAIL ) )
	{
		return false;
	}

	return true;
}

void CNPCBase::changeNextTask()
{
	clearCondition( CDT_TASK_SUCCESS );

	mSchdlestatus.failCode  = TF_NO_FAIL;
	mSchdlestatus.taskState = TKS_NEW;

	++mSchdlestatus.curTaskIndex;

	if ( mSchdlestatus.curTaskIndex == mSchdlestatus.data->numTask )
	{
		addCondition( CDT_SCHEDULE_END );
	}
}

void CNPCBase::onDying()
{
	BaseClass::onDying();
	if ( getCurTask()->type == TASK_WAIT_MOVEMENT )
	{
		int i = 0;
	}
	getNavigator()->cancelRoute();
}

void CNPCBase::update( long time )
{
	runAI();
	mNavigator->performMovement();
	BaseClass::update( time );
}
