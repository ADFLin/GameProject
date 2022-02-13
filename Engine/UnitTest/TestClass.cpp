#include "UnitTest.h"

namespace UnitTest
{

	Component*& getNode()
	{
		static Component* gNode = 0;
		return gNode;
	}

#if CORE_SHARE_CODE
	Component::Component()
	{
		mNext = getNode();
		getNode() = this;
	}

	bool Component::RunTest()
	{
		Component* node = getNode();

		while( node )
		{
			switch( node->run() )
			{
			case RR_SUCCESS:
				break;
			default:
				return false;
			}
			node = node->mNext;
		}
		return true;
	}
#endif //CORE_SHARE_CODE


}//namespace UnitTest