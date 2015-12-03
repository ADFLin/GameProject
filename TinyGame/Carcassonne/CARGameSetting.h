#ifndef CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
#define CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a

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

	struct RuleFunc
	{
		enum Enum
		{
			eInn  ,
			eCathedral ,
			eBigMeeple ,
			eBuilder ,
			eTrader ,
			eDragon ,
			eFariy ,
			eTower ,
			eAbbey ,
			eMayor ,

			TotalNum ,
		};
	};


	class GameSetting
	{
	public:
		GameSetting()
		{
			mExpansionMask = 0;
			mRuleMask = 0;
			mNumField = 0;
			mFarmScoreVersion = 3;
			std::fill_n( mFieldIndex , (int)FieldType::NUM , -1 );
		}

		bool haveUse( Expansion exp ) const { return ( mExpansionMask & BIT(exp) ) != 0 ; }
		bool haveRule( GameRule rule ) const { return ( mRuleMask & BIT(rule) ) != 0 ; }

		int  getFarmScoreVersion(){ return mFarmScoreVersion; }

		void calcUsageField( int numPlayer );

		unsigned getFollowerMask()
		{
			unsigned const BaseFollowrMask = 
				BIT( ActorType::eMeeple ) | BIT( ActorType::eBigMeeple ) |
				BIT( ActorType::eMayor ) | BIT( ActorType::eWagon ) |
				BIT( ActorType::eBarn );

			return BaseFollowrMask;
		}
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

		enum
		{
			NumRuleFunMask = ( RuleFunc::TotalNum - 1 ) / ( 8 * sizeof( unsigned ) ) + 1 ,
		};
		unsigned mRuleFuncMasks[ NumRuleFunMask ];
		unsigned mExpansionMask;
		unsigned mRuleMask;
	};



}//namespace CAR

#endif // CARGameSetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
