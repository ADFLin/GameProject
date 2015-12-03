#include "TDPCH.h"
#include "TDEntity.h"

#include "TDWorld.h"

namespace TowerDefend
{

	Entity::Entity()
	{
		mWorld = NULL;
		mType  = 0; 
		mFlag  = 0;
		mRefcount = 0;
	}

	Entity::~Entity()
	{

	}

	bool Entity::testFilter( EntityFilter const& filter )
	{
		if ( filter.bitType )
		{
			if ( !getType() & filter.bitType )
				return false;
		}
		return true;
	}


	EntityManager::~EntityManager()
	{
		cleanup();
	}

	void EntityManager::cleanup()
	{
		for( EntityList::iterator iter = mEntities.begin(); 
			iter != mEntities.end() ; ++iter )
		{
			(*iter)->onDestroy();
			delete (*iter);
		}
		mEntities.clear();
	}

	void EntityManager::updateFrame( int frame )
	{
		for( EntityList::iterator iter = mEntities.begin(); 
			iter != mEntities.end() ; ++iter )
		{
			(*iter)->onUpdateFrame( frame );
		}
	}

	void EntityManager::addEntity( Entity* entity )
	{
		assert( entity->mWorld == NULL );
		mEntities.push_back( entity );
		entity->mWorld = &mWorld;

		entity->onSpawn();
		sendEvent( EVT_EN_SPAWN , entity );
	}

	void EntityManager::tick()
	{
		for( EntityList::iterator iter = mEntities.begin(); 
			iter != mEntities.end() ;  )
		{
			if ( (*iter)->checkFlag( EF_DESTROY ) )
			{
				sendEvent( EVT_EN_DESTROY , *iter );
				(*iter)->onDestroy();
				mDelEntities.push_back( *iter );
				iter = mEntities.erase( iter );
			}
			else
			{
				(*iter)->onTick();
				++iter;
			}
		}

		for ( EntityList::iterator iter = mDelEntities.begin(); 
			iter != mDelEntities.end() ; )
		{
			if ( (*iter)->mRefcount == 0 )
			{
				delete (*iter);
				iter = mDelEntities.erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}

	Entity* EntityManager::findEntity( Vec2f const& pos , float radius , bool beMinRadius , EntityFilter& filter )
	{
		Entity* result = NULL;
		for( EntityList::iterator iter = mEntities.begin();
			iter != mEntities.end() ; ++iter )
		{

			if ( !(*iter)->testFilter( filter ) )
				continue;

			Vec2f offset = (*iter)->getPos() - pos;
			float dist = sqrt( offset.length2() );

			if ( dist < radius )
			{
				result = *iter;
				if ( !beMinRadius )
					return result;
				radius = dist;
			}
		}

		return result;
	}

	void EntityManager::render( Renderer& rs)
	{
		for( EntityList::iterator iter = mEntities.begin(); 
			iter != mEntities.end() ; ++iter )
		{
			(*iter)->render( rs );
		}
	}


	void EntityManager::listenEvent( EntityEventFun fun , EntityFilter const& filter )
	{
		if ( !fun )
			return;

		CallbackData data;
		data.filter   = filter;
		data.fun      = fun;
		mEventCallbacks.push_back( data );
	}

	void EntityManager::sendEvent( EntityEvent type , Entity* entity )
	{
		for ( CallbackVec::iterator iter = mEventCallbacks.begin();
			iter != mEventCallbacks.end() ; ++iter )
		{
			if ( !entity->testFilter( iter->filter ) )
				continue;
			iter->fun( type , entity );
		}
	}

}//namespace TowerDefend
