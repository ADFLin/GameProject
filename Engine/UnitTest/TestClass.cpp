#include "UnitTest.h"

namespace UnitTest
{

	Component*& getNode()
	{
		static Component* gNode = 0;
		return gNode;
	}

#if CORE_SHARE_CODE
	Component::Component(const char* name)
		:mName(name)
	{
		mNext = getNode();
		getNode() = this;
	}

	bool Component::RunTest()
	{
		Component* node = getNode();
		int numSuccess = 0;
		int numFail = 0;

		LogMsg("--------------------------------------------------");
		LogMsg("Unit Test Start");
		LogMsg("--------------------------------------------------");

		while( node )
		{
			LogMsg("Running Test: %s", node->getName());
			
			// We might want to wrap this in try-catch if exceptions are used
			switch( node->run() )
			{
			case RR_SUCCESS:
				numSuccess++;
				break;
			default:
				numFail++;
				LogMsg("!! FAILED !!");
				break;
			}
			node = node->mNext;
		}

		LogMsg("--------------------------------------------------");
		LogMsg("Unit Test Summary: %d Passed, %d Failed", numSuccess, numFail);
		LogMsg("--------------------------------------------------");

		return numFail == 0;
	}
#endif //CORE_SHARE_CODE


}//namespace UnitTest