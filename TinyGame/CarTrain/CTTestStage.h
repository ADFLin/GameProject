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

	struct HitProxyId
	{




		uint32 value;
	};

	struct HitProxy
	{
	public:
		virtual HitProxyId getId() = 0;
	};

	class HitProxyRenderProgram : public GlobalShaderProgram
	{



	};

	class HitProxyManager
	{



		struct ProxyData
		{
			Color4ub  color;
			uint32    id;
			HitProxy* Proxy;


			static uint32 ToId(Color4ub const& color)
			{
				return color.toRGBA();
			}
			static Color4ub ToColor(uint32 id)
			{
				return Color4ub::FromRGBA(id);
			}
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
			op & spawnPoint;
			op & boxObjects;
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

		CarEntity() {}

		void initialize(XForm2D const& transform)
		{
			XForm2D* xForm = addComponentT<XForm2D>();
			*xForm = transform;
		}

		virtual void beginPlay()
		{
			XForm2D& XForm = getComponentCheckedT<XForm2D>();

			GameBoxDef def;
			def.bCollisionResponse = false;
			def.bCollisionDetection = true;
			def.collisionType = ECollision::Car;
			def.collisionMask = BIT(ECollision::Wall);
			def.extend = Vector2(20, 10);
			def.transform = XForm;
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

		IPhysicsBody* mBody;
	};


	class Physics2DSystem : public ECS::ISystemSerivce
	{



	};


	class AgentCarEntiy : public CarEntity
		                , public AgentEntity
					    , public IEntityController
	{
	public:
		DECLARE_GAME_ENTITY(AgentCarEntiy,  CarEntity);

		AgentCarEntiy()
		{

		}

		void initialize(XForm2D const& transform)
		{
			CarEntity::initialize(transform);
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
			XForm2D* xForm = addComponentT<XForm2D>();

			BaseClass::postTick(deltaTime);
			if (!bDead)
			{
				updateDetectors();
				mAgent->genotype->fitness += deltaTime;

				box.addPoint(xForm->getPos());
				
				Vector2 size = box.getSize();
				float radius2 = 2 * moveSpeed / ( Math::DegToRad(MaxRotateAngle) / GDeltaTime );
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
			XForm2D& XForm = getComponentCheckedT< XForm2D >();

			for (int i = 0; i < NumDetectors; ++i)
			{
				Detector& detector = mDetectors[i];
				Vector2 dir = XForm.transformVector(DetectorLocalDirs[i]);
				dir.normalize();

				RayHitInfo info;
				uint16 collisionMask = BIT(ECollision::Wall);

				detector.bHitted = getWorld()->getPhysicsScene()->rayCast(XForm.getPos(), XForm.getPos() + MaxDetectDistance * dir, info, collisionMask);
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
			6 , 5 , 4, 3, 2, 1
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
				turnAngle = -Math::DegToRad(MaxRotateAngle * Math::Clamp<float>( (output - 0.6 ) / 0.4 , 0 , 1 ) );
			}
			else if (output < 0.4)
			{
				turnAngle = Math::DegToRad(MaxRotateAngle * Math::Clamp<float>(( output ) / 0.4, 0 , 1 ) );
			}
		}

	};


	class TrainWorld : public GameWorld
	{
	public:

		TrainWorld()
		{
			mEntitiesManager.registerToList();
			mEntitiesManager.registerComponentTypeT< XForm2D >();
		}

		void checkAliveCars()
		{
			int num = mAliveCars.size();
			for (int i = 0; i < num;)
			{
				if ( mAliveCars[i]->bDead )
				{
					RemoveIndexSwap(mAliveCars, i);
					--num;
				}
				else
				{
					++i;
				}
			}
		}

		void spawnAgents(TrainData& trainData, XForm2D const& startXForm)
		{
			for (auto& agentPtr : trainData.mAgents)
			{
				if (agentPtr->entity == nullptr)
				{
					AgentCarEntiy* entity = spawnEntity<AgentCarEntiy>(startXForm);
					entity->mAgent = agentPtr.get();
					agentPtr->entity = entity;
				}
			}
		}

		void restartAgents(TrainData& trainData, XForm2D const& startXForm)
		{
			mAliveCars.clear();
			for (auto& agentPtr : trainData.mAgents)
			{
				AgentCarEntiy* car = static_cast<AgentCarEntiy*>(agentPtr->entity);
				car->restart();

				if (car->mAgent->index % 2 || 1)
				{
					car->mBody->setTransform(startXForm);
					car->getComponentCheckedT<XForm2D>() = startXForm;
				}
				else
				{
					XForm2D  xForm = startXForm;
					xForm.rotate(Math::DegToRad(90));
					car->mBody->setTransform(xForm);
					car->getComponentCheckedT<XForm2D>() = xForm;
				}

				mAliveCars.push_back(car);
			}
		}

		std::vector< AgentCarEntiy* > mAliveCars;
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
			mWorld.spawnAgents(trainData, startXForm);
		}
		virtual void restart(TrainData& trainData)
		{
			mWorld.restartAgents(trainData, startXForm);
		}

		virtual void tick(TrainData& trainData)
		{
			mWorld.tick(GDeltaTime);
			if (mWorld.mAliveCars.size() == 0)
			{
				onTrainCompleted();
			}
		}



		XForm2D    startXForm;
		TrainWorld mWorld;
	};


	class IEditMode
	{
	public:
		virtual void render() {}
		virtual void tick() {}
		virtual MsgReply onMouse(MouseMsg const& msg) { return MsgReply::Unhandled(); }
	};



	class TestStage : public StageBase
					, public IGameRenderSetup
					, public IEntityController
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}



		TrainWorld mWorld;
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
				car.turnAngle = -Math::DegToRad(2);
			}
			else if (InputManager::Get().isKeyDown(EKeyCode::D))
			{
				car.turnAngle = Math::DegToRad(2);
			}
		}

		void randomizeAgentsCmd()
		{
			mTrainData.generation = 0;
			mTrainData.randomizeData();
			mGenePool.clear();
			restart();
		}

		void setSpawnPoint(Vector2 const& pos, float angle)
		{
			mLevelData->spawnPoint.setTranslation(pos);
			mLevelData->spawnPoint.setRoatation(Math::DegToRad(angle));
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
			mWorld.restartAgents(mTrainData, mLevelData->spawnPoint);
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

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (bEditMode && mEditMode)
			{
				MsgReply reply = mEditMode->onMouse(msg);
				if (reply.isHandled())
					return reply;
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

		MsgReply onKey(KeyMsg const& msg) override
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