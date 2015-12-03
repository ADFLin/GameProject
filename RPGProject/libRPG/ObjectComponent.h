#ifndef ObjectComponent_h__
#define ObjectComponent_h__

#include "common.h"

#include "GameObjectComponent.h"

class PhyRigid;
class SNSpatialComponent;

class ObjectLogicComponent : public ILevelObject
{



	PhyRigid* mRigidComp;
	SNSpatialComponent* mSpatialComp;
};


#include "CRenderComponent.h"


#endif // ObjectComponent_h__
