#include "MVObject.h"

namespace MV
{
	void NavNode::connect(NavNode& other)
	{
		assert( link == NULL && other.link == NULL );
		link = &other;
		other.link = this;
		if( getSurface()->block->group == other.getSurface()->block->group )
		{
			flag |= eStatic;
			other.flag|= eStatic;
		}
	}

}//namespace MV
