#include "CFlyPCH.h"
#include "CFSpatialNode.h"

namespace CFly
{
	SpatialNode::SpatialNode( NodeManager* manager ) 
		:NodeBase( manager )
	{
		init();
	}

	SpatialNode::SpatialNode( SpatialNode const& node ) 
		:NodeBase( node )
		,mCacheWorldTrans( node.mCacheWorldTrans )
		,mLinkTransOp( node.mLinkTransOp )
		,mLocalTrans( node.mLocalTrans )
	{

	}

	void SpatialNode::init()
	{
		mLocalTrans.setIdentity();
		mCacheWorldTrans.setIdentity();
		mLinkTransOp = CFTO_GLOBAL;
	}

	void SpatialNode::transform( Vector3 const& pos , Quaternion const& rotation  , TransOp op )
	{
		Matrix4 mat; mat.setTransform( pos ,rotation );
		FTransform::Transform( mLocalTrans , mat , op );
		onModifyTransform( true );
	}

	void SpatialNode::transform( Matrix4 const& mat , TransOp op )
	{
		FTransform::Transform( mLocalTrans , mat , op );
		onModifyTransform( true );
	}

	void SpatialNode::translate( Vector3 const& v , TransOp op )
	{
		FTransform::Translate( mLocalTrans , v , op );
		onModifyTransform( true );
	}

	void SpatialNode::rotate( Vector3 const& axis , float angle , TransOp op )
	{
		FTransform::Rotate( mLocalTrans , axis , angle , op );
		onModifyTransform( true );
	}

	void SpatialNode::rotate( AxisEnum axis , float angle , TransOp op )
	{
		FTransform::Rotate( mLocalTrans , axis , angle , op );
		onModifyTransform( true );
	}

	void SpatialNode::rotate( Quaternion const& q , TransOp op )
	{
		FTransform::Rotate( mLocalTrans , q , op );
		onModifyTransform( true );
	}

	void SpatialNode::scale( Vector3 const& factor , TransOp op )
	{
		FTransform::Scale( mLocalTrans , factor , op );
		onModifyTransform( true );
	}

	void SpatialNode::unlinkChildren( bool fixTrans /*= true */ )
	{
		NodeList::iterator iter = mChildren.begin();

		if ( fixTrans )
		{
			while( iter != mChildren.end() )
			{
				SpatialNode* child = static_cast< SpatialNode* >( *iter );
				++iter;
				child->transform( getLocalTransform() , CFTO_GLOBAL );
				child->setParent( getParent() );
			}
		}
		else
		{
			while( iter != mChildren.end() )
			{
				SpatialNode* child = static_cast< SpatialNode* >( *iter );
				++iter;
				child->setParent( getParent() );
			}
		}
	}

	Vector3 SpatialNode::getLocalPosition() const
	{
		return mLocalTrans.getTranslation();
	}

	void SpatialNode::onModifyTransform( bool beLocal )
	{
		if ( beLocal )
			setWorldTransformDirty();
	}

	void SpatialNode::setLocalPosition( Vector3 const& pos )
	{
		mLocalTrans.modifyTranslation( pos );
		onModifyTransform( true );
	}

	void SpatialNode::setLocalOrientation( Quaternion const& q )
	{
		mLocalTrans.modifyOrientation( q );
		onModifyTransform( true );
	}

	void SpatialNode::setLocalOrientation(AxisEnum axis, float angle)
	{
		Vector3 axisVector[] = { Vector3(1,0,0),Vector3(0,1,0),Vector3(0,0,1),Vector3(-1,0,0),Vector3(0,-1,0),Vector3(0,0,-1)};
		mLocalTrans.modifyOrientation( Quaternion::Rotate( axisVector[axis] , angle ) );
		onModifyTransform( true );

	}

	void SpatialNode::setLocalTransform( Matrix4 const& trans )
	{
		mLocalTrans = trans;
		onModifyTransform( true );
	}

	void SpatialNode::setWorldTransformDirty()
	{
		addFlag( NODE_WORLD_TRANS_DIRTY );
		for( NodeList::iterator iter = mChildren.begin();
			iter != mChildren.end() ; ++iter )
		{
			SpatialNode* node = static_cast< SpatialNode* >( *iter );
			node->setWorldTransformDirty();
		}
	}

	Matrix4 const& SpatialNode::_getWorldTransformInternal( bool parentUpdated )
	{
		if ( checkFlag( NODE_WORLD_TRANS_DIRTY ) )
		{
			if ( checkFlag( NODE_SUB_CHILD ) )
			{
				SpatialNode* parent = static_cast< SpatialNode* >( getParent() );
				parent->updateSubChildTransform( this );
			}
			else
			{
				updateWorldTransform( parentUpdated );
			}
		}
		return mCacheWorldTrans;
	}

	void SpatialNode::_updateWorldTransformOnBone( Matrix4 const& boneTrans )
	{
		assert( checkFlag( NODE_SUB_CHILD ) );

		mCacheWorldTrans = mLocalTrans;
		FTransform::Transform( mCacheWorldTrans , boneTrans , mLinkTransOp );

		removeFlag( NODE_WORLD_TRANS_DIRTY );
		onModifyTransform( false );
	}

	void SpatialNode::checkWorldTranform_R()
	{
		SpatialNode* parent = static_cast< SpatialNode* >( getParent() );

		if ( parent->getParent() != nullptr )
			parent->checkWorldTranform_R();

		if ( checkFlag( NODE_WORLD_TRANS_DIRTY ) )
		{
			mCacheWorldTrans = mLocalTrans;

			if ( parent->getParent() != nullptr )
			{
				FTransform::Transform( mCacheWorldTrans , parent->mCacheWorldTrans , mLinkTransOp );
			}

			removeFlag( NODE_WORLD_TRANS_DIRTY );
			onModifyTransform( false );
		}
	}

	void SpatialNode::updateWorldTransform( bool parentUpdated )
	{
		assert( checkFlag( NODE_WORLD_TRANS_DIRTY ) );

		mCacheWorldTrans = mLocalTrans;

		if ( getParent()->getParent() != nullptr )
		{
			SpatialNode* parent = static_cast< SpatialNode* >( getParent() );

			if ( parent )
			{
				if ( !parentUpdated )
					parent->checkWorldTranform_R();

				FTransform::Transform( mCacheWorldTrans , parent->mCacheWorldTrans , mLinkTransOp );
			}
		}

		removeFlag( NODE_WORLD_TRANS_DIRTY );
		onModifyTransform( false );
	}

	void SpatialNode::setWorldPosition( Vector3 const& pos )
	{
		assert( getParent() );
		if ( getParent()->getParent() != nullptr )
		{
			SpatialNode* parent = static_cast< SpatialNode* >( getParent() );
			Vector3 lPos = pos;

			Matrix4 const& trans = parent->getWorldTransform();
			lPos = Math::TransformVectorInverse( pos , trans ) - trans.getTranslation();
			setLocalPosition( lPos );
		}
		else
		{
			setLocalPosition( pos );
		}
	}

	Vector3 SpatialNode::getWorldPosition()
	{
		Matrix4 const& mat = _getWorldTransformInternal( false );
		return mat.getTranslation();
	}

	void SpatialNode::setWorldTransform( Matrix4 const& trans )
	{
		assert( getParent() );

		if ( getParent()->getParent() != nullptr )
		{
			SpatialNode* parent = static_cast< SpatialNode* >( getParent() );

			Matrix4 const& trans = parent->getWorldTransform();
			Matrix4 invTrans;
			float det;
			trans.inverseAffine( invTrans , det );
			transform( trans * invTrans , CFTO_REPLACE );
		}
		else
		{
			transform( trans , CFTO_REPLACE );
		}
	}

	Matrix4 const& SpatialNode::getWorldTransform()
	{
		return _getWorldTransformInternal( false );
	}

}//namespace CFly