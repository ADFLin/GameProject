#ifndef CARLevelActor_h__5cc5aa90_118b_43c8_88a1_c0a4b5e0dbfc
#define CARLevelActor_h__5cc5aa90_118b_43c8_88a1_c0a4b5e0dbfc

#include "CARCommon.h"

#include <vector>

namespace CAR
{
	class MapTile;
	class FeatureBase;


	typedef std::vector< class LevelActor* > ActorList;

	class ActorContainer
	{
	public:
		LevelActor* popActor();
		bool        haveActor(){ return mActors.empty() == false; }
		bool        haveOtherActor( int playerId );
		bool        havePlayerActor( int playerId , ActorType type );

		unsigned    getPlayerActorTypeMask( unsigned playerMask );
		bool        haveActorMask( unsigned actorTypeMask );
		bool        havePlayerActorMask(unsigned playerMask , unsigned actorTypeMask );
		LevelActor* iteratorActorMask( unsigned playerMask , unsigned actorTypeMask , int& iter);
		LevelActor* iteratorActorMask( unsigned playerMask , int& iter);
		LevelActor* findActor( unsigned playerMask , unsigned actorTypeMask );
		LevelActor* findActor( unsigned actorTypeMask );

		ActorList mActors;
	};

	class LevelActor
	{
	public:
		LevelActor()
		{
			feature = nullptr;
			mapTile = nullptr;
			owner   = nullptr;
			renderData = nullptr;
			binder = nullptr;
		}
		ActorType   type;
		
		PlayerBase*  owner;
		ActorPos     pos;
		MapTile*     mapTile;
		FeatureBase* feature;
		LevelActor*  binder;
		ActorList    followers;
		void*        renderData;

		LevelActor* popFollower();
		void removeFollower( LevelActor& actor );
		void addFollower( LevelActor& actor );	
	};

	




}//namespace CAR


#endif // CARLevelActor_h__5cc5aa90_118b_43c8_88a1_c0a4b5e0dbfc