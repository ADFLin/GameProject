#include "TEntityBase.h"
#include <cassert>
#include "TEntityManager.h"
#include "TEventSystem.h"
#include "shareDef.h"
#include "profile.h"

DEFINE_UPCAST( TLevelEntity , ET_LEVEL_ENTITY );

TLevelEntity::TLevelEntity()
{
	m_entityType |= ET_LEVEL_ENTITY;
	m_level = NULL;
}

DEFINE_HANDLE( TEntityBase )

void* TEntityBase::operator new( size_t size )
{
	assert( size != 0 );
	return ::operator new( size );
};


void TEntityBase::operator delete( void *pMem )
{
	::delete pMem;
}



TEntityBase::TEntityBase() 
	:m_entityType( ET_ENTITY_BASE )
	,m_entityFlag( EF_WORKING | EF_LEVEL )
{
	m_curThinkIndex = -1;
}

TEntityBase::~TEntityBase()
{

}
void TEntityBase::processThink()
{
	for( int i=0; i<m_thinkFunVec.size() ;++i )
	{
		ThinkContent& think = m_thinkFunVec[i];
		if ( think.mode == THINK_NOTHINK )
			continue;

		if ( think.nextTime <= g_GlobalVal.curtime )
		{
			float thinkTime = g_GlobalVal.curtime;

			m_curThinkIndex = i;
			if ( think.mode == THINK_ONCE )
				think.mode = THINK_NOTHINK;
			else if ( think.mode == THINK_LOOP )
				think.nextTime = thinkTime + think.loopTime; 

			(this->*think.fun)();
			think.lastTime = g_GlobalVal.curtime;
			m_curThinkIndex = -1;
		}
	}
}

size_t TEntityBase::addThink( fnThink fun, ThinkMode mode , float loopTime )
{

	for ( int i = 0 ; i < m_thinkFunVec.size() ; ++i )
	{
		if ( m_thinkFunVec[i].fun == fun )
		{
			m_thinkFunVec[i].mode = mode;
			m_thinkFunVec[i].loopTime = loopTime;

			return i;
		}
	}

	ThinkContent think;
	think.mode = mode;
	think.fun  = fun;
	think.loopTime = loopTime;
	m_thinkFunVec.push_back( think );

	return m_thinkFunVec.size()-1;
}

void TEntityBase::update()
{
	{
		TPROFILE("Think Process");
		processThink();
	}
	{
		TPROFILE("updateFrame");
		updateFrame();
	}
}

void TEntityBase::setNextThink( int index , float time )
{
	assert( index < m_thinkFunVec.size() );
	m_thinkFunVec[index].nextTime = time;
	if ( m_thinkFunVec[index].mode == THINK_NOTHINK )
		m_thinkFunVec[index].mode = THINK_ONCE;
}

void TEntityBase::setCurContextNextThink( float time )
{
	setNextThink( m_curThinkIndex , time );
}

ThinkContent& TEntityBase::getCurThinkContent()
{
	assert( m_curThinkIndex != -1 );
	return m_thinkFunVec[m_curThinkIndex];
}

void TEntityBase::setEntityState( EntityFlag val )
{
	assert ( val & EF_WORK_STATE_MASK );

	removeEntityFlag( EF_WORK_STATE_MASK );
	addEntityFlag( val );
}

unsigned TEntityBase::getEntityState() const
{
	return m_entityFlag & EF_WORK_STATE_MASK;
}

void TEntityBase::setGlobal( bool beG )
{
	removeEntityFlag( EF_ZONE_MAK );
	m_entityFlag |= ( beG ) ? EF_GLOBAL : EF_LEVEL;
}

bool TEntityBase::isGlobal()
{
	return ( m_entityFlag & EF_ZONE_MAK ) == EF_GLOBAL;
}

bool TEntityBase::isTemp()
{
	return ( m_entityFlag & EF_ZONE_MAK ) == EF_TEMP;
}

void TEntityBase::setTemp( bool beT )
{
	removeEntityFlag( EF_ZONE_MAK );
	m_entityFlag |= ( beT ) ? EF_TEMP : EF_LEVEL;
}