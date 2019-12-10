#include "GameModule.h"

#include "Cantan/CantanStage.h"
#include "MonumentValley/MVStage.h"
#include "Bejeweled/BJStage.h"
#include "Shoot2D/FlightGame.h"
#include "Mario/MBStage.h"

#define STAGE_INFO( DECL , CLASS , ... )\
	{ DECL , makeStageFactory< CLASS >() , __VA_ARGS__ } 


StageInfo gPreRegisterStageGroup[] =
{

	STAGE_INFO("Monument Valley"   , MV::TestStage , EStageGroup::Dev) ,
	STAGE_INFO("Bejeweled Test"    , Bejeweled::TestStage , EStageGroup::Dev) ,

	STAGE_INFO("Mario Test"        , Mario::TestStage , EStageGroup::Dev4) ,
	STAGE_INFO("Shoot2D Test"      , Shoot2D::TestStage , EStageGroup::Dev4) ,

};

class TestMiscModule : public IModuleInterface
{
public:
	bool initialize() override
	{
		static bool gbNeedRegisterStage = true;
		if( gbNeedRegisterStage )
		{
			for( auto& info : gPreRegisterStageGroup )
			{
				StageRegisterCollection::Get().registerStage(info);
			}
			gbNeedRegisterStage = false;
		}
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


EXPORT_MODULE(TestMiscModule);