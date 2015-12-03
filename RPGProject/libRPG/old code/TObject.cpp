#include "TObject.h"

#include "TResManager.h"
#include "TEntityManager.h"
#include "shareDef.h"
#include "TEffect.h"
#include "TLevel.h"


DEFINE_UPCAST( TObject , ET_OBJECT )

TObject::TObject( unsigned modelID , XForm const& trans )
	:TPhyEntity( PHY_RIGID_BODY , modelID )
{
	m_entityType |= ET_OBJECT;

	TResManager& manager = TResManager::instance();
	TObjModelData* data = manager.getModelData( modelID );

	FnObject obj; obj.Object( data->flyID );
	OBJECTid id =  obj.Instance(FALSE);

	m_obj.Object( id );
	//m_obj.SetDirectionAlignment( X_AXIS , Z_AXIS );
	setMotionState( new TObjMotionState( trans , id ) );
}

TObject::TObject( OBJECTid id , TPhyBodyInfo const& info ) 
	:TPhyEntity( PHY_RIGID_BODY , info )
{
	m_entityType |= ET_OBJECT;
	m_obj.Object( id );
	//m_obj.SetDirectionAlignment( X_AXIS , Z_AXIS );

	setMotionState( new TObjMotionState( info.m_startWorldTransform , id ) );
}

TObject::~TObject()
{
	FnScene scene;
	scene.Object( getFlyObj().GetScene() );
	scene.DeleteObject( getFlyObj().Object() );
}



TObjModelData& TObject::getModelData( unsigned modelID )
{
	TObjModelData* data = TResManager::instance().getModelData(modelID);
	assert( data != NULL );
	return *( data );
}

void TObject::setKimematic( bool beK )
{
	int flag = getPhyBody()->getCollisionFlags();
	if ( beK )
	{
		getPhyBody()->setActivationState( flag | btCollisionObject::CF_KINEMATIC_OBJECT );
	}
	else
	{
		getPhyBody()->setCollisionFlags( flag & ~(btCollisionObject::CF_KINEMATIC_OBJECT) );
	}
}

void TObject::OnSpawn()
{
	getFlyObj().ChangeScene( getLiveLevel()->getFlyScene().Object() );
}