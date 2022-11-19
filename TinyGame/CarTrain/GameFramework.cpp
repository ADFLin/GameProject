#include "GameFramework.h"

#define  int8 b2Int8
#include "Box2D/Box2D.h"
#undef   int8

#include "RHI/RHIGraphics2D.h"

namespace CarTrain
{

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
			g.setPen(Color3f(0, 0, 0));
			g.setBrush(B2Conv::To(color).rgb());
			drawPolygonInternal(vertices, vertexCount);
		}


		void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override
		{
			g.drawCircle(B2Conv::To(center), radius);
		}


		void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override
		{
			g.setPen(Color3f(0, 0, 0));
			g.setBrush(B2Conv::To(color).rgb());
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
			return 0.0f;
		}

		virtual XForm2D getTransform() const override
		{
			return B2Conv::To(mBody->GetTransform());
		}

		virtual void setTransform(XForm2D const& xform)
		{
			mBody->SetTransform(B2Conv::To(xform.getPos()), xform.getRotateAngle());
		}

		virtual PhyObjectDef* getDefine() override
		{
			return mPhyDef.get();
		}
		virtual Vector2 getLinearVel() const override
		{
			return B2Conv::To(mBody->GetLinearVelocity());
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
			mBody->SetLinearVelocity(B2Conv::To(vel));
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


		void createBoxFixture(b2Body* body, BoxObjectDef const& def, void* userData)
		{
			b2PolygonShape shape;
			shape.SetAsBox(def.extend.x / 2, def.extend.y / 2);
			b2FixtureDef   fixtureDef;
			fixtureDef.filter.categoryBits = BIT(def.collisionType);
			fixtureDef.filter.maskBits = def.collisionMask;
			//fixtureDef.density = 1.0;
			fixtureDef.shape = &shape;
			fixtureDef.isSensor = !def.bCollisionResponse;
			fixtureDef.userData = userData;
			body->CreateFixture(&fixtureDef);
		}
		IPhysicsBody*  createBox(BoxObjectDef const& def, XForm2D xForm) override
		{
			PhyBodyBox2D* result = new PhyBodyBox2D;

			b2BodyDef bodyDef;
			if (def.bEnablePhysics || def.bCollisionDetection)
			{
				bodyDef.type = b2_dynamicBody;
			}
			else
			{
				bodyDef.type = b2_staticBody;
			}

			bodyDef.position = B2Conv::To(xForm.getPos());
			bodyDef.angle = xForm.getRotateAngle();
			result->mBody = mWorld->CreateBody(&bodyDef);
			result->mPhyDef = std::make_unique< BoxObjectDef >(def);
			createBoxFixture(result->mBody, def, result);


			return result;
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
							outInfo.hitPos = B2Conv::To(point);
							outInfo.fraction = fraction;
							outInfo.normal = B2Conv::To(normal);
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
			mWorld->RayCast(&callback, B2Conv::To(startPos), B2Conv::To(endPos));

			return callback.bHitted;
		}

		void setupDebug(RHIGraphics2D& g) override
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

		virtual bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB) override
		{
			PhyBodyBox2D* bodyA = (PhyBodyBox2D*)fixtureA->GetUserData();
			PhyBodyBox2D* bodyB = (PhyBodyBox2D*)fixtureB->GetUserData();
			PhyObjectDef* defA = bodyA->mPhyDef.get();
			PhyObjectDef* defB = bodyB->mPhyDef.get();
			return (defA->collisionMask & BIT(defB->collisionType)) &&
				(defB->collisionMask & BIT(defA->collisionType));
		}

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