#include "CAR_PCH.h"
#include "CARPlayer.h"

#include "CARGameplaySetting.h"

namespace CAR
{
	void PlayerBase::setupSetting(GameplaySetting& setting)
	{
		mSetting = &setting;
		mFieldValues.resize( setting.getTotalFieldValueNum() , 0 );
	}

	int PlayerBase::getFieldValue(FieldType::Enum type , int index )
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == -1 )
			return 0;
		assert(index < mSetting->getFieldValueNum(type));
		return mFieldValues[idx+index];
	}

	void PlayerBase::setFieldArrayValues(FieldType::Enum type, int* values , int num )
	{
		int idx = mSetting->getFieldIndex(type);
		if( idx == -1 )
			return;

		assert(num < mSetting->getFieldValueNum(type));
		int* fillValues = &mFieldValues[idx];
		std::copy_n(values, num, fillValues);
	}

	void PlayerBase::setFieldValue(FieldType::Enum type , int value , int index )
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == -1 )
			return;
		assert(index < mSetting->getFieldValueNum(type));
		mFieldValues[idx+index] = value;
	}

	int PlayerBase::modifyFieldValue(FieldType::Enum type , int value /*= 1 */)
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == -1 )
			return 0;
		return mFieldValues[ idx ] += value;
	}

	void AddExpansionRule(GameplaySetting& setting, Expansion exp)
	{
		auto& MyProc = [&setting](Rule rule)
		{
			setting.addRule(rule);
		};
		ProcExpansionRule(exp, MyProc);
	}

}//namespace CAR