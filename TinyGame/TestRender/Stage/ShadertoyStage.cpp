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
#include "RHI/RHIUtility.h"
#include "Json.h"

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
		float sampleRate;
		Vector4 mouse;
		//float channelTime;
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
		None,
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
			ESampler::Filter      filter;
			ESampler::AddressMode addressMode;
		};
		TArray< Channel > channels;
	};

	struct RenderPass
	{
		ShaderInput* input;
		PooledRenderTargetRef renderTarget;
		Shader shader;
	};

	char const* AlphaSeq = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

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
			frame->addButton("Restart", [this](int event, GWidget*)
			{
				restart();
				return false;
			});
			frame->addButton("Compile", [this](int event, GWidget*)
			{
				compileShader();
				return false;
			});
			restart();
			return true;
		}

		TArray< std::unique_ptr<ShaderInput> > mSourceInputs;
		TArray< std::unique_ptr<RenderPass> >  mRenderPassList;

		bool compileShader(RenderPass& pass, std::vector<uint8> const& codeTemplate, ShaderInput* commonInput)
		{
			std::string channelCode;
			int channelIndex = 0;
			for (ShaderInput::Channel const& channel : pass.input->channels)
			{
				switch (channel.type)
				{
				case EChannelType::KeyBoard:
					channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", channelIndex);
					break;
				case EChannelType::Webcam:
				case EChannelType::Micrphone:
				case EChannelType::Soundcloud:
				case EChannelType::Buffer:
				case EChannelType::Texture:
				case EChannelType::Vedio:
				case EChannelType::Music:
					channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", channelIndex);
					break;
				case EChannelType::CubeMap:
				case EChannelType::CubeTexture:
					channelCode += InlineString<>::Make("uniform samplerCube iChannel%d;\n", channelIndex);
					break;
				case EChannelType::Volume:
					channelCode += InlineString<>::Make("uniform sampler3D iChannel%d;\n", channelIndex);
					break;
				}

				++channelIndex;
			}
			std::string code;
			Text::Format((char const*)codeTemplate.data(), TArrayView< std::string const >{ channelCode, commonInput ? commonInput->code : std::string(), pass.input->code }, code);

			ShaderCompileOption option;
			return ShaderManager::Get().loadFile(pass.shader, nullptr, { EShader::Pixel, "MainPS" }, option, code.c_str());
		}

		bool loadProject(char const* name)
		{
			mSourceInputs.clear();
			std::string dir = InlineString<>::Make("Shadertoy/%s" , name );
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
					path.format("%s/Buffer%c.sgc", dir.c_str(), AlphaSeq[index]);
					break;
				case Shadertoy::EPassType::Sound:
					path.format("%s/Sound.sgc", dir.c_str());
					break;
				case Shadertoy::EPassType::CubeMap:
					path.format("%s/CubeMap%c.sgc", dir.c_str(), AlphaSeq[index]);
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

			JsonObject* inputDoc = JsonObject::LoadFromFile((dir + "/input.json").c_str());
			if (inputDoc == nullptr)
			{
				return false;
			}
			auto LoadShaderInput2 = [this, inputDoc, &dir](EPassType type, int index = 0) -> ShaderInput*
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
					path.format("%s/Buffer%c.sgc", dir.c_str(), AlphaSeq[index]);
					break;
				case Shadertoy::EPassType::Sound:
					path.format("%s/Sound.sgc", dir.c_str());
					break;
				case Shadertoy::EPassType::CubeMap:
					path.format("%s/CubeMap%c.sgc", dir.c_str(), AlphaSeq[index]);
					break;
				}

				if (!FFileSystem::IsExist(path))
					return nullptr;

				auto input = std::make_unique<ShaderInput>();
				input->passType = type;
				input->typeIndex = index;

				if (!FFileUtility::LoadToString(path, input->code))
					return nullptr;


				mSourceInputs.push_back(std::move(input));

				JsonObject* inputObject = nullptr;
				switch (type)
				{
				case Shadertoy::EPassType::None:
					inputObject = inputDoc->getObject("Common");
					break;
				case Shadertoy::EPassType::Image:
					inputObject = inputDoc->getObject("Image");
					break;
				case Shadertoy::EPassType::Buffer:
					inputObject = inputDoc->getObject(InlineString<>::Make("Buffer%c", AlphaSeq[index]));
					break;
				case Shadertoy::EPassType::Sound:
					inputObject = inputDoc->getObject("Sound");
					break;
				case Shadertoy::EPassType::CubeMap:
					inputObject = inputDoc->getObject(InlineString<>::Make("CubeMap%c", AlphaSeq[index]));
					break;
				}

				if (inputObject)
				{



				}

				return mSourceInputs.back().get();
			};
			ShaderInput* commonInput = LoadShaderInput2(EPassType::None);
			for (int i = 0; i < 4; ++i)
			{
				LoadShaderInput2(EPassType::Buffer, i);
			}
			for (int i = 0; i < 1; ++i)
			{
				LoadShaderInput2(EPassType::CubeMap, i);
			}
			LoadShaderInput2(EPassType::Sound);
			LoadShaderInput2(EPassType::Image);


			//TODO
			if (FCString::Compare(name, "PathTracing") == 0)
			{
				for (auto& input : mSourceInputs)
				{
					switch (input->passType)
					{
					case EPassType::Image:
						input->channels = {{ EChannelType::Buffer , 1 }};
						break;
					case EPassType::Buffer:
						if ( input->typeIndex == 0)
							input->channels = { { EChannelType::Buffer , 1 } , { EChannelType::None , 0 } , { EChannelType::Texture , 0 }, { EChannelType::CubeTexture , 0 } };
						else if (input->typeIndex == 1)
							input->channels = { { EChannelType::Buffer , 1 } , { EChannelType::Buffer , 0 } , { EChannelType::KeyBoard , 0 } };
						break;
					}
				}
			}

			mRenderPassList.clear();
			for (auto& input : mSourceInputs)
			{
				if (input->passType == EPassType::None )
					continue;

				auto pass = std::make_unique<RenderPass>();
				pass->input = input.get();
				mRenderPassList.push_back(std::move(pass));
			}

			return compileShader(commonInput);
		}

		bool compileShader()
		{
			int commonIndex = mSourceInputs.findIndexPred([](auto const& input)
			{
				return input->passType == EPassType::None;
			});

			return compileShader( commonIndex != INDEX_NONE ? mSourceInputs[commonIndex].get() : nullptr);
		}
		bool compileShader(ShaderInput* commonInput)
		{
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
			FMemory::Zero( mKeyBoardBuffer, sizeof(mKeyBoardBuffer));
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
		bool  bUseMultisample = false;
		bool  bPause = false;
		Vector4 mMouse = Vector4::Zero();
		uint8   mKeyBoardBuffer[256 * 2];
		RHIFrameBufferRef mFrameBuffer;
		TStructuredBuffer< InputParam > mInputBuffer;


		RHITextureBase* getTexture(EChannelType type, int index)
		{
			switch (type)
			{
			case EChannelType::Buffer:
				{
					for (auto const& pass : mRenderPassList)
					{
						if (pass->input->passType != EPassType::Buffer || pass->input->typeIndex != index)
							continue;

						if (pass->renderTarget.isValid())
						{
							return pass->renderTarget->resolvedTexture;
						}
						else
						{
							return GBlackTexture2D;
						}
						break;
					}
				}
				break;
			case EChannelType::Texture:
				return mDefaultTex2D;
			case EChannelType::CubeTexture:
				return mDefaultCube;
			case EChannelType::KeyBoard:
				return mTexKeyboard;
			case EChannelType::CubeMap:
				{
					for (auto const& pass : mRenderPassList)
					{
						if (pass->input->passType != EPassType::CubeMap || pass->input->typeIndex != index)
							continue;

						if (pass->renderTarget.isValid())
						{
							return pass->renderTarget->resolvedTexture;
						}
						else
						{
							return GBlackTextureCube;
						}
						break;
					}
				}
				break;
			case EChannelType::Webcam:
			case EChannelType::Micrphone:
			case EChannelType::Soundcloud:
			case EChannelType::Vedio:
			case EChannelType::Music:
			case EChannelType::Volume:
				break;
			}

			return nullptr;
		}

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			GRenderTargetPool.freeAllUsedElements();
			auto screenSize = ::Global::GetScreenSize();

			InputParam inputParam;
			inputParam.resolution = Vector3(screenSize.x, screenSize.y, 1);
			inputParam.time = mTime;
			inputParam.frame = mFrameCount;
			inputParam.timeDelta = mTime - mTimePrev;
			inputParam.mouse = mMouse;
			inputParam.sampleRate = 22000;
			mInputBuffer.updateBuffer(inputParam);

			mKeyBoardBuffer[EKeyCode::W] = 255;
			RHIUpdateTexture(*mTexKeyboard , 0 , 0 , 256 , 2 , mKeyBoardBuffer);

			mTimePrev = mTime;
			++mFrameCount;

			PooledRenderTargetRef rtImage;
			{
				GPU_PROFILE("RenderPass");

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				for (auto& pass : mRenderPassList)
				{
					PooledRenderTargetRef rt;

					RenderTargetDesc desc;
					desc.format = ETexture::RGBA32F;
					desc.numSamples = 1;
					desc.size = screenSize;
					switch (pass->input->passType)
					{
					case EPassType::Buffer:
						{
							InlineString<> name;
							name.format("Buffer%c", AlphaSeq[pass->input->typeIndex]);
							desc.debugName = name;
						}
						break;
					case EPassType::Image:
						{
							desc.debugName = "Image";
						}
						break;
					}

					GPU_PROFILE(desc.debugName.c_str());

					rt = GRenderTargetPool.fetchElement(desc);
					mFrameBuffer->setTexture(0, *rt->texture);
					RHISetFrameBuffer(commandList, mFrameBuffer);

					RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);
					RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

					GraphicsShaderStateDesc state;
					state.vertex = mScreenVS->getRHI();
					state.pixel = pass->shader.getRHI();

					RHISetGraphicsShaderBoundState(commandList, state);
					int channelIndex = 0;
					for (auto const& channel : pass->input->channels)
					{
						char const* channelNames[] =
						{
							"iChannel0","iChannel1","iChannel2","iChannel3",
						};

						RHITextureBase* texture = getTexture(channel.type, channel.index);
						if (texture)
						{
							pass->shader.setTexture(commandList, channelNames[channelIndex], *texture);
						}
						++channelIndex;
					}
					SetStructuredUniformBuffer(commandList, pass->shader, mInputBuffer);
					DrawUtility::ScreenRect(commandList);

					RHISetFrameBuffer(commandList, nullptr);

					if (pass->renderTarget.isValid())
					{
						pass->renderTarget->bResvered = false;
					}

					rt->bResvered = true;
					pass->renderTarget = rt;
					if (pass->input->passType == EPassType::Image)
					{
						rtImage = rt;
					}
				}
			}

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

			if (rtImage.isValid())
			{
				GPU_PROFILE("CopyImageToBuffer");
				ShaderHelper::Get().copyTextureToBuffer(commandList, *rtImage->resolvedTexture);
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
			if (msg.onLeftDown())
			{
				mMouse.x = mMouse.z = msg.getPos().x;
				mMouse.y = mMouse.w = msg.getPos().y;
			}
			else if (msg.onLeftUp())
			{
				mMouse.z = -Math::Abs(mMouse.z);
				mMouse.w = -Math::Abs(mMouse.w);
			}
			else if (msg.isLeftDown() && msg.onMoving())
			{
				mMouse.x = msg.getPos().x;
				mMouse.y = msg.getPos().y;
			}
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				if (0 <= msg.getCode() && msg.getCode() < 256)
				{
					mKeyBoardBuffer[msg.getCode()] = 255;
					mKeyBoardBuffer[msg.getCode() + 256] = 255 - mKeyBoardBuffer[msg.getCode() + 256];
				}

				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				}
			}
			else
			{
				if (0 <= msg.getCode() && msg.getCode() < 256)
				{
					mKeyBoardBuffer[msg.getCode()] = 0;
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

		RHITexture2DRef mTexKeyboard;
		RHITexture2DRef mDefaultTex2D;
		RHITextureCubeRef mDefaultCube;


		ScreenVS* mScreenVS;
		Shader    mShaderPS;

		bool setupRenderResource(ERenderSystem systemName) override
		{
			ShaderHelper::Get().init();

			mFrameBuffer = RHICreateFrameBuffer();
			mInputBuffer.initializeResource(1);

			mDefaultTex2D = RHIUtility::LoadTexture2DFromFile("Shadertoy/Assets/Wood.jpg");
			char const* cubePaths[] = 
			{
				"Shadertoy/Assets/UffiziGallery.jpg",
				"Shadertoy/Assets/UffiziGallery_1.jpg",
				"Shadertoy/Assets/UffiziGallery_2.jpg",
				"Shadertoy/Assets/UffiziGallery_3.jpg",
				"Shadertoy/Assets/UffiziGallery_4.jpg",
				"Shadertoy/Assets/UffiziGallery_5.jpg",
			};
			mDefaultCube = RHIUtility::LoadTextureCubeFromFile(cubePaths);
			mTexKeyboard = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8U , 256 , 2));

			GTextureShowManager.registerTexture("Keyborad" , mTexKeyboard);

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