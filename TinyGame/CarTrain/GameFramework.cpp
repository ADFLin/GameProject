#include "GameFramework.h"

#define  int8 b2Int8
#include "Box2D/Box2D.h"
#undef   int8

#include "RHI/RHIGraphics2D.h"

namespace CarTrain
{

	struct Box2DConv
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

	class Box2DDraw : public b2Draw
	{
	public:
		Box2DDraw(RHIGraphics2D& g)
			:mGraphic(&g)
		{
			SetFlags(e_shapeBit);
		}

		void setGraphics(RHIGraphics2D& g)
		{
			mGraphic = &g;
		}

		void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
		{
			mGraphic->enableBrush(false);
			mGraphic->setPen(Box2DConv::To(color).rgb());
			drawPolygonInternal(vertices, vertexCount);
		}


		void drawPolygonInternal(const b2Vec2* vertices, int32 vertexCount)
		{
			Vector2 v[b2_maxPolygonVertices];
			assert(vertexCount <= ARRAY_SIZE(v));
			for (int i = 0; i < vertexCount; ++i)
			{
				v[i] = Box2DConv::To(vertices[i]);
			}
			mGraphic->drawPolygon(v, vertexCount);

		}

		void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
		{
			mGraphic->setPen(Color3f(0, 0, 0));
			mGraphic->setBrush(Box2DConv::To(color).rgb());
			drawPolygonInternal(vertices, vertexCount);
		}


		void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override
		{
			mGraphic->drawCircle(Box2DConv::To(center), radius);
		}


		void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override
		{
			mGraphic->setPen(Color3f(0, 0, 0));
			mGraphic->setBrush(Box2DConv::To(color).rgb());
			mGraphic->drawCircle(Box2DConv::To(center), radius);
		}


		void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override
		{
			mGraphic->drawLine(Box2DConv::To(p1), Box2DConv::To(p2));
		}


		void DrawTransform(const b2Transform& xf) override
		{

		}

		void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override
		{

		}


		RHIGraphics2D* mGraphic;
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
			return 0.0f;
		}

		virtual XForm2D getTransform() const override
		{
			return Box2DConv::To(mBody->GetTransform());
		}

		virtual void setTransform(XForm2D const& xform)
		{
			mBody->SetTransform(Box2DConv::To(xform.getPos()), xform.getRotateAngle());
		}

		virtual PhyObjectDef* getDefine() override
		{
			return mPhyDef.get();
		}
		virtual Vector2 getLinearVel() const override
		{
			return Box2DConv::To(mBody->GetLinearVelocity());
		}

		virtual float   getAngle() const override
		{
			return mBody->GetAngle();
		}

		virtual float getAngularVel() const override
		{
			return mBody->GetAngularVelocity();
		}

		virtual void setLinearVel(Vector2 const& vel) override
		{
			mBody->SetLinearVelocity(Box2DConv::To(vel));
		}

		virtual void    setRotation(float angle) override
		{
			mBody->SetTransform(mBody->GetPosition(), angle);
		}
		virtual float   getRotation() const
		{
			return mBody->GetAngle();
		}
		virtual void*   getUserData()
		{
			return mBody->GetUserData();
		}
		virtual void    setUserData(void* data)
		{
			mBody->SetUserData(data);
		}

		std::unique_ptr< PhyObjectDef > mPhyDef;
		b2Body* mBody;
	};

	class PhysicsSceneBox2D : public IPhysicsScene
						    , public b2ContactFilter
		                    , public b2ContactListener
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

			mWorld->SetContactListener(this);
			mWorld->SetContactFilter(this);
			return true;
		}

		void release() override
		{
			delete this;
		}

		void tick(float deltaTime) override
		{
			mWorld->Step(deltaTime, 250, 50);
		}


		PhyBodyBox2D* createShapeObject(PhyObjectDef const& def, b2Shape const& shape, XForm2D xForm, float density)
		{
			PhyBodyBox2D* result = new PhyBodyBox2D;
			b2BodyDef bodyDef;
			if (def.bEnablePhysics)
			{
				bodyDef.type = b2_dynamicBody;
			}
			else if (def.bCollisionDetection && def.bCollisionResponse)
			{
				bodyDef.type = b2_kinematicBody;
			}
			else
			{
				bodyDef.type = b2_staticBody;
			}

			bodyDef.position = Box2DConv::To(xForm.getPos());
			bodyDef.angle = xForm.getRotateAngle();
			result->mBody = mWorld->CreateBody(&bodyDef);

			b2FixtureDef   fixtureDef;
			fixtureDef.filter.categoryBits = BIT(def.collisionType);
			fixtureDef.filter.maskBits = def.collisionMask;
			fixtureDef.density = density;
			fixtureDef.restitution = def.restitution;
			fixtureDef.friction = def.friction;
			fixtureDef.shape = &shape;
			fixtureDef.isSensor = !def.bCollisionResponse;
			fixtureDef.userData = result;
			result->mBody->CreateFixture(&fixtureDef);

			return result;
		}

		IPhysicsBody*  createBox(BoxObjectDef const& def, XForm2D xForm) override
		{
			b2PolygonShape shape;
			shape.SetAsBox(def.extend.x / 2, def.extend.y / 2);
			PhyBodyBox2D* result = createShapeObject(def, shape, xForm, def.getDensity());
			result->mPhyDef = std::make_unique< BoxObjectDef >(def);
			return result;
		}

		IPhysicsBody* createCircle(CircleObjectDef const& def, XForm2D xForm) override
		{
			b2CircleShape shape;
			shape.m_radius = def.radius;
			PhyBodyBox2D* result = createShapeObject(def, shape, xForm, def.getDensity());
			result->mPhyDef = std::make_unique< CircleObjectDef >(def);
			return result;
		}
		virtual void setGravity(Vector2 const& g) override
		{
			mWorld->SetGravity(Box2DConv::To(g));
		}

		bool rayCast(Vector2 const& startPos, Vector2 const& endPos, RayHitInfo& outInfo, uint16 collisionMask) override
		{
			struct MyCallback : b2RayCastCallback
			{
				MyCallback(RayHitInfo& outInfo, uint16 collisionMask)
					:outInfo(outInfo), collisionMask(collisionMask)
				{
				}
				bool bHitted = false;
				float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction) override
				{
					PhyBodyBox2D* body = (PhyBodyBox2D*)fixture->GetUserData();
					if (collisionMask & BIT(body->mPhyDef->collisionType))
					{
						bHitted = true;
						if (fraction < outInfo.fraction)
						{
							outInfo.hitPos = Box2DConv::To(point);
							outInfo.fraction = fraction;
							outInfo.normal = Box2DConv::To(normal);
						}
						return 1;
					}

					return -1;

				}
				uint16     collisionMask;
				RayHitInfo& outInfo;
			};

			outInfo.fraction = 1.0f;
			MyCallback callback(outInfo, collisionMask);
			mWorld->RayCast(&callback, Box2DConv::To(startPos), Box2DConv::To(endPos));

			return callback.bHitted;
		}

		void setupDebug(RHIGraphics2D& g) override
		{
			if (mDebugDraw)
			{
				static_cast<Box2DDraw*>(mDebugDraw.get())->setGraphics(g);
			}
			else
			{
				mDebugDraw = std::make_unique< Box2DDraw >(g);
				mWorld->SetDebugDraw(mDebugDraw.get());
			}
		}

		void drawDebug() override
		{
			mWorld->DrawDebugData();
		}

		std::unique_ptr<b2Draw> mDebugDraw;
		std::unique_ptr<b2World> mWorld;

		//b2ContactFilter
		bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB) override
		{
			PhyBodyBox2D* bodyA = (PhyBodyBox2D*)fixtureA->GetUserData();
			PhyBodyBox2D* bodyB = (PhyBodyBox2D*)fixtureB->GetUserData();
			PhyObjectDef* defA = bodyA->mPhyDef.get();
			PhyObjectDef* defB = bodyB->mPhyDef.get();
			return (defA->collisionMask & BIT(defB->collisionType)) &&
				   (defB->collisionMask & BIT(defA->collisionType));
		}
		//~b2ContactFilter

		//b2ContactListener
		void BeginContact(b2Contact* contact) override
		{
			PhyBodyBox2D* bodyA = (PhyBodyBox2D*)contact->GetFixtureA()->GetUserData();
			PhyBodyBox2D* bodyB = (PhyBodyBox2D*)contact->GetFixtureB()->GetUserData();

			PhyObjectDef* defA = bodyA->mPhyDef.get();
			PhyObjectDef* defB = bodyB->mPhyDef.get();
			if (defA->bCollisionDetection && bodyA->onCollision)
				bodyA->onCollision(bodyB);
			if (defB->bCollisionDetection && bodyB->onCollision)
				bodyB->onCollision(bodyA);
		}

		void EndContact(b2Contact* contact) override { }

		void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
		{

		}

		void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override
		{

		}
		//~b2ContactListener



	};



	IPhysicsScene* IPhysicsScene::Create()
	{
		return new PhysicsSceneBox2D();
	}


	bool GameWorld::initialize()
	{
		VERIFY_RETURN_FALSE(mPhyScene = IPhysicsScene::Create());
		VERIFY_RETURN_FALSE(mPhyScene->initialize());
		return true;
	}

	void GameWorld::clearnup()
	{
		if (mPhyScene)
		{
			mPhyScene->release();
			mPhyScene = nullptr;
		}
	}

	void GameWorld::tick(float deltaTime)
	{
		updateControl();

		for (auto entity : mEntities)
		{
			entity->tick(deltaTime);
		}

		mPhyScene->tick(deltaTime);

		for (auto entity : mEntities)
		{
			entity->postTick(deltaTime);
		}

		if (!mPendingDestoryEntities.empty())
		{

			for (auto entity : mPendingDestoryEntities)
			{
				destroyEntity(entity);
			}
			mPendingDestoryEntities.clear();
		}
	}

	void GameEntity::destroyEntity()
	{
		if (getWorld())
		{
			getWorld()->markEntityDestroy(this);
		}
		else
		{
			delete this;
		}
	}


}//namespace CarTrain