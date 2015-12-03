#include "TPetNPC.h"

#include "TResManager.h"
#include "TNavigator.h"
#include "TLevel.h"
#include "TAnimation.h"
#include "TPhySystem.h"
#include "AICondition.h"
#include "profile.h"

#include "TTrigger.h"
#include "TEntityManager.h"
#include "TItemSystem.h"
#include "UtilityMath.h"

#include "TEventSystem.h"
#include "EventType.h"

#include "THero.h"


DEFINE_CLASS_SCHEDULE( TPetNPC , SCHE_NO_MOVE_IDLE )
	ADD_TASK( TASK_STOP_MOVE , 0 )
	ADD_TASK( TASK_WAIT      , 5.0f )
END_SCHDULE()

CLASS_AI_START( TPetNPC )
	ADD_SCHDULE( 
		SCHE_NO_MOVE_IDLE , 
		BIT( CDT_FIND_EMPTY ) |
		BIT( CDT_TAKE_DAMAGE )|
		BIT( CDT_OWNER_TAKE_DAMAGE ) |
		BIT( CDT_OWNER_TOO_FAR )
	)
CLASS_AI_END()

TPetNPC::TPetNPC( TActor* owner ,unsigned roleID , XForm const& trans ) 
	:TNPCBase( roleID , trans )
{
	m_owmer = owner;
	setTeam( owner->getTeam() );
	listenEvent( EVT_COMBAT_DAMAGE , owner->getRefID() );
}
TSchedule* TPetNPC::getScheduleByType( int sche )
{
	static int const size  = ARRAY_SIZE( GET_SCHEDULE_DATA( ThisClass ) );
	for ( int i = 0 ; i < size ; ++i )
	{
		if ( GET_SCHEDULE_DATA( ThisClass )[i].type == sche )
			return &GET_SCHEDULE_DATA( ThisClass )[i];
	}
	return BaseClass::getScheduleByType( sche );
}

void TPetNPC::startTask( TTask const& task )
{
	switch( task.type )
	{
	default:
		BaseClass::startTask( task );
	}
}

void TPetNPC::runTask( TTask const& task )
{
	switch( task.type )
	{

	default:
		BaseClass::runTask( task );
	}
}

int TPetNPC::selectSchedule()
{
	switch ( m_npcState )
	{
	case NPC_IDLE:
		if ( haveCondition( CDT_OWNER_ATTACK ) ||
			 haveCondition( CDT_OWNER_TAKE_DAMAGE ) )
		{
			setNPCState( NPC_COMBAT );
			m_targetPE = m_hEmpty;
			clearCondition( CDT_OWNER_TAKE_DAMAGE );
			clearCondition( CDT_OWNER_ATTACK );
			return SCHE_MOVE_TO_TARGET;
		}
		else if ( haveCondition( CDT_OWNER_TOO_FAR )  )
		{
			m_targetPE = m_owmer;
			return SCHE_MOVE_TO_TARGET;
		}
		else
		{
			return SCHE_NO_MOVE_IDLE;
		}
	}

	return BaseClass::selectSchedule();
}

void TPetNPC::gatherCondition()
{
	BaseClass::gatherCondition();

	clearCondition( CDT_OWNER_TOO_FAR );

	float dist = distance( m_owmer->getPosition() , getPosition() );
	if ( dist > 200 + getNavigator()->getOuterOffset() )
	{
		addCondition( CDT_OWNER_TOO_FAR );
	}
}

void TPetNPC::OnEvent( TEvent& event )
{
	if ( event.type == EVT_COMBAT_DAMAGE )
	{
		addCondition( CDT_OWNER_TAKE_DAMAGE );
		DamageInfo* info = (DamageInfo*) event.data;
		m_hEmpty = TActor::upCast( info->attacker );

	}
	BaseClass::OnEvent( event );
}
