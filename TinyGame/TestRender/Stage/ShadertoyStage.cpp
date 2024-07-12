#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/GpuProfiler.h"

#include "Renderer/RenderTargetPool.h"
#include "RenderDebug.h"

namespace Shadertoy
{
	using namespace Render;

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

			auto frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Pause", bPause);
			frame->addCheckBox("Use Multisample", bUseMultisample);

			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart()
		{
			mTime = 0;
		}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			if (bPause == false)
			{
				mTime += float(time) / 1000;
			}

			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}

		float mTime = 0;
		bool  bUseMultisample = true;
		bool  bPause = false;
		RHIFrameBufferRef mFrameBuffer;


		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			GRenderTargetPool.freeAllUsedElements();

			auto screenSize = ::Global::GetScreenSize();

			PooledRenderTargetRef rt;
			if (bUseMultisample)
			{
				RenderTargetDesc desc;
				desc.format = ETexture::RGB10A2;
				desc.numSamples = 8;
				desc.size = screenSize;
				desc.debugName = "Canvas";
				rt = GRenderTargetPool.fetchElement(desc);
				mFrameBuffer->setTexture(0, *rt->texture);
				RHISetFrameBuffer(commandList, mFrameBuffer);
			}
			else
			{
				RHISetFrameBuffer(commandList, nullptr);
			}

			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			{
				GPU_PROFILE("DrawCanvas");

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());


				GraphicsShaderStateDesc state;
				state.vertex = mScreenVS->getRHI();
				state.pixel = mShaderPS.getRHI();

				RHISetGraphicsShaderBoundState(commandList, state);

				mShaderPS.setParam(commandList, SHADER_PARAM(Time), mTime);
				mShaderPS.setParam(commandList, SHADER_PARAM(AspectRatio), float(screenSize.x) / screenSize.y);

				DrawUtility::ScreenRect(commandList);
			}
#if 0
			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();

			Vector2 posList[] = 
			{
				Vector2(200,100),
				Vector2(100,150),
				Vector2(300,250),
			};
			g.enablePen(false);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawPolygon(posList, ARRAY_SIZE(posList));
			g.endRender();
#endif

			if (bUseMultisample)
			{
				{
					GPU_PROFILE("ResolveTexture");
					RHIResolveTexture(commandList, *rt->resolvedTexture, 0, *rt->texture, 0);
				}

				{
					GPU_PROFILE("CopyToBackBuffer");
					RHISetFrameBuffer(commandList, nullptr);
					RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
					ShaderHelper::Get().copyTextureToBuffer(commandList, *rt->resolvedTexture);
				}
			}

			RHIFlushCommand(commandList);
			GTextureShowManager.registerRenderTarget(GRenderTargetPool);
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


		bool compileShader(std::string const& code)
		{
			return ShaderManager::Get().loadFile(mShaderPS, nullptr, EShader::Pixel, "MainPS" , code.c_str());
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
			systemConfigs.bVSyncEnable = false;
		}

		ScreenVS* mScreenVS;
		Shader    mShaderPS;

		bool setupRenderResource(ERenderSystem systemName) override
		{
			ShaderHelper::Get().init();

			mFrameBuffer = RHICreateFrameBuffer();

			ScreenVS::PermutationDomain domainVector;
			domainVector.set<ScreenVS::UseTexCoord>(true);
			mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector);
			
			char const* code = R"CODE_STRING_(
#include "Common.sgc"
#include "ScreenVertexShader.sgc"

uniform float Time;
uniform float AspectRatio;

float3 palette( float t ) 
{
    float3 a = float3(0.5, 0.5, 0.5);
    float3 b = float3(0.5, 0.5, 0.5);
    float3 c = float3(1.0, 1.0, 1.0);
    float3 d = float3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

PS_ENTRY_START(MainPS)
	PS_INPUT_STRUCT(VSOutputParameters VSOutput, 0)
	PS_OUTPUT(float4 OutColor, 0)
PS_ENTRY_END(MainPS)
{
    float2 uv = VSOutput.UVs * 2.0 - 1;
	uv.x *= AspectRatio;
	
    float2 uv0 = uv;
    float3 finalColor = float3(0,0,0);
    
    for (float i = 0.0; i < 5.0; i++) 
	{
        uv = frac(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        float3 col = palette(length(uv0) + i*0.4 + Time*0.4);

        d = sin(d*8.0 + Time)/8.0;
        d = abs(d);

        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }
        
    OutColor = float4(finalColor, 1.0);
}
)CODE_STRING_";

			compileShader(code);
			return true;
		}

	protected:
	};
	
	REGISTER_STAGE_ENTRY("Shadertoy", TestStage, EExecGroup::GraphicsTest, "Render");

}//namespace Shadertoy