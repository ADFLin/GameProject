#ifndef CARGameplaySetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
#define CARGameplaySetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a

#include "CARParamValue.h"
#include "CARExpansion.h"

#include <algorithm>
#include "Flag.h"


namespace CAR
{

	class GameplaySetting : public GameParamCollection
	{
	public:
		GameplaySetting();

		bool have( Rule ruleFunc ) const;
		void addRule( Rule ruleFunc );
		void addExpansion(Expansion exp)
		{
			mUseExpansionMask.add(exp);
		}
		int  getFarmScoreVersion() const { return mFarmScoreVersion; }

		void calcUsageOfField( int numPlayer );

		unsigned getFollowerMask()  const;
		inline bool isFollower( ActorType type ) const
		{
			return ( getFollowerMask() & BIT( type ) ) != 0;
		}

		int getTotalFieldValueNum() const
		{
			int result = 0;
			for( auto info : mFieldInfos )
			{
				result += info.num;
			}
			return result;
		}
		int getFieldNum() const
		{
			return mNumField;
		}
		int getFieldIndex( FieldType::Enum type ) const
		{
			return mFieldInfos[type].index;
		}

		int getFieldValueNum(FieldType::Enum type) const
		{
			return mFieldInfos[type].num;
		}

		bool use(Expansion exp) const { return mUseExpansionMask.check(exp); }

		struct FieldInfo
		{
			int index;
			int num;
		};
		int        mFarmScoreVersion;
		int        mNumField;
		FieldInfo  mFieldInfos[FieldType::NUM];

		FlagBits< (int)Rule::TotalNum > mRuleFlags;
		FlagBits< (int)NUM_EXPANSIONS > mUseExpansionMask;
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

#if CAR_USE_CONST_PARAMVALUE
#define VALUE(NAME) CAR_PARAM_VALUE(NAME)
#else
#define VALUE(NAME) setting.NAME
#endif

		FIELD_ACTOR(ActorType::eMeeple, VALUE(MeeplePlayerOwnNum));

		if (setting.have(Rule::eBigMeeple))
		{
			FIELD_ACTOR(ActorType::eBigMeeple, VALUE(BigMeeplePlayerOwnNum));
		}
		if (setting.have(Rule::eBuilder))
		{
			FIELD_ACTOR(ActorType::eBuilder, VALUE(BuilderPlayerOwnNum));
		}
		if (setting.have(Rule::ePig))
		{
			FIELD_ACTOR(ActorType::ePig, VALUE(PigPlayerOwnNum));
		}
		if (setting.have(Rule::eTraders))
		{
			FIELD_VALUE(FieldType::eWine, 0);
			FIELD_VALUE(FieldType::eGain, 0);
			FIELD_VALUE(FieldType::eCloth, 0);
		}
		if (setting.have(Rule::eBarn))
		{
			FIELD_ACTOR(ActorType::eBarn, VALUE(BarnPlayerOwnNum));
		}
		if (setting.have(Rule::eWagon))
		{
			FIELD_ACTOR(ActorType::eWagon, VALUE(WagonPlayerOwnNum));
		}
		if (setting.have(Rule::eMayor))
		{
			FIELD_ACTOR(ActorType::eMayor, VALUE(MayorPlayerOwnNum));
		}
		if (setting.have(Rule::eHaveAbbeyTile))
		{
			FIELD_VALUE(FieldType::eAbbeyPices, VALUE(AbbeyTilePlayerOwnNum));
		}
		if (setting.have(Rule::eTower))
		{
			FIELD_VALUE(FieldType::eTowerPices, VALUE(TowerPicesPlayerOwnNum[numPlayer - 1]));
		}
		if (setting.have(Rule::eBridge))
		{
			FIELD_VALUE(FieldType::eBridgePices, VALUE(BridgePicesPlayerOwnNum[numPlayer - 1]));
		}
		if (setting.have(Rule::eCastleToken))
		{
			FIELD_VALUE(FieldType::eCastleTokens, VALUE(CastleTokensPlayerOwnNum[numPlayer - 1]));
		}
		if (setting.have(Rule::eBazaar))
		{
			FIELD_VALUE(FieldType::eTileIdAuctioned, FAIL_TILE_ID);
		}
		if (setting.have(Rule::eShepherdAndSheep))
		{
			FIELD_ACTOR(ActorType::eShepherd, VALUE(ShepherdPlayerOwnNum));
		}
		if (setting.have(Rule::ePhantom))
		{
			FIELD_ACTOR(ActorType::ePhantom, VALUE(PhantomPlayerOwnNum));
		}
		if (setting.have(Rule::eHaveGermanCastleTile))
		{

		}
		if (setting.have(Rule::eGold))
		{
			FIELD_VALUE(FieldType::eGoldPieces, 0);
		}
		if (setting.have(Rule::eHaveHalflingTile))
		{

		}
		if (setting.have(Rule::eLaPorxada))
		{
			FIELD_VALUE(FieldType::eLaPorxadaFinishScoring, 0);
		}
		if (setting.have(Rule::eMessage))
		{
			FIELD_VALUE(FieldType::eWomenScore, 0);
		}
		if (setting.have(Rule::eLittleBuilding))
		{
			FIELD_VALUE(FieldType::eTowerBuildingTokens, VALUE(TowerBuildingTokensPlayerOwnNum));
			FIELD_VALUE(FieldType::eHouseBuildingTokens, VALUE(HouseBuildingTokensPlayerOwnNum));
			FIELD_VALUE(FieldType::eShedBuildingTokens, VALUE(ShedBuildingTokensPlayerOwnNum));
		}
		if (setting.have(Rule::eRobber))
		{
			FIELD_VALUE(FieldType::eRobberScorePos, -1);
		}

#undef FIELD_ACTOR
#undef FIELD_VALUE
#undef VALUE

	}

	template< class T, class ...TRule >
	void ProcSequenceRule(T& ProcFunc, Rule rule, TRule... rules)
	{
		ProcFunc(rule);
		ProcSequenceRule(ProcFunc, rules...);
	}

	template< class T, class ...TRule >
	void ProcSequenceRule(T& ProcFunc, Rule rule)
	{
		ProcFunc(rule);
	}

	template< class T >
	void ProcExpansionRule(Expansion exp, T& ProcFunc)
	{
#define CASE( EXP , ... )\
		case EXP: ProcSequenceRule( ProcFunc , __VA_ARGS__ ); break;

		switch (exp)
		{
		CASE(EXP_INNS_AND_CATHEDRALS, Rule::eInn, Rule::eCathedral);
		CASE(EXP_TRADERS_AND_BUILDERS, Rule::eBuilder, Rule::eTraders);
		CASE(EXP_THE_PRINCESS_AND_THE_DRAGON, Rule::eFariy, Rule::eDragon, Rule::ePrinecess);
		CASE(EXP_THE_TOWER, Rule::eTower);
		CASE(EXP_ABBEY_AND_MAYOR, Rule::eHaveAbbeyTile, Rule::eMayor, Rule::eBarn, Rule::eWagon);
		CASE(EXP_KING_AND_ROBBER, Rule::eKingAndRobber);
		CASE(EXP_BRIDGES_CASTLES_AND_BAZAARS, Rule::eBridge, Rule::eCastleToken, Rule::eBazaar);
		CASE(EXP_HILLS_AND_SHEEP, Rule::eShepherdAndSheep, Rule::eUseHill, Rule::eUseVineyard);
		CASE(EXP_CASTLES, Rule::eHaveGermanCastleTile);
		CASE(EXP_PHANTOM, Rule::ePhantom);
		CASE(EXP_CROP_CIRCLE_I, Rule::eCropCircle);
		CASE(EXP_CROP_CIRCLE_II, Rule::eCropCircle);
		CASE(EXP_THE_FLY_MACHINES, Rule::eFlyMahine);
		CASE(EXP_GOLDMINES, Rule::eGold);
		CASE(EXP_LA_PORXADA, Rule::eLaPorxada);
		CASE(EXP_MAGE_AND_WITCH, Rule::eMageAndWitch);
		CASE(EXP_THE_MESSSAGES, Rule::eMessage);
		CASE(EXP_THE_ROBBERS, Rule::eRobber);
		CASE(EXP_THE_SCHOOL, Rule::eTeacher);
		CASE(EXP_THE_FESTIVAL, Rule::eFestival);
		}
#undef CASE
	}

	void AddExpansionRule(GameplaySetting& setting, Expansion exp);

}//namespace CAR

#endif // CARGameplaySetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
