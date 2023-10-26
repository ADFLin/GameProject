#include "StageRegister.h"

#include "DrawEngine.h"
#include "TestStageHeader.h"

#include "Go/GoStage.h"

#include "MiscTestStage.h"
#include "Phy2DStage.h"


#define STAGE_INFO( DECL , CLASS , ... )\
	{ DECL , MakeChangeStageOperation< CLASS >() , __VA_ARGS__ } 

ExecutionEntryInfo GPreRegisterStageGroup[] =
{
	STAGE_INFO("Misc Test" , MiscTestStage , EExecGroup::Main) ,
	//STAGE_INFO("Cantan Test" , Cantan::LevelStage , EExecGroup::GraphicsTest) ,
	STAGE_INFO("RHIGraphics2D Test"   , RHIGraphics2DTestStage , EExecGroup::GraphicsTest) ,
	STAGE_INFO("B-Spline Test"   , BSplineTestStage , EExecGroup::GraphicsTest) ,

	STAGE_INFO("Bsp Test"       , Bsp2D::TestStage , EExecGroup::Test, "Algorithm") ,


	STAGE_INFO("RB Tree Test"   , TreeTestStage , EExecGroup::Test)  ,
	STAGE_INFO("Tween Test"     , TweenTestStage , EExecGroup::Test) ,

#if 1
	STAGE_INFO("TileMap Test"   , TileMapTestStage , EExecGroup::Test),
	STAGE_INFO("XML Prase Test" , XMLPraseTestStage , EExecGroup::Test) ,
#endif

	STAGE_INFO("GJK Col Test"   , Phy2D::CollideTestStage , EExecGroup::PhyDev) ,
	STAGE_INFO("RigidBody Test" , Phy2D::WorldTestStage , EExecGroup::PhyDev) ,

	STAGE_INFO("Go Test"        , Go::Stage , EExecGroup::Dev4) ,

};

#undef STAGE_INFO

void RegisterStageGlobal()
{
	static bool gbNeedRegisterStage = true;

	if( gbNeedRegisterStage )
	{
		for( auto& info : GPreRegisterStageGroup )
		{
			ExecutionRegisterCollection::Get().registerExecution(info);
		}
		gbNeedRegisterStage = false;
	}
}