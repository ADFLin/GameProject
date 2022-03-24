#include "GameModule.h"

class TestWorkingModule : public IModuleInterface
{
public:
	void startupModule() override
	{

	}
	void shutdownModule() override
	{
		
	}
};


EXPORT_MODULE(TestWorkingModule);