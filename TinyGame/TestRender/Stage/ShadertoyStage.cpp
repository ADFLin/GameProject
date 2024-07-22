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
#include "Audio/AudioDevice.h"
#include "Audio/AudioDecoder.h"
#include "DirectX/Include/audiodefs.h"

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
		Sound,
		Buffer,
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
		int resId;
		int channel;
		bool bVFlip = false;
		ESampler::Filter      filter = ESampler::Bilinear;
		ESampler::AddressMode addressMode = ESampler::Clamp;
	};

	struct RenderOutput
	{
		int bufferId;
		int resId;
		int channel;
		bool bGenMipmaps;
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
			if ( input.type == EInputType::Keyboard )
				return TStaticSamplerState< ESampler::Point, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI();
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
				RHITextureBase* texture = GetTexture(input.type, input.resId);
				if (texture)
				{
					setTexture(commandList, mParamInputs[Index], *texture, mParamInputSamplers[Index], GetSamplerState(input));
					channelSize[input.channel] = Vector3(texture->getDesc().dimension);
				}
				++Index;
			}

			if (mParamChannelResolution.isBound())
			{
				setParam(commandList, mParamChannelResolution, channelSize, ARRAY_SIZE(channelSize));
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

			uint8  numChannels;
			uint8  layout;
			uint16 format;
		};


		Header header;
		serializer >> header; 
		uint32 pixelByte = header.numChannels * (header.format == 10 ? sizeof(float) : sizeof(uint8));
		TArray<uint8> data;

		data.resize(header.sizeX * header.sizeY * header.sizeZ * pixelByte);
		serializer.read(data.data(), data.size());
		ETexture::Format format;

		switch (header.numChannels + header.format)
		{
		case 1: format = ETexture::R8; break;
		case 2: format = ETexture::RG8; break;
		case 3: format = ETexture::RGB8; break;
		case 4: format = ETexture::RGBA8; break;
		case 11: format = ETexture::R32F; break;
		case 12: format = ETexture::RG32F; break;
		case 13: format = ETexture::RGB32F; break;
		case 14: format = ETexture::RGBA32F; break;
		}

		TextureDesc desc = TextureDesc::Type3D(format, header.sizeX, header.sizeY, header.sizeZ);
		if ( bGenMipmap )
		{
			desc.numMipLevel = RHIUtility::CalcMipmapLevel( Math::Min(header.sizeX , Math::Min(header.sizeY , header.sizeZ)) );
			desc.AddFlags(TCF_GenerateMips);
		}
		else
		{
			desc.numMipLevel = 1;
		}

		return RHICreateTexture3D(desc, data.data());
	}


	struct RenderPassData
	{
		RenderPassInfo* info;
		RenderPassShader shader;
	};

	char const* AlphaSeq = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";


	class Renderer
	{
	public:

		void setRenderTarget(RHICommandList& commandList, RHITexture2D& target)
		{
			mFrameBuffer->setTexture(0, target);
			RHISetFrameBuffer(commandList, mFrameBuffer);
		}

		void setPixelShader(RHICommandList& commandList, RenderPassShader& shader)
		{
			GraphicsShaderStateDesc state;
			state.vertex = mScreenVS->getRHI();
			state.pixel = shader.getRHI();
			RHISetGraphicsShaderBoundState(commandList, state);
		}

		void setInputParameters(RHICommandList& commandList, RenderPassInfo const& passInfo, RenderPassShader& shader)
		{
			shader.setInputParameters(commandList, passInfo.inputs,
				[this](EInputType type, int id) { return getInputTexture(type, id); }
			);

			SetStructuredUniformBuffer(commandList, shader, mInputBuffer);
		};

		virtual RHITextureBase* getInputTexture(EInputType type, int id) = 0;

		ScreenVS* mScreenVS;
		RHIFrameBufferRef mFrameBuffer;
		TStructuredBuffer< InputParam > mInputBuffer;
	};

	class SoundRenderPassStreamSource : public IAudioStreamSource
	{
	public:
		static int const SampleRate = 44100;
		static constexpr int TextureSize = 256;

		WaveFormatInfo mFormat;

		SoundRenderPassStreamSource()
		{
			mFormat.tag = WAVE_FORMAT_PCM;
			mFormat.numChannels = 2;
			mFormat.sampleRate = SampleRate;
			mFormat.bitsPerSample = 8 * sizeof(int16);
			mFormat.blockAlign = mFormat.numChannels * mFormat.bitsPerSample / 8;
			mFormat.byteRate = mFormat.blockAlign * mFormat.sampleRate;
		}

		virtual void seekSamplePosition(int64 samplePos) override
		{

		}

		virtual void getWaveFormat(WaveFormatInfo& outFormat) override
		{
			outFormat = mFormat;
		}


		virtual int64 getTotalSampleNum() override
		{
			return 3 * 60 * SampleRate;
		}

		virtual EAudioStreamStatus generatePCMData(int64 samplePos, AudioStreamSample& outSample, int minSampleFrameRequired) override
		{
			int blockFrame = TextureSize * TextureSize;
			int genSampleFame = Math::Max(blockFrame, minSampleFrameRequired);

			outSample.handle = mSampleBuffer.fetchSampleData();
			WaveSampleBuffer::SampleData* sampleData = mSampleBuffer.getSampleData(outSample.handle);
			sampleData->data.resize(genSampleFame * mFormat.blockAlign);


			int numBlocks = AlignCount(genSampleFame, blockFrame);
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			mRenderer->setRenderTarget(commandList, *mTexSound);

			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0,0,0,0), 1);
			RHISetViewport(commandList, 0, 0, TextureSize, TextureSize);

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

			mRenderer->setPixelShader(commandList, mRenderPass->shader);
			mRenderer->setInputParameters(commandList, *mRenderPass->info, mRenderPass->shader);

			int16* pData = (int16*)sampleData->data.data();
			for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock)
			{
				int64 sampleOffset = samplePos + indexBlock * blockFrame;
				float timeOffset = float(sampleOffset) / mFormat.sampleRate;
				mRenderPass->shader.setParam(commandList, SHADER_PARAM(iTimeOffset), timeOffset);
				mRenderPass->shader.setParam(commandList, SHADER_PARAM(iSampleOffset), (int32)sampleOffset);

				DrawUtility::ScreenRect(commandList);
				RHIFlushCommand(commandList);

				int numRead = Math::Min(genSampleFame - indexBlock * blockFrame, blockFrame);
				readSampleData(pData, numRead);
				pData += numRead * mFormat.numChannels;
			}

			outSample.data = sampleData->data.data();
			outSample.dataSize = sampleData->data.size();

			return samplePos + genSampleFame < getTotalSampleNum() ? EAudioStreamStatus::Ok : EAudioStreamStatus::Eof;
		}

		TVector2<int16> Decode(uint8 const* pData)
		{
			int16 left = int16(MaxInt16 *(-1.0 + 2.0*(pData[0] + 256.0*pData[1]) / 65535.0));
			int16 right = int16(MaxInt16 *(-1.0 + 2.0*(pData[2] + 256.0*pData[3]) / 65535.0));
			return {left, right};
		}

		void readSampleData(int16* pOutData, int numSamples)
		{
			TArray<uint8> data;
			RHIReadTexture(*mTexSound, ETexture::RGBA8, 0, data);

			uint8 const* pData = data.data();
			for (int i = 0; i < numSamples; ++i)
			{
				auto sample = Decode(pData);
				pData += 4;

				pOutData[0] = sample.x;
				pOutData[1] = sample.y;
				pOutData += 2;
			}
		}

		virtual void releaseSampleData(uint32 sampleHadle) override
		{
			mSampleBuffer.releaseSampleData(sampleHadle);
		}


		void initializeRHI()
		{
			mTexSound = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, TextureSize, TextureSize).AddFlags(TCF_AllowCPUAccess | TCF_RenderTarget));
		}

		RenderPassData*  mRenderPass;
		Renderer*        mRenderer;

		WaveSampleBuffer mSampleBuffer;
		RHITexture2DRef  mTexSound;
	};

	class TestStage : public StageBase
		            , public Renderer
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
			frame->addButton("Reload Project", [this](int event, GWidget*)
			{
				loadProject(mLastProjectName.c_str());
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
			switch (pass.info->passType)
			{
			case EPassType::Buffer:
				option.addDefine(SHADER_PARAM(RENDER_BUFFER), 1);
				break;
			case EPassType::Sound:
				option.addDefine(SHADER_PARAM(RENDER_SOUND), 1);
				option.addDefine(SHADER_PARAM(SOUND_TEXTURE_SIZE), SoundRenderPassStreamSource::TextureSize);
				break;
			case EPassType::CubeMap:
				option.addDefine(SHADER_PARAM(RENDER_CUBE), 1);
				break;
			case EPassType::Image:
				option.addDefine(SHADER_PARAM(RENDER_IMAGE), 1);
				break;
			}
			bool bUseComputeShader = canUseComputeShader(pass);
			if (!ShaderManager::Get().loadFile(pass.shader, nullptr, { bUseComputeShader ? EShader::Compute : EShader::Pixel, bUseComputeShader ? "MainCS" : "MainPS" }, option, code.c_str()))
				return false;

			pass.shader.bindInputParameters(pass.info->inputs);
			return true;
		}


		
		bool loadResource(RenderInput const& input, std::string const& filePath)
		{

			if (mInputResources.isValidIndex(input.resId) && mInputResources[input.resId].isValid())
				return true;

			std::string loadPath = std::string("Shadertoy") + filePath;

			auto AddResource = [this, &input](RHITextureBase& texture)
			{
				if (mInputResources.size() < input.resId + 1)
				{
					mInputResources.resize(input.resId + 1);
				}
				mInputResources[input.resId] = &texture;
				GTextureShowManager.registerTexture( InlineString<>::Make("Res%d" , input.resId ).c_str(), &texture);
			};

			switch (input.type)
			{
			case EInputType::Keyboard:
			case EInputType::Webcam:
			case EInputType::Micrphone:
			case EInputType::Soundcloud:
			case EInputType::Buffer:
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

		RHITextureBase* getInputTexture(EInputType type, int id)
		{
			if (mInputResources.isValidIndex(id) && mInputResources[id].isValid())
			{
				return mInputResources[id];
			}

			switch (type)
			{
			case EInputType::Buffer:
				return GBlackTexture2D;
			case EInputType::Texture:
				return mDefaultTex2D;
			case EInputType::CubeMap:
				return mDefaultCube;
			case EInputType::Volume:
				return GBlackTexture3D;
			case EInputType::Webcam:
			case EInputType::Micrphone:
			case EInputType::Soundcloud:
			case EInputType::Vedio:
			case EInputType::Music:
				break;
			}

			return nullptr;
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
					output.resId = iter->second;
				}
				else
				{
					output.resId = nextResourceId++;
					nameToIdMap.emplace(idName, output.resId);
				}

				output.bufferId = nextBufferId++;
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
					input.resId = iter->second;
				}
				else
				{
					input.resId = nextResourceId++;
					switch (input.type)
					{
					case EInputType::Buffer:
						break;
					case EInputType::Texture:
					case EInputType::CubeMap:
					case EInputType::Volume:
						{
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
					case EInputType::Keyboard:
						{
							if (mInputResources.size() < input.resId + 1)
							{
								mInputResources.resize(input.resId + 1);
							}
							mInputResources[input.resId] = mTexKeyboard;
						}
						break;
					case EInputType::Vedio:
					case EInputType::Music:
					case EInputType::Webcam:
					case EInputType::Micrphone:
						break;
					}

					nameToIdMap.emplace(idName , input.resId); 
				}

				return true;
			};

			int bufferIndex = 0;
			int CubeBufferIndex = 0;
			auto ParseoRenderPassOutput = [&](JsonObject& jsonObject, RenderPassInfo& info) -> bool
			{
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
				else if (typeName == "cubemap")
				{
					info.passType = EPassType::CubeMap;
					info.typeIndex = CubeBufferIndex++;
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

				return true;
			};

			for (auto const& renderPassValue : renderPassArray)
			{
				if (!renderPassValue.isObject())
					return false;

				auto info = std::make_unique<RenderPassInfo>();
				if (!ParseoRenderPassOutput(renderPassValue.asObject(), *info))
				{
					return false;
				}

				mRenderPassInfos.push_back(std::move(info));
			}

			int index = 0;
			for (auto const& renderPassValue : renderPassArray)
			{
				if (!ParseRenderPass(renderPassValue.asObject(), *mRenderPassInfos[index]))
				{
					return false;
				}
				++index;
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
			mInputResources.resize(nextResourceId);
			return true;
		}


		bool loadOverridedCode(char const* dir, EPassType type, int index, std::string& outCode)
		{
			InlineString<> path;
			switch (type)
			{
			case EPassType::None:
				path.format("%s/Common.sgc", dir);
				break;
			case EPassType::Image:
				path.format("%s/Image.sgc", dir);
				break;
			case EPassType::Buffer:
				path.format("%s/Buffer%c.sgc", dir, AlphaSeq[index]);
				break;
			case EPassType::Sound:
				path.format("%s/Sound.sgc", dir);
				break;
			case EPassType::CubeMap:
				path.format("%s/CubeMap%c.sgc", dir, AlphaSeq[index]);
				break;
			default:
				return false;
			}

			if (!FFileSystem::IsExist(path))
				return false;

			if (!FFileUtility::LoadToString(path, outCode))
				return false;

			return true;
		}

		std::string mLastProjectName;

		bool loadProject(char const* name)
		{
			if (!loadProjectActual(name) || !compileShader())
			{
				mRenderPassList.clear();
				mOutputBuffers.clear();
				mInputResources.clear();
				return false;
			}

			mLastProjectName = name;
			resetInputParams();
			return true;
		}

		bool loadProjectActual(char const* name)
		{
			mRenderPassInfos.clear();
			mRenderPassList.clear();
			mOutputBuffers.clear();
			mInputResources.clear();
			if (mSoundHandle != ERROR_AUDIO_HANDLE)
			{
				mAudioDevice->stopSound(mSoundHandle);
			}

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


				std::string dir = InlineString<>::Make("Shadertoy/%s", name);
				for (auto const& info : mRenderPassInfos)
				{
					loadOverridedCode(dir.c_str(), info->passType, info->typeIndex, info->code);
				}

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

				int nextId = 0;
				auto LoadRenderPassInfo = [this, inputDoc, &dir, &nextId](EPassType type, int index = 0) -> RenderPassInfo*
				{
					std::string code;
					if (!loadOverridedCode(dir.c_str(), type, index, code))
						return nullptr;

					auto info = std::make_unique<RenderPassInfo>();
					info->passType = type;
					info->typeIndex = index;
					info->code = std::move(code);

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
										if (!inputObject.tryGet("ID", input.resId))
										{
											input.resId = 0;
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

					if (type == EPassType::Buffer || type == EPassType::Image || type == EPassType::CubeMap)
					{
						RenderOutput output;
						output.channel = 0;
						output.resId = nextId++;
						output.bufferId = output.resId;
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
					if (info->passType == EPassType::Buffer || info->passType == EPassType::CubeMap)
					{
						for (auto const& output : info->outputs)
						{
							maxBufferId = Math::Max(maxBufferId, output.bufferId);
						}
					}
				}
				mOutputBuffers.resize(maxBufferId + 1);
				mInputResources.resize(mOutputBuffers.size());
			}

			for (auto& info : mRenderPassInfos)
			{
				if (info->passType == EPassType::None)
					continue;

				auto pass = std::make_unique<RenderPassData>();

				pass->info = info.get();

				if (info->passType == EPassType::Sound)
				{
					initAudio(*pass);
				}

				mRenderPassList.push_back(std::move(pass));
			}

			return true;
		}

		int mSoundHandle = ERROR_AUDIO_HANDLE;

		void resetInputParams()
		{
			mTime = 0;
			mTimePrev = 0;
			mFrameCount = 0;
			FMemory::Zero(mKeyboardData, sizeof(mKeyboardData));

			if (mAudioDevice)
			{
				if (mSoundHandle != ERROR_AUDIO_HANDLE)
				{
					mAudioDevice->stopSound(mSoundHandle);
				}
				mSoundHandle = mAudioDevice->playSound2D(*mSoundWave);
			}
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

		std::unique_ptr<AudioDevice> mAudioDevice;
		std::unique_ptr<SoundRenderPassStreamSource> mStreamSource;
		std::unique_ptr<SoundWave>    mSoundWave;
		void initAudio(RenderPassData& pass)
		{
			if (mAudioDevice.get())
				return;

			mAudioDevice = std::unique_ptr<AudioDevice>(AudioDevice::Create());
			mStreamSource = std::make_unique<SoundRenderPassStreamSource>();
			mStreamSource->initializeRHI();
			mStreamSource->mRenderer = this;
			mStreamSource->mRenderPass = &pass;

			mSoundWave = std::make_unique<SoundWave>();
			mSoundWave->setupStream(mStreamSource.get());

			GTextureShowManager.registerTexture("SoundTexture", mStreamSource->mTexSound);

			SystemPlatform::Sleep(100);
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

			if (mAudioDevice)
			{
				mAudioDevice->update(time / 1000.0f);
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
		uint8   mKeyboardData[256 * 3];


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
			inputParam.iSampleRate = SoundRenderPassStreamSource::SampleRate;

			DateTime now = SystemPlatform::GetLocalTime();
			inputParam.iDate = Vector4( now.getYear() , now.getMonth() , now.getDay(), now.getSecond() );
			mInputBuffer.updateBuffer(inputParam);

			RHIUpdateTexture(*mTexKeyboard, 0, 0, 256, 3, mKeyboardData);

			mTimePrev = mTime;
			++mFrameCount;

			PooledRenderTargetRef rtImage;
			{
				GPU_PROFILE(bAllowComputeShader ? "RenderPassCS" : "RenderPass");

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				int const CubeMapSize = 1024;

				for (auto& pass : mRenderPassList)
				{
					if (pass->info->passType == EPassType::Sound )
						continue;

					bool bUseComputeShader = canUseComputeShader(*pass);

					TArray< PooledRenderTargetRef , TInlineAllocator<4> > outputBuffers;
					HashString debugName;
					int index;
					bool bRenderCubeMap = pass->info->passType == EPassType::CubeMap;
					IntVector2 viewportSize;
					index = 0;
					for (auto const& output : pass->info->outputs)
					{
						RenderTargetDesc desc;
						desc.format = ETexture::RGBA32F;
						desc.numSamples = 1;

						if (bRenderCubeMap)
						{
							desc.type = ETexture::TypeCube;
							desc.size = IntVector2(CubeMapSize, CubeMapSize);
						}
						else
						{
							desc.type = ETexture::Type2D;
							desc.size = screenSize;
						}


						if (index == 0)
						{
							switch (pass->info->passType)
							{
							case EPassType::Buffer:
							{
								desc.debugName = InlineString<>::Make("Buffer%c", AlphaSeq[pass->info->typeIndex]);;
							}
							break;
							case EPassType::CubeMap:
							{
								desc.debugName = InlineString<>::Make("CubeMap%c", AlphaSeq[pass->info->typeIndex]);;
							}
							break;
							case EPassType::Image:
							{
								desc.debugName = "Image";
							}
							break;
							}
							debugName = desc.debugName;
							viewportSize = desc.size;
						}
						else
						{

							switch (pass->info->passType)
							{
							case EPassType::Buffer:
							{
								desc.debugName = InlineString<>::Make("Buffer%c(%d)", AlphaSeq[pass->info->typeIndex], output.channel);
							}
							break;
							case EPassType::CubeMap:
							{
								desc.debugName = InlineString<>::Make("CubeMap%c(%d)", AlphaSeq[pass->info->typeIndex], output.channel);
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
	
					auto SetupParam = [&](RHICommandList& commandList, int subPassIndex)
					{							
						setInputParameters(commandList, *pass->info, pass->shader);

						if (pass->info->passType == EPassType::CubeMap)
						{
							Vector3 faceDir = ETexture::GetFaceDir(ETexture::Face(subPassIndex));
							Vector3 faceUpDir = ETexture::GetFaceUpDir(ETexture::Face(subPassIndex));
							pass->shader.setParam(commandList, SHADER_PARAM(iFaceDir), faceDir);
							pass->shader.setParam(commandList, SHADER_PARAM(iFaceVDir), faceUpDir);
							pass->shader.setParam(commandList, SHADER_PARAM(iFaceUDir), faceDir.cross(faceUpDir));
							pass->shader.setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
							pass->shader.setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
						}
					};

					GPU_PROFILE(debugName.c_str());

					int subPassCount = pass->info->passType == EPassType::CubeMap ? 6 : 1;

					if (bUseComputeShader)
					{
						RHISetComputeShader(commandList, pass->shader.getRHI());

						SetupParam(commandList, 0);

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
							RHISetViewport(commandList, 0, 0, viewportSize.x, viewportSize.y);

							setPixelShader(commandList, pass->shader);
							SetupParam(commandList, subPassIndex);

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
							auto& buffer = mOutputBuffers[output.bufferId];
							if (buffer.isValid())
							{
								buffer->desc.debugName = EName::None;
								buffer->bResvered = false;
							}

							buffer = outputBuffers[index];
							buffer->bResvered = true;
							++index;

							mInputResources[output.resId] = buffer->resolvedTexture;
						}
					}
				}


#if 0
				for (int k = 0; k < 256; k++)
				{
					mKeyboardData[k + 1 * 256] = 0;
				}
#endif
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
				if (0 <= msg.getCode() && msg.getCode() < 256 && mKeyboardData[msg.getCode() + 0 * 256] != 255)
				{
					mKeyboardData[msg.getCode() + 0 * 256] = 255;
					mKeyboardData[msg.getCode() + 1 * 256] = 255;
					mKeyboardData[msg.getCode() + 2 * 256] = 255 - mKeyboardData[msg.getCode() + 2 * 256];
				}
			}
			else
			{
				if (0 <= msg.getCode() && msg.getCode() < 256)
				{
					mKeyboardData[msg.getCode() + 0 * 256] = 0;
					mKeyboardData[msg.getCode() + 1 * 256] = 0;
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

		bool bAllowComputeShader = false;



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
			mTexKeyboard = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8 , 256 , 3));
			GTextureShowManager.registerTexture("DefaultCube", mDefaultCube);
			GTextureShowManager.registerTexture("Keyborad" , mTexKeyboard);

			ScreenVS::PermutationDomain domainVector;
			domainVector.set<ScreenVS::UseTexCoord>(false);
			mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector);

			loadProject("Test");
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