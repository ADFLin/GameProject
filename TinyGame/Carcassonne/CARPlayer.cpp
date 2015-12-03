#include "CARPlayer.h"

#include "CARGameSetting.h"

namespace CAR
{
	void PlayerBase::setupSetting(GameSetting& setting)
	{
		mSetting = &setting;
		mFieldValues.resize( setting.getFieldNum() , 0 );
	}

	int PlayerBase::getFieldValue(FieldType::Enum type)
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == -1 )
			return 0;
		return mFieldValues[idx];
	}

	void PlayerBase::setFieldValue(FieldType::Enum type , int value)
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == -1 )
			return;
		mFieldValues[idx] = value;
	}

	int PlayerBase::modifyFieldValue(FieldType::Enum type , int value /*= 1 */)
	{
		int idx = mSetting->getFieldIndex( type );
		if ( idx == -1 )
			return 0;
		return mFieldValues[ idx ] += value;
	}

}//namespace CAR