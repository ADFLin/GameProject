#include "CFBase.h"
#include "CFNodeBase.h"

#include "CFTransformUtility.h"

namespace CFly
{

	class SpatialNode : public NodeBase
	{
		typedef NodeBase BaseClass;
	public:

		enum
		{
			NODE_WORLD_TRANS_DIRTY = BaseClass::NEXT_NODE_FLAG,
			NODE_SUB_CHILD         ,
			NEXT_NODE_FLAG         ,
		};

		SpatialNode( NodeManager* manager );
		SpatialNode( SpatialNode const& node );

		void transform( Matrix4 const& mat , TransOp op = CFTO_GLOBAL );
		void transform( Vector3 const& pos , Quaternion const& rotation , TransOp op = CFTO_GLOBAL );
		//void transform( Matrix3 const& mat , TransOp op = CFTO_GLOBAL );
		void scale( Vector3 const& factor , TransOp op = CFTO_GLOBAL );
		void translate( Vector3 const& v , TransOp op = CFTO_GLOBAL );
		void rotate( Vector3 const& axis, float angle , TransOp op = CFTO_GLOBAL );
		void rotate( AxisEnum axis, float angle , TransOp op = CFTO_GLOBAL );
		void rotate( Quaternion const& q , TransOp op = CFTO_GLOBAL );

		void unlinkChildren( bool fixTrans = true );
		void setLocalTransform( Matrix4 const& trans );
		void setLocalOrientation( Quaternion const& q );
		void setLocalOrientation( AxisEnum axis, float angle );
		void setLocalPosition( Vector3 const& pos );

		Matrix4 const& getLocalTransform() const { return mLocalTrans; }
		Matrix4&       getLocalTransform()       { return mLocalTrans; }

		void    setWorldPosition( Vector3 const& pos );
		Vector3 getWorldPosition();
		void    setWorldTransform( Matrix4 const& trans );
		Matrix4 const& getWorldTransform();

		Vector3 getLocalPosition() const;
		void    setLinkTransOperator( TransOp op ){ mLinkTransOp = op; }

	public:
		Matrix4 const& _getWorldTransformInternal( bool parentUpdated );
		void _updateWorldTransformOnBone( Matrix4 const& boneTrans );

	protected:

		void    checkWorldTranform_R();
		void    updateWorldTransform( bool parentUpdated );
		void    setWorldTransformDirty();
		virtual void updateSubChildTransform( SpatialNode* subChild ){}
		virtual void onModifyTransform( bool beLocal );

		void init();

		mutable Matrix4  mCacheWorldTrans;
		TransOp  mLinkTransOp;
		Matrix4  mLocalTrans;

		//Vector3    mLocalPos;
		//Vector3    mLocalScale;
		//Quaternion mLocalRotation;
	};


}//namespace CFly