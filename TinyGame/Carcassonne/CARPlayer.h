#ifndef CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
#define CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b

#include "CARCommon.h"
#include <vector>

namespace CAR
{
	class GameParamCollection;
	class GameplaySetting;

	class PlayerBase
	{
	public:
		PlayerBase()
		{
			mScore = 0;
			mSetting = nullptr;
		}
		virtual ~PlayerBase(){}
		
		void startTurn(){}
		void endTurn(){}

		int  getId(){ return mId; }

		void setupSetting( GameplaySetting& setting );

		unsigned getUsageActorMask()
		{
			unsigned result = 0;
			for( int i = 0 ; i < NUM_PLAYER_ACTOR_TYPE ; ++i )
			{
				if ( getActorValue( ActorType(i) ) )
					result |= BIT(i);
			}
			return result;
		}

		bool haveActor( ActorType type ){ return getActorValue( type ) != 0; }
		int  getActorValue( ActorType type )
		{ return getFieldValue( FieldType::Enum( FieldType::eActorStart + type ) ); }
		void setActorValue( ActorType type , int value )
		{  setFieldValue( FieldType::Enum( FieldType::eActorStart + type ) , value );  }
		int  modifyActorValue( ActorType type , int value = 1 )
		{  return modifyFieldValue( FieldType::Enum( FieldType::eActorStart + type ) , value ); }

		int  getFieldValue( FieldType::Enum type , int index = 0 );
		void setFieldArrayValues(FieldType::Enum type, int* values, int num);
		void setFieldValue(FieldType::Enum type, int value , int index = 0 );
		int  modifyFieldValue( FieldType::Enum type , int value = 1 );

		static int const MaxAcotrSlotNum = 10;

		int mPlayOrder;
		int mId;
		int mTeam;

		int mScore;
		std::vector< int > mFieldValues;

		GameplaySetting* mSetting;

		GameParamCollection& GetParamCollection(PlayerBase& t);
	};


	class GamePlayerManager
	{
	public:
		GamePlayerManager();
		int         getPlayerNum() { return mNumPlayer; }
		PlayerBase* getPlayer(PlayerId id) { assert(id != CAR_ERROR_PLAYER_ID); return mPlayerMap[id]; }
		void        addPlayer(PlayerBase* player);
		void        clearAllPlayer(bool bDelete);


		PlayerBase* mPlayerMap[MaxPlayerNum];
		int mNumPlayer;
	};

}//namespace CAR


#endif // CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
