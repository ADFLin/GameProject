#include "ProjectPCH.h"
#include "CRigidObject.h"

#include "PhysicsSystem.h"
#include "CSceneLevel.h"
#include "IScriptTable.h"

#include "SpatialComponent.h"
#include "CRenderComponent.h"
#include "TResManager.h"

bool CRigidObject::init( GameObject* gameObj , GameObjectInitHelper const& helper )
{
	IScriptTable* scriptTable = getEntity()->getScriptTable();

	char const* modelName;
	scriptTable->getValue( "modelName" , modelName );
	ObjectModelRes* modelRes = TResManager::Get().getObjectModel( modelName );

	CRenderComponent* renderComp = getEntity()->getComponentT< CRenderComponent >( COMP_RENDER );

	SRenderEntityCreateParams params;
	params.resType = RES_OBJ_MODEL;
	params.name    = modelName;
	params.scene   = helper.sceneLevel->getRenderScene();
	CObjectModel* modelComp = static_cast< CObjectModel* >( renderComp->createRenderEntity( 0 , params ) );

	SNSpatialComponent* spatialComp = new SNSpatialComponent( 
		modelComp->getRenderNode() , 
		getEntity()->getComponentT< PhyCollision >( COMP_PHYSICAL )  );

	getEntity()->addComponent( spatialComp );

	return true;
}

void CRigidScript::createComponent( CEntity& entity , EntitySpawnParams& params )
{
	CSceneLevel* sceneLevel = params.helper->sceneLevel;
	PhysicsWorld* world = sceneLevel->getPhysicsWorld();
	IScriptTable* scriptTable = entity.getScriptTable();

	char const* modelName;
	scriptTable->getValue( "modelName" , modelName );
	ObjectModelRes* modelRes = TResManager::Get().getObjectModel( modelName );

	CRenderComponent* renderComp = new CRenderComponent;
	entity.addComponent( renderComp );

	PhyRigid* phyicalComp = sceneLevel->getPhysicsWorld()->createRigidComponent( modelRes->phyData.shape , modelRes->phyData.mass );
	entity.addComponent( phyicalComp );
}

