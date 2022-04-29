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
		virtual ~PlayerBase() = default;
		
		void startTurn(){}
		void endTurn(){}

		int  getId(){ return mId; }

		void setupSetting( GameplaySetting& setting );

		unsigned getSupportActorMask()
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
		int  modifyFieldValue( EActor::Type type , int value = 1 )
		{  
			return modifyFieldValue( EField::Type( EField::ActorStart + type ) , value ); 
		}

		int  getFieldValue( EField::Type type , int index = 0 ) const;
		void setFieldArrayValues(EField::Type type, int* values, int num);
		void setFieldValue(EField::Type type, int value , int index = 0 );
		int  modifyFieldValue( EField::Type type , int value = 1 );

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
		PlayerBase* getPlayer(PlayerId id) const { assert(id != CAR_ERROR_PLAYER_ID); return mPlayerMap[id + 1]; }
		void        addPlayer(PlayerBase* player);
		void        clearAllPlayer(bool bNeedDelete);


		PlayerBase* mPlayerMap[MaxPlayerNum];
		int mNumPlayer;
	};

}//namespace CAR


#endif // CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
