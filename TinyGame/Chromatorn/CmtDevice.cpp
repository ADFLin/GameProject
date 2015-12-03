#include "CmtPCH.h"
#include "CmtDevice.h"

namespace Chromatron
{
	Device::Device( DeviceInfo& info , Dir dir, Color color )
		:mDir(dir)
		,mColor(color)
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


	void Device::changeDir( Dir dir , bool beForce )
	{
		if ( !isRotatable() )
			return;

		if( !isStatic() || beForce ) 
			mDir = dir;
	}

	void Device::rotate(Dir dir  , bool beForce )
	{
		if ( !isRotatable() )
			return;

		if( !isStatic() || beForce ) 
			mDir += dir;
	}

}//namespace Chromatron
