#include "GameModule.h"

#include "Cantan/CantanStage.h"
#include "MonumentValley/MVStage.h"
#include "Bejeweled/BJStage.h"
#include "Shoot2D/FlightGame.h"
#include "Mario/MBStage.h"

#define STAGE_INFO( DECL , CLASS , ... )\
	{ DECL , MakeChangeStageOperation< CLASS >() , __VA_ARGS__ } 


ExecutionEntryInfo GPreRegisterStageGroup[] =
{
	STAGE_INFO("Monument Valley"   , MV::TestStage , EExecGroup::Dev, "Game") ,
	STAGE_INFO("Bejeweled Test"    , Bejeweled::TestStage , EExecGroup::Dev, "Game") ,
	STAGE_INFO("Mario Test"        , Mario::TestStage , EExecGroup::Dev4, "Game") ,
	STAGE_INFO("Shoot2D Test"      , Shoot2D::TestStage , EExecGroup::Dev4, "Game") ,
};

class TestMiscModule : public IModuleInterface
{
public:
	void startupModule() override
	{
		static bool GbNeedRegisterStage = true;
		CHECK(GbNeedRegisterStage);
		{
			GbNeedRegisterStage = false;
			for( auto const& info : GPreRegisterStageGroup )
			{
				ExecutionRegisterCollection::Get().registerExecution(info);
			}
		}
	}

	void shutdownModule() override
	{

	}
};


EXPORT_MODULE(TestMiscModule);