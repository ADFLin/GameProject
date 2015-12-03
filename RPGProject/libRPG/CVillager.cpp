#include "ProjectPCH.h"
#include "CVillager.h"

#include "SpatialComponent.h"

#include "TRoleTemplate.h"
#include "AICondition.h"

#include "EventType.h"
#include "UtilityGlobal.h"

#include "ui/CStoreFrame.h"
#include "TTalkSystem.h"


DEFINE_AI_ACTION_SCHEDULE( CVillager , SCHE_SELL_ITEMS )
	ADD_TASK( TASK_FACE_TARGET       , Math::Deg2Rad( 10 ) )
	ADD_TASK( TASK_TALK              , 1 )
	ADD_TASK( TASK_WAIT              , 1.0f )
	ADD_TASK( TASK_TRADE_ITEMS       , 0 )
	ADD_TASK( TASK_WAIT              , 0.5f )
	ADD_TASK( TASK_WAIT_TRADE        , 0 )
	ADD_TASK( TASK_TALK              , 2 )
	ADD_TASK( TASK_WAIT_TALK         , 0 )
END_SCHDULE()

DEFINE_AI_ACTION_SCHEDULE( CVillager , SCHE_SAY_HOLLOW )
	ADD_TASK( TASK_FACE_TARGET       , Math::Deg2Rad( 10 ) )
	ADD_TASK( TASK_TALK              , 3 )
	ADD_TASK( TASK_WAIT_TALK         , 0 )
END_SCHDULE()


DEFINE_AI_ACTION_SCHEDULE( CVillager , SCHE_RAND_WALK )
	ADD_TASK( TASK_SAVE_RAND_2D_POS   , 100.0f )
	ADD_TASK( TASK_NAV_GOAL_POS      , 20.0f )
	ADD_TASK( TASK_WAIT_MOVEMENT     , 1.0f )
	ADD_TASK( TASK_WAIT              , 2.0f )
	ADD_TASK( TASK_SCHEDULE_MAX_TIME , 5.0f )
	ADD_TASK( TASK_CHANGE_TASK       , 0 )
END_SCHDULE()


CLASS_AI_START( CVillager )

	ADD_SCHDULE( 
		SCHE_SELL_ITEMS  , 
		0 
	)

	ADD_SCHDULE( 
		SCHE_RAND_WALK  , 
		BIT( CDT_FIND_EMPTY ) 
	)


	ADD_SCHDULE( 
		SCHE_SAY_HOLLOW  , 
		BIT( CDT_FIND_EMPTY ) 
	)

CLASS_AI_END()


CVillager::CVillager() 
{
	m_mode = SELLER;
	setTeam( TEAM_VILLAGER );
}

bool CVillager::init( GameObject* gameObject , GameObjectInitHelper const& helper )
{
	if ( !BaseClass::init( gameObject , helper ) )
		return false;

	UG_ConnectEvent( EVT_TALK_END , m_roleID , 
		EvtCallBack( this , &CVillager::onEvent ) , this );
	UG_ConnectEvent( EVT_TALK_SECTION_END , m_roleID , 
		EvtCallBack( this , &CVillager::onEvent ) , this );

	return true;
}

void CVillager::addSellItem( unsigned item )
{
	getItemStorage().addItem( item );
}

int CVillager::selectSchedule()
{
	switch ( m_AIState )
	{
	case NPC_IDLE:
		if ( !haveCondition( CDT_FIND_EMPTY ) )
		{
			int val  = TRandom() % 10;
			if ( val > 3 )
			{
				return SCHE_NO_MOVE_IDLE ;
			}
			else if ( val < 7 )
			{
				return SCHE_RAND_WALK;
			}
			else
			{
				return SCHE_GUARD_POSION;
			}
		}
		break;
	}

	return BaseClass::selectSchedule();
}

void CVillager::onInteract( ILevelObject* entity )
{
	if ( m_mode == SELLER)
	{
		m_targetPE = entity;
		AIActionSchedule* sche = getScheduleByType( SCHE_SELL_ITEMS );
		changeSchedule( sche );
	}
	else
	{
		m_targetPE = entity;
		AIActionSchedule* sche = getScheduleByType( SCHE_SAY_HOLLOW );
		changeSchedule( sche );
	}

}

AIActionSchedule* CVillager::getScheduleByType( int sche )
{
	static int const size  = ARRAY_SIZE( GET_SCHEDULE_DATA( ThisClass ) );
	for ( int i = 0 ; i < size ; ++i )
	{
		if ( GET_SCHEDULE_DATA( ThisClass )[i].type == sche )
			return &GET_SCHEDULE_DATA( ThisClass )[i];
	}
	return BaseClass::getScheduleByType( sche );
}

void CVillager::startTask( AITask const& task )
{
	switch( task.type )
	{
	case  TASK_TRADE_ITEMS :
		if ( CActor* actor = CActor::downCast( m_targetPE ) )
		{
			clearCondition( CDT_TRADE_END );
			getStorePanel()->startTrade( SELL_ITEMS , this , actor );
			taskFinish();
		}
		else
		{
			taskFail( TF_NO_TARGET );
		}
		break;

	default:
		BaseClass::startTask( task );
	}

}

void CVillager::runTask( AITask const& task )
{
	float const tradeExitDist = 150;
	switch( task.type )
	{
	case TASK_TALK:
		clearCondition( CDT_TALK_END );
		if ( TTalkSystem::getInstance().speak( this , task.IntVal ) )
		{
			taskFinish( );
		}
	case TASK_WAIT_TALK :
		if ( haveCondition( CDT_TALK_END ) )
			taskFinish();
		break;

	case  TASK_WAIT_TRADE :
		if ( !m_targetPE )
		{
			taskFail( TF_NO_TARGET );
		}
		else if ( haveCondition( CDT_TRADE_END ) )
		{
			float dist2 = Math::distance2( getPosition(), m_targetPE->getPosition() );

			if ( dist2 > tradeExitDist * tradeExitDist )
			{
				getStorePanel()->show( false );
				taskFinish();
			}
		}
		break;

	default:
		BaseClass::runTask( task );
	}
}

void CVillager::onEvent( TEvent const& event )
{
	if ( event.type == EVT_TALK_SECTION_END )
	{
		addCondition( CDT_TALK_SECTION_END );
		onTalkSectionEnd();
	}
	else if ( event.type == EVT_TALK_END  )
	{
		addCondition( CDT_TALK_END );
		onTalkEnd();
	}
}
