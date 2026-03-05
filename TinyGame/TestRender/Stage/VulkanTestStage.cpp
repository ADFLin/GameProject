#include "Stage/TestStageHeader.h"


#include "RHI/ShaderCore.h"
#include "RHI/GpuProfiler.h"
#include "Platform/Windows/WindowsProcess.h"
#include "FileSystem.h"
#include "Core/ScopeGuard.h"

#include "RHI/VulkanCommon.h"
#include "RHI/VulkanCommand.h"
#include "RHI/VulkanShaderCommon.h"

#include "RHI/ShaderProgram.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/RHIUtility.h"

#include "DrawEngine.h"
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "Misc/Format.h"

#include "RHI/DrawUtility.h"

//#TODO REMOVEME
#pragma comment(lib , "vulkan-1.lib")

namespace RenderVulkan
{
	using namespace Meta;
	using namespace Render;
	
	class ComputeTestShader : public GlobalShader
	{
		DECLARE_SHADER(ComputeTestShader, Global);
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, DestTexture);
			BIND_SHADER_PARAM(parameterMap, FillColor);
		}
		static char const* GetShaderFileName() { return "Shader/Test/ComputeTest"; }

		DEFINE_SHADER_PARAM(DestTexture);
		DEFINE_SHADER_PARAM(FillColor);
	};
	IMPLEMENT_SHADER(ComputeTestShader, EShader::Compute, SHADER_ENTRY(MainCS));



	class SampleData
	{
	public:

		VkDevice mDevice = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT mCallback = VK_NULL_HANDLE;
		VkExtent2D     mSwapChainExtent;
		VkFormat       mSwapChainImageFormat;

		VulkanSystem* mSystem;

		bool InitBirdge()
		{
			mSystem = static_cast<VulkanSystem*>(GRHISystem);
			mDevice = mSystem->mDevice->logicalDevice;
			mSwapChainExtent = mSystem->mSwapChain->mImageSize;
			mSwapChainImageFormat = mSystem->mSwapChain->mImageFormat;

			return true;
		}


		RHIBufferRef mVertexBuffer;
		RHIBufferRef  mIndexBuffer;

		ShaderProgram mShaderProgram;
		ShaderProgram mTestSGCProgram;
		RHIInputLayoutRef mInputLayout;

		RHIFrameBufferRef mTestFrameBuffer;
		RHITexture2DRef   mTestColorTexture;

		RHITexture2DRef   mComputeTexture;
		ComputeTestShader* mComputeShader = nullptr;

		bool createSimplePipepline()
		{
			{
				// Use the swap chain render pass from VulkanSystem
				// mSimpleRenderPass is now managed by VulkanSystem::mSwapChainRenderPass
			}

			struct VertexData
			{
				Vector2 pos;
				Vector2 uv;
				Vector3 color;
			};

			VertexData v[] =
			{
				Vector2(0.5, -0.5),Vector2(1, 0),Vector3(1.0, 0.0, 0.0),
				Vector2(0.5, 0.5),Vector2(1, 1), Vector3(0.0, 1.0, 0.0),
				Vector2(-0.5, 0.5),Vector2(0, 1),Vector3(1.0, 1.0, 1.0),
				Vector2(-0.5, -0.5),Vector2(0, 0),Vector3(1.0, 1.0, 1.0),
			};
			VERIFY_RETURN_FALSE(mVertexBuffer = RHICreateVertexBuffer(sizeof(VertexData), ARRAY_SIZE(v), BCF_DefalutValue, v));
			uint32 indices[] =
			{
				0,1,2,
				0,2,3,
			};
			VERIFY_RETURN_FALSE(mIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), true , BCF_DefalutValue , indices));


			GraphicsRenderStateDesc graphicsState;
			graphicsState.primitiveType = EPrimitive::TriangleList;
			graphicsState.rasterizerState = &TStaticRasterizerState<ECullMode::None>::GetRHI();
			graphicsState.depthStencilState = &TStaticDepthStencilState<>::GetRHI();
			graphicsState.blendState = &TStaticBlendState<>::GetRHI();

			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mShaderProgram, "Shader/Test/VulkanTest", "MainVS", "MainPS"));

			std::string outCode;
			std::vector<uint8> codeTemplate;
			if (FFileUtility::LoadToBuffer("Shader/Template/Test.sgc", codeTemplate, true))
			{
				std::vector<std::string> segments = { "", "return float4(Parameters.texCoords[0], 0, 1);" };
				Text::Format((char const*)codeTemplate.data(), MakeConstView(segments), outCode);
			}

			ShaderCompileOption option;
			option.addCode(outCode.c_str());
			VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mTestSGCProgram, nullptr, { { EShader::Vertex , "ScreenVS" },{ EShader::Pixel , "MainPS" } }, option));

			RHIShaderProgram* boundShaderProgram = mShaderProgram.getRHI();
			
			texture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg");

			InputLayoutDesc desc;
			desc.addElement(0, 0, EVertex::Float2);
			desc.addElement(0, 1, EVertex::Float2);
			desc.addElement(0, 2, EVertex::Float3);
			VERIFY_RETURN_FALSE(mInputLayout = RHICreateInputLayout(desc));
			graphicsState.inputLayout = mInputLayout;

			return true;
		}

		void cleanupSimplePipeline()
		{
		}

		RHITexture2DRef texture;

		bool generateRenderCommand(VkCommandBuffer commandBuffer)
		{
			auto& drawContext = mSystem->mDrawContext;

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			// Dispatch Compute Shader first, before any render pass is started
			if (mComputeShader && mComputeTexture)
			{
				RHISetComputeShader(commandList, mComputeShader->getRHI());
				mComputeShader->setRWTexture(commandList, mComputeShader->mParamDestTexture, *mComputeTexture, 0, EAccessOp::WriteOnly);
				Vector4 fillColor(0.2f, 0.8f, 0.4f, 1.0f); // Nice green
				mComputeShader->setParam(commandList, mComputeShader->mParamFillColor, fillColor);
				RHIDispatchCompute(commandList, (mComputeTexture->getSizeX() + 15) / 16, (mComputeTexture->getSizeY() + 15) / 16, 1);
				
				// Transition texture for graphics sampling
				RHIResource* transitionResources[] = { mComputeTexture.get() };
				RHIResourceTransition(commandList, transitionResources, EResourceTransition::SRV);
			}

			// [Test] Draw to a custom render target first, BEFORE opening the swap chain pass
			{

				RHIResourceTransition(commandList, { mTestColorTexture.get() }, EResourceTransition::RenderTarget);


				RHISetFrameBuffer(commandList, mTestFrameBuffer);
				LinearColor clearColor = { 0.0f, 0.0f, 1.0f, 1.0f };
				RHIClearRenderTargets(commandList, EClearBits::Color, &clearColor, 1, 1.0f, 0);

				ViewportInfo vpTest;
				vpTest.x = 0; vpTest.y = 0;
				vpTest.w = mSwapChainExtent.width;
				vpTest.h = mSwapChainExtent.height;
				vpTest.zNear = 0; vpTest.zFar = 1;
				RHISetViewport(commandList, vpTest);
				RHISetScissorRect(commandList, 0, 0, vpTest.w, vpTest.h);

				RHISetShaderProgram(commandList, mTestSGCProgram.getRHI());
				
				DrawUtility::ScreenRect(commandList);

				//RHIResourceTransition(commandList, { mTestColorTexture.get() }, EResourceTransition::SRV);
			}

			// Switch to swap chain (back buffer). This ends the test framebuffer pass
			// and starts the swap chain pass. We leave this pass OPEN on return so
			// RHIGraphics2D (called after this function) can draw into the same pass.
			drawContext.RHISetFrameBuffer(nullptr);
			LinearColor clearColor = { 1.0f, 0.5f, 0.0f, 1.0f };
			RHIClearRenderTargets(commandList, EClearBits::Color, &clearColor, 1, 1.0f, 0);

			ViewportInfo vp;
			vp.x = 0; vp.y = 0;
			vp.w = mSwapChainExtent.width;
			vp.h = mSwapChainExtent.height;
			vp.zNear = 0; vp.zFar = 1;
			RHISetViewport(commandList, vp);
			RHISetScissorRect(commandList, 0, 0, vp.w, vp.h);

			RHISetShaderProgram(commandList, mShaderProgram.getRHI());
			ShaderParameter paramTexture;
			ShaderParameter paramTextureSampler;
			if (mShaderProgram.getRHI()->getParameter("SamplerColor", paramTexture) && 
				mShaderProgram.getRHI()->getParameter("SamplerColorSampler", paramTextureSampler))
			{
				drawContext.setShaderTexture(*mShaderProgram.getRHI(), paramTexture, *texture.get());
				drawContext.setShaderSampler(*mShaderProgram.getRHI(), paramTextureSampler, TStaticSamplerState<>::GetRHI());
			}

			ShaderParameter paramOffset;
			mShaderProgram.getRHI()->getParameter("Offset", paramOffset);

			ShaderParameter paramColor;
			mShaderProgram.getRHI()->getParameter("GlobalColor", paramColor);

			InputStreamInfo inputStream;
			inputStream.buffer = mVertexBuffer;
			inputStream.offset = 0;
			RHISetInputStream(commandList, mInputLayout, &inputStream, 1);
			RHISetIndexBuffer(commandList, mIndexBuffer);

			if (paramOffset.isBound())
			{
				Vector2 offset(0, 0);
				drawContext.setShaderValue(*mShaderProgram.getRHI(), paramOffset, &offset, 1);
			}
			if (paramColor.isBound())
			{
				Vector4 color(1, 0, 0, 1);
				drawContext.setShaderValue(*mShaderProgram.getRHI(), paramColor, &color, 1);
			}
			drawContext.RHIDrawIndexedPrimitive(EPrimitive::TriangleList, 0, 6, 0);

			if (paramOffset.isBound())
			{
				Vector2 offset(0.5, 0.5);
				drawContext.setShaderValue(*mShaderProgram.getRHI(), paramOffset, &offset, 1);
			}
			if (paramColor.isBound())
			{
				Vector4 color(0, 1, 0, 1);
				drawContext.setShaderValue(*mShaderProgram.getRHI(), paramColor, &color, 1);
			}
			RHIDrawIndexedPrimitive(commandList, EPrimitive::TriangleList, 0, 6, 0);

			return true;
		}

	};


	class TestStage : public StageBase
		            , public SampleData
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}


		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			frame->addText("aaa");

			restart();
			return true;
		}

		virtual void onEnd()
		{
			vkDeviceWaitIdle(mDevice);
			cleanupSimplePipeline();
			BaseClass::onEnd();
		}

		void restart() {}

		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);
		}


		void onRender(float dFrame)
		{
			generateRenderCommand(mSystem->mDrawContext.mActiveCmdBuffer);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
#if 1
			g.beginRender();

			//g.beginBlend(0.5);
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::White);
			g.drawRect(Vector2(100, 100), Vector2(200, 100));


			RenderUtility::SetFont(g, FONT_S24);
			g.setTextColor(Color3f(1, 0, 0));
			g.drawTexture(*mTestColorTexture, Vector2(0,0), Vector2(500,500));


			g.drawText(Vector2(10, 10), "AAAAAAAAAA");

			if (mComputeTexture)
			{
				g.drawTexture(*mComputeTexture, Vector2(mSwapChainExtent.width - 260, 10), Vector2(250, 250));
			}

			g.endRender();
#endif
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if( msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::Vulkan;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			::Global::GetDrawEngine().bUsePlatformBuffer = false;

			//VERIFY_RETURN_FALSE( initializeSystem( ::Global::GetDrawEngine().getWindow().getHWnd() ) );
			//VERIFY_RETURN_FALSE( createWindowSwapchain() );
			InitBirdge();
			VERIFY_RETURN_FALSE(createSimplePipepline());

			mTestColorTexture = RHICreateTexture2D(ETexture::RGBA8, mSwapChainExtent.width, mSwapChainExtent.height, 0, 1, TCF_RenderTarget | TCF_CreateSRV);
			mTestFrameBuffer = RHICreateFrameBuffer();
			mTestFrameBuffer->addTexture(*mTestColorTexture);

			mComputeShader = ShaderManager::Get().getGlobalShaderT<ComputeTestShader>();
			mComputeTexture = RHICreateTexture2D(ETexture::RGBA8, 512, 512, 0, 1, TCF_CreateSRV | TCF_CreateUAV);

			return true;
		}

	protected:
	};


	REGISTER_STAGE_ENTRY("Vulkan Test", TestStage, EExecGroup::FeatureDev, "Render|RHI");
}

char const* VSCode = CODE_STRING(
	#version 450
	#extension GL_ARB_separate_shader_objects : enable
	layout(location = 0) out vec3 fragColor;

	vec2 positions[3] = vec2[](
		vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5)
	);
	vec3 colors[3] = vec3[](
		vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0)
	);
	void main()
	{
		gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
		fragColor = colors[gl_VertexIndex];
	}
);