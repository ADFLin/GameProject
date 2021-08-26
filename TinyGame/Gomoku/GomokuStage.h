#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "GomokuCore.h"
#include "Go/GoRenderer.h"


namespace Gomoku
{
	using namespace Render;
	using RenderContext = Go::RenderContext;

	class BoardRenderer : public Go::BoardRendererBase
	{
	public:
		void draw(RHIGraphics2D& g, SimpleRenderState& renderState, RenderContext const& context, int const* overrideStoneState = nullptr);
	};

	class TestStage : public StageBase
					, public IGameRenderSetup
		            , public IDebugListener
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			GDebugListener = this;
			mGame.setup();

			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			mGame.restart();
		}
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

		void onRender(float dFrame) override;

		Vector2 BoardPos = Vector2(100, 55);
		float const RenderBoardScale = 1.2;

		bool onMouse(MouseMsg const& msg) override;

		bool onKey(KeyMsg const& msg) override
		{
			if (!msg.isDown())
				return false;

			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
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

		bool setupRenderSystem(ERenderSystem systemName) override;
		void preShutdownRenderSystem(bool bReInit = false) override;


		void emitDebugPos(int index, int type) override;

	protected:
		std::vector< Vec2i > mPosList;
		Game  mGame;
		BoardRenderer mBoardRenderer;
	};




}