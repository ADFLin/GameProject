#include "CARLevelActor.h"

#include "CARPlayer.h"

#include <algorithm>

namespace CAR
{

	bool ActorContainer::haveOtherActor(int playerId)
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];
			if ( actor->owner && actor->owner->getId() != playerId )
				return true;
		}
		return false;
	}

	unsigned ActorContainer::getPlayerActorTypeMask( unsigned playerMask )
	{
		unsigned result = 0;
		int idx = 0;
		for(;;)
		{
			LevelActor* actor = iteratorActorMask( playerMask , idx );
			if ( actor == nullptr )
				break;
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
			if ( actor->owner == nullptr )
				continue;
			if (( playerMask & BIT(actor->owner->getId()) ) == 0 )
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
			if ( actor->owner == nullptr )
				continue;
			if (( playerMask & BIT(actor->owner->getId()) ) == 0 )
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
		for(;;)
		{
			LevelActor* actor = iteratorActorMask( playerMask , idx );
			if ( actor == nullptr )
				break;
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
		actor->feature = nullptr;
		return actor;
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