#ifndef rcControl_h__
#define rcControl_h__

#include "rcBase.h"
class rcLevelCity;

class rcControl
{
public:
	enum ActionMode
	{
		eBuild  ,
		eClear  ,
	};
	rcControl();

	void setLevelCity( rcLevelCity* city ){ mLevelCity = city; }
	void changeBuildTag( unsigned tag );
	void action( Vec2i const& mapPos );
	void swapBuildingAxis( bool beS ){ mSwapAxis = beS; }
	void setMode( ActionMode mode ){ mMode = mode; }

	void render()
	{



	}

private:
	ActionMode    mMode;
	rcLevelCity*  mLevelCity;
	bool          mSwapAxis;
	unsigned      mChioceBuildingTag;
	unsigned      mIdxModel;
};
#endif // rcControl_h__