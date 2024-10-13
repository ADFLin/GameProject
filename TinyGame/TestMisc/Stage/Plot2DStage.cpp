#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"
#include "RHI/ShaderProgram.h"
#include "FileSystem.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

using namespace Render;

class Plot2DStage : public StageBase
                  , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	Plot2DStage() {}


	ShaderProgram mProgPlot2D;


	bool generateShader()
	{
		std::string code;


		std::vector<uint8> codeTemplate;
		if (!FFileUtility::LoadToBuffer("Shader/Game/Plot2DTemplate.sgc", codeTemplate, true))
		{
			return false;
		}

		ShaderCompileOption option;
		option.addCode((char const*)codeTemplate.data());
		if (!ShaderManager::Get().loadFile(mProgPlot2D, nullptr, "ScreenVS", "MainPS", option))
		{
			return false;
		}
		return true;
	}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(frame->addSlider("Width") , mWidth , 0.02 , 1 );
		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);
	}

	Vector2 mRangeX = Vector2(-10, 10);
	float   mWidth = 0.05f;

	void onRender(float dFrame) override
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(1, 1, 1, 1), 1);


		float len = mRangeX.y - mRangeX.x;
		RenderTransform2D worldToScreen = RenderTransform2D::LookAt(screenSize, Vector2(0,0), Vector2(0,1), float(screenSize.x / len ));
		
		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();
		g.transformXForm(worldToScreen, false);

		RenderUtility::SetPen(g, EColor::Black);
		g.drawLine(Vector2(mRangeX.x, 0) , Vector2(mRangeX.y, 0));
		g.drawLine(Vector2(0,mRangeX.x), Vector2(0, mRangeX.y));

		g.endRender();


		{
			GPU_PROFILE("Plot2D");

			RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
			RHISetShaderProgram(commandList, mProgPlot2D.getRHI());

			float aspect = float(screenSize.y) / screenSize.x;

			Vector2 scale = Vector2(len, -len * aspect);
			Vector2 offset = 0.5 * (Vector2((mRangeX.x + mRangeX.y), 0) - scale);
			
			mProgPlot2D.setParam(commandList, SHADER_PARAM(E), 1.0f / (2 * len * screenSize.x) );
			mProgPlot2D.setParam(commandList, SHADER_PARAM(HalfWidth), 0.5f * mWidth);
			mProgPlot2D.setParam(commandList, SHADER_PARAM(Color), LinearColor(1, 0, 0, 1));
			mProgPlot2D.setParam(commandList, SHADER_PARAM(OffsetAndScale), Vector4(offset, scale));
			DrawUtility::ScreenRect(commandList, EScreenRenderMethod::Rect);
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

	bool setupRenderResource(ERenderSystem systemName) override
	{
		if ( !generateShader() )
			return false;

		return true;
	}


	void preShutdownRenderSystem(bool bReInit = false) override
	{
		mProgPlot2D.releaseRHI();
	}

protected:
};

REGISTER_STAGE_ENTRY("Plot 2D", Plot2DStage, EExecGroup::MiscTest);