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


}//namespace CAR

#endif // CARGameplaySetting_h__85a4781f_a1e7_4eeb_a352_d8c9ee34798a
