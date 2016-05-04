#include "CmtPCH.h"
#include "CmtDevice.h"

namespace Chromatron
{
	Device::Device( DeviceInfo& info , Dir dir, Color color )
		:mDir(dir)
		,mColor(color)
		,mPos(0,0)
		,mUserData(nullptr)
	{
		changeType( info );
	};

	void Device::changeType( DeviceInfo& info )
	{
		mInfo = &info;
		mFlag.setValue( info.flag );

		if ( !isRotatable() )
			mDir = Dir::ValueNoCheck( 0 );

		if ( ( getFlag() & DFB_USE_LOCKED_COLOR ) == 0 )
			setColor( COLOR_NULL );
	}

	void Device::setStatic( bool beS )
	{
		if ( beS )
		{
			mFlag.addBits( DFB_STATIC );
		}
		else
		{
			mFlag.removeBits( DFB_STATIC );
		}
	}


	bool Device::changeDir( Dir dir , bool beForce )
	{
		if ( !isRotatable() )
			return false;

		if( isStatic() && beForce == false )
			return false;

		mDir = dir;
		return true;
	}

	bool Device::rotate(Dir dir  , bool beForce )
	{
		if ( !isRotatable() )
			return false;

		if (isStatic() && beForce == false)
			return false;

		mDir += dir;
		return true;
	}

}//namespace Chromatron
