#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"

#include "GameFramework.h"

#include "MarcoCommon.h"
#include "AI/TrainManager.h"

#include "ConsoleSystem.h"
#include "Collision2D/SATSolver.h"
#include "RHI/RHIGraphics2D.h"
#include "InputManager.h"
#include "Math/GeometryPrimitive.h"

namespace CarTrain
{
	using namespace AI;
	using namespace Render;

	float GDeltaTime = gDefaultTickTime / 1000.0f;

	struct HitProxy
	{
	public:
		virtual void draw() = 0;
	};

	class HitProxyRenderProgram : public GlobalShaderProgram
	{



	};

	class HitProxyManager
	{



		struct ProxyData
		{
			Color3ub color;
			uint32 id;
			HitProxy* Proxy;
		};

		void initializeRHI(IntVector2 const& screenSize)
		{
			mTexture = RHICreateTexture2D(ETexture::RGBA8, screenSize.x, screenSize.y, 0, 1, TCF_AllowCPUAccess | TCF_RenderTarget);
			mFrameBuffer = RHICreateFrameBuffer();
			mFrameBuffer->addTexture(*mTexture);
		}

		void releaseRHI()
		{
			mTexture.release();
			mFrameBuffer.release();
		}


		RHITexture2DRef   mTexture;
		RHIFrameBufferRef mFrameBuffer;
	};


	class Renderer
	{
	public:

		struct DrawElement
		{
			enum EType
			{
				Rect,
				Circle,
			};
			EType     type;
			Render::RenderTransform2D xform;
			Vector2   pos;
			HitProxy* proxy;
			union
			{
				Vector2 size;
				float radius;
			};
		};

		void drawRect(Vector2 const& pos, Vector2 const& size, HitProxy* proxy = nullptr)
		{
			DrawElement element;
			element.type = DrawElement::Rect;
			element.pos = pos;
			element.size = size;
			element.proxy = proxy;
			element.xform = mXFormStack.get();
			mElements.push_back(element);
		}
		void drawCircle(Vector2 const& pos, float radius, HitProxy* proxy = nullptr)
		{
			DrawElement element;
			element.type = DrawElement::Circle;
			element.pos = pos;
			element.radius = radius;
			element.proxy = proxy;
			element.xform = mXFormStack.get();
			mElements.push_back(element);
		}

		Render::TransformStack2D& getXFormStack() { return mXFormStack; }
		Render::TransformStack2D mXFormStack;
		std::vector<DrawElement> mElements;
	};


	struct GameBoxDef : BoxObjectDef
	{
		typedef BoxObjectDef BaseClass;
		XForm2D transform;
		template< class OP >
		void serialize(OP& op)
		{
			BaseClass::serialize(op);
			op & transform;
		}
	};

	class LevelData
	{
	public:
		void setup(GameWorld& world)
		{
			for (auto const& def : boxObjects)
			{
				world.getPhysicsScene()->createBox(def, def.transform);
			}
		}


		template< class OP >
		void serialize(OP& op)
		{
			op & mBoxObjects;
		}

		XForm2D spawnPoint;
		std::vector< GameBoxDef > boxObjects;
	};


	enum class EAxisControl
	{
		X,
		Y,
		XY,
	};


	class ISceneProxy
	{
	public:
		virtual void render(RHIGraphics2D& g) = 0;
	};


	class CarEntity : public GameEntity
		//, public ISceneProxy
	{
	public:
		DECLARE_GAME_ENTITY(CarEntity, GameEntity);

		CarEntity(XForm2D const& transform)
			:mTransform(transform)
		{

		}

		virtual void beginPlay()
		{
			GameBoxDef def;
			def.bCollisionResponse = false;
			def.bCollisionDetection = true;
			def.collisionType = ECollision::Car;
			def.collisionMask = BIT(ECollision::Wall);
			def.extend = Vector2(20, 10);
			def.transform = mTransform;
			mBody = getWorld()->getPhysicsScene()->createBox(def, def.transform);
			mBody->setLinearVel(Vector2(1, 0));
			mBody->setUserData(this);
			mBody->onCollision = [this](IPhysicsBody*)
			{
				die();
			};
		}


		void die()
		{
			bDead = true;
			bActive = false;
			mBody->setLinearVel(Vector2(0, 0));
		}

		void tick(float deltaTime);

		bool  bDead = false;
		float turnAngle = 0;
		float moveSpeed = 350;

		XForm2D  mTransform;
		IPhysicsBody* mBody;
	};



	class CarAgentEntiy : public CarEntity
		                , public AgentEntity
					    , public IEntityController
	{
	public:
		DECLARE_GAME_ENTITY(CarAgentEntiy,  CarEntity);

		CarAgentEntiy(XForm2D const& transform)
			:BaseClass(transform)
		{

		}

		void release()
		{
			destroyEntity();
		}
		TrainAgent* mAgent;

		void beginPlay() override
		{
			BaseClass::beginPlay();
			getWorld()->mControlList.push_back({this , this});
			updateDetectors();
		}

		void restart()
		{
			bDead = false;
			bActive = true;
			mAgent->genotype->fitness = 0;
			stayTime = 0;
			box.invalidate();
		}

		float stayTime = 0.0f;
		using BoundBox = Math::TAABBox< Vector2 >;
		BoundBox box;


		struct Detector
		{
			float   fraction = 0.0;
			float   fractionDelta = 0.0;
			bool    bHitted;
		};

		static int constexpr NumDetectors = 3;
		static float constexpr MaxDetectDistance = 300.0f;
		static Vector2 constexpr DetectorLocalDirs[] =
		{
			Vector2(1,1),
			Vector2(1,0),
			Vector2(1,-1),
			Vector2(0,1),
			Vector2(0,-1),
		};
		Detector mDetectors[NumDetectors];

		constexpr static float MaxRotateAngle = 4;
		void postTick(float deltaTime) override
		{
			BaseClass::postTick(deltaTime);
			if (!bDead)
			{
				updateDetectors();
				mAgent->genotype->fitness += deltaTime;

				box.addPoint(mTransform.getPos());
				
				Vector2 size = box.getSize();
				float radius2 = 2 * moveSpeed / ( Math::Deg2Rad(MaxRotateAngle) / GDeltaTime );
				if ( size.x < 1.2 * radius2 && size.y < 1.2 * radius2 && size.x * size.y < radius2 * radius2)
				{
					stayTime += deltaTime;
					if (stayTime > 4)
					{
						die();
					}
				}
				else
				{
					box.invalidate();
					stayTime = 0;
				}
			}
		}

		void updateDetectors()
		{
			for (int i = 0; i < NumDetectors; ++i)
			{
				Detector& detector = mDetectors[i];
				Vector2 dir = mTransform.transformVector(DetectorLocalDirs[i]);
				dir.normalize();

				RayHitInfo info;
				uint16 collisionMask = BIT(ECollision::Wall);

				detector.bHitted = getWorld()->getPhysicsScene()->rayCast(mTransform.getPos(), mTransform.getPos() + MaxDetectDistance * dir, info, collisionMask);
				if (detector.bHitted)
				{
					detector.fractionDelta = info.fraction - detector.fraction;
					detector.fraction = info.fraction;
				}
				else
				{
					detector.fraction = 1.0f;
				}
			}
		}

		void drawDetector(RHIGraphics2D& g);


#if 0
		static constexpr uint32 Topology[] =
		{
			10, 9 , 8 , 7 , 6 , 5 , 4, 3, 2, 1
		};
#else
		static constexpr uint32 Topology[] =
		{
			6 , 8 , 8 , 5 , 4, 3, 2, 1
		};

#endif

		void updateInput(GameEntity& entity) override
		{
			NNScalar inputs[2 * NumDetectors];

			for (int i = 0; i < NumDetectors; ++i)
			{
				inputs[2 * i ] = float(mDetectors[i].bHitted ? mDetectors[i].fraction : 1.0);
				inputs[2 * i + 1] = float(mDetectors[i].fractionDelta);
			}

			NNScalar output;
			mAgent->FNN.calcForwardFeedback(inputs, &output);
			if ( output > 0.6)
			{
				turnAngle = -Math::Deg2Rad(MaxRotateAngle * Math::Clamp<float>( (output - 0.6 ) / 0.4 , 0 , 1 ) );
			}
			else if (output < 0.4)
			{
				turnAngle = Math::Deg2Rad(MaxRotateAngle * Math::Clamp<float>(( output ) / 0.4, 0 , 1 ) );
			}
		}

	};


	class TrainWorld : public GameWorld
	{
	public:





	};


	class AgentGameWorld : public AgentWorld

	{
	public:


		virtual void release()
		{
			delete this;
		}
		virtual void setup(TrainData& trainData)
		{
			SpawnAgents(mWorld, trainData, startXForm);
		}
		virtual void restart(TrainData& trainData)
		{
			RestartAgents(trainData, startXForm);
		}

		virtual void tick(TrainData& trainData)
		{
			mWorld.tick(GDeltaTime);
			if (CheckTrainEnd(trainData))
			{
				onTrainCompleted();
			}

		}

		static bool CheckTrainEnd(TrainData& trainData)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				CarAgentEntiy* car = static_cast<CarAgentEntiy*>(agentPtr->entity);
				if (!car->bDead)
				{
					return false;
				}
			}
			return true;
		}

		static void SpawnAgents(GameWorld& world , TrainData& trainData, XForm2D const& startXForm)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				if (agentPtr->entity == nullptr)
				{
					CarAgentEntiy* entity = world.spawnEntity<CarAgentEntiy>(startXForm);
					entity->mAgent = agentPtr.get();
					agentPtr->entity = entity;
					
				}
			}
		}

		static void RestartAgents(TrainData& trainData, XForm2D const& startXForm)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				CarAgentEntiy* car = static_cast<CarAgentEntiy*>(agentPtr->entity);
				car->restart();
				
				if (car->mAgent->index % 2 || 1)
				{
					car->mBody->setTransform(startXForm);
					car->mTransform = startXForm;
				}
				else
				{
					XForm2D  xForm = startXForm;
					xForm.rotate(Math::Deg2Rad(90));
					car->mBody->setTransform(xForm);
					car->mTransform = xForm;
				}
			}
		}


		XForm2D   startXForm;
		GameWorld mWorld;
	};


	class IEditMode
	{
	public:
		virtual void render() {}
		virtual void tick() {}
		virtual bool onMouse(MouseMsg const& msg) { return true; }
	};



	class TestStage : public StageBase
					, public IGameRenderSetup
					, public IEntityController
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}



		GameWorld mWorld;
		std::unique_ptr< LevelData > mLevelData;

		FCNNLayout mNNLayout;
		TrainData mTrainData;
		GenePool  mGenePool;
		TrainDataSetting mTrainSettings;
		bool onInit() override;


		virtual void updateInput(GameEntity& entity) override
		{
			auto& car = static_cast<CarEntity&>(entity);
			if (InputManager::Get().isKeyDown(EKeyCode::A))
			{
				car.turnAngle = -Math::Deg2Rad(2);
			}
			else if (InputManager::Get().isKeyDown(EKeyCode::D))
			{
				car.turnAngle = Math::Deg2Rad(2);
			}
		}

		void randomizeAgentsCmd()
		{
			mTrainData.generation = 0;
			mTrainData.randomizeData();
			mGenePool.clear();
			restart();
		}

		void saveTrainPool(char const* name);
		void loadTrainPool(char const* name);

		void onInitFail() override
		{
			mWorld.clearnup();
			BaseClass::onInitFail();
		}

		void onEnd() override
		{
			mWorld.clearnup();
			BaseClass::onEnd();
		}

		void restart() 
		{
			AgentGameWorld::RestartAgents(mTrainData, mLevelData->spawnPoint);
		}

		CarEntity* mCar = nullptr;
		void tick();
		void updateFrame(int frame) {}

		void onUpdate(long time) override;


		RenderTransform2D mWorldToScreen = RenderTransform2D::Identity();

		void onRender(float dFrame) override;


		Vector2 mRayCastStart = Vector2(0, 0);
		Vector2 mRayCastEnd = Vector2(100, 100);
		Vector2 mRayCastHit;
		bool    mbRayCastHitted = false;

		bool onMouse(MouseMsg const& msg) override
		{
			if (bEditMode && mEditMode)
			{
				if (!mEditMode->onMouse(msg))
					return false;
			}

			if (msg.onLeftDown() || msg.onRightDown())
			{
				if (msg.onLeftDown())
				{
					mRayCastStart = msg.getPos();
				}
				else
				{
					mRayCastEnd = msg.getPos();
				}

				RayHitInfo info;
				mbRayCastHitted = mWorld.getPhysicsScene()->rayCast(mRayCastStart, mRayCastEnd, info);
				if (mbRayCastHitted)
				{
					mRayCastHit = info.hitPos;
				}
			}

			return BaseClass::onMouse(msg);
		}

		bool onKey(KeyMsg const& msg) override
		{

			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::E:
					mTrainData.runEvolution(&mGenePool);
					restart();
					break;
				case EKeyCode::P:
					{
						if (mCar == nullptr)
						{
							mCar = mWorld.spawnEntity< CarEntity >(mLevelData->spawnPoint);
							assert(mCar->cast<CarEntity>());
							mWorld.mControlList.push_back({ this, mCar });
						}
						mCar->mBody->setTransform(mLevelData->spawnPoint);
						mCar->bDead = false;
						mCar->bActive = true;
					}
					break;

				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		bool bEditMode = false;
		IEditMode* mEditMode;
	};
}//namespace CarTrain