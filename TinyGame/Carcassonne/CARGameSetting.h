#ifndef CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
#define CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a

#include <algorithm>
#include "Flag.h"

namespace CAR
{
	enum GameRule
	{
		eHardcore ,

		eSmallCity ,
		eDoubleTurnDrawImmediately ,
		eCantMoveFairy ,
		ePrincessTileMustRemoveKnightOrBuilder ,
		eMoveDragonBeforeScoring ,
		eTowerCaptureEverything ,
	};

	enum class Rule
	{
		eInn,
		eCathedral,
		eBigMeeple,
		eBuilder,
		eTraders,
		ePig,
		eKingAndRobber,
		ePrinecess,
		eDragon,
		eFariy,
		eTower,
		eAbbey,
		eWagon ,
		eMayor,
		eBarn,
		ePhantom,
		eUseHill ,
		eShepherdAndSheep ,
		eBazaar ,
		eBridge ,
		eCastleToken ,

		eGermanCastles ,

		//////////////
		eHardcore,
		eSmallCity,
		eDoubleTurnDrawImmediately,
		eCantMoveFairy,
		ePrincessTileMustRemoveKnightOrBuilder,
		eMoveDragonBeforeScoring,
		eTowerCaptureEverything,

		TotalNum,
	};

	class GameSetting
	{
	public:
		GameSetting();

		bool haveRule( Rule ruleFunc ) const;
		void addRule( Rule ruleFunc );
		int  getFarmScoreVersion(){ return mFarmScoreVersion; }

		void calcUsageField( int numPlayer );

		unsigned getFollowerMask();
		inline bool isFollower( ActorType type )
		{
			return ( getFollowerMask() & BIT( type ) ) != 0;
		}

		int getFieldNum()
		{
			return mNumField;
		}
		int getFieldIndex( FieldType::Enum type )
		{
			return mFieldIndex[type];
		}

		int      mFarmScoreVersion;
		int      mNumField;
		int      mFieldIndex[FieldType::NUM];

		FlagBits< (unsigned)Rule::TotalNum > mRuleFlags;
		unsigned mExpansionMask;
	};



}//namespace CAR

#endif // CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
