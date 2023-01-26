#include "CAR_PCH.h"
#include "CARPlayer.h"

#include "CARGameplaySetting.h"

namespace CAR
{
	GameParamCollection& PlayerBase::GetParamCollection(PlayerBase& t)
	{
		return *t.mSetting;
	}

	void PlayerBase::setupSetting(GameplaySetting& setting)
	{
		mSetting = &setting;
		mFieldValues.resize( setting.getTotalFieldValueNum() , 0 );
	}

	int PlayerBase::getFieldValue(EField::Type type , int index ) const
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == INDEX_NONE )
			return 0;
		assert(index < mSetting->getFieldValueNum(type));
		return mFieldValues[idx+index];
	}

	void PlayerBase::setFieldArrayValues(EField::Type type, int* values , int num )
	{
		int idx = mSetting->getFieldIndex(type);
		if( idx == INDEX_NONE )
			return;

		assert(num < mSetting->getFieldValueNum(type));
		int* fillValues = &mFieldValues[idx];
		std::copy_n(values, num, fillValues);
	}

	void PlayerBase::setFieldValue(EField::Type type , int value , int index )
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == INDEX_NONE )
			return;
		assert(index < mSetting->getFieldValueNum(type));
		mFieldValues[idx+index] = value;
	}

	int PlayerBase::modifyFieldValue(EField::Type type , int value)
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == INDEX_NONE )
			return 0;
		return mFieldValues[ idx ] += value;
	}



}//namespace CAR