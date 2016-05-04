#include "CmtPCH.h"
#include "CmtLightTrace.h"

#include <algorithm>

namespace Chromatron
{
	LightTrace::LightTrace( Vec2D const& start, Color color , Dir dir,int param , int age ) 
		:mStartPos(start)
		,mEndPos(start)
		,mColor(color)
		,mDir(dir)
		,mParam(param)
		,mAge( age )
	{
		assert( 0 <= mColor && mColor <= COLOR_W );
	}

	const Vec2D gDirOffset[ NumDir ]=
	{ 
		Vec2D(1,0),Vec2D(1,-1),Vec2D(0,-1),Vec2D(-1,-1),
		Vec2D(-1,0),Vec2D(-1,1),Vec2D(0,1),Vec2D(1,1) 
	};


	Vec2D LightTrace::GetDirOffset( Dir dir )
	{
		return gDirOffset[ dir ];
	}

}//namespace Chromatron
