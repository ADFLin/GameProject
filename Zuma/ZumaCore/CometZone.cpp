#include "CometZone.h"

#include "CometRandom.h"

#include <cmath>

namespace Comet
{
	IRandom* getDefaultRandom()
	{
		return NULL;
	}

	RectZone::RectZone( CoordType const& min , CoordType const& max , IRandom* random ) 
		:minRect( min ) 
		,maxRect(max)
		,mRandom( random )
	{
		if ( !mRandom )
		{
			mRandom = getDefaultRandom();
		}
	}


	SphereZone::SphereZone( float min , float max , IRandom* random ) 
		:minRadius( min ) , maxRadius(max)
		,mRandom( random )
	{
		center = CoordType::Zero();

		if ( !mRandom )
		{
			mRandom = getDefaultRandom();
		}
	}

	CoordType SphereZone::calcPos()
	{
		float r = mRandom->getRangeReal( minRadius , maxRadius );
		float theta = mRandom->getRangeReal( 0 , 2 *PI );

		return CoordType( r*cos(theta), r*sin(theta) );
	}

}//namespace Comet

