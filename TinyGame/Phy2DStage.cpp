#include "Phy2DStage.h"

#include "Coroutine.h"

#include "RenderUtility.h"
#include "GameGUISystem.h"

#include "GameGlobal.h"
#include "RHI/RHIGraphics2D.h"

namespace Phy2D
{
	FunctionJumper gDebugJumper;
	bool gDebugStep = true;

	void jumpDebug()
	{
		if( gDebugStep )
			gDebugJumper.jump();
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

	void Phy2DStageBase::onUpdate(long time)
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	void Phy2DStageBase::renderObject(RHIGraphics2D& g, CollideObject& obj)
	{
		Shape* shape = obj.mShape;

		glPushMatrix();

		glTranslatef(obj.getPos().x, obj.getPos().y, 0);
		glRotatef(Math::Rad2Deg(obj.mXForm.getRotateAngle()), 0, 0, 1);
		glTranslatef(-obj.getPos().x, -obj.getPos().y, 0);

		switch( shape->getType() )
		{
		case Shape::eBox:
		{
			Vector2 ext = static_cast<BoxShape*>(shape)->mHalfExt;
			g.drawRect(obj.getPos() - ext, Vec2i(2 * ext));
		}
		break;
		case Shape::eCircle:
			g.drawCircle(obj.getPos(), static_cast<CircleShape*>(shape)->getRadius());
			break;
		case Shape::ePolygon:
		{
			PolygonShape* poly = static_cast<PolygonShape*>(shape);
			glTranslatef(obj.getPos().x, obj.getPos().y, 0);
			g.drawPolygon(&poly->mVertices[0], poly->mVertices.size());
		}
		break;
		}
		glPopMatrix();
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
		mShape1.mHalfExt = Vector2(1, 1);
		mShape2.setRadius(1);
		mObjects[0].mShape = &mShape1;
		mObjects[0].mXForm.setTranslation(Vector2(0, 0));
		mObjects[1].mShape = &mShape2;
		//mObjects[1].mXForm.setRoatation( Math::Deg2Rad( 45 ) );
		mObjects[1].mXForm.setTranslation(Vector2(1, 1));


		restart();
		return true;
	}

	void CollideTestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		g.beginRender();

		float scale = 50;
		Vector2 offset = Vector2(300, 300);

		glPushMatrix();
		glTranslatef(offset.x, offset.y, 0);
		glScalef(scale, scale, scale);


		RenderUtility::SetPen(g, EColor::Red);
		g.drawLine(Vector2(-100, 0), Vector2(100, 0));
		RenderUtility::SetPen(g, EColor::Green);
		g.drawLine(Vector2(0, -100), Vector2(0, 100));


		RenderUtility::SetPen(g, (mIsCollided) ? EColor::White : EColor::Gray);
		RenderUtility::SetBrush(g, EColor::Null);
		for( int i = 0; i < 2; ++i )
		{
			CollideObject& obj = mObjects[i];
			renderObject(g, obj);

		}
		InlineString< 256 > str;

		//if ( mIsCollided )
		{
			RenderUtility::SetPen(g, EColor::Green);
			g.drawLine(mContact.pos[0], mObjects[0].getPos());
			g.drawLine(mContact.pos[1], mObjects[1].getPos());
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawLine(mObjects[0].getPos() + mContact.normal * mContact.depth, mObjects[0].getPos());
			RenderUtility::SetPen(g, EColor::Purple);
			g.drawLine(mObjects[0].getPos() - mContact.normal, mObjects[0].getPos());


			XForm2D const& worldTrans = mObjects[0].mXForm;

			glPushMatrix();
			glTranslatef(worldTrans.getPos().x, worldTrans.getPos().y, 0);
			glRotatef(Math::Rad2Deg(worldTrans.getRotateAngle()), 0, 0, 1);

#if PHY2D_DEBUG	

#if 0
			RenderUtility::setPen(g, Color::eOrange);
			for( int n = 0; n < 3; ++n )
			{
				Vector2 size = Vector2(0.1, 0.1);
				g.drawRect(gGJK.mSv[n]->vObj[0] - size / 2, size);
			}
#endif
#endif //PHY2D_DEBUG
#if 0
			RenderUtility::setPen(g, Color::eBlue);
			for( int n = 0; n < 3; ++n )
			{
				Vector2 size = Vector2(0.1, 0.1);
				g.drawRect(gGJK.mSv[n]->v - size / 2, size);
			}
#endif
#if 1
			for( int n = 0; n < gGJK.mNumEdge; ++n )
			{
				GJK::Edge& edge = gGJK.mEdges[n];
				Vector2 size = Vector2(0.05, 0.05);
				RenderUtility::SetPen(g, EColor::Cyan);
				g.drawRect(edge.sv->v - size / 2, size);
				RenderUtility::SetPen(g, EColor::Cyan);
				g.drawLine(edge.sv->v, edge.sv->v + Math::GetNormal(edge.sv->d));
			}
#endif
#if PHY2D_DEBUG	
			for( int n = 0; n < gGJK.mDBG.size(); ++n )
			{
				GJK::Simplex& sv = gGJK.mDBG[n];
				Vector2 pos = mObjects[0].mXForm.transformPosition(sv.v);
				Vector2 size = Vector2(0.05, 0.05);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawRect(pos - size / 2, size);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawLine(pos, pos + 0.5 * Math::GetNormal(sv.d));
			}

#endif //PHY2D_DEBUG

			glPopMatrix();
		}

		glPopMatrix();

		if( mIsCollided )
		{
			str.format("%f , %f , %f", mContact.normal.x, mContact.normal.y, mContact.depth);
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
				mObjects[1].mXForm.rotate(0.01); break;
			case EKeyCode::Right:
				mObjects[1].mXForm.rotate(-0.01); break;
			case EKeyCode::Num1:
				++idx; if (idx >= ARRAY_SIZE(mShapes)) idx = 0;
				mObjects[0].mShape = mShapes[idx];
				break;
			case EKeyCode::Num2:
				--idx; if (idx < 0) idx = ARRAY_SIZE(mShapes) - 1;
				mObjects[0].mShape = mShapes[idx];
				break;
			case EKeyCode::Num3:
				++idx2; if (idx2 >= ARRAY_SIZE(mShapes)) idx2 = 0;
				mObjects[1].mShape = mShapes[idx2];
				break;
			case EKeyCode::Num4:
				--idx2; if (idx2 < 0) idx2 = ARRAY_SIZE(mShapes) - 1;
				mObjects[1].mShape = mShapes[idx2];
				break;
			case EKeyCode::E:
				mIsCollided = mCollision.test(&mObjects[0], &mObjects[1], mContact);
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
		mCircleShape.setRadius(1);

		BodyInfo info;
		info.linearDamping = 0.2;
		RigidBody* body;
		body = mWorld.createRigidBody(&mBoxShape[0], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(0, 0));

		body = mWorld.createRigidBody(&mBoxShape[0], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(0, 40));

		body = mWorld.createRigidBody(&mBoxShape[1], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(-10, 20));

		body = mWorld.createRigidBody(&mBoxShape[1], info);
		body->setMotionType(BodyMotion::eStatic);
		body->setPos(Vector2(10, 20));


		body = mWorld.createRigidBody(&mCircleShape, info);
		body->setPos(Vector2(0, 6));
		mBody[0] = body;

		body = mWorld.createRigidBody(&mCircleShape, info);
		body->setPos(Vector2(0, 8));
		mBody[1] = body;



		//body = mWorld.createRigidBody( &mCircleShape , info );
		//body->setPos( Vector2( 0 , 20 ) );


		std::function< void() > func = std::bind(&WorldTestStage::debugEntry, this);
		gDebugJumper.start(func);

		restart();
		return true;
	}

	void WorldTestStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		g.beginRender();

		float scale = 10;
		Vector2 offset = Vector2(300, 500);

		glPushMatrix();
		glTranslatef(offset.x, offset.y, 0);
		glScalef(scale, -scale, scale);


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

			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawRect(proxy->aabb.min, proxy->aabb.max - proxy->aabb.min);

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

		InlineString< 256 > str;

		glPopMatrix();

		if( !mWorld.mColManager.mMainifolds.empty() )
		{
			ContactManifold& cm = *mWorld.mColManager.mMainifolds[0];
			Contact& c = cm.mContect;

			RigidBody* bodyA = static_cast<RigidBody*>(c.object[0]);
			RigidBody* bodyB = static_cast<RigidBody*>(c.object[1]);

			Vector2 cp = 0.5 * (c.pos[0] + c.pos[1]);

			Vector2 vA = bodyA->getVelFromWorldPos(cp);
			Vector2 vB = bodyB->getVelFromWorldPos(cp);
			Vector2 rA = cp - bodyA->mPosCenter;
			Vector2 rB = cp - bodyB->mPosCenter;
			Vector2 vrel = vB - vA;
			float vn = vrel.dot(c.normal);

			Vector2 cpA = bodyA->mXForm.transformPosition(c.posLocal[0]);
			Vector2 cpB = bodyB->mXForm.transformPosition(c.posLocal[1]);

			//#TODO: normal change need concerned
			float depth2 = c.normal.dot(cpA - cpB);

			str.format("vn = %f depth = %f", vn, depth2);
			g.drawText(100, 20, str);
		}
		str.format("v = %f %f", mBody[0]->mLinearVel.x, mBody[0]->mLinearVel.y);
		g.drawText(100, 30, str);
		str.format("v = %f %f", mBody[1]->mLinearVel.x, mBody[1]->mLinearVel.y);
		g.drawText(100, 40, str);


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
					body->setPos(Vector2(0, 30));
				}
				break;
			case EKeyCode::F2:
				{
					if (gDebugStep)
					{
						gDebugStep = false;
						gDebugJumper.jump();
					}
					else
					{
						gDebugStep = true;
						std::function< void() > func = std::bind(&WorldTestStage::debugEntry, this);
						gDebugJumper.start(func);
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