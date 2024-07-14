#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/GpuProfiler.h"

#include "Renderer/RenderTargetPool.h"
#include "RenderDebug.h"
#include "FileSystem.h"
#include "Misc/Format.h"



namespace Shadertoy
{
	using namespace Render;
	struct GPU_ALIGN InputParam
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(InputBlock);

		Vector3 resolution;
		float time;
		float timeDelta;
		int   frame;
		float frameRate;
#if 0
		float channelTime;
#endif
	};

	enum class EPassType
	{
		None,
		Image,
		Buffer,
		Sound,
		CubeMap,
	};

	enum class EChannelType
	{
		KeyBoard,
		Webcam,
		Micrphone,
		Soundcloud,
		Buffer,
		CubeMap,
		Texture,
		CubeTexture,
		Volume,
		Vedio,
		Music,
	};

	struct ShaderInput
	{
		EPassType passType;
		int typeIndex;
		std::string code;
		struct Channel
		{
			EChannelType type;
			int index;
		};
		TArray< Channel > channels;
	};

	struct RenderPass
	{
		ShaderInput* input;

		PooledRenderTargetRef renderTargets[2];
		Shader shader;
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

			auto frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Pause", bPause);
			frame->addCheckBox("Use Multisample", bUseMultisample);

			restart();
			return true;
		}

		TArray< std::unique_ptr<ShaderInput> > mSourceInputs;
		TArray< std::unique_ptr<RenderPass> >  mRenderPassList;

		bool compileShader(RenderPass& pass, std::vector<uint8> const& codeTemplate, ShaderInput* commonInput)
		{
			std::string channelCode;
			for (ShaderInput::Channel const& channel : pass.input->channels)
			{
				switch (channel.type)
				{
				case EChannelType::KeyBoard:
				case EChannelType::Webcam:
				case EChannelType::Micrphone:
				case EChannelType::Soundcloud:
				case EChannelType::Buffer:
				case EChannelType::Texture:
				case EChannelType::Vedio:
				case EChannelType::Music:
					channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", channel.index);
					break;
				case EChannelType::CubeMap:
				case EChannelType::CubeTexture:
					channelCode += InlineString<>::Make("uniform samplerCube iChannel%d;\n", channel.index);
					break;
				case EChannelType::Volume:
					channelCode += InlineString<>::Make("uniform sampler3D iChannel%d;\n", channel.index);
					break;
				}
			}
			std::string code;
			Text::Format((char const*)codeTemplate.data(), TArrayView< std::string const >{ channelCode, commonInput ? commonInput->code : std::string(), pass.input->code }, code);

			ShaderCompileOption option;
			return ShaderManager::Get().loadFile(pass.shader, nullptr, { EShader::Pixel, "MainPS" }, option, code.c_str());
		}

		bool loadProject(char const* name)
		{
			mSourceInputs.clear();
			std::string dir = InlineString<>::Make("Shadertoy/%s/" , name );
			auto LoadShaderInput = [this,&dir](EPassType type, int index = 0) -> ShaderInput*
			{
				InlineString<> path;
				switch (type)
				{
				case Shadertoy::EPassType::None:
					path.format("%s/Common.sgc", dir.c_str());
					break;
				case Shadertoy::EPassType::Image:
					path.format("%s/Image.sgc", dir.c_str());
					break;
				case Shadertoy::EPassType::Buffer:
					path.format("%s/Buffer%c.sgc", dir.c_str(), "ABCDEFG"[index]);
					break;
				case Shadertoy::EPassType::Sound:
					path.format("%s/Sound.sgc", dir.c_str());
					break;
				case Shadertoy::EPassType::CubeMap:
					path.format("%s/CubeMap%c.sgc", dir.c_str(), "ABCDEFG"[index]);
					break;
				}

				if (!FFileSystem::IsExist(path))
					return nullptr;

				auto doc = std::make_unique<ShaderInput>();
				doc->passType = type;
				doc->typeIndex = index;

				if (!FFileUtility::LoadToString(path, doc->code))
					return nullptr;


				mSourceInputs.push_back(std::move(doc));
				return mSourceInputs.back().get();
			};
			
			ShaderInput* commonInput = LoadShaderInput(EPassType::None);
			for (int i = 0; i < 4; ++i)
			{
				LoadShaderInput(EPassType::Buffer, i);
			}
			for (int i = 0; i < 1; ++i)
			{
				LoadShaderInput(EPassType::CubeMap, i);
			}
			LoadShaderInput(EPassType::Sound);
			LoadShaderInput(EPassType::Image);

			mRenderPassList.clear();
			for (auto& doc : mSourceInputs)
			{
				if ( doc->passType == EPassType::None )
					continue;

				auto pass = std::make_unique<RenderPass>();
				pass->input = doc.get();
				mRenderPassList.push_back(std::move(pass));
			}

			std::vector<uint8> codeTemplate;
			if (!FFileUtility::LoadToBuffer("Shader/Game/ShadertoyTemplate.sgc", codeTemplate, true))
			{
				LogWarning(0, "Can't load ShadertoyTemplate file");
				return false;
			}

			for (auto& pass : mRenderPassList)
			{
				if (!compileShader(*pass, codeTemplate, commonInput))
				{
					return false;
				}
			}

			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart()
		{
			mTime = 0;
			mTimePrev = 0;
			mFrameCount = 0;
			loadProject("PathTracing");
			//loadProject("ShaderArt");
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

		int   mFrameCount = 0;
		float mTime = 0;
		float mTimePrev = 0;
		bool  bUseMultisample = true;
		bool  bPause = false;
		RHIFrameBufferRef mFrameBuffer;
		TStructuredBuffer< InputParam > mInputBuffer;

		void onRender(float dFrame) override
		{
			++mFrameCount;

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			GRenderTargetPool.freeAllUsedElements();

			auto screenSize = ::Global::GetScreenSize();

			InputParam inputParam;
			inputParam.resolution = Vector3(screenSize.x, screenSize.y, 1);
			inputParam.time = mTime;
			inputParam.frame = mFrameCount;
			inputParam.timeDelta = mTime - mTimePrev;
			mTimePrev = mTime;
			mInputBuffer.updateBuffer(inputParam);

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
				GPU_PROFILE("RenderPass");

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				for (auto& pass : mRenderPassList)
				{
					GraphicsShaderStateDesc state;
					state.vertex = mScreenVS->getRHI();
					state.pixel = pass->shader.getRHI();

					RHISetGraphicsShaderBoundState(commandList, state);
					SetStructuredUniformBuffer(commandList, pass->shader, mInputBuffer);
					DrawUtility::ScreenRect(commandList);
				}
			}

#if 1
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
					rt->resolve(commandList);
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
			mInputBuffer.initializeResource(1);

			ScreenVS::PermutationDomain domainVector;
			domainVector.set<ScreenVS::UseTexCoord>(true);
			mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector);

			return true;
		}


		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::OpenGL;
		}

	protected:
	};
	
	REGISTER_STAGE_ENTRY("Shadertoy", TestStage, EExecGroup::GraphicsTest, "Render");

}//namespace Shadertoy