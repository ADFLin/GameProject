#include "TEntityManager.h"
#include "TPhySystem.h"
#include "TEntityBase.h"
#include "CHandle.h"
#include "TEffect.h"
#include "profile.h"

#include "TEventSystem.h"
#include "EventType.h"
#include "UtilityGlobal.h"

#include <algorithm>


TEntityManager::TEntityManager()
{
	m_debugShow = false;

}

TEntityManager::~TEntityManager()
{

}


void TEntityManager::findEntityInSphere( Vec3D const& pos , Float r , EntityResultCallBack* callback )
{
	iterator iter = m_PEList.begin();
	float r2 = r * r;
	while ( iter != m_PEList.end() )
	{
		TPhyEntity* pe = TPhyEntity::upCast( *iter );
		if ( pe && distance2( pos , pe->getPosition() )  < r2 )
		{
			if ( !callback->processResult( pe ) )
				break;
		}
		++iter;
	}
}

TPhyEntity* TEntityManager::findEntityInSphere( iterator& iter , Vec3D const& pos , Float r2 )
{
	while ( iter != m_PEList.end() )
	{
		TPhyEntity* pe = TPhyEntity::upCast( *iter );

		++iter;

		if ( pe && distance2( pos , pe->getPosition() )  < r2 )
		{
			return pe;
		}
	}
	return NULL;
}

void TEntityManager::updateFrame()
{
	{
		TPROFILE("Physics Simulation");
		Vec3D pos;
		TPhySystem::instance().stepSimulation( g_GlobalVal.frameTime );
	}

	{
		TPROFILE("Effect update");
		updateEffect();
	}
	{
		TPROFILE("Collision Process");
		for( iterator iter = m_PEList.begin(); iter != m_PEList.end() ; ++iter )
		{
			TEntityBase* entity = *iter;
			if ( entity->getEntityState() == EF_WORKING )
			{
				TPhyEntity* phyEntity = TPhyEntity::upCast( entity );
				TPhyBody* body = NULL;

				if ( phyEntity )
					body = TPhyBody::upcast( phyEntity->getPhyBody() );

				if ( body && phyEntity->needTestCollision() )
				{
					TPhySystem::instance().processCollision( *body );
				}
			}
		}
	}


	m_deleteList.clear();
	{
		TPROFILE("Entity Update");
		for( iterator iter = m_PEList.begin(); iter != m_PEList.end() ;)
		{
			TEntityBase* entity = *iter;

			switch ( entity->getEntityState() )
			{
			case EF_WORKING:
				entity->update();
				++iter;
				break;
			case EF_FREEZE:
				++iter;
				break;
			case EF_DESTORY:
				{
					m_deleteList.push_back( entity );
					iter = m_PEList.erase( iter );

					TEvent event( EVT_ENTITY_DESTORY , entity->getRefID() , this , entity );
					UG_SendEvent( event );
				}
				break;
			}
		}
	}


	for( std::vector< TEntityBase*>::iterator iter = m_deleteList.begin(); 
		iter != m_deleteList.end() ; ++iter )
	{
		TEntityBase* entity = *iter;
		clearEntitySet( entity );
		delete entity;
	}
	m_deleteList.clear();
}

void TEntityManager::addEntity( TEntityBase* entity , bool needMark /*= true */ )
{
	if ( entity->getEntityFlag() & EF_MANAGING )
	{
		Msg("Entity already had be added !");
		return;
	}

	if ( needMark )
	{
		THandleManager::instance().markRefHandle( entity );
	}

	TPhyEntity* body = TPhyEntity::upCast( entity );
	if ( body )
	{
		TPhySystem::instance().addEntity( body );
	}

	entity->addEntityFlag( EF_MANAGING );
	m_PEList.push_back( entity );
}


void TEntityManager::addEffect( TEntityBase* entity , TEffectBase* effect )
{
	EffectData data;

	data.effect = effect;
	data.handle = entity;	
	m_EffectList.push_back( data );

	effect->OnStart( entity );
}

void TEntityManager::clearEffect()
{
	EffectList::iterator iter = m_EffectList.begin();
	while ( iter != m_EffectList.end() )
	{
		TEffectBase* effect = iter->effect;
		TEntityBase* entity = iter->handle;

		if ( entity )
			effect->OnEnd( entity );

		delete effect;
		++iter;
	}
}

bool TEntityManager::removeEffect( TEntityBase* entity , TEffectBase* effect )
{
	EffectData data;
	data.effect = effect;
	data.handle = entity;

	struct CmpEqu
	{
		CmpEqu( EffectData& data):data_(data){}

		bool operator()( EffectData& data )
		{
			return data.effect == data_.effect &&
				   data.handle == data_.handle;
		}
		EffectData& data_;
	};
	EffectList::iterator iter = std::find_if( m_EffectList.begin() , m_EffectList.end() , CmpEqu( data )  );

	if ( iter != m_EffectList.end() )
	{
		effect->OnEnd( entity );
		delete effect;
		m_EffectList.erase( iter );

		return true;
	}

	return false;
}

void TEntityManager::updateEffect()
{
	EffectList::iterator iter = m_EffectList.begin();

	while( iter != m_EffectList.end() )
	{
		TEffectBase* effect = iter->effect;
		TEntityBase* entity = iter->handle;

		if ( entity == NULL )
		{
			delete effect;
			iter = m_EffectList.erase( iter );
		}
		else if (  effect->needRemove(entity) )
		{
			effect->OnEnd(entity);
			delete effect;
			iter = m_EffectList.erase( iter );
		}
		else
		{
			effect->update(entity);
			++iter;
		}
	}
}

void TEntityManager::clearEntitySet( TEntityBase* entity )
{
	TPhyEntity* body = TPhyEntity::upCast( entity );
	if ( body )
	{
		TPhySystem::instance().removeEntity( body );
	}
	entity->removeEntityFlag( EF_MANAGING );

}

void TEntityManager::removeEntity( TEntityBase* entity , bool beDel )
{
	if ( !( entity->getEntityFlag() & EF_MANAGING ) )
	{
		return;
	}

	m_PEList.erase( std::find( m_PEList.begin() , m_PEList.end() , entity ) );

	clearEntitySet( entity );

	if ( beDel )
		delete entity;
}

void TEntityManager::removeEntity( EntityType type ,bool beDel )
{
	for( iterator iter = m_PEList.begin(); iter != m_PEList.end() ;)
	{
		TEntityBase* entity = *iter;
		if ( entity->isKindOf( type ) && !entity->isGlobal() )
		{
			clearEntitySet( entity );

			iter = m_PEList.erase( iter );
			if ( beDel )
				delete entity;
		}
		else
		{
			++iter;
		}
	}
}

void TEntityManager::applyExplosion( Vec3D const& pos , float impulse , float range )
{
	iterator iter = getIter();

	float r2 =range * range;
	while( TEntityBase* entity = findEntityInSphere( iter , pos , r2 ) )
	{
		if ( TObject* obj = TObject::upCast(entity ) )
		{
			Vec3D dir = ( obj->getPosition() - pos ).normalize();
			dir += Vec3D( TRandom( -1,1) ,TRandom( -1,1),TRandom( -1,1) );
			dir.normalize();
			obj->getPhyBody()->applyCentralImpulse( impulse * dir );
			obj->getPhyBody()->activate();
		}
	}
}

TEntityBase* TEntityManager::visitEntity( iterator& iter )
{
	TEntityBase* result = NULL;
	if ( iter != m_PEList.end() )
	{
		result = *iter;
		++iter;
	}
	return result;
}

void TEntityManager::debugDraw( bool beDebug )
{
	if ( m_debugShow == beDebug )
		return;

	for( iterator iter = m_PEList.begin(); iter != m_PEList.end() ;)
	{
		TEntityBase* entity = *iter;

		switch ( entity->getEntityState() )
		{
		case EF_WORKING:
			entity->debugDraw( beDebug );
			++iter;
			break;
		case EF_FREEZE:
			++iter;
			break;
		case EF_DESTORY:
			++iter;
			break;
		}
	}

	m_debugShow = beDebug;
}

void UG_AddEffect( TEntityBase* entity , TEffectBase* effect )
{
	TEntityManager::instance().addEffect( entity , effect );
}

bool UG_RemoveEffect( TEntityBase* entity , TEffectBase* effect )
{
	return TEntityManager::instance().removeEffect( entity , effect );
}

