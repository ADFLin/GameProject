#include "ProjectPCH.h"
#include "GameObjectComponent.h"

#include "SpatialComponent.h"
#include "CSceneLevel.h"

#include "UtilityMath.h"

DEFINE_HANDLE( ILevelObject )

ILevelObject::ILevelObject()
{
	mSceneLevel = nullptr;
	mObjectType = LOT_UNKNOWN;
	setAlignDirectionAxis( CFly::CF_AXIS_X , CFly::CF_AXIS_Z );
}


ISpatialComponent* ILevelObject::getSpatialComponent() const
{
	if ( getEntity() )
		return static_cast< ISpatialComponent* >( getEntity()->getComponent( COMP_SPATIAL ) );
	return NULL;
}

void ILevelObject::update( long time )
{
	BaseClass::update( time );
	Thinkable::update( time );
}

bool ILevelObject::init( GameObject* gameObj , GameObjectInitHelper const& helper )
{
	helper.sceneLevel->addObject( this );
	
	return true;
}

void ILevelObject::postInit()
{
	mSpatialCompoent = static_cast< ISpatialComponent* >( getEntity()->getComponent( COMP_SPATIAL ) );
}

void ILevelObject::release()
{
	

}

void ILevelObject::setFrontUpDirection( Vec3D const& front , Vec3D const& up )
{
	Quat q = UM_FrontUpToQuat( mFrontAxis , front  , mUpAxis , g_UpAxisDir );
	getSpatialComponent()->setOrientation( q );
}

void ILevelObject::setAlignDirectionAxis( CFly::AxisEnum frontAxis , CFly::AxisEnum upAxis )
{
	mFrontAxis = frontAxis;
	mUpAxis    = upAxis;
}

void ILevelObject::getFrontUpDirection( Vec3D* front , Vec3D* up ) const
{
	Mat4x4 const& xform = getSpatialComponent()->getTransform();
	if ( front )
		*front = Math::rotate( CFly::getAxisDirection( mFrontAxis ) , xform );
	if ( up )
		*up = Math::rotate( CFly::getAxisDirection( mUpAxis ) , xform );
}
