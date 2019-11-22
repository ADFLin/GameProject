#include "AQBot.h"
#include "GTPOutput.h"

namespace Go
{
	char const* AQAppRun::InstallDir = nullptr;

	bool AQAppRun::buildPlayGame()
	{
		FixString<256> path;
		path.format("%s/%s", InstallDir, "AQ.exe");
		FixString<512> command;
		return buildProcessT< GTPOutputThread >(path, command);
	}

	bool AQBot::initialize(void* settingData)
	{
		if (!mAI.buildPlayGame())
			return false;

		static_cast<GTPOutputThread*>(mAI.outputThread)->bLogMsg = false;
		return true;
	}

}//namespace Go

