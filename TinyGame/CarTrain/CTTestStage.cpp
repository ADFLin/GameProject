#include "CTTestStage.h"
#include "Serialize/FileStream.h"
#include "FileSystem.h"

namespace CarTrain
{

	REGISTER_STAGE_ENTRY("Car Train", TestStage, EExecGroup::Dev, "AI");


	TConsoleVariable<bool> CVarShowDebugDraw{ true, "CT.ShowDebugDraw", CVF_TOGGLEABLE };
	TConsoleVariable<int>  CVarFrameTickFactor{ 1, "CT.FrameTickFactor", CVF_TOGGLEABLE };
#define CARTRAIN_DIR "CarTrain"
	class TestLevelData : public LevelData
	{
	public:
		TestLevelData()
		{
			Vector2 RectSize = Vector2(760, 560);
			Vector2 CenterPos = 0.5 * Vector2(::Global::GetScreenSize());

			spawnPoint = XForm2D(Vector2(420, 470), Math::DegToRad(135));

			GameBoxDef def;
			def.bCollisionDetection = false;
			def.bCollisionResponse = true;
			def.collisionType = ECollision::Wall;
			{
				def.extend = Vector2(RectSize.x - 20, 20);
				def.transform = XForm2D(CenterPos - Vector2(0, RectSize.y / 2));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(RectSize.x - 20, 20);
				def.transform = XForm2D(CenterPos + Vector2(0, RectSize.y / 2));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(20, RectSize.y + 20);
				def.transform = XForm2D(CenterPos - Vector2(RectSize.x / 2, 0));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, RectSize.y + 20);
				def.transform = XForm2D(CenterPos + Vector2(RectSize.x / 2, 0));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(400, 200);
				def.transform = XForm2D(CenterPos);
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(100, 20);
				def.transform = XForm2D( Vector2( 60 , 200 ));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(100, 20);
				def.transform = XForm2D(Vector2(160, 300));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(100, 20);
				def.transform = XForm2D(Vector2( 60,  400));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 120);
				def.transform = XForm2D(Vector2(300, 80));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 170);
				def.transform = XForm2D(Vector2(400, 190));
				boxObjects.push_back(def);
			}


			{
				def.extend = Vector2(20, 120);
				def.transform = XForm2D(Vector2(500, 80));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(20, 170);
				def.transform = XForm2D(Vector2(600, 150));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(130, 20);
				def.transform = XForm2D(Vector2(670, 70));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(60, 20);
				def.transform = XForm2D(Vector2(750, 155));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(130, 20);
				def.transform = XForm2D(Vector2(670, 240));
				boxObjects.push_back(def);
			}



			{
				def.extend = Vector2(20, 200);
				def.transform = XForm2D(Vector2(160, 450), Math::DegToRad(45));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 200);
				def.transform = XForm2D(Vector2(220, 520), Math::DegToRad(45));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 200);
				def.transform = XForm2D(Vector2(400, 450), Math::DegToRad(45));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 200);
				def.transform = XForm2D(Vector2(500, 450), -Math::DegToRad(45));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 200);
				def.transform = XForm2D(Vector2(400, 600), Math::DegToRad(45));
				boxObjects.push_back(def);
			}
			{
				def.extend = Vector2(20, 200);
				def.transform = XForm2D(Vector2(500, 600), -Math::DegToRad(45));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(50, 50);
				def.transform = XForm2D(Vector2(700, 150), Math::DegToRad(45));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(50, 50);
				def.transform = XForm2D(Vector2(700, 350), Math::DegToRad(45));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(50, 50);
				def.transform = XForm2D(Vector2(700, 500), Math::DegToRad(45));
				boxObjects.push_back(def);
			}

			{
				def.extend = Vector2(50, 50);
				def.transform = XForm2D(Vector2(60, 160), Math::DegToRad(45));
				boxObjects.push_back(def);
			}


			{
				def.extend = Vector2(50, 50);
				def.transform = XForm2D(Vector2(160, 60), Math::DegToRad(45));
				boxObjects.push_back(def);
			}
		}
	};

	void CarEntity::tick(float deltaTime)
	{
		XForm2D& XForm = getComponentCheckedT< XForm2D >();
		if (!bDead)
		{
			if (turnAngle != 0)
			{
				float angle = mBody->getRotation();
				angle += turnAngle;
				mBody->setRotation(angle);
				turnAngle = 0;
			}

		
			XForm = mBody->getTransform();
			Vector2 moveDir = XForm.transformVector(Vector2(1, 0));
			mBody->setLinearVel(moveSpeed * moveDir);
		}
		else
		{
			XForm = mBody->getTransform();
		}
	}


	void AgentCarEntiy::drawDetector(RHIGraphics2D& g)
	{
		XForm2D& XForm = getComponentCheckedT< XForm2D >();

		TRANSFORM_PUSH_SCOPE(g.getTransformStack(), [&]()
		{
			g.translateXForm(XForm.getPos().x, XForm.getPos().y);
			g.rotateXForm(XForm.getRotateAngle());
		});

		for (int i = 0; i < NumDetectors; ++i)
		{
			RenderUtility::SetPen(g, mDetectors[i].bHitted ? EColor::Red : EColor::Yellow);
			g.drawLine(Vector2::Zero(), MaxDetectDistance * mDetectors[i].fraction * GetNormal(DetectorLocalDirs[i]));
		}

	}

	bool TestStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		VERIFY_RETURN_FALSE(mWorld.initialize());

		mLevelData = std::make_unique<TestLevelData>();
		mLevelData->setup(mWorld);


		mNNLayout.init(AgentCarEntiy::Topology, ARRAY_SIZE(AgentCarEntiy::Topology));

		mTrainSettings.netLayout = &mNNLayout;
		mTrainSettings.initWeightSeed = GenerateRandSeed();
		mTrainSettings.numAgents = 200;
		mTrainSettings.numTrainDataSelect = 25;
		mTrainSettings.numPoolDataSelectUsed = 30;
		mTrainSettings.mutationValueDelta = 0.1;
		mTrainSettings.mutationValueProb = 0.8;
		mTrainSettings.mutationGeneProb = 0.5;
		mGenePool.maxPoolNum = 100;

		mTrainData.init(mTrainSettings);
		mTrainData.bestAgent = mTrainData.mAgents[0].get();

		mWorld.spawnAgents(mTrainData, mLevelData->spawnPoint);


		auto& console = IConsoleSystem::Get();
#define REGISTER_COM( NAME , FUNC )\
	console.registerCommand(NAME, &TestStage::FUNC, this);

		REGISTER_COM("CT.RandomizeAgents", randomizeAgentsCmd);
		REGISTER_COM("CT.SaveTrainPool", saveTrainPool);
		REGISTER_COM("CT.LoadTrainPool", loadTrainPool);
		REGISTER_COM("CT.SpawnPoint", setSpawnPoint);

		::Global::GUI().cleanupWidget();
		restart();
		return true;
	}

	void TestStage::saveTrainPool(char const* name)
	{
		if (!FFileSystem::IsExist(CARTRAIN_DIR))
		{
			FFileSystem::CreateDirectorySequence(CARTRAIN_DIR);
		}

		OutputFileSerializer serializer;
		//serializer.registerVersion(EName::None, ELevelSaveVersion::LastVersion);

		if (serializer.open(InlineString<>::Make(CARTRAIN_DIR"/%s.data", name)))
		{
			TrainManager::OutputData(mNNLayout, mGenePool, serializer);
			LogMsg("Save Train Pool %s Success", name);
		}
		else
		{
			LogMsg("Save Train Pool %s Fail", name);
		}
	}

	void TestStage::loadTrainPool(char const* name)
	{
		InputFileSerializer serializer;
		if (serializer.open(InlineString<>::Make(CARTRAIN_DIR"/%s.data", name)))
		{
			TrainManager::IntputData(mNNLayout, mGenePool, serializer);
			LogMsg("Load Train Pool  %s Success", name);
		}
		else
		{
			LogMsg("Load Train Pool  %s Fail", name);
		}
	}

	void TestStage::tick()
	{
		mWorld.tick(GDeltaTime);

		mWorld.checkAliveCars();
		if (mWorld.mAliveCars.size() > 0)
		{
			mTrainData.findBestAgnet();
		}
		else
		{
			mTrainData.runEvolution(&mGenePool);
			restart();
		}
	}

	void TestStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);

		int frame = long(deltaTime) / gDefaultTickTime;
		int tickCount = CVarFrameTickFactor * frame;
		for (int i = 0; i < tickCount; ++i)
			tick();

	}

	void TestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		using namespace Render;

		Vec2i screenSize = ::Global::GetScreenSize();
		RHICommandList& commandList = g.getCommandList();

		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.8, 0.8, 0.8, 0), 1);

		g.beginRender();

		{
			TRANSFORM_PUSH_SCOPE(g.getTransformStack(), mWorldToScreen, true);

			if (CVarShowDebugDraw)
			{
				mWorld.getPhysicsScene()->setupDebug(g);
				mWorld.drawDebug();
			}

			for (auto entity : mWorld.mEntities)
			{
				AgentCarEntiy* car = entity->cast<AgentCarEntiy>();
				if (car && !car->bDead)
				{
					car->drawDetector(g);
				}
			}

			RenderUtility::SetPen(g, mbRayCastHitted ? EColor::Red : EColor::Yellow);
			g.drawLine(mRayCastStart, mRayCastEnd);
			if (mbRayCastHitted)
			{
				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, EColor::White);
				g.drawRect(mRayCastHit - Vector2(1, 1), Vector2(2, 2));
			}
		}
		SimpleTextLayout textLayout;
		textLayout.posX = 100;
		textLayout.posY = 100;
		textLayout.offset = 15;
		textLayout.show(g, "Generation = %d , MaxFitness = %.2f" , mTrainData.generation , mTrainData.bestAgent->genotype->fitness);
		g.endRender();
	}


}