#include "GameModule.h"

class TestRenderModule : public IModuleInterface
{
public:
	virtual void startupModule() override
	{

	}
	virtual void shutdownModule() override
	{
		
	}
};


EXPORT_MODULE(TestRenderModule);