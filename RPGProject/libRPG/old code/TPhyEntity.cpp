#include "TPhyEntity.h"

#include "TResManager.h"
#include "TEntityManager.h"
#include "UtilityMath.h"
#include "UtilityFlyFun.h"
#include "shareDef.h"
#include "TEffect.h"


DEFINE_UPCAST( TPhyEntity , ET_PHYENTITY );

DEFINE_HANDLE( TPhyEntity );

TPhyBodyInfo TPhyEntity::createPhyBodyInfo( TObjModelData& data )
{
	bool isDynamic = (data.mass != 0.f);
	Vec3D localInertia(0,0,0);
	if (isDynamic)
		data.shape->calculateLocalInertia(data.mass,localInertia);
	TPhyBodyInfo info( data.mass, NULL , data.shape , localInertia );

	return info;
}

TPhyBodyInfo TPhyEntity::createPhyBodyInfo( Float mass , btCollisionShape* shape )
{
	bool isDynamic = ( mass != 0.f);
	Vec3D localInertia(0,0,0);
	if (isDynamic)
		shape->calculateLocalInertia( mass , localInertia );

	TPhyBodyInfo info( mass, NULL , shape , localInertia );

	info.m_linearSleepingThreshold *= g_GraphicScale;
	//info.m_additionalDamping = true;
	//info.m_angularDamping = 0.01;

	return info;
}

TPhyEntity::TPhyEntity( PhyBodyType type , unsigned modelID ) 
	:m_ModelID(modelID)
	,m_motionState(NULL)
	,m_testCol( false )
{
	m_entityType |= ET_PHYENTITY;

	TPhyBodyInfo& info = createPhyBodyInfo( getModelData(modelID) );
	m_phyBody = createPhyBody( type , info );
	m_phyBody->setUserPointer( this );

}

TPhyEntity::TPhyEntity( PhyBodyType type , TPhyBodyInfo const& info ) 
	:m_motionState(NULL)
	,m_testCol( false )
{
	m_entityType |= ET_PHYENTITY;
	m_phyBody = createPhyBody( type , info );
	m_phyBody->setUserPointer( this );

}

TPhyEntity::~TPhyEntity()
{
	delete m_motionState;
}

TObjModelData& TPhyEntity::getModelData( unsigned modelID )
{
	TObjModelData* data = TResManager::instance().getModelData(modelID);
	assert( data != NULL );
	return *(data);
}


void TPhyEntity::setPosition( Vec3D const& pos )
{
	getPhyBody()->getWorldTransform().setOrigin( pos );
	if ( getMotionState() )
		getMotionState()->setWorldTransform( m_phyBody->getWorldTransform() );
}

void TPhyEntity::setTransform( XForm const& trans )
{
	getPhyBody()->setWorldTransform( trans );
	if ( getMotionState() )
		getMotionState()->setWorldTransform( trans  );
}

void TPhyEntity::updateTransformFromFlyData()
{
	if ( getMotionState() )
		getMotionState()->getWorldTransform( m_phyBody->getWorldTransform() );
}

Float TPhyEntity::getMoveSpeed()
{
	return 0;
}

void TPhyEntity::OnTakeDamage( DamageInfo const& info )
{

}

void TPhyEntity::OnCollision( TPhyEntity* colObj , Float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj )
{

}

PhyRigidBody* TPhyEntity::createPhyBody( PhyBodyType type , TPhyBodyInfo const& info )
{
	if ( type == PHY_SENSOR_BODY )
	{
		return new TPhyBody( info );
	}
	return new PhyRigidBody( info );
}

void TPhyEntity::setMotionState( TObjMotionState* state )
{
	m_motionState = state;
	getPhyBody()->setMotionState( state );
}

void TPhyEntity::setRotation( Quat const& q )
{
	XForm trans( q , getPosition() );
	setTransform( trans );
}

TObjMotionState::TObjMotionState( XForm const& trans , OBJECTid id , Vec3D& offset /*= Vec3D(0,0,0) */ ) 
{
	m_p2fTrans.setIdentity();
	setOffset( offset );

	m_obj.Object( id );

	setFlyTransform( trans );
}

void TObjMotionState::getWorldTransform( btTransform& phyTrans ) const
{
	Mat3x3& mat = phyTrans.getBasis(); 
	float* m = m_obj.GetMatrix(TRUE);

	mat[0][0] = m[0]; mat[1][0] = m[1]; mat[2][0] = m[2];
	mat[0][1] = m[4]; mat[1][1] = m[5]; mat[2][1] = m[6];
	mat[0][2] = m[8]; mat[1][2] = m[9]; mat[2][2] = m[10];

	phyTrans.setOrigin( Vec3D( m[12] , m[13] , m[14] ) );

	phyTrans *= m_f2pTrans;
}

void TObjMotionState::setFlyTransform( const btTransform& phyTrans )
{
	XForm  trans = phyTrans * m_p2fTrans;

	float m[12];
	Vec3D  const& pos = trans.getOrigin();
	Mat3x3 const& mat = trans.getBasis(); 

	m[0] = mat[0][0]; m[1] = mat[1][0]; m[2] = mat[2][0];
	m[3] = mat[0][1]; m[4] = mat[1][1]; m[5] = mat[2][1];
	m[6] = mat[0][2]; m[7] = mat[1][2]; m[8] = mat[2][2];
	m[9] = pos.x();   m[10] = pos.y()  ; m[11] = pos.z();

	m_obj.SetMatrix( m , REPLACE );

}

void TObjMotionState::setWorldTransform( const btTransform& phyTrans )
{
	setFlyTransform( phyTrans );
}

void TObjMotionState::setOffset( Vec3D const& offset )
{
	m_p2fTrans.setOrigin( offset );
	m_f2pTrans = m_p2fTrans.inverse();
}