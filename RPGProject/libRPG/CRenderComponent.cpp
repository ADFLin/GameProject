#include "ProjectPCH.h"
#include "CRenderComponent.h"

#include "CFObject.h"
#include "TResManager.h"

IRenderEntity* CRenderComponent::createRenderEntity( unsigned slot /*= 0 */, SRenderEntityCreateParams const& params )
{
	IRenderEntity* result = NULL;

	switch( params.resType )
	{
	case RES_ACTOR_MODEL:
		{
			ActorModelRes* modelRes = TResManager::Get().getActorModel( params.name );
			CActorModel* model = modelRes->createModel( params.scene );
			result = model;
		}
		break;
	case RES_OBJ_MODEL:
		{
			ObjectModelRes* modelRes = TResManager::Get().getObjectModel( params.name );
			CObjectModel* model = modelRes->createModel( params.scene );
			result = model;
		}
		break;
	case RES_EQUIP_MODEL:
		break;
	}


	if ( result )
	{
		if ( slot >= mRenderEntities.size() )
			mRenderEntities.resize( slot + 1 );
		mRenderEntities[ slot ] = result;
	}

	return result;
}

void CRenderComponent::releaseRenderEntity( unsigned slot )
{
	if ( slot >= mRenderEntities.size() )
		return;

	if ( mRenderEntities[slot] )
	{
		mRenderEntities[slot]->release();
		mRenderEntities[slot] = NULL;
	}
}

IRenderEntity* CRenderComponent::getRenderEntity( unsigned slot /*= 0 */ )
{
	if ( slot >= mRenderEntities.size() )
		return NULL;
	return mRenderEntities[ slot ];
}

void CObjectModel::release()
{
	mCFObject->release();
}

void CObjectModel::changeScene( RenderScene* scene )
{
	mCFObject->changeScene( scene );
}

RenderNode* CObjectModel::getRenderNode()
{
	return mCFObject;
}
