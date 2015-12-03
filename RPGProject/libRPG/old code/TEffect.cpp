#include "TEffect.h"
#include "TActor.h"
#include "shareDef.h"
#include "TResManager.h"
#include "TCombatSystem.h"
#include "UtilityGlobal.h"


CycleDamgeEffect::CycleDamgeEffect( DamageInfo* damage , ActorState state , int totalCount , float ctime ) 
	: EfCycle( totalCount ) 
	, EfTimer( ctime )
{
	m_damage = damage;
	m_state = state;
}

CycleDamgeEffect::~CycleDamgeEffect()
{
	delete m_damage;
}

void CycleDamgeEffect::update( TEntityBase* entity )
{
	if ( OnTimer() )
	{
		TActor* actor = TActor::upCast( entity );

		if  ( actor->isDead() ) 
			return;

		m_damage->defender = actor;

		DamageInfo* damage = DamageInfo::create();
		*damage = *m_damage;
		UG_InputDamage( *damage );
		increase();
	}
}

void CycleDamgeEffect::OnEnd( TEntityBase* entity )
{
	TActor* actor = TActor::upCast( entity );
	actor->removeStateBit( m_state );
}

void CycleDamgeEffect::OnStart( TEntityBase* entity )
{
	TActor* actor = TActor::upCast( entity );
	assert( actor != NULL );

	actor->addStateBit( m_state );
}

bool CycleDamgeEffect::needRemove( TEntityBase* entity )
{
	return isCycleEnd();
}

