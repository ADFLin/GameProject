#include "rcPCH.h"
#include "rcControl.h"

#include "rcLevelCity.h"
#include "rcMapLayerTag.h"

rcControl::rcControl()
{
	mMode = eBuild;
	mChioceBuildingTag = BT_ERROR_TAG;
	mSwapAxis = false;
	mIdxModel = 0;
}

void rcControl::changeBuildTag( unsigned tag )
{
	mMode = eBuild;
	mChioceBuildingTag = tag;
	mIdxModel = 0;
}

void rcControl::action( Vec2i const& mapPos )
{
	switch( mMode )
	{
	case eBuild:
		if ( mChioceBuildingTag != BT_ERROR_TAG )
		{
			mLevelCity->buildContruction( mapPos , mChioceBuildingTag ,  mIdxModel , mSwapAxis );
		}
		break;
	case eClear:
		mLevelCity->destoryContruction( mapPos );
		break;
	}
}
