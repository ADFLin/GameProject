#ifndef SpatialComponent_h__
#define SpatialComponent_h__

#include "common.h"
#include "CEntity.h"

class IPhyiscalEntity;
class PhyCollision;

class ISpatialComponent : public IComponent
{
public:
	ISpatialComponent():IComponent( COMP_SPATIAL ){}

	virtual Vec3D getPosition() const = 0;
	virtual void  setPosition( Vec3D const& pos ) = 0;
	virtual void  setOrientation( Quat const& q ) = 0;
	virtual void  rotate( Quat const& q ) = 0;
	virtual Mat4x4 const& getTransform() const = 0;
};

class SpatialDetector
{
public:




};



class SNSpatialComponent : public ISpatialComponent
{
public:
	SNSpatialComponent( SceneNode* node , PhyCollision* comp = NULL );

	//ISpatialComponent
	Mat4x4 const& getTransform() const;
	Vec3D getPosition() const;
	void  rotate( Quat const& q );
	void  update( long time );
	void  setOrientation( Quat const& q );
	void  setPosition( Vec3D const& pos );

	void  setTransform( Vec3D const& pos , Quat const& q );
	Vec3D getLocalPosition() const;

	void  setLocalOrientation( Quat const& q );
	void  setLocalPosition( Vec3D const& pos );
	void  setPhyOffset( Vec3D const& offset );
	void  updatePhyicalCompTransform( Mat4x4 const& worldTrans );

private:
	bool          mUseOffsetTrans;
	Mat4x4        mOffsetTrans;
	Mat4x4        mInvOffsetTrans;
	SceneNode*    mSceneNode;
	PhyCollision* mPhyicalComp;
};

#endif // SpatialComponent_h__