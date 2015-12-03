#include "ProjectPCH.h"
#include "CEntity.h"
#include "IScriptTable.h"
#include "SpatialComponent.h"
#include "ISystem.h"

IComponent::IComponent( CompGroup group /*= COMP_UNKNOWN */ )
{
	mGroup = group;
	mPriority = 0;
	mFlag  = 0;
	mOwner = NULL;
}

CEntity::CEntity()
{
	std::fill( mCompments , mCompments + COMP_GROUP_NUM , (IComponent*) 0 );
	mFlag = 0;
}

void CEntity::addComponentInternal( IComponent* comp )
{
	ComponentList::iterator iter = mCompList.begin();
	ComponentList::iterator iterEnd = mCompList.end();
	for( ; iter != iterEnd; ++iter)
	{
		if ( comp->mPriority <= (*iter)->mPriority )
			break;
	}
	mCompList.insert( iter , comp );
	mCompments[ comp->mGroup ] = comp;

	comp->mOwner = this;
	comp->onLink();
}

void CEntity::update( long time )
{
	switch( mUpdatePolicy )
	{
	case EUP_NEVER:
		return;
	}

	for( ComponentList::iterator iter = mCompList.begin(), iterEnd = mCompList.end();
		iter != iterEnd; )
	{
		IComponent* comp = *iter;

		if ( comp->mFlag & IComponent::eRemove )
		{
			comp->onUnlink();
			iter = mCompList.erase( iter );
			delete comp;
			continue;
		}

		//if ( comp->mFlag & IComponent::eLogic )
			comp->update( time );

		++iter;
	}
}

IComponent* CEntity::getComponent( CompGroup group )
{
	return mCompments[ group ];
}

void CEntity::sendEvent( EntityEvent const& event )
{
	for ( int i = 0 ; i < COMP_GROUP_NUM ; ++i )
	{
		if ( mCompments[i] )
			mCompments[i]->onEvent( event );
	}
}

void EntitySystem::update( long time )
{
	for( EntityList::iterator iter = mEntityList.begin(); 
		iter != mEntityList.end() ; )
	{
		CEntity* entity = (*iter);

		if ( entity->getFlag() & EF_DESTROY )
		{
			delete  entity;
			iter = mEntityList.erase( iter );
		}
		else if ( entity->getFlag() & EF_FREEZE )
		{
			++iter;	
		}
		else
		{
			entity->update( time );
			++iter;
		}
	}
}

CEntity* EntitySystem::spawnEntity( EntitySpawnParams& params )
{
	CEntity* entity = new CEntity;

	IEntityClass* eClass = params.entityClass;
	if ( params.propertyTable )
	{
		params.propertyTable->setDefalutTable( eClass->getScriptTable() );
		entity->mScriptTable = params.propertyTable;
	}
	else if (  eClass->getScriptTable() )
	{
		IScriptTable* table = gEnv->scriptSystem->createScriptTable();
		table->setDefalutTable( eClass->getScriptTable() );
		entity->mScriptTable = table;
	}

	IEntityScript* entityScript = eClass->getEntityScript();
	if ( entityScript )
	{
		entityScript->createComponent( *entity , params );
		for( int i = 0 ; i < COMP_GROUP_NUM ; ++i )
		{
			IComponent* comp = entity->mCompments[ i ];
			if ( comp )
				comp->init( entity , params );
		}
	}

	IEntityConstructor* constructor = eClass->getConstructor();
	if ( !constructor->construct( *entity , params ) )
	{
		delete entity;
		return NULL;
	}

	ISpatialComponent* spatialComp = entity->getComponentT< ISpatialComponent >( COMP_SPATIAL );
	if ( spatialComp )
	{
		spatialComp->setPosition( params.pos );
		spatialComp->setOrientation( params.q );
	}

	mEntityList.push_back( entity );
	return entity;
}

bool EntitySystem::registerClass( IEntityClass* entityClass )
{
	EntityClassMap::iterator iter = mClassMap.find( entityClass->getName() );

	if ( iter != mClassMap.end() )
	{

		return false;
	}
	mClassMap.insert( std::make_pair( entityClass->getName() , entityClass ) );
	return true;
}

IEntityClass* EntitySystem::findClass( char const* name )
{
	EntityClassMap::iterator iter = mClassMap.find( name );
	if ( iter != mClassMap.end() )
	{
		return iter->second;
	}
	return NULL;
}
