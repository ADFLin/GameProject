#include "CAR_PCH.h"
#include "CARLevelActor.h"
#include <algorithm>

namespace CAR
{


	bool ActorContainer::haveOtherActor(int playerId)
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];
			if ( actor->ownerId != CAR_ERROR_PLAYER_ID &&  actor->ownerId != playerId )
				return true;
		}
		return false;
	}

	unsigned ActorContainer::getPlayerActorTypeMask( unsigned playerMask )
	{
		unsigned result = 0;
		int idx = 0;
		while ( LevelActor* actor = iteratorActorMask( playerMask , idx ) )
		{ 
			result |= BIT(actor->type);
		}
		return result;
	}

	bool ActorContainer::havePlayerActor(int playerId , ActorType type)
	{
		return havePlayerActorMask( BIT(playerId) , BIT(type) );
	}

	LevelActor* ActorContainer::iteratorActorMask(unsigned playerMask , int& iter)
	{
		for( ; iter < mActors.size() ; ++iter )
		{
			LevelActor* actor = mActors[iter];
			if ( actor->ownerId == CAR_ERROR_PLAYER_ID )
				continue;
			if (( playerMask & BIT( actor->ownerId ) ) == 0 )
				continue;

			++iter;
			return actor;
		}
		return nullptr;
	}

	LevelActor* ActorContainer::iteratorActorMask(unsigned playerMask , unsigned actorTypeMask , int& iter)
	{
		for( ; iter < mActors.size() ; ++iter )
		{
			LevelActor* actor = mActors[iter];
			if( actor->ownerId == CAR_ERROR_PLAYER_ID )
				continue;
			if( (playerMask & BIT(actor->ownerId)) == 0 )
				continue;
			if (( actorTypeMask & BIT(actor->type) ) == 0 )
				continue;

			++iter;
			return actor;
		}
		return nullptr;
	}

	bool ActorContainer::havePlayerActorMask(unsigned playerMask , unsigned actorTypeMask )
	{
		int idx = 0;
		while( LevelActor* actor = iteratorActorMask(playerMask, idx) )
		{
			if ( BIT( actor->type ) & actorTypeMask )
				return true;
		}
		return false;
	}

	bool ActorContainer::haveActorMask(unsigned actorTypeMask)
	{
		for(int idx = 0; idx < mActors.size(); ++idx )
		{
			LevelActor* actor = mActors[idx];
			if ( BIT( actor->type ) & actorTypeMask )
				return true;
		}
		return false;
	}


	LevelActor* ActorContainer::popActor()
	{
		if ( mActors.empty() )
			return nullptr;
		LevelActor* actor = mActors.back();
		mActors.pop_back();
		return actor;
	}

	LevelActor* ActorContainer::findActor(unsigned playerMask , unsigned actorTypeMask)
	{
		int iter = 0;
		return iteratorActorMask( playerMask , actorTypeMask , iter );
	}

	LevelActor* ActorContainer::findActor(unsigned actorTypeMask)
	{
		int iter = 0;
		for( ; iter < mActors.size() ; ++iter )
		{
			LevelActor* actor = mActors[iter];
			if (( actorTypeMask & BIT(actor->type) ) == 0 )
				continue;
			return actor;
		}
		return nullptr;
	}

	LevelActor* LevelActor::popFollower()
	{
		if ( followers.empty() )
			return nullptr;
		LevelActor* actor = followers.back();
		followers.pop_back();
		actor->binder = nullptr;
		return actor;
	}

	void LevelActor::removeFollower(LevelActor& actor)
	{
		assert ( actor.binder == this );
		followers.erase( std::find( followers.begin() , followers.end() , &actor ) );
		actor.binder = nullptr;
	}

	void LevelActor::addFollower(LevelActor& actor)
	{
		if ( actor.binder )
		{
			actor.binder->removeFollower( actor );
		}
		actor.binder = this;
		followers.push_back( &actor );
	}

}