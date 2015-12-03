#ifndef CRigidObject_h__
#define CRigidObject_h__

#include "common.h"
#include "GameObjectComponent.h"

class CRigidObject : public ILevelObject
{
public:
	bool init( GameObject* gameObj , GameObjectInitHelper const& helper );
};


class CRigidScript : public IEntityScript
{
public:
	virtual void  createComponent( CEntity& entity , EntitySpawnParams& params );
};


#endif // CRigidObject_h__
