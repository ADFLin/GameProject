#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Math/PrimitiveTest.h"
#include "RHI/DrawUtility.h"
#include "CurveBuilder/ColorMap.h"

using namespace Render;

class MultiViewOnePassStage : public StageBase
	                        , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:

	MultiViewOnePassStage()
	{

	}

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();
		auto frame = WidgetUtility::CreateDevFrame();

		restart();


		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart()
	{

	}


	void onUpdate(long time) override
	{

	}

	bool bUseMultiViewports = true;

	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vector2 screenSize = ::Global::GetScreenSize();
		auto& commandList = g.getCommandList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);
		if (bUseMultiViewports)
		{
			float with = screenSize.x / 2;
			float height = screenSize.y / 2;
			ViewportInfo vps[4] =
			{
				ViewportInfo(0,0, with, height),
				ViewportInfo(with,0, with, height),
				ViewportInfo(0,height, with, height),
				ViewportInfo(with,height, with, height),
			};
			RHISetViewports(commandList, vps, ARRAY_SIZE(vps));
		}
		else
		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
		}
		Vector2 v[] =
		{
			0.5 * Vector2(-1,-1),
			0.5 * Vector2( 1,-1),
			0.5 * Vector2( 1, 1),
			0.5 * Vector2(-1, 1),
		};

		RHISetFixedShaderPipelineState(commandList, Matrix4::Identity(), LinearColor(1,1,1,1));
		TRenderRT<RTVF_XY>::DrawInstanced(commandList, EPrimitive::Quad, v, ARRAY_SIZE(v), bUseMultiViewports ? 4 : 1);
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
		return BaseClass::onWidgetEvent(event, id, ui);
	}

	RHIFrameBufferRef mFrameBuffer;
	RHITexture2DRef   mImage;
	bool setupRenderResource(ERenderSystem systemName) override
	{

		ShaderHelper::Get().init();

		auto screenSize = ::Global::GetScreenSize();
		mImage = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA32F, 2 * screenSize.x, 2 * screenSize.y).AddFlags(TCF_RenderTarget));

		mFrameBuffer = RHICreateFrameBuffer();
		mFrameBuffer->addTexture(*mImage);

		return true;
	}


	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1024;
		systemConfigs.screenHeight = 768;
	}

protected:
};

REGISTER_STAGE_ENTRY("MultilView OnePass", MultiViewOnePassStage, EExecGroup::FeatureDev, 1, "Render|Test");