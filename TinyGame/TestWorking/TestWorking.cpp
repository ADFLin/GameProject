#include "GameModule.h"

class TestWorkingModule : public IModuleInterface
{
public:
	bool initialize() override
	{
		return true;
	}
	void cleanup() override
	{
		
	}
	bool isGameModule() const override
	{
		return false;
	}

	void deleteThis() override
	{
		delete this;
	}
};


EXPORT_MODULE(TestWorkingModule);