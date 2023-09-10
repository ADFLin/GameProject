#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIType.h"

#include "Math/TVector2.h"
#include "Renderer/RenderTransform2D.h"



DEFINE_TYPE_HASH_TO_STD(TVector2<int>, (TVector2<int> const& v) 
{ 
	uint32 result = HashValue(v.x);
	result = HashCombine(result, v.y);
	return result;
})
namespace Transport
{
	using Vec2i = TVector2<int>;
	using TPPoint = Math::Vector2;
	using RenderTransform2D = Render::RenderTransform2D;

	struct TPNamedLine
	{

	};

	enum EEndpointType
	{

	};

	struct TPLink;

	struct TPEndpoint
	{
		TPPoint pos;
		std::vector< TPLink* > connections;
	};

	struct TPLink
	{
		TPEndpoint* points[2];

		enum EShapeType
		{
			ShapeStraightLine,
			ShapeSpline,
			ShapeCircle,
		};
		struct
		{
			float radius;


		};

#if 0
		virtual TPPoint getPoint(float s) const = 0;
		virtual float   getLength() const = 0;
#endif
	};

	struct TPGraph
	{
		static float constexpr CoordSnapValue = 0.1;


		TPEndpoint* getEndPoint(TPPoint const& pos)
		{
			Vec2i posSnaped = SnapPos(pos);
			auto iter = mEndpoints.find(posSnaped);
			if (iter == mEndpoints.end())
				return nullptr;
			return iter->second;
		}

		static Vec2i SnapPos(TPPoint const& pos)
		{
			Vec2i result;
			result.x = Math::RoundToInt(pos.x / CoordSnapValue);
			result.y = Math::RoundToInt(pos.y / CoordSnapValue);
			return result;
		}
		std::unordered_map< Vec2i, TPEndpoint* > mEndpoints;
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
			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() {}
		void tick() {}
		void updateFrame(int frame) {}


		struct Viewport
		{
			RenderTransform2D worldToScreen;
			RenderTransform2D screenToWorld;


			Viewport()
			{
				worldToScreen = RenderTransform2D::Identity();
				screenToWorld = worldToScreen.inverse();
			}

		};

		Viewport mViewport;

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
			using namespace Render;
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			RHICommandList& cmdList =  g.getCommandList();
			RHISetFrameBuffer(cmdList, nullptr);
			RHIClearRenderTargets(cmdList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
			g.beginRender();

			g.beginXForm();
			g.transformXForm(mViewport.worldToScreen, true);
			switch (mEdit.phase)
			{
			case 1:
				{
					g.setBrush(Color3f(1, 0, 0));
					g.drawCircle(mEdit.startPos, 10);

					g.setPenWidth(10);
					g.drawLine(mEdit.startPos, mEdit.endPos);
				}
				break;
			case 2:
				{

				}
				break;
			}
			g.finishXForm();
			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				switch (mEdit.phase)
				{
				case 0:
					{
						Vector2 worldPos = mViewport.screenToWorld.transformPosition(msg.getPos());
						mEdit.startPos = worldPos;
						++mEdit.phase;
					}
					break;
				case 1:
					{


					}
					break;
				}

			}
			else if (msg.onRightDown())
			{
				mEdit.phase = 0;
			}
			else if (msg.onMoving())
			{
				if (mEdit.phase == 1)
				{
					Vector2 worldPos = mViewport.screenToWorld.transformPosition(msg.getPos());
					mEdit.endPos = worldPos;
				}

			}
			return BaseClass::onMouse(msg);
		}



		enum EBuildMode
		{
			StraightLine,
			

		};
		struct EditData
		{
			TPPoint startPos;
			TPPoint endPos;
			EBuildMode mode = EBuildMode::StraightLine;

			int phase = 0;
		};

		EditData mEdit;

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


		//IGameRenderSetup
		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
		}
		bool isRenderSystemSupported(ERenderSystem systemName) override
		{
			return true;
		}
		bool setupRenderResource(ERenderSystem systemName) override
		{
			return true;
		}
		void preShutdownRenderSystem(bool bReInit = false) override
		{
		}
		//~IGameRenderSetup
	protected:
	};

	REGISTER_STAGE_ENTRY("Transport", TestStage, EExecGroup::Dev, "Game");
}
