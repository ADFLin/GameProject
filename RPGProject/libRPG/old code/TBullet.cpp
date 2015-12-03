#include "TBullet.h"

#include "TActor.h"
#include "TCombatSystem.h"
#include "UtilityGlobal.h"

DEFINE_UPCAST( TBullet , ET_BULLET )

TBullet::TBullet( TPhyBodyInfo& info )
	:TPhyEntity( PHY_SENSOR_BODY , info )
	,m_lastColBody( NULL )
	,m_collisionTime(0)
{
	m_entityType |= ET_BULLET;
	int flag = getPhyBody()->getCollisionFlags();
	getPhyBody()->setCollisionFlags(  flag | btCollisionObject::CF_NO_CONTACT_RESPONSE
										  /* | btCollisionObject::CF_KINEMATIC_OBJECT */);
	getPhyBody()->setActivationState( DISABLE_DEACTIVATION );
}

TPhyEntity* TBullet::getLastColEntity()
{
	if ( m_lastColBody )
		return (TPhyEntity*)m_lastColBody->getUserPointer();
	return NULL;
}

