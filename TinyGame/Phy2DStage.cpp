#include "Phy2DStage.h"

#include "Async/Coroutines.h"

#include "RenderUtility.h"
#include "GameGUISystem.h"

#include "GameGlobal.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/GlobalShader.h"
#include "RHI/ShaderManager.h"
#include "RandomUtility.h"

namespace Phy2D
{
	using namespace Render;


	Coroutines::ExecutionHandle GExecHandle;
	bool gDebugStep = false;

	class MinkowskiSunProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(MinkowskiSunProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/MinkowskiSun";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
		
		}

	};

	IMPLEMENT_SHADER_PROGRAM(MinkowskiSunProgram);

	void jumpDebug()
	{
		if (gDebugStep)
			CO_YEILD(nullptr);
	}


	bool Phy2DStageBase::onInit()
	{
		GDebugJumpFun = jumpDebug;

		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();
		return true;
	}

	void Phy2DStageBase::onEnd()
	{
		BaseClass::onEnd();
	}

	void Phy2DStageBase::onUpdate(GameTimeSpan time)
	{
		BaseClass::onUpdate(time);

		int frame = long(time) / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void Phy2DStageBase::renderObject(RHIGraphics2D& g, CollideObject& obj)
	{
		Shape* shape = obj.mShape;
		g.pushXForm();

		g.translateXForm(obj.getPos().x, obj.getPos().y);
		g.rotateXForm(obj.mXForm.getRotateAngle());
		g.translateXForm(-obj.getPos().x, -obj.getPos().y);

		switch( shape->getType() )
		{
		case Shape::eBox:
		{
			Vector2 ext = static_cast<BoxShape*>(shape)->mHalfExt;
			g.drawRect(obj.getPos() - ext, 2.0 * ext);
		}
		break;
		case Shape::eCircle:
			g.drawCircle(obj.getPos(), static_cast<CircleShape*>(shape)->getRadius());
			break;
		case Shape::ePolygon:
		{
			PolygonShape* poly = static_cast<PolygonShape*>(shape);
			g.translateXForm(obj.getPos().x, obj.getPos().y);
			g.drawPolygon(&poly->mVertices[0], poly->mVertices.size());
		}
		break;
		}

		g.popXForm();

	}

	bool CollideTestStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		mShapes[0] = &mShape1;
		mShapes[1] = &mShape2;
		mShapes[2] = &mShape3;
		mShape3.mVertices.push_back(Vector2(2, 0));
		mShape3.mVertices.push_back(Vector2(1, 3));
		mShape3.mVertices.push_back(Vector2(-2, 0));
		mShape3.mVertices.push_back(Vector2(-1, -2));
		mShape1.mHalfExt = Vector2(2, 1);
		mShape2.setRadius(1);
		mObjects[1].mShape = &mShape2;
		mObjects[1].mXForm.setTranslation(Vector2(0, 0));
		mObjects[0].mShape = &mShape3;
		mObjects[0].mXForm.setTranslation(Vector2(1, 1));

		doCollisionTest();

		restart();
		return true;
	}

	void CollideTestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		using namespace Render;

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(RHICommandList::GetImmediateList(), EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

		g.beginRender();

		float scale = 50;
		Vector2 offset = Vector2(300, 300);

		g.pushXForm();
		g.translateXForm(offset.x, offset.y);
		g.scaleXForm(scale,scale);

		RenderUtility::SetPen(g, EColor::Red);
		g.drawLine(Vector2(-100, 0), Vector2(100, 0));
		RenderUtility::SetPen(g, EColor::Green);
		g.drawLine(Vector2(0, -100), Vector2(0, 100));

		ObjectParamData paramData;
		paramData.ObjectA.set(mObjects[0]);
		paramData.ObjectB.set(mObjects[1]);
		mObjectParams.updateBuffer(paramData);

		AABB bound;
		bound.invalidate();
		bound += mObjects[0].getAABB();
		bound += mObjects[1].getAABB();
		bound.expand(Vector2(1, 1));

		MinkowskiSunProgram* program = ShaderManager::Get().getGlobalShaderT< MinkowskiSunProgram >();
		g.drawCustomFunc([program, &g, bound, this](Render::RHICommandList& commandList, Matrix4 const& baseTransform, Render::RenderBatchedElement& element)
		{
			RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
			RHISetShaderProgram(commandList, program->getRHI());
			program->setParam(commandList, SHADER_PARAM(XForm), element.transform.toMatrix4() * baseTransform);
			SetStructuredUniformBuffer(commandList, *program, mObjectParams);
			SetStructuredStorageBuffer<VertexData>(commandList, *program, mVertexBuffer);

			LinearColor color = LinearColor(0, 0, 1, 0.2);

			struct Vertex
			{
				Vector2 pos;
				LinearColor color;
			} v[] =
			{
				{ bound.min, color },
				{ Vector2(bound.max.x,bound.min.y), color },
				{ bound.max, color },
				{ Vector2(bound.min.x,bound.max.y), color },
			};

			TRenderRT<RTVF_XY_CA>::Draw(commandList, EPrimitive::Quad, v, 4);
		});

		RenderUtility::SetPen(g, (mIsCollided) ? EColor::White : EColor::Gray);
		RenderUtility::SetBrush(g, EColor::Null);
		for( int i = 0; i < 2; ++i )
		{
			CollideObject& obj = mObjects[i];
			renderObject(g, obj);
		}

		InlineString< 256 > str;

		if ( mIsCollided )
		{
			for(int i = 0; i < mManifold.mNumContacts; ++i)
			{
				ManifoldPoint& p = mManifold.mPoints[i];
				RenderUtility::SetPen(g, EColor::Green);
				g.drawLine(p.pos[0], mObjects[0].getPos());
				g.drawLine(p.pos[1], mObjects[1].getPos());
				RenderUtility::SetPen(g, EColor::Yellow);
				g.drawLine(mObjects[0].getPos() + mManifold.mContact.normal * p.depth, mObjects[0].getPos());
			}
			RenderUtility::SetPen(g, EColor::Purple);
			g.drawLine(mObjects[0].getPos() - mManifold.mContact.normal, mObjects[0].getPos());

			XForm2D const& worldTrans = mObjects[0].mXForm;
			//g.pushXForm();
			//g.translateXForm(worldTrans.getPos().x, worldTrans.getPos().y);
			//g.rotateXForm(worldTrans.getRotateAngle());

			auto& gjk = gGJK;
#if 0
			RenderUtility::setPen(g, EColor::Blue);
			for( int n = 0; n < 3; ++n )
			{
				Vector2 size = Vector2(0.1, 0.1);
				g.drawRect(gjk.mSv[n]->v - size / 2, size);
			}
#endif


#if 1
			for( int n = 0; n < gjk.mNumEdge; ++n )
			{
				MinkowskiBase::Edge& edge = gjk.mEdges[n];
				Vector2 size = Vector2(0.05, 0.05);
				RenderUtility::SetPen(g, EColor::Cyan);
				auto const& xForm = mObjects[0].mXForm;
				Vector2 v = xForm.transformVector(edge.sv->v);
				g.drawRect(v - size / 2, size);
				RenderUtility::SetPen(g, EColor::Cyan);
				g.drawLine(v, v + Math::GetNormal(xForm.transformVector(edge.sv->dir)));
			}
#endif
#if PHY2D_DEBUG	
			g.setTextRemoveScale(true);
			for( int n = 0; n < gjk.mDBG.size(); ++n )
			{
				MinkowskiBase::Vertex& sv = gjk.mDBG[n];

				auto const& xForm = mObjects[0].mXForm;
				Vector2 v = xForm.transformVector(sv.v);
				Vector2 size = Vector2(0.05, 0.05);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawRect(v - size / 2, size);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawLine(v, v + 0.5 * Math::GetNormal(xForm.transformVector(sv.dir)));
				g.drawTextF(v, "%d", n);
			}
			g.setTextRemoveScale(false);

#endif //PHY2D_DEBUG
			//g.popXForm();
		}

		g.popXForm();

		if( mIsCollided && mManifold.mNumContacts > 0 )
		{
			str.format("%f , %f , %f", mManifold.mContact.normal.x, mManifold.mContact.normal.y, mManifold.mPoints[0].depth);
			g.drawText(10, 20, str);
		}



		g.endRender();
	}

	MsgReply CollideTestStage::onKey(KeyMsg const& msg)
	{
		if(msg.isDown())
		{
			static int idx = 0;
			static int idx2 = 0;
			float speed = 0.013;
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::D: moveObject(Vector2(speed, 0)); break;
			case EKeyCode::A: moveObject(Vector2(-speed, 0)); break;
			case EKeyCode::W: moveObject(Vector2(0, speed)); break;
			case EKeyCode::S: moveObject(Vector2(0, -speed)); break;
			case EKeyCode::Left:
				mObjects[0].mXForm.rotate(0.01);
				doCollisionTest();
				break;
			case EKeyCode::Right:
				mObjects[0].mXForm.rotate(-0.01);
				doCollisionTest();
				break;
			case EKeyCode::Num1:
				++idx; if (idx >= ARRAY_SIZE(mShapes)) idx = 0;
				mObjects[0].mShape = mShapes[idx];
				doCollisionTest();
				break;
			case EKeyCode::Num2:
				--idx; if (idx < 0) idx = ARRAY_SIZE(mShapes) - 1;
				mObjects[0].mShape = mShapes[idx];
				doCollisionTest();
				break;
			case EKeyCode::Num3:
				++idx2; if (idx2 >= ARRAY_SIZE(mShapes)) idx2 = 0;
				mObjects[1].mShape = mShapes[idx2];
				doCollisionTest();
				break;
			case EKeyCode::Num4:
				--idx2; if (idx2 < 0) idx2 = ARRAY_SIZE(mShapes) - 1;
				mObjects[1].mShape = mShapes[idx2];
				doCollisionTest();
				break;
			case EKeyCode::E:
				doCollisionTest();
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

	bool WorldTestStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		mShape3.mVertices.push_back(Vector2(2, 0));
		mShape3.mVertices.push_back(Vector2(1, 3));
		mShape3.mVertices.push_back(Vector2(-2, 0));
		mShape3.mVertices.push_back(Vector2(-1, -2));
		mBoxShape[0].mHalfExt = Vector2(20, 5);
		mBoxShape[1].mHalfExt = Vector2(5, 20);
		mBoxShape2.mHalfExt = Vector2(4, 4);
		mDynamicBoxShape.mHalfExt = Vector2(2, 2);  // Dynamic box: 4x4 size
		mCircleShape.setRadius(1);

		BodyInfo info;
		info.linearDamping = 0.2;
		info.angularDamping = 0.5;  // Add angular damping to stop rotation
		RigidBody* body;
		body = mWorld.createRigidBody(&mBoxShape[0], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(0, 0));

		body = mWorld.createRigidBody(&mBoxShape[0], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(0, 40));

#if 0
		body = mWorld.createRigidBody(&mBoxShape[1], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(-10, 20));

		body = mWorld.createRigidBody(&mBoxShape[1], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(10, 20));
#endif


#if 0
		body = mWorld.createRigidBody(&mCircleShape, info);
		body->setPos(Vector2(0, 10));  // Raised from 6 to 10
		mBody[0] = body;

		body = mWorld.createRigidBody(&mCircleShape, info);
		body->setPos(Vector2(0, 14));  // Raised from 8 to 14
		mBody[1] = body;

		// Add initial dynamic boxes (Box2D style)
		body = mWorld.createRigidBody(&mDynamicBoxShape, info);
		body->setPos(Vector2(-3, 20));
		body->mXForm.rotate(0.2f);
		body->synTransform();
#endif

		body = mWorld.createRigidBody(&mDynamicBoxShape, info);
		body->setPos(Vector2(3, 25));
		body->mXForm.rotate(-0.15f);
		body->synTransform();

		//body = mWorld.createRigidBody( &mCircleShape , info );
		//body->setPos( Vector2( 0 , 20 ) );
		GExecHandle = Coroutines::Start([this] {
			debugEntry();
		});

		restart();
		return true;
	}

	void WorldTestStage::onRender(float dFrame)
	{
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);

		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		g.beginRender();

		float scale = 10;
		Vector2 offset = Vector2(300, 500);

		g.pushXForm();
		g.translateXForm(offset.x, offset.y);
		g.scaleXForm(scale, -scale);


		RenderUtility::SetPen(g, EColor::Red);
		g.drawLine(Vector2(-100, 0), Vector2(100, 0));
		RenderUtility::SetPen(g, EColor::Green);
		g.drawLine(Vector2(0, -100), Vector2(0, 100));


		RenderUtility::SetBrush(g, EColor::Null);
		for( World::RigidBodyList::iterator iter = mWorld.mRigidBodies.begin(), itEnd = mWorld.mRigidBodies.end();
			iter != itEnd; ++iter )
		{
			RigidBody* body = *iter;
			RenderUtility::SetPen(g, EColor::White);
			renderObject(g, *body);
			CollisionProxy* proxy = body->mProxy;
#if 0
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawRect(proxy->aabb.min, proxy->aabb.max - proxy->aabb.min);
#endif

			RenderUtility::SetPen(g, EColor::Red);
			Vector2 const xDir = body->mXForm.getRotation().getXDir();
			Vector2 const yDir = body->mXForm.getRotation().getYDir();
			RenderUtility::SetPen(g, EColor::Red);
			g.drawLine(body->getPos(), body->getPos() + 0.5 * xDir);
			RenderUtility::SetPen(g, EColor::Green);
			g.drawLine(body->getPos(), body->getPos() + 0.5 * yDir);

			RenderUtility::SetPen(g, EColor::Orange);
			g.drawLine(body->getPos(), body->getPos() + body->mLinearVel);

		}
		g.popXForm();

		InlineString< 256 > str;

		if( !mWorld.mColManager.mMainifolds.empty() )
		{
			ContactManifold& cm = *mWorld.mColManager.mMainifolds[0];
			Contact& c = cm.mContact;

			RigidBody* bodyA = static_cast<RigidBody*>(c.object[0]);
			RigidBody* bodyB = static_cast<RigidBody*>(c.object[1]);

			// Use first contact point for debug display
			if (cm.mNumContacts > 0)
			{
				ManifoldPoint& mp = cm.mPoints[0];
				Vector2 cp = 0.5 * (mp.pos[0] + mp.pos[1]);

				Vector2 vA = bodyA->getVelFromWorldPos(cp);
				Vector2 vB = bodyB->getVelFromWorldPos(cp);
				Vector2 rA = cp - bodyA->mPosCenter;
				Vector2 rB = cp - bodyB->mPosCenter;
				Vector2 vrel = vB - vA;
				float vn = vrel.dot(c.normal);

				Vector2 cpA = bodyA->mXForm.transformPosition(mp.posLocal[0]);
				Vector2 cpB = bodyB->mXForm.transformPosition(mp.posLocal[1]);

				//#TODO: normal change need concerned
				float depth2 = c.normal.dot(cpA - cpB);

				str.format("vn = %f depth = %f numPoints = %d", vn, depth2, cm.mNumContacts);
				g.drawText(100, 20, str);
			}
		}
#if 0
		str.format("v = %f %f", mBody[0]->mLinearVel.x, mBody[0]->mLinearVel.y);
		g.drawText(100, 30, str);
		str.format("v = %f %f", mBody[1]->mLinearVel.x, mBody[1]->mLinearVel.y);
		g.drawText(100, 40, str);
#endif


		//g.drawText( 10 , 10 , str.format( "%f , %f , %f" , mContact.normal.x , mContact.normal.y , mContact.depth ) );


		g.endRender();
	}

	MsgReply WorldTestStage::onKey(KeyMsg const& msg)
	{
		if(msg.isDown())
		{
			float speed = 0.013;
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::D:  break;
			case EKeyCode::A: break;
			case EKeyCode::W: break;
			case EKeyCode::S: break;
			case EKeyCode::E:
				{
					BodyInfo info;
					RigidBody* body = mWorld.createRigidBody(&mCircleShape, info);
					body->setPos(Vector2( 0.1 * RandFloat(), 30));
				}
				break;
			case EKeyCode::B:  // Spawn dynamic box (Box2D style)
				{
					BodyInfo info;
					info.linearDamping = 0.1f;
					info.angularDamping = 0.1f;
					RigidBody* body = mWorld.createRigidBody(&mDynamicBoxShape, info);
					body->setPos(Vector2(2.0f * RandFloat() - 1.0f, 30));
					// Add slight rotation to make stacking more interesting
					body->mXForm.rotate(0.1f * RandFloat() - 0.05f);
					body->synTransform();
				}
				break;
			case EKeyCode::F2:
				{
					if (gDebugStep)
					{
						gDebugStep = false;
						Coroutines::Resume(GExecHandle);
					}
					else
					{
						gDebugStep = true;
						GExecHandle = Coroutines::Start([this]
						{
							debugEntry();
						});
					}
				}
				break;
			case 'Q': jumpDebug(); break;
			}
		}
		return BaseClass::onKey(msg);
	}

	void WorldTestStage::debugEntry()
	{
		jumpDebug();
		while( gDebugStep )
		{
			mWorld.simulate(gDefaultTickTime / 1000.0f);
		}
	}

	void WorldTestStage::tick()
	{
		if( !gDebugStep )
			mWorld.simulate(gDefaultTickTime / 1000.0f);
	}

}