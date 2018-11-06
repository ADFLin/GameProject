#include "StageRegister.h"

#include "TestStageHeader.h"

#include "Cantan/CantanStage.h"
#include "MonumentValley/MVStage.h"
#include "TripleTown/TTStage.h"
#include "Bejeweled/BJStage.h"
#include "Shoot2D/FlightGame.h"
#include "Go/GoStage.h"

#include "LightingStage2D.h"
#include "MiscTestStage.h"
#include "Phy2DStage.h"
#include "BloxorzStage.h"
#include "AStarStage.h"
#include "FlappyBird/FBStage.h"
#include "GGJStage.h"
#include "RubiksStage.h"
#include "RenderGL/RenderGLStage.h"

#define STAGE_INFO( DECL , CLASS , ... )\
	{ DECL , makeStageFactory< CLASS >() , __VA_ARGS__ } 


StageInfo gPreRegisterStageGroup[] =
{
	STAGE_INFO("Misc Test" , MiscTestStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("Cantan Test" , Cantan::LevelStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("GGJ Test" , GGJ::TestStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("2D Lighting Test"     , Lighting2D::TestStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("GLGraphics2D Test"   , GLGraphics2DTestStage , EStageGroup::GraphicsTest) ,
	STAGE_INFO("B-Spline Test"   , BSplineTestStage , EStageGroup::GraphicsTest) ,

	STAGE_INFO("Bsp Test"    , Bsp2D::TestStage , EStageGroup::Test) ,
	STAGE_INFO("A-Star Test" , AStar::TestStage , EStageGroup::Test) ,
	STAGE_INFO("SAT Col Test" , G2D::TestStage , EStageGroup::Test) ,

	STAGE_INFO("QHull Test"   , G2D::QHullTestStage , EStageGroup::Test) ,
	STAGE_INFO("RB Tree Test"   , TreeTestStage , EStageGroup::Test)  ,
	STAGE_INFO("Tween Test"  , TweenTestStage , EStageGroup::Test) ,

#if 1
	STAGE_INFO("TileMap Test"  , TileMapTestStage , EStageGroup::Test),
	STAGE_INFO("Corontine Test" , CoroutineTestStage , EStageGroup::Test) ,
	STAGE_INFO("XML Prase Test" , XMLPraseTestStage , EStageGroup::Test) ,
#endif

	STAGE_INFO("GJK Col Test" , Phy2D::CollideTestStage , EStageGroup::PhyDev) ,
	STAGE_INFO("RigidBody Test" , Phy2D::WorldTestStage , EStageGroup::PhyDev) ,

	STAGE_INFO("Monument Valley"   , MV::TestStage , EStageGroup::Dev) ,
	STAGE_INFO("Triple Town Test"  , TripleTown::LevelStage , EStageGroup::Dev) ,
	STAGE_INFO("Bejeweled Test"    , Bejeweled::TestStage , EStageGroup::Dev) ,
	STAGE_INFO("Bloxorz Test"      , Bloxorz::TestStage , EStageGroup::Dev , 2 ),
	STAGE_INFO("FlappyBird Test"   , FlappyBird::LevelStage , EStageGroup::Dev , 2) ,

	STAGE_INFO("Rubiks Test"       , Rubiks::TestStage , EStageGroup::Dev4) ,
	STAGE_INFO("Mario Test"        , Mario::TestStage , EStageGroup::Dev4) ,
	STAGE_INFO("Shoot2D Test"      , Shoot2D::TestStage , EStageGroup::Dev4) ,
	STAGE_INFO("Go Test"           , Go::Stage , EStageGroup::Dev4) ,

	STAGE_INFO("Shader Test"  , Render::SampleStage , EStageGroup::FeatureDev,10),
};

#undef INFO

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