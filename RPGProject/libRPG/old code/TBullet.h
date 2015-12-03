#ifndef TBullet_h__
#define TBullet_h__

#include "common.h"

#include "TObject.h"

struct DamageInfo;
class  eF3DFX;

class TBullet : public TPhyEntity
{
	DESCRIBLE_ENTITY_CLASS( TBullet , TPhyEntity )
public:
	TBullet( TPhyBodyInfo& info );
	PhyRigidBody*   getLastColBody(){ return m_lastColBody; }
	TPhyEntity*   getLastColEntity();

	void OnCollision( TPhyEntity* colObj , Float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj )
	{
		if ( m_lastColBody != colObj->getPhyBody() )
		{
			m_lastColBody = colObj->getPhyBody();
			++m_collisionTime;
		}
	}
	void OnCollisionTerrain( )
	{
		//if ( m_lastColBody != )
	}

protected:
	PhyRigidBody*  m_lastColBody;   
	bool      m_collisionTime;
};

#endif // TBullet_h__