
#include "StageBase.h"
#include "GameRenderSetup.h"

#include "Renderer/RenderTransform2D.h"
#include "CarTrain/GameFramework.h"
#include "RHI/RHIGraphics2D.h"
#include "RandomUtility.h"

namespace P2G
{
	using namespace Render;
	using namespace CarTrain;


	class TestStage : public StageBase
					, public IGameRenderSetup

	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		GameWorld mWorld;

		bool onInit() override
		{
			mWorld.initialize();
			mWorld.getPhysicsScene()->setGravity(Vector2(0, -9.8));


			float length = 300;
			float width = 10;
			BoxObjectDef def;
			def.extend = Vector2(length, width);
			def.bEnablePhysics = false;
			def.bCollisionDetection = true;
			def.bCollisionResponse  = true;

			{
				mWorld.getPhysicsScene()->createBox(def, XForm2D());
			}

			{
				def.extend = Vector2(width, length);
				mWorld.getPhysicsScene()->createBox(def, XForm2D(Vector2((length + width ) / 2, (length - width) / 2)));
				mWorld.getPhysicsScene()->createBox(def, XForm2D(Vector2(-(length + width) / 2, (length - width) / 2)));
			}
			Vector2 lookPos = Vector2(0, length / 2);
			mWorldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), lookPos, Vector2(0,-1), ::Global::GetScreenSize().x / 400.0f);
			mScreenToWorld = mWorldToScreen.inverse();
			::Global::GUI().cleanupWidget();

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

		void restart()
		{

		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			mWorld.tick(deltaTime);
		}


		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			using namespace Render;

			Vec2i screenSize = ::Global::GetScreenSize();
			RHICommandList& commandList = g.getCommandList();

			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.1, 0.1, 0.1, 0), 1);

			g.beginRender();

			{

				TRANSFORM_PUSH_SCOPE(g.getTransformStack(), mWorldToScreen, true);

				mWorld.getPhysicsScene()->setupDebug(g);
				mWorld.getPhysicsScene()->drawDebug();

				RenderUtility::SetPen(g, EColor::Red);
				g.drawLine(Vector2::Zero(), Vector2(100, 0));
				RenderUtility::SetPen(g, EColor::Green);
				g.drawLine(Vector2::Zero(), Vector2(0, 100));
			}

			g.endRender();

		}


		MsgReply onMouse(MouseMsg const& msg) override
		{

			if (msg.onLeftDown())
			{
				BoxObjectDef def;
				def.extend = Vector2( RandFloat(10, 30) , RandFloat(10, 30));
				def.bEnablePhysics = true;
				def.bCollisionDetection = true;
				def.bCollisionResponse = true;

				XForm2D transform;
				transform.setTranslation(mScreenToWorld.transformPosition(msg.getPos()));
				mWorld.getPhysicsScene()->createBox(def, transform);
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
				case EKeyCode::P:
					{

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

	
	};


	REGISTER_STAGE_ENTRY("Phy2D Game", TestStage, EExecGroup::Test , "Physics|Game" );

}