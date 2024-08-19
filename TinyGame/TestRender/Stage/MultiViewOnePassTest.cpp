#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "Math/PrimitiveTest.h"
#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"
#include "RenderDebug.h"

using namespace Render;

class MultiViewOnePassProgram : public ShaderProgram
{
	DECLARE_SHADER_PROGRAM(MultiViewOnePassProgram, Global);
public:
	static GlobalShaderProgram* CreateShader() { assert(0); return nullptr; }
	static void SetupShaderCompileOption(ShaderCompileOption& option) {}

	static char const* GetShaderFileName()
	{
		return "Shader/Game/MultiViewOnePass";
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
};

IMPLEMENT_SHADER_PROGRAM(MultiViewOnePassProgram);

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

	MultiViewOnePassProgram* mProgram;
	bool bUseViewportArray = true;
	bool bUseRTArray = true;
	void onRender(float dFrame) override
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();

		Vector2 screenSize = ::Global::GetScreenSize();
		auto& commandList = g.getCommandList();

		if (bUseRTArray)
		{
			mFrameBuffer->setTextureArray(0, *mCubeImage);
			RHISetFrameBuffer(commandList, mFrameBuffer);
		}
		else
		{
			RHISetFrameBuffer(commandList, nullptr);
		}

		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);

		Vector2 renderTargetSize = (bUseRTArray) ? Vector2(mCubeImage->getSize(), mCubeImage->getSize()) : screenSize;
		if (bUseViewportArray)
		{
			float with = renderTargetSize.x / 2;
			float height = renderTargetSize.y / 2;

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
			RHISetViewport(commandList, 0, 0, renderTargetSize.x, renderTargetSize.y);
		}
		Vector2 v[] =
		{
			Vector2(-1,-1),
			Vector2( 1,-1),
			Vector2( 1, 1),
			Vector2(-1, 1),
		};

		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
		RHISetShaderProgram(commandList, mProgram->getRHI());
		int numInstance = 1;
		if (bUseViewportArray)
			numInstance *= 4;
		if (bUseRTArray)
			numInstance *= 6;
		TRenderRT<RTVF_XY>::DrawInstanced(commandList, EPrimitive::Quad, v, ARRAY_SIZE(v), numInstance);

		RHISetFrameBuffer(commandList, nullptr);
		if (bUseRTArray)
		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);
			Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, 0, screenSize.y, -1, 1));
			DrawUtility::DrawCubeTexture(commandList, porjectMatrix, *mCubeImage, Vector2(0, 0), 250);
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
		return BaseClass::onWidgetEvent(event, id, ui);
	}

	RHIFrameBufferRef mFrameBuffer;

	RHITextureCubeRef mCubeImage;
	bool setupRenderResource(ERenderSystem systemName) override
	{

		ShaderHelper::Get().init();

		auto screenSize = ::Global::GetScreenSize();

		mCubeImage = RHICreateTextureCube(TextureDesc::TypeCube(ETexture::RGBA8, 1024).AddFlags(TCF_RenderTarget));
		mFrameBuffer = RHICreateFrameBuffer();
		mProgram = ShaderManager::Get().getGlobalShaderT<MultiViewOnePassProgram>();


		GTextureShowManager.registerTexture("CubeImage", mCubeImage);
		return true;
	}


	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1024;
		systemConfigs.screenHeight = 768;
	}


	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::OpenGL;
	}

protected:
};

REGISTER_STAGE_ENTRY("MultilView OnePass", MultiViewOnePassStage, EExecGroup::FeatureDev, 1, "Render|Test");