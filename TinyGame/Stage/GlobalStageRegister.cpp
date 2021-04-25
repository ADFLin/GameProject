#include "StageRegister.h"

#include "DrawEngine.h"
#include "TestStageHeader.h"

#include "Go/GoStage.h"

#include "MiscTestStage.h"
#include "Phy2DStage.h"
#include "AStarStage.h"


#define STAGE_INFO( DECL , CLASS , ... )\
	{ DECL , MakeChangeStageOperation< CLASS >() , __VA_ARGS__ } 

StageInfo gPreRegisterStageGroup[] =
{
	STAGE_INFO("Misc Test" , MiscTestStage , EStageGroup::Main) ,
	//STAGE_INFO("Cantan Test" , Cantan::LevelStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("GLGraphics2D Test"   , GLGraphics2DTestStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("B-Spline Test"   , BSplineTestStage , EStageGroup::GraphicsTest) ,

	STAGE_INFO("Bsp Test"       , Bsp2D::TestStage , EStageGroup::Test, "Algorithm") ,
	STAGE_INFO("A-Star Test"    , AStar::TestStage , EStageGroup::Test, "AI") ,

	STAGE_INFO("RB Tree Test"   , TreeTestStage , EStageGroup::Test)  ,
	STAGE_INFO("Tween Test"     , TweenTestStage , EStageGroup::Test) ,

#if 1
	STAGE_INFO("TileMap Test"   , TileMapTestStage , EStageGroup::Test),
	STAGE_INFO("Corontine Test" , CoroutineTestStage , EStageGroup::Test) ,
	STAGE_INFO("XML Prase Test" , XMLPraseTestStage , EStageGroup::Test) ,
#endif

	STAGE_INFO("GJK Col Test"   , Phy2D::CollideTestStage , EStageGroup::PhyDev) ,
	STAGE_INFO("RigidBody Test" , Phy2D::WorldTestStage , EStageGroup::PhyDev) ,

	STAGE_INFO("Go Test"        , Go::Stage , EStageGroup::Dev4) ,

};

#undef STAGE_INFO

void RegisterStageGlobal()
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
}