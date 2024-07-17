#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIUtility.h"

#include "Renderer/RenderTargetPool.h"
#include "RenderDebug.h"

#include "FileSystem.h"
#include "Misc/Format.h"
#include "Json.h"
#include "Core/ScopeGuard.h"

namespace Shadertoy
{
	static uint32 AlignCount(uint32 size, uint32 alignment)
	{
		return (size + alignment - 1) / alignment;
	}

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
		Buffer,
		Sound,
		CubeMap,
		Image,
	};

	enum class EInputType
	{
		Keyboard,
		Webcam,
		Micrphone,
		Soundcloud,
		Buffer,
		CubeBuffer,
		Texture,
		CubeMap,
		Volume,
		Vedio,
		Music,

		COUNT,
	};

	struct RenderInput
	{
		EInputType type;
		int id;
		int channel;

		ESampler::Filter      filter;
		ESampler::AddressMode addressMode;
	};

	struct RenderOutput
	{
		int id;
		int channel;
	};


	struct RenderPassInfo
	{
		EPassType passType;
		int typeIndex;
		std::string code;

		TArray<RenderInput>  inputs;
		TArray<RenderOutput> outputs;
	};

	class RenderPassShader : public Shader
	{
	public:
		void bindInputParameters(TArray<RenderInput> const& inputs)
		{
			mParamInputs.resize(inputs.size());
			mParamInputSamplers.resize(inputs.size());
			int index = 0;
			for (auto const& input : inputs)
			{
				getParameter(InlineString<>::Make("iChannel%d", input.channel), mParamInputs[index]);
				++index;
			}
		}

		static RHISamplerState& GetSamplerState(RenderInput const& input)
		{
			if (input.type == EInputType::Texture)
				return TStaticSamplerState< ESampler::Bilinear, ESampler::Warp, ESampler::Warp >::GetRHI();
			return TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
		}

		template< typename TGetTexture >
		void setInputParameters(RHICommandList& commandList, TArray<RenderInput> const& inputs, TGetTexture& GetTexture)
		{
			int Index = 0;
			for (auto const& input : inputs)
			{
				RHITextureBase* texture = GetTexture(input.type, input.id);
				if (texture)
				{
					setTexture(commandList, mParamInputs[Index], *texture, mParamInputSamplers[Index], GetSamplerState(input));
				}
				++Index;
			}
		}
		TArray< ShaderParameter > mParamInputs;
		TArray< ShaderParameter > mParamInputSamplers;
	};


	struct RenderPassData
	{
		RenderPassInfo* info;
		RenderPassShader shader;
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
			frame->addCheckBox("Use ComputeShader", [this](int event, GWidget*)
			{
				bUseComputeShader = !bUseComputeShader;
				compileShader();
				return false;
			})->bChecked = bUseComputeShader;
			
			restart(true);
			return true;
		}

		TArray< std::unique_ptr<RenderPassInfo> > mRenderPassInfos;
		TArray< std::unique_ptr<RenderPassData> > mRenderPassList;
		TArray< PooledRenderTargetRef > mOutputBuffers;

		bool compileShader(RenderPassData& pass, std::vector<uint8> const& codeTemplate, RenderPassInfo* commonInput)
		{
			std::string channelCode;
			for (RenderInput const& input : pass.info->inputs)
			{
				switch (input.type)
				{
				case EInputType::Keyboard:
					channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", input.channel);
					break;
				case EInputType::Webcam:
				case EInputType::Micrphone:
				case EInputType::Soundcloud:
				case EInputType::Buffer:
				case EInputType::Texture:
				case EInputType::Vedio:
				case EInputType::Music:
					channelCode += InlineString<>::Make("uniform sampler2D iChannel%d;\n", input.channel);
					break;
				case EInputType::CubeBuffer:
				case EInputType::CubeMap:
					channelCode += InlineString<>::Make("uniform samplerCube iChannel%d;\n", input.channel);
					break;
				case EInputType::Volume:
					channelCode += InlineString<>::Make("uniform sampler3D iChannel%d;\n", input.channel);
					break;
				}
			}
			std::string code;
			Text::Format((char const*)codeTemplate.data(), TArrayView< std::string const >{ channelCode, commonInput ? commonInput->code : std::string(), pass.info->code }, code);

			ShaderCompileOption option;
			if (!ShaderManager::Get().loadFile(pass.shader, nullptr, { bUseComputeShader ? EShader::Compute : EShader::Pixel, bUseComputeShader ? "MainCS" : "MainPS" }, option, code.c_str()))
				return false;

			pass.shader.bindInputParameters(pass.info->inputs);
			return true;
		}


		bool parseWebFile(JsonFile& webFile)
		{
			if (!webFile.isArray())
				return false;
			auto webArray = webFile.getArray();
			if ( webArray.empty())
				return false;

			auto webObject = webArray[0].asObject();

			auto renderPassArray = webObject.getArray("renderpass");

			std::unordered_map< std::string, int > nameToIdMap;

			int nextIdMap[ (int)EInputType::COUNT ];
			std::fill_n( nextIdMap , ARRAY_SIZE(nextIdMap), 0);

			auto ParseOutput = [&](JsonObject& jsonObject, RenderOutput& output) -> bool
			{
				std::string idName;
				if (!jsonObject.tryGet("id", idName))
					return false;

				auto iter = nameToIdMap.find(idName);
				if (iter != nameToIdMap.end())
				{
					output.id = iter->second;
				}
				else
				{
					output.id = nextIdMap[(int)EInputType::Buffer];
					++nextIdMap[(int)EInputType::Buffer];
					nameToIdMap.emplace(idName, output.id);
				}

				if (!jsonObject.tryGet("channel", output.channel))
					return false;

				return true;
			};

			auto ParseInput = [&](JsonObject& jsonObject, RenderInput& input) -> bool
			{
				std::string typeName;
				if (!jsonObject.tryGet("type", typeName))
					return false;

				if (typeName == "buffer")
				{
					input.type = EInputType::Buffer;
				}
				else if (typeName == "texture")
				{
					input.type = EInputType::Texture;
				}
				else if (typeName == "cubemap")
				{
					input.type = EInputType::CubeMap;
				}
				else if (typeName == "keyboard")
				{
					input.type = EInputType::Keyboard;
				}

				std::string idName;
				if (!jsonObject.tryGet("id", idName))
					return false;

				auto iter = nameToIdMap.find(idName);
				if (iter != nameToIdMap.end())
				{
					input.id = iter->second;
				}
				else
				{
					input.id = nextIdMap[(int)input.type];
					++nextIdMap[(int)input.type];
					nameToIdMap.emplace(idName , input.id); 
				}


				if (!jsonObject.tryGet("channel", input.channel))
					return false;

				JsonObject samplerObject = jsonObject.getObject("sampler");
				if (samplerObject.isVaild())
				{

				}
				else
				{


				}

				return true;
			};

			int bufferIndex = 0;
			auto ParseRenderPass = [&](JsonObject& jsonObject, RenderPassInfo& info) -> bool
			{
				if (!jsonObject.tryGet("code", info.code))
					return false;

				std::string typeName;
				if (!jsonObject.tryGet("type", typeName))
					return false;

				if (typeName == "common")
				{
					info.passType = EPassType::None;
					info.typeIndex = 0;
				}
				else if (typeName == "buffer")
				{
					info.passType = EPassType::Buffer;
					info.typeIndex = bufferIndex++;
				}
				else if (typeName == "image")
				{
					info.passType = EPassType::Image;
					info.typeIndex = 0;
				}
				else
				{
					return false;
				}

				TArray<JsonValue> inputArray = jsonObject.getArray("inputs");
				for (auto const& inputValue : inputArray)
				{
					if (!inputValue.isObject())
						return false;
					RenderInput input;
					if (!ParseInput(inputValue.asObject(), input) )
						return false;

					info.inputs.push_back(input);
				}
				TArray<JsonValue> outputArray = jsonObject.getArray("outputs");
				for (auto const& outputValue : outputArray)
				{
					if (!outputValue.isObject())
						return false;
					RenderOutput output;
					if (!ParseOutput(outputValue.asObject(), output))
						return false;

					info.outputs.push_back(output);
				}

				return true;
			};

			for (auto const& renderPassValue : renderPassArray)
			{
				if (!renderPassValue.isObject())
					return false;

				auto info = std::make_unique<RenderPassInfo>();
				if (!ParseRenderPass(renderPassValue.asObject(), *info))
				{
					return false;
				}

				mRenderPassInfos.push_back(std::move(info));
			}

			std::stable_sort( mRenderPassInfos.begin() , mRenderPassInfos.end() ,
				[](auto const& lhs, auto const& rhs)
				{
					if (lhs->passType == rhs->passType)
						return lhs->typeIndex < rhs->typeIndex;

					return lhs->passType < rhs->passType;
				}
			);

			return true;
		}

		bool loadProject(char const* name)
		{
			mRenderPassInfos.clear();
			mRenderPassList.clear();
#if 0
			JsonFile* webDoc = JsonFile::Load(InlineString<>::Make("Shadertoy/%s.json", name));
			if ( webDoc )
			{
				ON_SCOPE_EXIT
				{
					webDoc->release();
				};

				if ( !parseWebFile(*webDoc) )
					return false;
			}
			else
#endif
			{
				std::string dir = InlineString<>::Make("Shadertoy/%s", name);

				JsonFile* inputDoc = JsonFile::Load((dir + "/input.json").c_str());

				ON_SCOPE_EXIT
				{
					if (inputDoc)
					{
						inputDoc->release();
					}
				};

				auto LoadRenderPassInfo = [this, inputDoc, &dir](EPassType type, int index = 0) -> RenderPassInfo*
				{
					InlineString<> path;
					switch (type)
					{
					case EPassType::None:
						path.format("%s/Common.sgc", dir.c_str());
						break;
					case EPassType::Image:
						path.format("%s/Image.sgc", dir.c_str());
						break;
					case EPassType::Buffer:
						path.format("%s/Buffer%c.sgc", dir.c_str(), AlphaSeq[index]);
						break;
					case EPassType::Sound:
						path.format("%s/Sound.sgc", dir.c_str());
						break;
					case EPassType::CubeMap:
						path.format("%s/CubeMap%c.sgc", dir.c_str(), AlphaSeq[index]);
						break;
					}

					if (!FFileSystem::IsExist(path))
						return nullptr;

					auto info = std::make_unique<RenderPassInfo>();
					info->passType = type;
					info->typeIndex = index;

					if (!FFileUtility::LoadToString(path, info->code))
						return nullptr;

					if (inputDoc && inputDoc->isObject())
					{
						auto inputDocObject = inputDoc->getObject();

						JsonObject inputObject;
						switch (type)
						{
						case EPassType::None:
							inputObject = inputDocObject.getObject("Common");
							break;
						case EPassType::Image:
							inputObject = inputDocObject.getObject("Image");
							break;
						case EPassType::Buffer:
							inputObject = inputDocObject.getObject(InlineString<>::Make("Buffer%c", AlphaSeq[index]));
							break;
						case EPassType::Sound:
							inputObject = inputDocObject.getObject("Sound");
							break;
						case EPassType::CubeMap:
							inputObject = inputDocObject.getObject(InlineString<>::Make("CubeMap%c", AlphaSeq[index]));
							break;
						}

						if (inputObject.isVaild())
						{
							TArray<JsonValue> inputValues = inputObject.getArray("Inputs");
							if (inputValues.size())
							{
								for (auto inputValue : inputValues)
								{
									int id = 0;
									JsonObject inputObject = inputValue.asObject();
									if (inputObject.isVaild())
									{
										RenderInput input;
										std::string typeName;
										if (inputObject.tryGet("Type", typeName))
										{
											if (typeName == "Buffer")
											{
												input.type = EInputType::Buffer;
											}
											else if (typeName == "Texture")
											{
												input.type = EInputType::Texture;
											}
											else if (typeName == "CubeTexture")
											{
												input.type = EInputType::CubeMap;
											}
											else if (typeName == "Keyboard")
											{
												input.type = EInputType::Keyboard;
											}
										}
										if (!inputObject.tryGet("ID", input.id))
										{
											input.id = 0;
										}
										if (!inputObject.tryGet("Channel", input.channel))
										{
											input.channel = 0;
										}
										info->inputs.push_back(input);
									}
								}
							}
						}
					}

					if (type == EPassType::Buffer || type == EPassType::Image)
					{
						RenderOutput output;
						output.channel = 0;
						output.id = index;
						info->outputs.push_back(output);
					}
					mRenderPassInfos.push_back(std::move(info));
					return mRenderPassInfos.back().get();
				};

				LoadRenderPassInfo(EPassType::None);
				for (int i = 0; i < 4; ++i)
				{
					LoadRenderPassInfo(EPassType::Buffer, i);
				}
				for (int i = 0; i < 1; ++i)
				{
					LoadRenderPassInfo(EPassType::CubeMap, i);
				}
				LoadRenderPassInfo(EPassType::Sound);
				LoadRenderPassInfo(EPassType::Image);
			}

			mRenderPassList.clear();
			int maxBufferId = -1;
			for (auto& info : mRenderPassInfos)
			{
				if ( info->passType == EPassType::None )
					continue;

				auto pass = std::make_unique<RenderPassData>();
				pass->info = info.get();
				mRenderPassList.push_back(std::move(pass));

				if ( info->passType == EPassType::Buffer )
				{
					for( auto const& output : info->outputs)
					{
						maxBufferId = Math::Max(maxBufferId , output.id);
					}
				}
			}

			mOutputBuffers.resize(maxBufferId + 1);
			return true;
		}

		bool compileShader()
		{
			int commonIndex = mRenderPassInfos.findIndexPred([](auto const& input)
			{
				return input->passType == EPassType::None;
			});

			return compileShader( commonIndex != INDEX_NONE ? mRenderPassInfos[commonIndex].get() : nullptr);
		}
		bool compileShader(RenderPassInfo* commonInput)
		{
			std::vector<uint8> codeTemplate;
			if (!FFileUtility::LoadToBuffer("Shader/Game/ShadertoyTemplate.sgc", codeTemplate, true))
			{
				LogWarning(0, "Can't load ShadertoyTemplate file");
				return false;
			}

			int indexPass = 0;
			for (auto& pass : mRenderPassList)
			{
				if (!compileShader(*pass, codeTemplate, commonInput))
				{
					LogWarning(0, "Compile RenderPass Fail index = %d" , indexPass);
					return false;
				}
				++indexPass;
			}

			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart(bool bInit = false)
		{
			mTime = 0;
			mTimePrev = 0;
			mFrameCount = 0;
			FMemory::Zero( mKeyBoardBuffer, sizeof(mKeyBoardBuffer));
			loadProject("PathTracing");
			//loadProject("ShaderArt");
			if (!bInit)
			{
				compileShader();
			}
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


		RHITextureBase* getInputTexture(EInputType type, int id)
		{
			switch (type)
			{
			case EInputType::Buffer:
				{
					if (mOutputBuffers[id].isValid())
					{
						return mOutputBuffers[id]->resolvedTexture;
					}
					else
					{
						return GBlackTexture2D;
					}
				}
				break;
			case EInputType::Texture:
				return mDefaultTex2D;
			case EInputType::CubeMap:
				return mDefaultCube;
			case EInputType::Keyboard:
				return mTexKeyboard;
			case EInputType::CubeBuffer:
				{

					return GBlackTextureCube;

				}
				break;
			case EInputType::Webcam:
			case EInputType::Micrphone:
			case EInputType::Soundcloud:
			case EInputType::Vedio:
			case EInputType::Music:
			case EInputType::Volume:
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

			RHIUpdateTexture(*mTexKeyboard , 0 , 0 , 256 , 2 , mKeyBoardBuffer);

			mTimePrev = mTime;
			++mFrameCount;

			PooledRenderTargetRef rtImage;
			{
				GPU_PROFILE(bUseComputeShader ? "RenderPassCS" : "RenderPass");

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				for (auto& pass : mRenderPassList)
				{
					TArray< PooledRenderTargetRef , TInlineAllocator<4> > outputBuffers;
					HashString debugName;
					int index;

					index = 0;
					for (auto const& output : pass->info->outputs)
					{
						RenderTargetDesc desc;
						desc.format = ETexture::RGBA32F;
						desc.numSamples = 1;
						desc.size = screenSize;

						if (index == 0)
						{
							switch (pass->info->passType)
							{
							case EPassType::Buffer:
							{
								desc.debugName = InlineString<>::Make("Buffer%c", AlphaSeq[pass->info->typeIndex]);;
							}
							break;
							case EPassType::Image:
							{
								desc.debugName = "Image";
							}
							break;
							}
							debugName = desc.debugName;
						}
						else
						{

							switch (pass->info->passType)
							{
							case EPassType::Buffer:
							{
								InlineString<> name;
								desc.debugName = InlineString<>::Make("Buffer%c(%d)", AlphaSeq[pass->info->typeIndex], output.channel);
							}
							break;
							case EPassType::Image:
							{
								desc.debugName = InlineString<>::Make("Image(%d)", output.channel);;
							}
							break;
							}

						}

						if (bUseComputeShader)
						{
							desc.creationFlags |= TCF_CreateUAV;
						}
						PooledRenderTargetRef rt;
						rt = GRenderTargetPool.fetchElement(desc);
						outputBuffers.push_back(rt);
						++index;
					}
	

					GPU_PROFILE(debugName.c_str());

					if (bUseComputeShader)
					{
						RHISetComputeShader(commandList, pass->shader.getRHI());

						pass->shader.setInputParameters(commandList, pass->info->inputs,
							[this](EInputType type, int index) { return getInputTexture(type, index); }
						);
						SetStructuredUniformBuffer(commandList, pass->shader, mInputBuffer);

						char const* OutputNames[] = { "OutTexture", "OutTexture1" , "OutTexture2" , "OutTexture3" };
						index = 0;
						for (auto const& output : pass->info->outputs)
						{
							pass->shader.setRWTexture(commandList, OutputNames[output.channel], *outputBuffers[index]->resolvedTexture, EAccessOperator::AO_WRITE_ONLY);
							++index;
						}

						int const GROUP_SIZE = 8;

						RHIDispatchCompute(commandList, AlignCount(screenSize.x, GROUP_SIZE), AlignCount(screenSize.y, GROUP_SIZE), 1);

						index = 0;
						for (auto const& output : pass->info->outputs)
						{
							pass->shader.clearRWTexture(commandList, OutputNames[output.channel]);
							++index;
						}
					}
					else
					{
						index = 0;
						for (auto const& output : pass->info->outputs)
						{
							mFrameBuffer->setTexture(output.channel, *outputBuffers[index]->texture);
							++index;
						}

						RHISetFrameBuffer(commandList, mFrameBuffer);
						RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

						GraphicsShaderStateDesc state;
						state.vertex = mScreenVS->getRHI();
						state.pixel = pass->shader.getRHI();

						RHISetGraphicsShaderBoundState(commandList, state);

						pass->shader.setInputParameters(commandList, pass->info->inputs,
							[this](EInputType type, int index) { return getInputTexture(type, index); }
						);
						SetStructuredUniformBuffer(commandList, pass->shader, mInputBuffer);
						DrawUtility::ScreenRect(commandList);

						RHISetFrameBuffer(commandList, nullptr);
					}

					if (pass->info->passType == EPassType::Image)
					{
						rtImage = outputBuffers[0];
					}
					else
					{
						index = 0;
						for (auto const& output : pass->info->outputs)
						{
							if (mOutputBuffers[output.id].isValid())
							{
								mOutputBuffers[output.id]->desc.debugName = EName::None;
								mOutputBuffers[output.id]->bResvered = false;
							}

							outputBuffers[index]->bResvered = true;
							mOutputBuffers[output.id] = outputBuffers[index];
							++index;
						}
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

		bool bUseComputeShader = true;
		ScreenVS* mScreenVS;


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
			mTexKeyboard = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8 , 256 , 2));

			GTextureShowManager.registerTexture("Keyborad" , mTexKeyboard);

			ScreenVS::PermutationDomain domainVector;
			domainVector.set<ScreenVS::UseTexCoord>(false);
			mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector);

			compileShader();

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