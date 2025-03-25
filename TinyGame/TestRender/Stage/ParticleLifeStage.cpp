#include "stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "Renderer/RenderTransform2D.h"
#include "RandomUtility.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "ProfileSystem.h"
#include "ReflectionCollect.h"

#include <functional>

#define CHAISCRIPT_NO_THREADS
#include "chaiscript/chaiscript.hpp"


namespace Chai = chaiscript;

namespace ParticleLife
{
	using namespace Render;

	constexpr int ParticleTypeMaxCount = 10;
	struct Paricle 
	{
		int type;
		Vector2 force;
		Vector2 pos;
		Vector2 vel;
	};

	struct Settings
	{
		Vector2 boundSize = Vector2(50,50);
		float   forceScale = 10.0f;
		float   damping = 0.6;
		float   renderRadius = 0.2f; 


		REFLECT_STRUCT_BEGIN(Settings)
			REF_PROPERTY(boundSize)
			REF_PROPERTY(forceScale)
			REF_PROPERTY(damping)
			REF_PROPERTY(renderRadius)
		REFLECT_STRUCT_END()
	};

	template< class ModuleType >
	class TScriptCollector
	{

	public:
		TScriptCollector(ModuleType& inModule)
			:module(inModule)
		{}

		template< class T >
		void collect()
		{
			REF_COLLECT_TYPE(T, *this);
		}

		template< class T >
		void beginClass(char const* name)
		{
			module.add(Chai::user_type< T >(), name);
		}
		template< class T >
		void addProperty(T var, char const* name)
		{
			module.add(Chai::fun(var), name);
		}

		void endClass()
		{

		}
		ModuleType& module;
	};

	class TestStage : public StageBase
	                , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			mInteractMap.resize(ParticleTypeMaxCount * ParticleTypeMaxCount);
			restart();
			return true;
		}

		void postLoadScene()
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * mSettings.boundSize, Vector2(0, -1), screenSize.y / mSettings.boundSize.y, false);
			mScreenToWorld = mWorldToScreen.inverse();
		}

		Settings mSettings;
		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		void addParticles(int type, int num)
		{
			for (int i = 0; i < num; ++i)
			{
				Paricle& p = *new (mParticles) Paricle;
				p.pos.x = RandFloat() * mSettings.boundSize.x;
				p.pos.y = RandFloat() * mSettings.boundSize.y;
				p.vel = Vector2::Zero();
				p.force = Vector2::Zero();
				p.type = type;
			}
		}

		TArray<Paricle> mParticles;
		struct InteractDesc
		{
			float factor;
			float radius;
		};
		TArray<InteractDesc> mInteractMap;
		float& getFactor(int i, int j)
		{
			return mInteractMap[i * ParticleTypeMaxCount + j].factor;
		}

		void interact(Paricle& p1, Paricle& p2)
		{
			Vector2 offset = p2.pos - p1.pos;
			float distSquare = offset.length2();
			float force = distSquare > 1e-4 ? 1.0 / distSquare : 0.0;
			p1.force -= (force * getFactor(p1.type, p2.type)) * offset;
			p2.force += (force * getFactor(p2.type, p1.type)) * offset;
		}

		void simulate(float dt)
		{
			PROFILE_ENTRY("Simulate");

			for (int i = 0; i < mParticles.size(); ++i)
			{
				auto& pi = mParticles[i];
				for (int j = i + 1; j < mParticles.size(); ++j)
				{
					auto& pj = mParticles[j];
					interact(pi, pj);
				}
			}

			float forceScale = mSettings.forceScale * dt;
			for (int i = 0; i < mParticles.size(); ++i)
			{
				auto& p = mParticles[i];
				p.vel += p.force * forceScale;
				p.vel *= mSettings.damping;
				p.pos += p.vel * dt;
				limitBound(p);
				p.force = Vector2::Zero();
			}
		}

		void limitBound(Paricle& p)
		{
			Reflect(p.pos.x, p.vel.x, mSettings.boundSize.x);
			Reflect(p.pos.y, p.vel.y, mSettings.boundSize.y);
		}
		static void Reflect(float& x, float& v, float len)
		{
			if (x > len)
			{
				if (v > 0) v = -v;
			}
			else if (x < 0)
			{
				if (v < 0) v = -v;
			}
		}
		static void WarpPos(float& x, float len)
		{
			if (x > len)
				x -= len;
			else if (x < 0.0)
				x += len;
		}

		static void WarpPos(Vector2& pos, Vector2 const& size)
		{
			WarpPos(pos.x, size.x);
			WarpPos(pos.y, size.y);
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

#define SCRIPT_NAME( NAME ) #NAME



		void registerFunc(Chai::ChaiScript& script)
		{
			script.add(Chai::constructor< Vector2(float, float) >(), SCRIPT_NAME(Vector2));
			script.add(Chai::fun(static_cast<Vector2& (Vector2::*)(Vector2 const&)>(&Vector2::operator=)), SCRIPT_NAME(=));
			script.add(Chai::fun(&TestStage::addParticles, this), SCRIPT_NAME(AddParticles));
			script.add(Chai::fun(static_cast<void (TestStage::*)(int, int, float)>(&TestStage::setFactor), this), SCRIPT_NAME(SetFactor));
			script.add(Chai::fun(static_cast<void (TestStage::*)(int, int, float, float)>(&TestStage::setFactor), this), SCRIPT_NAME(SetFactor));

			TScriptCollector< Chai::ChaiScript > collector(script);
			collector.collect< Settings >();
		}

		bool loadScene(char const* fileName)
		{
			InlineString< 512 > path;
			path.format("Script/ParticleLife/%s.chai", fileName);
			Chai::ChaiScript script;
			registerFunc(script);

			try
			{
				TIME_SCOPE("Script Eval");
				script.eval_file(path.c_str());
				auto LoadFunc = script.eval<std::function<void( Settings& )>>("LoadScene");
				if (LoadFunc)
				{
					LoadFunc(mSettings);
				}	
			}
			catch (const std::exception& e)
			{
				LogMsg("Load Scene Fail!! : %s", e.what());
				return false;
			}

			return true;
		}

		void setFactor(int i, int j, float value)
		{
			getFactor(i,j) = value;
		}
		void setFactor(int i, int j, float factor, float radius)
		{
			auto& desc = mInteractMap[i * ParticleTypeMaxCount + j];
			desc.factor = factor;
			desc.radius = radius;
		}
		void restart() 
		{
			mParticles.clear();
			for (auto& desc : mInteractMap)
			{
				desc.factor = 0.0f;
			}


#if 1
			loadScene("Test");
#else
			setFactor(0, 0, 0.2f);
			setFactor(1, 0, 0.5f);
			setFactor(0, 1, -0.3f);
			setFactor(1, 1, -0.8f);
			addParticles(0, 1000);
			addParticles(1, 500);
#endif
			postLoadScene();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			simulate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.pushXForm();
			g.transformXForm(mWorldToScreen, false);
			RenderUtility::SetPen(g, EColor::Null);

			int ColorMap[] = { EColor::Red , EColor::Green, EColor::Blue, EColor::Yellow };

			Vector2 renderSize = Vector2(mSettings.renderRadius, mSettings.renderRadius);
			Vector2 renderSizeHalf = 0.5 * renderSize;
			for (auto const& p : mParticles)
			{
				RenderUtility::SetBrush(g, ColorMap[p.type]);
				g.drawRect( p.pos - renderSizeHalf, renderSize);
			}
			g.popXForm();
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

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1600;
			systemConfigs.screenHeight = 900;
		}

	protected:
	};

	REGISTER_STAGE_ENTRY("Particle Life", TestStage, EExecGroup::Dev, "Game");

}//ParticleLife

