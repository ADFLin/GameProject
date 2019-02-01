#ifndef CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
#define CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b

#include "CARCommon.h"

#include <vector>

namespace CAR
{
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
	};


	class GamePlayerManager
	{
	public:
		GamePlayerManager();
		int         getPlayerNum() { return mNumPlayer; }
		PlayerBase* getPlayer(int id) { assert(id != CAR_ERROR_PLAYER_ID); return mPlayerMap[id]; }
		void        addPlayer(PlayerBase* player);
		void        clearAllPlayer(bool bDelete);


		PlayerBase* mPlayerMap[MaxPlayerNum];
		int mNumPlayer;
	};

	template< class FieldProc >
	void ProcessFields(GameplaySetting& setting, int numPlayer, FieldProc& proc)
	{
#define FIELD_ACTOR( TYPE , VALUE )\
	proc.exec( FieldType::Enum( FieldType::eActorStart + TYPE ) , VALUE )
#define FIELD_VALUE( TYPE , VALUE )\
	proc.exec( TYPE , VALUE )
#define FIELD_ARRAY_VALUES( TYPE , VALUES , NUM )\
	proc.exec( TYPE , VALUES , NUM )

#define VALUE( NAME ) setting.##NAME

		FIELD_ACTOR(ActorType::eMeeple, VALUE(MeeplePlayerOwnNum));

		if( setting.have(Rule::eBigMeeple) )
		{
			FIELD_ACTOR(ActorType::eBigMeeple, VALUE(BigMeeplePlayerOwnNum));
		}
		if( setting.have(Rule::eBuilder) )
		{
			FIELD_ACTOR(ActorType::eBuilder, VALUE(BuilderPlayerOwnNum));
		}
		if( setting.have(Rule::ePig) )
		{
			FIELD_ACTOR(ActorType::ePig, VALUE(PigPlayerOwnNum));
		}
		if( setting.have(Rule::eTraders) )
		{
			FIELD_VALUE(FieldType::eWine, 0);
			FIELD_VALUE(FieldType::eGain, 0);
			FIELD_VALUE(FieldType::eCloth, 0);
		}
		if( setting.have(Rule::eBarn) )
		{
			FIELD_ACTOR(ActorType::eBarn, VALUE(BarnPlayerOwnNum));
		}
		if( setting.have(Rule::eWagon) )
		{
			FIELD_ACTOR(ActorType::eWagon, VALUE(WagonPlayerOwnNum));
		}
		if( setting.have(Rule::eMayor) )
		{
			FIELD_ACTOR(ActorType::eMayor, VALUE(MayorPlayerOwnNum));
		}
		if( setting.have(Rule::eHaveAbbeyTile) )
		{
			FIELD_VALUE(FieldType::eAbbeyPices, VALUE(AbbeyTilePlayerOwnNum));
		}
		if( setting.have(Rule::eTower) )
		{
			FIELD_VALUE(FieldType::eTowerPices, VALUE(TowerPicesPlayerOwnNum[numPlayer - 1]));
		}
		if( setting.have(Rule::eBridge) )
		{
			FIELD_VALUE(FieldType::eBridgePices, VALUE(BridgePicesPlayerOwnNum[numPlayer - 1]));
		}
		if( setting.have(Rule::eCastleToken) )
		{
			FIELD_VALUE(FieldType::eCastleTokens, VALUE(CastleTokensPlayerOwnNum[numPlayer - 1]));
		}
		if( setting.have(Rule::eBazaar) )
		{
			FIELD_VALUE(FieldType::eTileIdAuctioned, FAIL_TILE_ID);
		}
		if( setting.have(Rule::eShepherdAndSheep) )
		{
			FIELD_ACTOR(ActorType::eShepherd, VALUE(ShepherdPlayerOwnNum));
		}
		if( setting.have(Rule::ePhantom) )
		{
			FIELD_ACTOR(ActorType::ePhantom, VALUE(PhantomPlayerOwnNum));
		}
		if( setting.have(Rule::eHaveGermanCastleTile) )
		{

		}
		if( setting.have(Rule::eGold) )
		{
			FIELD_VALUE(FieldType::eGoldPieces, 0);
		}
		if( setting.have(Rule::eHaveHalflingTile) )
		{

		}
		if( setting.have(Rule::eLaPorxada) )
		{
			FIELD_VALUE(FieldType::eLaPorxadaFinishScoring, 0);
		}
		if( setting.have(Rule::eMessage) )
		{
			FIELD_VALUE(FieldType::eWomenScore, 0);
		}
		if( setting.have(Rule::eLittleBuilding) )
		{
			FIELD_VALUE(FieldType::eTowerBuildingTokens, VALUE(TowerBuildingTokensPlayerOwnNum));
			FIELD_VALUE(FieldType::eHouseBuildingTokens, VALUE(HouseBuildingTokensPlayerOwnNum));
			FIELD_VALUE(FieldType::eShedBuildingTokens, VALUE(ShedBuildingTokensPlayerOwnNum));
		}

#undef FIELD_ACTOR
#undef FIELD_VALUE
#undef VALUE
	}

}//namespace CAR


#endif // CARPlayer_h__43b4df64_d035_4b93_80b6_62c64773873b
