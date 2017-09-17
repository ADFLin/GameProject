#include "StageRegister.h"

#include "TestStageHeader.h"

#include "Cantan/CantanStage.h"
#include "MonumentValley/MVStage.h"
#include "TripleTown/TTStage.h"
#include "Bejeweled/BJStage.h"
#include "Shoot2D/FlightGame.h"
#include "Go/GoStage.h"

#include "LightingStage.h"
#include "MiscTestStage.h"
#include "Phy2DStage.h"
#include "BloxorzStage.h"
#include "AStarStage.h"
#include "FBirdStage.h"
#include "RenderGL/RenderGLStage.h"
#include "GGJStage.h"
#include "RubiksStage.h"

#define STAGE_INFO( DECL , CLASS , GROUP )\
	{ DECL , makeStageFactory< CLASS >() , GROUP } 

StageInfo gPreRegisterStageGroup[] =
{
	STAGE_INFO("Misc Test" , MiscTestStage , StageRegisterGroup::GraphicsTest) ,
	STAGE_INFO("Cantan Test" , Cantan::LevelStage , StageRegisterGroup::GraphicsTest) ,
	STAGE_INFO("GGJ Test" , GGJ::TestStage , StageRegisterGroup::GraphicsTest) ,
	STAGE_INFO("2D Lighting Test"     , Lighting::TestStage , StageRegisterGroup::GraphicsTest) ,
	STAGE_INFO("Shader Test"  , RenderGL::SampleStage , StageRegisterGroup::GraphicsTest),
	STAGE_INFO("GLGraphics2D Test"   , GLGraphics2DTestStage , StageRegisterGroup::GraphicsTest) ,
	STAGE_INFO("B-Spline Test"   , BSplineTestStage , StageRegisterGroup::GraphicsTest) ,

	STAGE_INFO("Bsp Test"    , Bsp2D::TestStage , StageRegisterGroup::Test) ,
	STAGE_INFO("A-Star Test" , AStar::TestStage , StageRegisterGroup::Test) ,
	STAGE_INFO("SAT Col Test" , G2D::TestStage , StageRegisterGroup::Test) ,

	STAGE_INFO("QHull Test"   , G2D::QHullTestStage , StageRegisterGroup::Test) ,
	STAGE_INFO("RB Tree Test"   , TreeTestStage , StageRegisterGroup::Test)  ,
	STAGE_INFO("Tween Test"  , TweenTestStage , StageRegisterGroup::Test) ,

#if 1
	STAGE_INFO("TileMap Test"  , TileMapTestStage , StageRegisterGroup::Test),
	STAGE_INFO("Corontine Test" , CoroutineTestStage , StageRegisterGroup::Test) ,
	STAGE_INFO("XML Prase Test" , XMLPraseTestStage , StageRegisterGroup::Test) ,
#endif

	STAGE_INFO("GJK Col Test" , Phy2D::CollideTestStage , StageRegisterGroup::PhyDev) ,
	STAGE_INFO("RigidBody Test" , Phy2D::WorldTestStage , StageRegisterGroup::PhyDev) ,

	STAGE_INFO("Monument Valley"   , MV::TestStage , StageRegisterGroup::Dev) ,
	STAGE_INFO("Triple Town Test"  , TripleTown::LevelStage , StageRegisterGroup::Dev) ,
	STAGE_INFO("Bejeweled Test"    , Bejeweled::TestStage , StageRegisterGroup::Dev) ,
	STAGE_INFO("Bloxorz Test"      , Bloxorz::TestStage , StageRegisterGroup::Dev),
	STAGE_INFO("FlappyBird Test"   , FlappyBird::TestStage , StageRegisterGroup::Dev) ,

	STAGE_INFO("Rubiks Test"       , Rubiks::TestStage , StageRegisterGroup::Dev4) ,
	STAGE_INFO("Mario Test"        , Mario::TestStage , StageRegisterGroup::Dev4) ,
	STAGE_INFO("Shoot2D Test"      , Shoot2D::TestStage , StageRegisterGroup::Dev4) ,
	STAGE_INFO("Go Test"           , Go::Stage , StageRegisterGroup::Dev4) ,
};

#undef INFO

void RegisterStageGlobal()
{
	static bool gbNeedRegisterStage = true;

	if( gbNeedRegisterStage )
	{
		for( auto& info : gPreRegisterStageGroup )
		{
			gStageRegisterCollection.registerStage(info);
		}
		gbNeedRegisterStage = false;
	}
}