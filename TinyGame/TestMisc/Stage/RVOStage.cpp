#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"


namespace RVO
{
	using namespace Render;


	class Agent
	{
	public:
		Vector2 pos;
		Vector2 vel;
		Vector2 goal;
		float   speed;
		float   radius;

		TArray< Vector2 > trace;

		void calcVel(Agent const& other)
		{
			Vector2 posRel = pos - other.pos;
			Vector2 velRel = vel - other.vel;

			float radiusCombined = radius + other.radius;
			float radiusCombinedSq = Math::Square(radiusCombined);
			float distSq = posRel.length2();

			if (distSq > radiusCombinedSq)
			{
				Vector2 dir = goal - pos;
				vel = speed * Math::GetNormal(dir);
			}
			else
			{
				Vector2 dir = Math::GetNormal( Vector2( posRel.y , -posRel.x) );
				vel = speed * dir;
			}
		}

		void update(float dt)
		{
			pos += vel * dt;
			trace.push_back(pos);
		}
	};

	class TestStage : public StageBase , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		TArray< Agent > mAgents;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();
			restart();

			WidgetUtility::CreateDevFrame();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			mAgents.clear();
			{
				Agent agent;
				agent.pos = Vector2(100, 200);
				agent.goal = Vector2(600, 200);
				agent.speed = 800;
				agent.radius = 100;
				mAgents.push_back(agent);
			}
			{
				Agent agent;
				agent.pos = Vector2(350, 200);
				agent.goal = Vector2(100, 200);
				agent.speed = 0;
				agent.radius = 100;
				mAgents.push_back(agent);
			}
		}

		void tick() 
		{
			mAgents[0].calcVel(mAgents[1]);
			mAgents[1].calcVel(mAgents[0]);

			for (auto& agent : mAgents)
			{
				agent.update(float(gDefaultTickTime) / 1000 );
			}
		}
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

			auto& commandList = g.getCommandList();
			RHIClearRenderTargets(commandList, EClearBits::Color , &LinearColor(0.2,0.2,0.2,1) , 1);

			RenderUtility::SetPen(g , EColor::Black);
			RenderUtility::SetBrush(g , EColor::Yellow);
			for (auto const& agent : mAgents)
			{
				g.drawCircle( agent.pos, agent.radius );

				RenderUtility::SetPen(g, EColor::Red);

				if (agent.trace.size() > 2)
				{
					g.drawLineStrip(agent.trace.data(), agent.trace.size());
				}				
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
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

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}


		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.bWasUsedPlatformGraphics = true;
		}

	protected:
	};

	REGISTER_STAGE_ENTRY("RVO Test", TestStage, EExecGroup::MiscTest, "AI");
}