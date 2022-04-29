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

		bool have( ERule ruleFunc ) const;
		void addRule( ERule ruleFunc );
		void addExpansion(Expansion exp)
		{
			mUseExpansionMask.add(exp);
		}
		int  getFarmScoreVersion() const { return mFarmScoreVersion; }

		void calcUsageOfField( int numPlayer );

		unsigned getFollowerMask()  const;
		inline bool isFollower( EActor::Type type ) const
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
		int getFieldIndex( EField::Type type ) const
		{
			return mFieldInfos[type].index;
		}

		int getFieldValueNum(EField::Type type) const
		{
			return mFieldInfos[type].num;
		}

		bool haveUse(Expansion exp) const { return mUseExpansionMask.check(exp); }

		struct FieldInfo
		{
			int index;
			int num;
		};
		int        mFarmScoreVersion;
		int        mNumField;
		FieldInfo  mFieldInfos[EField::COUNT];

		FlagBits< (int)ERule::COUNT > mRuleFlags;
		FlagBits< (int)NUM_EXPANSIONS > mUseExpansionMask;
	};

	template< class FieldProc >
	void ProcessFields(GameplaySetting& setting, int numPlayer, FieldProc& proc)
	{
#define FIELD_VALUE( TYPE , VALUE )\
	proc.exec( TYPE , VALUE )
#define FIELD_ARRAY_VALUES( TYPE , VALUES , COUNT )\
	proc.exec( TYPE , VALUES , COUNT )

#if CAR_USE_CONST_PARAMVALUE
#define VALUE(NAME) CAR_PARAM_VALUE(NAME)
#else
#define VALUE(NAME) setting.NAME
#endif

		FIELD_VALUE(EActor::Meeple, VALUE(MeeplePlayerOwnNum));

		if (setting.have(ERule::BigMeeple))
		{
			FIELD_VALUE(EActor::BigMeeple, VALUE(BigMeeplePlayerOwnNum));
		}
		if (setting.have(ERule::Builder))
		{
			FIELD_VALUE(EActor::Builder, VALUE(BuilderPlayerOwnNum));
		}
		if (setting.have(ERule::Pig))
		{
			FIELD_VALUE(EActor::Pig, VALUE(PigPlayerOwnNum));
		}
		if (setting.have(ERule::Traders))
		{
			FIELD_VALUE(EField::Wine, 0);
			FIELD_VALUE(EField::Gain, 0);
			FIELD_VALUE(EField::Cloth, 0);
		}
		if (setting.have(ERule::Barn))
		{
			FIELD_VALUE(EActor::Barn, VALUE(BarnPlayerOwnNum));
		}
		if (setting.have(ERule::Wagon))
		{
			FIELD_VALUE(EActor::Wagon, VALUE(WagonPlayerOwnNum));
		}
		if (setting.have(ERule::Mayor))
		{
			FIELD_VALUE(EActor::Mayor, VALUE(MayorPlayerOwnNum));
		}
		if (setting.have(ERule::HaveAbbeyTile))
		{
			FIELD_VALUE(EField::AbbeyPices, VALUE(AbbeyTilePlayerOwnNum));
		}
		if (setting.have(ERule::Tower))
		{
			FIELD_VALUE(EField::TowerPices, VALUE(TowerPicesPlayerOwnNum[numPlayer - 1]));
		}
		if (setting.have(ERule::Bridge))
		{
			FIELD_VALUE(EField::BridgePices, VALUE(BridgePicesPlayerOwnNum[numPlayer - 1]));
		}
		if (setting.have(ERule::CastleToken))
		{
			FIELD_VALUE(EField::CastleTokens, VALUE(CastleTokensPlayerOwnNum[numPlayer - 1]));
		}
		if (setting.have(ERule::Bazaar))
		{
			FIELD_VALUE(EField::TileIdAuctioned, FAIL_TILE_ID);
		}
		if (setting.have(ERule::ShepherdAndSheep))
		{
			FIELD_VALUE(EActor::Shepherd, VALUE(ShepherdPlayerOwnNum));
		}
		if (setting.have(ERule::Phantom))
		{
			FIELD_VALUE(EActor::Phantom, VALUE(PhantomPlayerOwnNum));
		}
		if (setting.have(ERule::HaveGermanCastleTile))
		{

		}
		if (setting.have(ERule::Gold))
		{
			FIELD_VALUE(EField::GoldPieces, 0);
		}
		if (setting.have(ERule::HaveHalflingTile))
		{

		}
		if (setting.have(ERule::LaPorxada))
		{
			FIELD_VALUE(EField::LaPorxadaFinishScoring, 0);
		}
		if (setting.have(ERule::Message))
		{
			FIELD_VALUE(EField::WomenScore, 0);
		}
		if (setting.have(ERule::LittleBuilding))
		{
			FIELD_VALUE(EField::TowerBuildingTokens, VALUE(TowerBuildingTokensPlayerOwnNum));
			FIELD_VALUE(EField::HouseBuildingTokens, VALUE(HouseBuildingTokensPlayerOwnNum));
			FIELD_VALUE(EField::ShedBuildingTokens, VALUE(ShedBuildingTokensPlayerOwnNum));
		}
		if (setting.have(ERule::Robber))
		{
			FIELD_VALUE(EField::RobberScorePos, -1);
		}

#undef FIELD_VALUE
#undef VALUE

	}

	template< class T, class ...TRule >
	void ProcSequenceRule(T& ProcFunc, ERule rule, TRule... rules)
	{
		ProcFunc(rule);
		ProcSequenceRule(ProcFunc, rules...);
	}

	template< class T >
	void ProcSequenceRule(T& ProcFunc, ERule rule)
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
		CASE(EXP_INNS_AND_CATHEDRALS, ERule::Inn, ERule::Cathedral);
		CASE(EXP_TRADERS_AND_BUILDERS, ERule::Builder, ERule::Traders);
		CASE(EXP_THE_PRINCESS_AND_THE_DRAGON, ERule::Fariy, ERule::Dragon, ERule::Prinecess);
		CASE(EXP_THE_TOWER, ERule::Tower);
		CASE(EXP_ABBEY_AND_MAYOR, ERule::HaveAbbeyTile, ERule::Mayor, ERule::Barn, ERule::Wagon);
		CASE(EXP_KING_AND_ROBBER, ERule::KingAndRobber);
		CASE(EXP_BRIDGES_CASTLES_AND_BAZAARS, ERule::Bridge, ERule::CastleToken, ERule::Bazaar);
		CASE(EXP_HILLS_AND_SHEEP, ERule::ShepherdAndSheep, ERule::UseHill, ERule::UseVineyard);
		CASE(EXP_CASTLES_IN_GERMANY, ERule::HaveGermanCastleTile);
		CASE(EXP_THE_PHANTOM, ERule::Phantom);
		CASE(EXP_CROP_CIRCLE_I, ERule::CropCircle);
		CASE(EXP_CROP_CIRCLE_II, ERule::CropCircle);
		CASE(EXP_THE_FLIER, ERule::FlyMahine);
		CASE(EXP_THE_GOLDMINES, ERule::Gold);
		CASE(EXP_LA_PORXADA, ERule::LaPorxada);
		CASE(EXP_MAGE_AND_WITCH, ERule::MageAndWitch);
		CASE(EXP_THE_MESSSAGES, ERule::Message);
		CASE(EXP_THE_ROBBERS, ERule::Robber);
		CASE(EXP_THE_SCHOOL, ERule::Teacher);
		CASE(EXP_THE_FESTIVAL, ERule::Festival);
		}
#undef CASE
	}

	void AddExpansionRule(GameplaySetting& setting, Expansion exp);

}//namespace CAR

#endif // CARGameplaySetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
