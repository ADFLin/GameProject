#include "CmtPCH.h"
#include "CmtLightTrace.h"

#include <algorithm>

namespace Chromatron
{
	LightTrace::LightTrace( Vec2i const& start, Color color , Dir dir,int param , int age ) 
		:mStartPos(start)
		,mEndPos(start)
		,mColor(color)
		,mDir(dir)
		,mParam(param)
		,mAge( age )
	{
		assert( 0 <= mColor && mColor <= COLOR_W );
	}

	const Vec2i gDirOffset[ NumDir ]=
	{ 
		Vec2i(1,0),Vec2i(1,-1),Vec2i(0,-1),Vec2i(-1,-1),
		Vec2i(-1,0),Vec2i(-1,1),Vec2i(0,1),Vec2i(1,1) 
	};


	Vec2i LightTrace::GetDirOffset( Dir dir )
	{
		return gDirOffset[ dir ];
	}

}//namespace Chromatron
