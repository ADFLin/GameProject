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
#include "Serialize/FileStream.h"

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

		Vector3 iResolution;
		float  iTime;
		float  iTimeDelta;
		int    iFrame;
		float  iFrameRate;
		float  iSampleRate;
		Vector4 iMouse;
		Vector4 iDate;
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
		bool bVFlip = false;
		ESampler::Filter      filter = ESampler::Bilinear;
		ESampler::AddressMode addressMode = ESampler::Clamp;
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

			getParameter("iChannelResolution", mParamChannelResolution);
		}

		static RHISamplerState& GetSamplerState(RenderInput const& input)
		{
			switch (input.filter)
			{
			default:
				break;
			case ESampler::Point:
				switch (input.addressMode)
				{
				case ESampler::Wrap:
					return TStaticSamplerState< ESampler::Point, ESampler::Wrap, ESampler::Wrap, ESampler::Wrap>::GetRHI();
				case ESampler::Clamp:
					return TStaticSamplerState< ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
				}
				break;
			case ESampler::Bilinear:
				switch (input.addressMode)
				{
				case ESampler::Wrap:
					return TStaticSamplerState< ESampler::Bilinear, ESampler::Wrap, ESampler::Wrap, ESampler::Wrap>::GetRHI();
				case ESampler::Clamp:
					return TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
				}
				break;
			case ESampler::Trilinear:
				switch (input.addressMode)
				{
				case ESampler::Wrap:
					return TStaticSamplerState< ESampler::Trilinear, ESampler::Wrap, ESampler::Wrap, ESampler::Wrap>::GetRHI();
				case ESampler::Clamp:
					return TStaticSamplerState< ESampler::Trilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
				}
				break;
			}

			return TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI();
		}

		template< typename TGetTexture >
		void setInputParameters(RHICommandList& commandList, TArray<RenderInput> const& inputs, TGetTexture& GetTexture)
		{
			Vector3 channelSize[4] = { Vector3(1,0,0) , Vector3(0,1,0), Vector3(0,0,1) , Vector3(1,1,1) };
			int Index = 0;
			for (auto const& input : inputs)
			{
				RHITextureBase* texture = GetTexture(input.type, input.id);
				if (texture)
				{
					setTexture(commandList, mParamInputs[Index], *texture, mParamInputSamplers[Index], GetSamplerState(input));
					channelSize[input.channel] = Vector3(texture->getDesc().dimension);
				}
				++Index;
			}

			if (mParamChannelResolution.isBound())
			{
				setParam(commandList, mParamChannelResolution, channelSize, 4);
			}
		}
		ShaderParameter mParamChannelResolution;
		TArray< ShaderParameter > mParamInputs;
		TArray< ShaderParameter > mParamInputSamplers;
	};

	RHITexture3D* LoadBinTexture(char const* path, bool bGenMipmap)
	{
		InputFileSerializer serializer;
		if (!serializer.openNoHeader(path))
			return nullptr;

		struct Header
		{
			char magic[4];
			uint32 sizeX;
			uint32 sizeY;
			uint32 sizeZ;
			uint32 pixelByte;
		};
		Header header;
		serializer >> header; 

		TArray<uint8> data;

		data.resize(header.sizeX * header.sizeY * header.sizeZ * header.pixelByte);
		serializer.read(data.data(), data.size());
		ETexture::Format format;


		int level = 1;
		if ( bGenMipmap )
		{
			level = RHIUtility::CalcMipmapLevel( Math::Min(header.sizeX , Math::Min(header.sizeY , header.sizeZ)) );
		}

		switch (header.pixelByte)
		{
		case 1: format = ETexture::R8; break;
		//case 2: format = ETexture::RG8; break;
		case 3: format = ETexture::RGB8; break;
		case 4: format = ETexture::RGBA8; break;
		}

		return RHICreateTexture3D(TextureDesc::Type3D(format, header.sizeX, header.sizeY, header.sizeZ).MipLevel(level).AddFlags(TCF_GenerateMips), data.data());
	}


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
			auto choice = frame->addChoice();
			choice->addItem("Test");
			FileIterator fileIter;
			if ( FFileSystem::FindFiles("Shadertoy", ".json" , fileIter) )
			{
				for (;fileIter.haveMore(); fileIter.goNext())
				{
					choice->addItem(FFileUtility::GetBaseFileName(fileIter.getFileName()).toMutableCString());
				}
			}
			choice->onEvent = [this](int event, GWidget* widget)->bool
			{
				loadProject(widget->cast<GChoice>()->getSelectValue());
				return false;
			};
			frame->addButton("Reset Params", [this](int event, GWidget*)
			{
				resetInputParams();
				return false;
			});
			frame->addCheckBox("Pause", bPause);
			frame->addCheckBox("Use Multisample", bUseMultisample);
			frame->addCheckBox("Use ComputeShader", [this](int event, GWidget*)
			{
				bAllowComputeShader = !bAllowComputeShader;
				compileShader();
				return false;
			})->bChecked = bAllowComputeShader;

			return true;
		}

		TArray< std::unique_ptr<RenderPassInfo> > mRenderPassInfos;
		TArray< std::unique_ptr<RenderPassData> > mRenderPassList;
		TArray< PooledRenderTargetRef > mOutputBuffers;
		TArray< RHITextureRef > mInputResources;

		bool compileShader(RenderPassData& pass, std::vector<uint8> const& codeTemplate, RenderPassInfo* commonInfo)
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
			Text::Format((char const*)codeTemplate.data(), { StringView(channelCode), commonInfo ? StringView(commonInfo->code) : StringView(), StringView(pass.info->code) }, code);

			ShaderCompileOption option;
			bool bUseComputeShader = canUseComputeShader(pass);
			if (!ShaderManager::Get().loadFile(pass.shader, nullptr, { bUseComputeShader ? EShader::Compute : EShader::Pixel, bUseComputeShader ? "MainCS" : "MainPS" }, option, code.c_str()))
				return false;

			pass.shader.bindInputParameters(pass.info->inputs);
			return true;
		}



		bool loadResource(RenderInput const& input, std::string const& filePath)
		{

			if (mInputResources.isValidIndex(input.id) && mInputResources[input.id].isValid())
				return true;

			std::string loadPath = std::string("Shadertoy") + filePath;

			auto AddResource = [this, &input](RHITextureBase& texture)
			{
				if (mInputResources.size() < input.id + 1)
				{
					mInputResources.resize(input.id + 1);
				}
				mInputResources[input.id] = &texture;
				GTextureShowManager.registerTexture( InlineString<>::Make("Res%d" , input.id ).c_str(), &texture);
			};

			switch (input.type)
			{
			case EInputType::Keyboard:
			case EInputType::Webcam:
			case EInputType::Micrphone:
			case EInputType::Soundcloud:
			case EInputType::Buffer:
			case EInputType::CubeBuffer:
				break;
			case EInputType::Texture:
				{
					TextureLoadOption option;
					option.FlipV(input.bVFlip);
					option.bAutoMipMap = input.filter == ESampler::Trilinear;
					RHITexture2DRef texture = RHIUtility::LoadTexture2DFromFile(loadPath.c_str(), option);
					if (!texture.isValid())
						return false;

					AddResource(*texture);
				}
				break;
			case EInputType::CubeMap:
				{
					char const* extension = FFileUtility::GetExtension(loadPath.c_str());
					if (extension)
						--extension;

					StringView pathName = StringView(loadPath.c_str(), extension - loadPath.c_str());

					std::string facePath[5];
					char const* cubePaths[6];
					cubePaths[0] = loadPath.c_str();
					for (int i = 0; i < 5; ++i)
					{
						facePath[i] = InlineString<>::Make("%s_%d%s", (char const*)pathName.toCString(), i + 1 , extension).c_str();
						cubePaths[i + 1] = facePath[i].c_str();
					}

					TextureLoadOption option;
					option.bAutoMipMap = input.filter == ESampler::Trilinear;
					RHITextureCubeRef texture = RHIUtility::LoadTextureCubeFromFile(cubePaths, option);
					if (!texture.isValid())
						return false;

					AddResource(*texture);
				}
				break;
			case EInputType::Volume:
				{
					RHITexture3DRef texture = LoadBinTexture(loadPath.c_str(), input.filter == ESampler::Trilinear );

					if (!texture.isValid())
						return false;

					AddResource(*texture);
				}
				break;
			case EInputType::Vedio:
			case EInputType::Music:
				break;
			}

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

			int nextBufferId = 0;
			int nextResourceId = 0;
			auto ParseOutput = [&](JsonObject& jsonObject, RenderOutput& output) -> bool
			{
				if (!jsonObject.tryGet("channel", output.channel))
					return false;

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
					output.id = nextBufferId++;
					nameToIdMap.emplace(idName, output.id);
				}

				return true;
			};

			auto ParseInput = [&](JsonObject& jsonObject, RenderInput& input) -> bool
			{

				if (!jsonObject.tryGet("channel", input.channel))
					return false;

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
				else if (typeName == "volume")
				{
					input.type = EInputType::Volume;
				}
				else if (typeName == "keyboard")
				{
					input.type = EInputType::Keyboard;
				}

				JsonObject samplerObject = jsonObject.getObject("sampler");
				if (samplerObject.isVaild())
				{
					samplerObject.tryGet("vflip", input.bVFlip);

					std::string name;
					if (samplerObject.tryGet("filter", name))
					{
						if (name == "linear")
						{
							input.filter = ESampler::Bilinear;
						}
						else if (name == "nearest")
						{
							input.filter = ESampler::Point;
						}
						else if (name == "mipmap")
						{
							input.filter = ESampler::Trilinear;
						}
					}

					if (samplerObject.tryGet("wrap", name))
					{
						if (name == "clamp")
						{
							input.addressMode = ESampler::Clamp;
						}
						else if (name == "repeat")
						{
							input.addressMode = ESampler::Wrap;
						}
					}
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
					switch (input.type)
					{
					case EInputType::Buffer:
					case EInputType::CubeBuffer:
						input.id = nextBufferId++;
						break;
					case EInputType::Texture:
					case EInputType::CubeMap:
					case EInputType::Volume:
						{
							input.id = nextResourceId++;
							std::string filePath;
							if (!jsonObject.tryGet("filepath", filePath))
								return false;

							if (!loadResource(input, filePath))
							{
								LogWarning(0, "LoadResource Fial");
								return false;
							}
						}
						break;
					case EInputType::Vedio:
					case EInputType::Music:
					case EInputType::Keyboard:
					case EInputType::Webcam:
					case EInputType::Micrphone:
					case EInputType::Soundcloud:
						input.id = 0;
						break;
					}

					nameToIdMap.emplace(idName , input.id); 
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
				else if (typeName == "sound")
				{
					info.passType = EPassType::Sound;
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

			mOutputBuffers.resize(nextBufferId);
			return true;
		}

		bool loadProject(char const* name)
		{
			if (!loadProjectActual(name) || !compileShader())
			{
				mRenderPassList.clear();
				mOutputBuffers.clear();
				mInputResources.clear();
				return false;
			}

			resetInputParams();
			return true;
		}

		bool loadProjectActual(char const* name)
		{
			mRenderPassInfos.clear();
			mRenderPassList.clear();
			mOutputBuffers.clear();
			mInputResources.clear();

#if 1
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

				int maxBufferId = -1;
				for (auto& info : mRenderPassInfos)
				{
					if (info->passType == EPassType::Buffer)
					{
						for (auto const& output : info->outputs)
						{
							maxBufferId = Math::Max(maxBufferId, output.id);
						}
					}
				}
				mOutputBuffers.resize(maxBufferId + 1);
			}

			for (auto& info : mRenderPassInfos)
			{
				if (info->passType == EPassType::None)
					continue;
				if (info->passType == EPassType::Sound)
					continue;
				auto pass = std::make_unique<RenderPassData>();
				pass->info = info.get();
				mRenderPassList.push_back(std::move(pass));
			}

			return true;
		}

		void resetInputParams()
		{
			mTime = 0;
			mTimePrev = 0;
			mFrameCount = 0;
			FMemory::Zero(mKeyBoardBuffer, sizeof(mKeyBoardBuffer));
		}

		bool compileShader()
		{
			int commonIndex = mRenderPassInfos.findIndexPred([](auto const& input)
			{
				return input->passType == EPassType::None;
			});

			return compileShader( commonIndex != INDEX_NONE ? mRenderPassInfos[commonIndex].get() : nullptr);
		}
		bool compileShader(RenderPassInfo* commonInfo)
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
				if (!compileShader(*pass, codeTemplate, commonInfo))
				{
					LogWarning(0, "Compile RenderPass Fail index = %d" , indexPass);
					return false;
				}
				++indexPass;
			}

			return true;
		}

		void initAudio()
		{




		}

		void onEnd() override
		{
			BaseClass::onEnd();
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
				if (mOutputBuffers[id].isValid())
				{
					return mOutputBuffers[id]->resolvedTexture;
				}	
				return GBlackTexture2D;
			case EInputType::Texture:
				if (mInputResources.isValidIndex(id))
				{
					return mInputResources[id];
				}
				return mDefaultTex2D;
			case EInputType::CubeMap:
				if (mInputResources.isValidIndex(id))
				{
					return mInputResources[id];
				}
				return mDefaultCube;
			case EInputType::Volume:
				if (mInputResources.isValidIndex(id))
				{
					return mInputResources[id];
				}
				return GBlackTexture3D;
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
			inputParam.iResolution = Vector3(screenSize.x, screenSize.y, 1);
			inputParam.iTime = mTime;
			inputParam.iFrame = mFrameCount;
			inputParam.iTimeDelta = mTime - mTimePrev;
			inputParam.iMouse = mMouse;
			inputParam.iSampleRate = 22000;

			DateTime now = SystemPlatform::GetLocalTime();
			inputParam.iDate = Vector4( now.getYear() , now.getMonth() , now.getDay(), now.getSecond() );
			mInputBuffer.updateBuffer(inputParam);

			RHIUpdateTexture(*mTexKeyboard , 0 , 0 , 256 , 2 , mKeyBoardBuffer);

			mTimePrev = mTime;
			++mFrameCount;

			PooledRenderTargetRef rtImage;
			{
				GPU_PROFILE(bAllowComputeShader ? "RenderPassCS" : "RenderPass");

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				for (auto& pass : mRenderPassList)
				{
					bool bUseComputeShader = canUseComputeShader(*pass);

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

					int subPassCount = pass->info->passType == EPassType::CubeMap ? 6 : 1;

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
							pass->shader.setRWTexture(commandList, OutputNames[output.channel], *outputBuffers[index]->resolvedTexture, 0, EAccessOp::WriteOnly);
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
						for (int subPassIndex = 0; subPassIndex < subPassCount; ++subPassIndex)
						{
							index = 0;
							for (auto const& output : pass->info->outputs)
							{
								mFrameBuffer->setTexture(output.channel, *outputBuffers[index]->texture, subPassIndex);
								++index;
							}

							RHISetFrameBuffer(commandList, mFrameBuffer);
							if (pass->info->passType == EPassType::CubeMap)
							{
								int size = outputBuffers[0]->texture->getDesc().dimension.x;
								RHISetViewport(commandList, 0, 0, size, size);
							}
							else
							{
								RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
							}

							GraphicsShaderStateDesc state;
							state.vertex = mScreenVS->getRHI();
							state.pixel = pass->shader.getRHI();

							RHISetGraphicsShaderBoundState(commandList, state);

							pass->shader.setInputParameters(commandList, pass->info->inputs,
								[this](EInputType type, int index) { return getInputTexture(type, index); }
							);
							SetStructuredUniformBuffer(commandList, pass->shader, mInputBuffer);

							if (pass->info->passType == EPassType::CubeMap)
							{
	
							}

							DrawUtility::ScreenRect(commandList);
							RHISetFrameBuffer(commandList, nullptr);
						}
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
				ShaderHelper::Get().copyTextureToBuffer(commandList, static_cast<RHITexture2D&>(*rtImage->resolvedTexture));
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
					ShaderHelper::Get().copyTextureToBuffer(commandList, static_cast<RHITexture2D&>(*rt->resolvedTexture));
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
				mMouse.y = mMouse.w = ::Global::GetScreenSize().y - msg.getPos().y;
			}
			else if (msg.onLeftUp())
			{
				mMouse.z = -Math::Abs(mMouse.z);
				mMouse.w = -Math::Abs(mMouse.w);
			}
			else if (msg.isLeftDown() && msg.onMoving())
			{
				mMouse.x = msg.getPos().x;
				mMouse.y = ::Global::GetScreenSize().y - msg.getPos().y;
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
#if 1
			systemConfigs.screenWidth = 1280;
			systemConfigs.screenHeight = 768;
#else
			systemConfigs.screenWidth = 768;
			systemConfigs.screenHeight = 432;
#endif
			systemConfigs.bVSyncEnable = false;
		}

		RHITexture2DRef mTexKeyboard;
		RHITexture2DRef mDefaultTex2D;
		RHITextureCubeRef mDefaultCube;

		bool canUseComputeShader(RenderPassData const& pass)
		{
			return bAllowComputeShader && pass.info->passType != EPassType::CubeMap;
		}

		bool bAllowComputeShader = true;
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

			loadProject("Clouds");
			//loadProject("ShaderArt");
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