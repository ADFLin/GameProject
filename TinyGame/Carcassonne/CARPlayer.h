#ifndef CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
#define CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b

#include "CARCommon.h"

#include "DataStructure/Array.h"

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
		virtual ~PlayerBase() = default;
		
		void startTurn(){}
		void endTurn(){}

		PlayerId  getId(){ return mId; }

		void setupSetting( GameplaySetting& setting );

		unsigned getSupportedActorMask()
		{
			unsigned result = 0;
			for( int i = 0 ; i < EActor::PLAYER_TYPE_COUNT ; ++i )
			{
				if ( getFieldValue( EActor::Type(i) ) )
					result |= BIT(i);
			}
			return result;
		}

		bool haveActor( EActor::Type type ) const { return getFieldValue( type ) != 0; }
		int  getFieldValue( EActor::Type type ) const
		{ 
			return getFieldValue( EField::Type( EField::ActorStart + type ) );
		}
		void setFieldValue( EActor::Type type , int value )
		{  
			setFieldValue( EField::Type( EField::ActorStart + type ) , value );  
		}
		int  modifyFieldValue( EActor::Type type , int value )
		{  
			return modifyFieldValue( EField::Type( EField::ActorStart + type ) , value ); 
		}

		int  getFieldValue( EField::Type type , int index = 0 ) const;
		void setFieldArrayValues(EField::Type type, int* values, int num);
		void setFieldValue(EField::Type type, int value , int index = 0 );
		int  modifyFieldValue( EField::Type type , int value );

		int mPlayOrder;
		PlayerId mId;
		int mTeam;

		int mScore;

		TArray< int > mFieldValues;
		GameplaySetting* mSetting;

		static GameParamCollection& GetParamCollection(PlayerBase& t);
	};


	class GamePlayerManager
	{
	public:
		GamePlayerManager();
		int         getPlayerNum() { return mNumPlayer; }
		PlayerBase* getPlayerByIndex(int index) const { return mPlayerMap[index]; }
		PlayerBase* getPlayer(PlayerId id) const { CHECK(id != CAR_ERROR_PLAYER_ID); return mPlayerMap[id]; }
		void        addPlayer(PlayerBase* player);
		void        clearAllPlayer(bool bNeedDelete);


		PlayerBase* mPlayerMap[MaxPlayerNum];
		int mNumPlayer;
	};

}//namespace CAR


#endif // CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
