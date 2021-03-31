#include "CAR_PCH.h"
#include "CARLevelActor.h"
#include <algorithm>

namespace CAR
{


	bool ActorCollection::haveOtherActor(int playerId)
	{
		for(LevelActor* actor : mActors)
		{
			if ( actor->ownerId != CAR_ERROR_PLAYER_ID &&  actor->ownerId != playerId )
				return true;
		}
		return false;
	}

	unsigned ActorCollection::getPlayerActorTypeMask( unsigned playerMask ) const
	{
		unsigned result = 0;
		int idx = 0;
		while ( LevelActor* actor = iteratorActorFromPlayer( playerMask , idx ) )
		{ 
			result |= BIT(actor->type);
		}
		return result;
	}

	LevelActor* ActorCollection::iteratorActorFromPlayer(unsigned playerMask , int& iter) const
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

	LevelActor* ActorCollection::iteratorActorFromType( unsigned actorTypeMask, int& iter) const
	{
		for( ; iter < mActors.size(); ++iter )
		{
			LevelActor* actor = mActors[iter];
			if( (actorTypeMask & BIT(actor->type)) == 0 )
				continue;
			++iter;
			return actor;
		}
		return nullptr;
	}

	LevelActor* ActorCollection::iteratorActor(unsigned playerMask , unsigned actorTypeMask , int& iter)  const
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


	LevelActor* ActorCollection::findActor(unsigned playerMask, unsigned actorTypeMask) const
	{
		int iter = 0;
		return iteratorActor(playerMask, actorTypeMask, iter);
	}

	LevelActor* ActorCollection::findActorFromType(unsigned actorTypeMask) const
	{
		int iter = 0;
		return iteratorActorFromType(actorTypeMask, iter);
	}

	LevelActor* ActorCollection::findActorFromPlayer(unsigned playerMask) const
	{
		int iter = 0;
		return iteratorActorFromPlayer(playerMask, iter);
	}

	int ActorCollection::countActor(unsigned playerMask, unsigned actorTypeMask) const
	{
		int result = 0;
		int iter = 0;
		while (auto actor = iteratorActor(playerMask, actorTypeMask, iter))
		{
			++result;
		}
		return result;
	}

	LevelActor* ActorCollection::popActor()
	{
		if ( mActors.empty() )
			return nullptr;
		LevelActor* actor = mActors.back();
		mActors.pop_back();
		return actor;
	}


	LevelActor* LevelActor::popFollower()
	{
		if ( followers.empty() )
			return nullptr;
		LevelActor* actor = followers.back();
		assert(actor->binder == this);
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