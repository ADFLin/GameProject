#include "ProjectPCH.h"
#include "SpatialComponent.h"

#include "PhysicsSystem.h"
#include "CFSceneNode.h"

using namespace CFly;

SNSpatialComponent::SNSpatialComponent( SceneNode* node , PhyCollision* comp /*= NULL */ ) 
	:mSceneNode( node )
	,mPhyicalComp( comp )
	,mUseOffsetTrans( false )
{
	if ( mPhyicalComp )
	{
		enableLogical();
		setPriority( mPhyicalComp->getPriority() + 1 );
		mPhyicalComp->setTransform( node->getLocalTransform() );
	}
}

void SNSpatialComponent::updatePhyicalCompTransform( Mat4x4 const& worldTrans )
{
	if ( !mPhyicalComp )
		return;

	if ( mUseOffsetTrans )
	{
		Mat4x4 trans = worldTrans * mOffsetTrans;
		mPhyicalComp->setTransform( trans );
	}
	else
	{
		mPhyicalComp->setTransform( worldTrans );
	}

}

void SNSpatialComponent::setPosition( Vec3D const& pos )
{
	mSceneNode->setWorldPosition( pos );

	if ( mPhyicalComp )
	{
		if ( mUseOffsetTrans )
		{
			Vec3D lPos = pos * mOffsetTrans;
			mPhyicalComp->setPostion( lPos );
		}
		else
		{
			mPhyicalComp->setPostion( pos );
		}
	}
}

void SNSpatialComponent::setLocalPosition( Vec3D const& pos )
{
	mSceneNode->setLocalPosition( pos );

	if ( mPhyicalComp )
	{
		Vec3D lPos;
		if ( mUseOffsetTrans )
			lPos = mSceneNode->getWorldPosition() * mOffsetTrans;
		else
			lPos = mSceneNode->getWorldPosition();

		mPhyicalComp->setPostion( lPos );
	}
}

void SNSpatialComponent::update( long time )
{
	assert( mPhyicalComp );
	if ( !mPhyicalComp->isSleeping() )
	{
		Mat4x4 trans;
		mPhyicalComp->getTransform( trans );

		if ( mUseOffsetTrans )
			trans = trans * mInvOffsetTrans;

		mSceneNode->setWorldTransform( trans );
	}
}

void SNSpatialComponent::setPhyOffset( Vec3D const& offset )
{
	mInvOffsetTrans.setTranslation( offset );
	float det;
	mInvOffsetTrans.inverse( mOffsetTrans , det ); 
	mUseOffsetTrans = true;
}

void SNSpatialComponent::rotate( Quat const& q )
{
	mSceneNode->rotate( q , CFTO_LOCAL );
	updatePhyicalCompTransform( mSceneNode->getWorldTransform() );
}

void SNSpatialComponent::setOrientation( Quat const& q )
{
	Mat4x4 trans;
	trans.setTransform( mSceneNode->getWorldPosition() , q );
	mSceneNode->setWorldTransform( trans );
	updatePhyicalCompTransform( trans );
}

void SNSpatialComponent::setLocalOrientation( Quat const& q )
{
	mSceneNode->setLocalOrientation( q );
	updatePhyicalCompTransform( mSceneNode->getWorldTransform() );
}

Mat4x4 const& SNSpatialComponent::getTransform() const
{
	return mSceneNode->getWorldTransform();
}

Vec3D SNSpatialComponent::getLocalPosition() const
{
	return mSceneNode->getLocalPosition();
}

Vec3D SNSpatialComponent::getPosition() const
{
	return mSceneNode->getWorldPosition();
}

void SNSpatialComponent::setTransform( Vec3D const& pos , Quat const& q )
{
	Mat4x4 trans;
	trans.setTransform( pos , q );
	mSceneNode->setWorldTransform( trans );
	updatePhyicalCompTransform( trans );
}
