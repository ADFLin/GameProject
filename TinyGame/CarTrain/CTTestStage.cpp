#include "Stage/TestStageHeader.h"

#include "Math/Math2D.h"

#include "Collision2D/SATSolver.h"



#include "MarcoCommon.h"
#include "RHI/RHIGraphics2D.h"

#define  int8 b2Int8
#include "Box2D/Box2D.h"
#include "GameRenderSetup.h"
#undef   int8

#if _DEBUG
#pragma comment( lib , "Box2Dd.lib" )
#else
#pragma comment( lib , "Box2D.lib" )
#endif // _DEBUG

namespace CarTrain
{
	using Math::XForm2D;
	using Math::Rotation2D;
	using Math::Vector2;

	namespace ECollision
	{
		enum Type : uint8
		{
			Wall,
			Car,
		};
	}

	enum class EPhysicsBodyType
	{
		Kinematic ,
		Static ,
		Dynamic ,
	};

	struct PhyObjectDef
	{
		EPhysicsBodyType bodyType;
		ECollision::Type collisionType;
		uint32  collisionMask;
		float   mass;

		PhyObjectDef()
		{
			bodyType = EPhysicsBodyType::Static;
			collisionType = ECollision::Wall;
			collisionMask = 0xffffffff;
		}

		template< class OP >
		void serialize(OP& op)
		{
			op & bodyType & collisionType & collisionMask & mass;
		}
	};

	struct BoxObjectDef : PhyObjectDef
	{
		Vector2 extend;

		template< class OP >
		void serialize(OP& op)
		{
			PhyObjectDef::serialize(op);
			op & extend;
		}
	};

	struct RayHitInfo
	{
		float   fraction;
		Vector2 normal;
		Vector2 hitPos;
	};

	class IPhysicsBody
	{
	public:
		virtual float getMass() = 0;
		virtual PhyObjectDef* getDefine() = 0;
		virtual XForm2D getTransform() const = 0;
		virtual Vector2 getLinearVel() const = 0;
		virtual float   getAngularVel() const = 0;
	};

	class IPhysicsScene
	{
	public:
		virtual bool initialize() = 0;
		virtual void release() = 0;
		virtual void addBox(BoxObjectDef const& def, XForm2D xForm) = 0;

		virtual bool rayCast(Vector2 const& startPos, Vector2 const& endPos, RayHitInfo& outInfo) = 0;

		virtual void setupDebugView(RHIGraphics2D& g) = 0;
		virtual void drawDebug() = 0;

		static IPhysicsScene* Create();
	};

	struct B2Conv
	{
		static b2Vec2   To(Vector2 const& v) { return b2Vec2(v.x, v.y); }
		static Vector2  To(b2Vec2 const& v) { return Vector2(v.x, v.y); }
		static XForm2D  To(b2Transform const& xForm)
		{
			XForm2D result;
			result.setTranslation(To(xForm.p));
			result.setRoatation(xForm.q.GetAngle());
			return result;
		}
		static Color4f  To(b2Color const& c) { return Color4f(c.r, c.g, c.b, c.a); }
	};

	class HitProxy
	{

	};


	class HitProxyManager
	{




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

		void drawRect(Vector2 const& pos , Vector2 const& size , HitProxy* proxy = nullptr)
		{
			DrawElement element;
			element.type = DrawElement::Rect;
			element.pos = pos;
			element.size = size;
			element.proxy = proxy;
			element.xform = mXFormStack.get();
			mElements.push_back(element);
		}
		void drawCircle( Vector2 const& pos , float radius, HitProxy* proxy = nullptr)
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

	class Box2DDraw : public b2Draw
	{
	public:
		Box2DDraw(RHIGraphics2D& g)
			:g(g)
		{
			SetFlags(e_shapeBit);
		}

		void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
		{
			g.enableBrush(false);
			g.setPen(B2Conv::To(color).rgb());
			drawPolygonInternal(vertices, vertexCount);
		}


		void drawPolygonInternal(const b2Vec2* vertices, int32 vertexCount)
		{
			Vector2 v[b2_maxPolygonVertices];
			assert(vertexCount <= ARRAY_SIZE(v));
			for (int i = 0; i < vertexCount; ++i)
			{
				v[i] = B2Conv::To(vertices[i]);
			}
			g.drawPolygon(v, vertexCount);

		}

		void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
		{
			g.enableBrush(true);
			g.setPen(Color3f(0,0,0));
			g.setBrush( B2Conv::To(color).rgb() );
			drawPolygonInternal(vertices, vertexCount);
		}


		void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override
		{
			g.drawCircle(B2Conv::To(center), radius);
		}


		void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override
		{
			g.drawCircle(B2Conv::To(center), radius);
		}


		void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override
		{
			g.drawLine(B2Conv::To(p1), B2Conv::To(p2));
		}


		void DrawTransform(const b2Transform& xf) override
		{

		}

		void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override
		{

		}

		RHIGraphics2D& g;

	};


	class PhyBodyBox2D : public IPhysicsBody
	{
	public:
		void initialize(b2Body* body, PhyObjectDef& def)
		{
			mBody = body;
		}
		virtual float getMass()
		{


		}

		virtual XForm2D getTransform() const override
		{
			return B2Conv::To( mBody->GetTransform() );

		}
		virtual PhyObjectDef* getDefine() override
		{

		}
		virtual Vector2 getLinearVel() const override
		{
			return B2Conv::To(mBody->GetLinearVelocity());
		}

		virtual float getAngularVel() const override
		{
			return mBody->GetAngularVelocity();
		}

		std::unique_ptr< PhyObjectDef >mPhyDef;
		b2Body* mBody;
	};
	class PhysicsSceneBox2D : public IPhysicsScene
	{
	public:

		PhysicsSceneBox2D()
		{

		}

		~PhysicsSceneBox2D()
		{

		}

		bool initialize() override
		{
			mWorld = std::make_unique< b2World >(b2Vec2(0, 0));
			return true;
		}

		void release() override
		{
			delete this;
		}

		void addBox(BoxObjectDef const& def, XForm2D xForm) override
		{
			b2BodyDef bodyDef;
			bodyDef.type = b2_staticBody;
			bodyDef.position = B2Conv::To(xForm.getPos());
			bodyDef.angle = xForm.getRotateAngle();
			auto body = mWorld->CreateBody(&bodyDef);
			b2PolygonShape shape;
			shape.SetAsBox(def.extend.x / 2, def.extend.y / 2);
			b2FixtureDef   fixtureDef;
			//fixtureDef.density = 1.0;
			fixtureDef.shape = &shape;
			body->CreateFixture(&fixtureDef);

		}

		bool rayCast(Vector2 const& startPos, Vector2 const& endPos, RayHitInfo& outInfo) override
		{
			struct MyCallback : b2RayCastCallback
			{
				MyCallback(RayHitInfo& outInfo):outInfo(outInfo){}
				bool bHitted = false;
				float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction) override
				{
					bHitted = true;
					outInfo.hitPos = B2Conv::To(point);
					outInfo.fraction = fraction;
					outInfo.normal = B2Conv::To(normal);
					return 0;
				}

				RayHitInfo& outInfo;
			};
			MyCallback callback(outInfo);
			mWorld->RayCast( &callback , B2Conv::To(startPos), B2Conv::To(endPos));

			return callback.bHitted;
		}

		void setupDebugView(RHIGraphics2D& g) override
		{
			mDebugDraw = std::make_unique< Box2DDraw >(g);
			mWorld->SetDebugDraw(mDebugDraw.get());
		}

		void drawDebug() override
		{
			mWorld->DrawDebugData();
		}

		std::unique_ptr<b2Draw> mDebugDraw;
		std::unique_ptr<b2World> mWorld;
	};



	IPhysicsScene* IPhysicsScene::Create()
	{
		return new PhysicsSceneBox2D();
	}

	class GameWorld
	{
	public:
		bool initialize()
		{
			VERIFY_RETURN_FALSE(mPhyScene = IPhysicsScene::Create());
			VERIFY_RETURN_FALSE(mPhyScene->initialize());
			return true;
		}
		void clearnup()
		{
			if (mPhyScene)
			{
				mPhyScene->release();
				mPhyScene = nullptr;
			}
		}

		void drawDebug()
		{
			mPhyScene->drawDebug();
		}

		IPhysicsScene* getPhysicsScene() { return mPhyScene; }
		IPhysicsScene* mPhyScene = nullptr;
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
		void setup(GameWorld& world )
		{
			for (auto const& def : mBoxObjects)
			{
				world.getPhysicsScene()->addBox(def , def.transform);
			}



		}


		template< class OP >
		void serialize( OP op )
		{
			op & mBoxObjects;
		}
		std::vector< GameBoxDef > mBoxObjects;
	};

	enum class EAxisControl
	{
		X ,
		Y ,
		XY ,
	};

	class GHitBox : public GWidget
	{
	public:
		bool onMouseMsg(MouseMsg const& msg) override
		{
			return false;
		}
	};

	class Car
	{
	public:
		static int constexpr NumDetectors = 3;
		static float constexpr MaxDetectDistance = 200.0f;
		static Vector2 constexpr detectorLocalDirs[NumDetectors] =
		{
			Vector2(1,1),
			Vector2(1,0),
			Vector2(1,-1),
		};


		void updateDetectors( GameWorld& world , float deltaTime)
		{
			for(int i = 0 ; i < NumDetectors ; ++i )
			{
				Detector& detector = mDetectors[i];
				Vector2 dir = mTransform.transformVector(detectorLocalDirs[i]);
				RayHitInfo info;
				detector.bHitted = world.getPhysicsScene()->rayCast(mTransform.getPos(), mTransform.getPos() + MaxDetectDistance * dir, info);
				if (detector.bHitted)
				{
					detector.fraction = info.fraction;
				}
				else
				{
					detector.fraction = 1.0f;
				}
			}
		}

		struct Detector
		{
			float   fraction;
			bool    bHitted;
		};


		Detector mDetectors[NumDetectors];
		XForm2D  mTransform;
	};

	class IEditMode
	{
	public:
		virtual void render() {}
		virtual void tick() {}
		virtual bool onMouse(MouseMsg const& msg) { return true; }
	};

	class GameEntity
	{
	public:




		IPhysicsBody* body;
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		GameWorld mWorld;
		std::unique_ptr< LevelData > mLevelData;
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			VERIFY_RETURN_FALSE(mWorld.initialize());

			mWorld.getPhysicsScene()->setupDebugView(Global::GetRHIGraphics2D());

			mLevelData = std::make_unique<LevelData>();
			{
				GameBoxDef def;
				def.extend = Vector2(20, 5);
				def.transform = XForm2D(Vector2(200, 100));
				mLevelData->mBoxObjects.push_back(def);
			}

			{
				GameBoxDef def;
				def.extend = Vector2(20, 5);
				def.transform = XForm2D(Vector2(100, 100));
				mLevelData->mBoxObjects.push_back(def);
			}

			mLevelData->setup(mWorld);
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

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

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

			g.beginRender();

			for(;;)
			{





			}

			if (bDrawPhyDebug)
			{
				mWorld.drawDebug();
			}

			RenderUtility::SetPen(g, mbRayCastHitted ? EColor::Red : EColor::Yellow);
			g.drawLine(mRayCastStart, mRayCastEnd);
			if (mbRayCastHitted)
			{
				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, EColor::White);
				g.drawRect(mRayCastHit - Vector2(1, 1), Vector2(2, 2));
			}

			g.endRender();
		}


		Vector2 mRayCastStart = Vector2(0,0);
		Vector2 mRayCastEnd = Vector2(100,100);
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

			if ( msg.isDown() )
			{	
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
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
		bool bDrawPhyDebug = false;
		IEditMode* mEditMode;
	};


	REGISTER_STAGE_ENTRY("Car Train", TestStage, EExecGroup::Dev);
}