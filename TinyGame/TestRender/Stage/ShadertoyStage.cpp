#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "ShadertoyCore.h"

#include "RHI/ShaderManager.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/GpuProfiler.h"
#include "RHI/GlobalShader.h"
#include "RHI/RHIUtility.h"

#include "RenderDebug.h"

#include "FileSystem.h"
#include "Json.h"
#include "Core/ScopeGuard.h"
#include "Serialize/FileStream.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioDecoder.h"

#include "Mmreg.h"
#include "Core/FNV1a.h"
#include "Audio/XAudio2/MFDecoder.h"
#include "ProfileSystem.h"
#include <mfreadwrite.h>
#include <atomic>


#pragma comment(lib , "mfplat.lib")
#pragma comment(lib , "mfuuid.lib")


namespace Shadertoy
{
	class N12ConvertPS : public GlobalShader
	{
		DECLARE_SHADER(N12ConvertPS, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(FlipV, "DO_FLIP_V");
		using PermutationDomain = TShaderPermutationDomain<FlipV>;


		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_SHADER_PARAM(parameterMap, ColorTransform);
			BIND_SHADER_PARAM(parameterMap, SampleWidth);
			BIND_SHADER_PARAM(parameterMap, UVScale);
			BIND_TEXTURE_PARAM(parameterMap, Texture);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/MediaShaders";
		}

		void setParameters(RHICommandList& commandList, RHITexture2D& texSample, Matrix4 const& colorTransform, Vector2 const& uvScale, float sampleWidth)
		{
			SET_SHADER_PARAM(commandList, *this, UVScale, uvScale);
			SET_SHADER_PARAM(commandList, *this, SampleWidth, sampleWidth);
			SET_SHADER_PARAM(commandList, *this, ColorTransform, colorTransform);
			auto& sampler = TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI();
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, Texture, texSample, sampler);
		}

		DEFINE_SHADER_PARAM(ColorTransform);
		DEFINE_SHADER_PARAM(SampleWidth);
		DEFINE_SHADER_PARAM(UVScale);
		DEFINE_TEXTURE_PARAM(Texture);
	};

	IMPLEMENT_SHADER(N12ConvertPS, EShader::Pixel, SHADER_ENTRY(N12ConvertPS));

	class MediaPlayer : public IMFSourceReaderCallback
	{
	public:
		bool open(char const* path)
		{
			static MediaFoundationScope MFScope;

			//path = "Shadertoy/Assets/dd.mp4";
			path = "D:/AA/3DSVR-1534UF/3DSVR-1534-B.mp4";
#if 1
			TComPtr< IMFSourceResolver > sourceResolver;
			CHECK_RETURN(MFCreateSourceResolver(&sourceResolver), false);
			MF_OBJECT_TYPE objectType;
			TComPtr<IUnknown> object;
			CHECK_RETURN(sourceResolver->CreateObjectFromURL(FCString::CharToWChar(path).c_str(), MF_RESOLUTION_MEDIASOURCE, nullptr, &objectType, &object), false);
			object.castTo(mMediaSource);
			mMediaSource->AddRef();

			TComPtr<IMFAttributes> attributes;
			CHECK_RETURN(MFCreateAttributes(&attributes, 1), false);

			attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
#if 1
			attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);
#endif
			CHECK_RETURN(MFCreateSourceReaderFromMediaSource(mMediaSource, attributes, &mMFSourceReader), false);
#else
			CHECK_RETURN(MFCreateSourceReaderFromURL(FCString::CharToWChar(path).c_str(), NULL, &mMFSourceReader), false);
#endif
			FMFDecodeUtil::ConfigureVideoStream(mMFSourceReader, &mVideoType);

			UINT32 DimX,DimY;
			::MFGetAttributeSize(mVideoType, MF_MT_FRAME_SIZE, &DimX, &DimY);
			mFrameSize.x = DimX;
			mFrameSize.y = DimY;
			mAligendFrameSize.x = Math::AlignUp<int>(DimX, 16);
			mAligendFrameSize.y = Math::AlignUp<int>(DimY, 16);


			GUID SubType;
			mVideoType->GetGUID(MF_MT_SUBTYPE, &SubType);


			int size = mAligendFrameSize.x * mAligendFrameSize.y * 3 / 2;
			mSampleData.resize(size);

			mTexFrame  = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, mFrameSize.x, mFrameSize.y));
			mTexSample = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8, mAligendFrameSize.x, mAligendFrameSize.y * 3 / 2));
			mFrameBuffer = RHICreateFrameBuffer();
			mFrameBuffer->addTexture(*mTexFrame);

			PROPVARIANT DurationAttrib;
			mMFSourceReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &DurationAttrib);
			const int64 Duration = (DurationAttrib.vt == VT_UI8) ? (int64)DurationAttrib.uhVal.QuadPart : 0;
			mDuration = double(Duration) / SecondTimeUnit;
			::PropVariantClear(&DurationAttrib);

			mCurrentTime = 300;
			commitPosition(mCurrentTime);
			mHaveSample = false;
			mbReadSampleRequested = false;
			return true;
		}
		static constexpr int64 SecondTimeUnit = 10000000LL;

		double mDuration = 0.0f;
		double mPlayRate = 1.0f;
		double mCurrentTime = 0.0;
		double mLastSampleTime = -1.0f;

		void commitPosition(double time)
		{
			int64 duration = int64(time * SecondTimeUnit);
			PROPVARIANT var;
			HRESULT hr = InitPropVariantFromInt64(duration, &var);
			hr = mMFSourceReader->SetCurrentPosition(GUID_NULL, var);
			PropVariantClear(&var);
		}

		void render(RHICommandList& commandList)
		{
			if (!mHaveSample)
				return;

			RHIUpdateTexture(*mTexSample, 0, 0, mAligendFrameSize.x, mAligendFrameSize.y * 3 / 2, mSampleData.data());
			RHISetFrameBuffer(commandList, mFrameBuffer);
			RHISetViewport(commandList, 0, 0, mFrameSize.x, mFrameSize.y);

			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			ScreenVS::PermutationDomain domainVectorVS;
			domainVectorVS.set<ScreenVS::UseTexCoord>(true);
			auto screenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVectorVS, true);

			N12ConvertPS::PermutationDomain domainVectorPS;
			domainVectorPS.set<N12ConvertPS::FlipV>(true);
			auto n12ConvertPS = ShaderManager::Get().getGlobalShaderT<N12ConvertPS>(domainVectorPS, true);

			GraphicsShaderStateDesc stateDesc;
			stateDesc.vertex = screenVS->getRHI();
			stateDesc.pixel = n12ConvertPS->getRHI();
			RHISetGraphicsShaderBoundState(commandList, stateDesc);

			Vector2 uvScale = Vector2(float(mFrameSize.x) / mAligendFrameSize.x, float(mFrameSize.y) / mAligendFrameSize.y);
			Matrix4 const colorTransform = Matrix4(
				1.16438356164f, 0.0f, 1.792652263418f, 0.0f,
				1.16438356164f, -0.213237021569f, -0.533004040142f, 0.0f,
				1.16438356164f, 2.112419281991f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f
			) * Matrix4(
				1.0f, 0.0f, 0.0f, -0.06274509803921568627f,
				0.0f, 1.0f, 0.0f, -0.5019607843137254902f,
				0.0f, 0.0f, 1.0f, -0.5019607843137254902f,
				0.0f, 0.0f, 0.0f, 1.0f
			);
			n12ConvertPS->setParameters(commandList, *mTexSample, colorTransform, uvScale, float(mAligendFrameSize.x));
			DrawUtility::ScreenRect(commandList);

			mHaveSample = false;
		}

		void update(float dt)
		{
			mCurrentTime += mPlayRate * dt;
			if (!mHaveSample)
			{
				if (mCurrentTime > mDuration)
				{
					mCurrentTime = 0.0;
					mLastSampleTime = -1.0f;
					mHaveSample = false;
					mbReadSampleRequested = false;
					commitPosition(mCurrentTime);
				}

				if (mCurrentTime > mLastSampleTime)
				{
					requestReadSample();
				}
			}
		}


		IntVector2 mFrameSize;
		IntVector2 mAligendFrameSize;

		bool requestReadSample()
		{
			if (mbReadSampleRequested)
				return true;
				 

			DWORD dwFlags = 0;
			TComPtr<IMFSample> pSample;
			DWORD streamIndex;
			LONGLONG  timestamp;
			HRESULT hr;
			hr = mMFSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr,nullptr,nullptr,nullptr);
			if (FAILED(hr)) 
			{ 
				return false; 
			}

			mbReadSampleRequested = true;
			return true;
		}

		TArray< uint8 >    mSampleData;
		std::atomic<bool>  mHaveSample;
		std::atomic<bool>  mbReadSampleRequested;
		RHITexture2DRef    mTexSample;
		RHIFrameBufferRef  mFrameBuffer;
		virtual HRESULT STDMETHODCALLTYPE OnReadSample(
			_In_  HRESULT hrStatus,
			_In_  DWORD dwStreamIndex,
			_In_  DWORD dwStreamFlags,
			_In_  LONGLONG llTimestamp,
			_In_opt_  IMFSample *pSample)
		{
			mbReadSampleRequested = false;

			if (pSample == nullptr)
				return S_OK;

			//TIME_SCOPE("Read Video Sample");

			TComPtr<IMFMediaBuffer> pBuffer;
			CHECK_RETURN(pSample->ConvertToContiguousBuffer(&pBuffer), E_INVALIDARG);
			{
				DWORD cbBuffer = 0;
				BYTE *pData = NULL;
				CHECK_RETURN(pBuffer->Lock(&pData, NULL, &cbBuffer), E_INVALIDARG);
				ON_SCOPE_EXIT{ pBuffer->Unlock(); };
				FMemory::Copy(mSampleData.data(), pData, cbBuffer);
				mLastSampleTime = double(llTimestamp) / SecondTimeUnit;
				mHaveSample = true;
			}

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE OnFlush(
			/* [annotation][in] */
			_In_  DWORD dwStreamIndex)
		{		
			mHaveSample = false;
			return S_OK;		
		}

		virtual HRESULT STDMETHODCALLTYPE OnEvent(
			/* [annotation][in] */
			_In_  DWORD dwStreamIndex,
			/* [annotation][in] */
			_In_  IMFMediaEvent *pEvent)
		{
			return S_OK;		
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [annotation][iid_is][out] */
			_COM_Outptr_  void **ppvObject)
		{
			if (ppvObject == NULL)
			{
				return E_INVALIDARG;
			}

			if ((riid == IID_IUnknown) || (riid == IID_IMFSourceReaderCallback))
			{
				*ppvObject = (LPVOID)this;
				AddRef();
				return NOERROR;
			}

			*ppvObject = NULL;

			return E_NOINTERFACE;
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void) 
		{
			return 1;

		}

		virtual ULONG STDMETHODCALLTYPE Release(void)
		{
			return 1;
		}

		TComPtr< IMFMediaSource > mMediaSource;
		TComPtr< IMFSourceReader  > mMFSourceReader;
		TComPtr< IMFMediaType > mVideoType;
		RHITexture2DRef mTexFrame;
	};

	class SoundRenderPassStreamSource : public IAudioStreamSource
	{
	public:
		static constexpr int SampleRate = Renderer::SoundSampleRate;
		static constexpr int TextureSize = Renderer::SoundTextureSize;

		WaveFormatInfo mFormat;

		bool bFullFlush = false;

		SoundRenderPassStreamSource()
		{
			mFormat.tag = WAVE_FORMAT_PCM;
			mFormat.numChannels = 2;
			mFormat.sampleRate = SampleRate;
			mFormat.bitsPerSample = 8 * sizeof(int16);
			mFormat.blockAlign = mFormat.numChannels * mFormat.bitsPerSample / 8;
			mFormat.byteRate = mFormat.blockAlign * mFormat.sampleRate;

			mLastSampleData.data = nullptr;
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

		virtual EAudioStreamStatus generatePCMData(int64 samplePos, AudioStreamSample& outSample, int minSampleFrameRequired, bool bNeedFlush) override
		{
			if (mRenderPass == nullptr)
				return EAudioStreamStatus::Eof;

			int minGenerateSampleCount = Math::Max(TextureSize * TextureSize, minSampleFrameRequired);

			auto CheckRequireSample = [&](int64 inSamplePos)
			{
				if (inSamplePos < getTotalSampleNum())
				{
					mSamplePosRequired = inSamplePos;
					mNumSampleRequired = minGenerateSampleCount;
				}
			};

			int sampleCount = 0;
			if (bNeedFlush)
			{
				RHICommandList& commandList = RHICommandList::GetImmediateList();
				sampleCount = bFullFlush ? getTotalSampleNum() : minGenerateSampleCount;
				generateStreamingSample(commandList, samplePos, sampleCount, outSample);
			}
			else if (mLastSampleData.data)
			{
				if (mLastSamplePos == samplePos)
				{
					outSample = mLastSampleData;
					sampleCount = mLastSampleData.dataSize / mFormat.blockAlign;
					mLastSampleData.data = nullptr;
					mLastSampleData.handle = INDEX_NONE;

					CheckRequireSample(samplePos + sampleCount);
				}
				else
				{
					mSampleBuffer.releaseSampleData(mLastSampleData.handle);
					mLastSampleData.data = nullptr;
					mLastSampleData.handle = INDEX_NONE;

					CheckRequireSample(samplePos);
					return EAudioStreamStatus::NoSample;
				}
			}
			else
			{
				CheckRequireSample(samplePos);
				return EAudioStreamStatus::NoSample;
			}

			if ((samplePos + sampleCount) > getTotalSampleNum())
				return EAudioStreamStatus::Eof;

			return EAudioStreamStatus::Ok;
		}

		int generateStreamingSample(RHICommandList& commandList, int64 samplePos, int sampleCount, AudioStreamSample& outSample)
		{
			CHECK(mRenderPass);
			outSample = mSampleBuffer.allocSamples(sampleCount * mFormat.blockAlign);
			mRenderer->renderSound(commandList, *mRenderPass, mFormat, samplePos, sampleCount, outSample);
			return sampleCount;
		}

		void checkGenerateRequiredStreamingSample(RHICommandList& commandList)
		{
			if (mRenderPass == nullptr && mNumSampleRequired == 0)
				return;

			generateStreamingSample(commandList, mSamplePosRequired, mNumSampleRequired, mLastSampleData);
			mLastSamplePos = mSamplePosRequired;
			mNumSampleRequired = 0;
		}

		virtual void releaseSampleData(uint32 sampleHandle) override
		{
			mSampleBuffer.releaseSampleData(sampleHandle);
		}

		RenderPassData*  mRenderPass;
		Renderer*        mRenderer;

		int64 mSamplePosRequired = INDEX_NONE;
		int   mNumSampleRequired = 0;

		int64             mLastSamplePos;
		AudioStreamSample mLastSampleData;

		WaveSampleBuffer mSampleBuffer;

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
			auto choice = frame->addChoice();
			choice->addItem("Test");
			choice->addItem("ShaderArt");
			FileIterator fileIter;
			if (FFileSystem::FindFiles("Shadertoy", ".json", fileIter))
			{
				for (; fileIter.haveMore(); fileIter.goNext())
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
			frame->addButton("Export Code", [this](int event, GWidget*)
			{
				InlineString<> dir;
				dir.format("Shadertoy/Export/%s", mLastProjectName.c_str());
				if (!FFileSystem::IsExist(dir))
				{
					FFileSystem::CreateDirectorySequence(dir);
				}

				for (auto& info : mRenderPassInfos)
				{
					info->code;
					InlineString<> path;
					if (!getOverrideCodePath(dir, info->passType, info->typeIndex, path))
					{
						continue;
					}

					FFileUtility::SaveFromBuffer(path.c_str(),(uint8 const*)info->code.data(), info->code.size());
				}
				return false;
			});
			frame->addCheckBox("Pause", bPause);
			frame->addCheckBox("Use Multisample", mRenderer.bUseMultisample);
			frame->addCheckBox("Use ComputeShader", [this](int event, GWidget*)
			{
				mRenderer.bAllowComputeShader = !mRenderer.bAllowComputeShader;
				compileAllShaders();
				return false;
			})->bChecked = mRenderer.bAllowComputeShader;
			frame->addCheckBox("Render Cube OnePass", [this](int event, GWidget*)
			{
				mRenderer.bAllowRenderCubeOnePass = !mRenderer.bAllowRenderCubeOnePass;
				compileAllShaders(BIT(uint32(EPassType::CubeMap)));
				return false;
			})->bChecked = mRenderer.bAllowRenderCubeOnePass;
			return true;
		}

		TArray< std::unique_ptr<RenderPassInfo> > mRenderPassInfos;
		TArray< std::unique_ptr<RenderPassData> > mRenderPassList;

		Renderer mRenderer;
		
		bool parseWebFile(JsonFile& webFile)
		{
			if (!webFile.isArray())
				return false;
			auto webArray = webFile.getArray();
			if (webArray.empty())
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
				else if (typeName == "video")
				{
					input.type = EInputType::Video;
				}
				else
				{
					LogWarning(0,"Unknown type Name %s", typeName.c_str());
					return false;
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
					case EInputType::Video:
					case EInputType::Music:
						{
							std::string filePath;
							if (!jsonObject.tryGet("filepath", filePath))
								return false;

							if (!loadResource(input, filePath))
							{
								LogWarning(0, "LoadResource Fail %s", filePath.c_str());
								return false;
							}
						}
						break;
					case EInputType::Keyboard:
						{
							mRenderer.addInputResource(input, *mRenderer.mTexKeyboard);
						}
						break;

					case EInputType::Webcam:
					case EInputType::Micrphone:
						break;
					}

					nameToIdMap.emplace(idName, input.resId);
				}

				return true;
			};

			int bufferIndex = 0;
			int cubeBufferIndex = 0;
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
					info.typeIndex = cubeBufferIndex++;
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
					if (!ParseInput(inputValue.asObject(), input))
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

			std::stable_sort(mRenderPassInfos.begin(), mRenderPassInfos.end(),
				[](auto const& lhs, auto const& rhs)
				{
					if (lhs->passType != rhs->passType)
						return lhs->passType < rhs->passType;

					return lhs->typeIndex < rhs->typeIndex;
				}
			);

			mRenderer.mOutputBuffers.resize(nextBufferId);
			mRenderer.mInputResources.resize(nextResourceId);
			return true;
		}


		bool getOverrideCodePath(char const* dir, EPassType type, int index, InlineString<>& outPath)
		{
			switch (type)
			{
			case EPassType::None:
				outPath.format("%s/Common.sgc", dir);
				break;
			case EPassType::Image:
				outPath.format("%s/Image.sgc", dir);
				break;
			case EPassType::Buffer:
				outPath.format("%s/Buffer%c.sgc", dir, AlphaSeq[index]);
				break;
			case EPassType::Sound:
				outPath.format("%s/Sound.sgc", dir);
				break;
			case EPassType::CubeMap:
				outPath.format("%s/CubeMap%c.sgc", dir, AlphaSeq[index]);
				break;
			default:
				return false;
			}

			return true;
		}

		bool loadOverridedCode(char const* dir, EPassType type, int index, std::string& outCode)
		{
			InlineString<> path;
			if (!getOverrideCodePath(dir, type, index, path))
				return false;

			if (!FFileSystem::IsExist(path))
				return false;

			if (!FFileUtility::LoadToString(path, outCode))
				return false;

			return true;
		}

		std::string mLastProjectName;



		bool loadProject(char const* name)
		{
			mLastProjectName = name;

			if (!loadProjectActual(name) || !compileAllShaders())
			{
				mRenderPassList.clear();
				mRenderer.mOutputBuffers.clear();
				mRenderer.mInputResources.clear();
				clearnupMedia();
				return false;
			}

			resetInputParams();
			return true;
		}

		bool loadProjectActual(char const* name)
		{
			mRenderPassInfos.clear();
			mRenderPassList.clear();
			mRenderer.mOutputBuffers.clear();
			mRenderer.mInputResources.clear();
			clearnupMedia();
			if (mSoundHandle != ERROR_AUDIO_HANDLE)
			{
				mAudioDevice->stopSound(mSoundHandle);
				mStreamSource->mRenderPass = nullptr;
			}

#if 1
			JsonFile* webDoc = JsonFile::Load(InlineString<>::Make("Shadertoy/%s.json", name));
			if (webDoc)
			{
				ON_SCOPE_EXIT
				{
					webDoc->release();
				};

				if (!parseWebFile(*webDoc))
					return false;

				std::string dir = InlineString<>::Make("Shadertoy/%s", name);
				if (FFileSystem::IsExist(dir.c_str()))
				{
					for (auto const& info : mRenderPassInfos)
					{
						loadOverridedCode(dir.c_str(), info->passType, info->typeIndex, info->code);
					}
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
									JsonObject inputObject = inputValue.asObject();
									if (!inputObject.isVaild())
										continue;

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
									input.resId = inputObject.getInt("ID", 0);
									input.channel = inputObject.getInt("Channel", 0);
									info->inputs.push_back(input);
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
				mRenderer.mOutputBuffers.resize(maxBufferId + 1);
				mRenderer.mInputResources.resize(mRenderer.mOutputBuffers.size());
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

		bool compileAllShaders(uint32 filterMask = 0)
		{
			int commonIndex = mRenderPassInfos.findIndexPred([](auto const& input)
			{
				return input->passType == EPassType::None;
			});

			return compileAllShaders(commonIndex != INDEX_NONE ? mRenderPassInfos[commonIndex].get() : nullptr, filterMask);
		}

		bool compileAllShaders(RenderPassInfo* commonInfo, uint32 filterMask = 0)
		{
			TArray<uint8> codeTemplate;
			if (!FFileUtility::LoadToBuffer("Shader/Game/ShadertoyTemplate.sgc", codeTemplate, true))
			{
				LogWarning(0, "Can't load ShadertoyTemplate file");
				return false;
			}

			int indexPass = 0;
			for (auto& pass : mRenderPassList)
			{
				if (filterMask == 0 || (filterMask & BIT(uint32(pass->info->passType))))
				{
					if (!mRenderer.compileShader(*pass, codeTemplate, commonInfo))
					{
						LogWarning(0, "Compile RenderPass Fail index = %d", indexPass);
						return false;
					}
				}
				++indexPass;
			}

			return true;
		}

		bool loadResource(RenderInput const& input, std::string const& filePath)
		{
			if (mRenderer.mInputResources.isValidIndex(input.resId) && mRenderer.mInputResources[input.resId].isValid())
				return true;

			std::string loadPath = std::string("Shadertoy") + filePath;

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

					mRenderer.addInputResource(input, *texture);
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
						facePath[i] = InlineString<>::Make("%s_%d%s", (char const*)pathName.toMutableCString(), i + 1, extension).c_str();
						cubePaths[i + 1] = facePath[i].c_str();
					}

					TextureLoadOption option;
					option.bAutoMipMap = input.filter == ESampler::Trilinear;
					RHITextureCubeRef texture = RHIUtility::LoadTextureCubeFromFile(cubePaths, option);
					if (!texture.isValid())
						return false;

					mRenderer.addInputResource(input, *texture);
				}
				break;
			case EInputType::Volume:
				{
					RHITexture3DRef texture = LoadBinTexture(loadPath.c_str(), input.filter == ESampler::Trilinear);

					if (!texture.isValid())
						return false;

					mRenderer.addInputResource(input, *texture);
				}
			break;
			case EInputType::Video:
				{
					MediaPlayer* mediaPlayer = new MediaPlayer;
					mediaPlayer->open(loadPath.c_str());
					mMediaPlayers.push_back(mediaPlayer);

					mRenderer.addInputResource(input, *mediaPlayer->mTexFrame);
				}
				break;
			case EInputType::Music:
				break;
			}

			return true;
		}

		TArray< MediaPlayer* > mMediaPlayers;

		void clearnupMedia()
		{
			for( auto player : mMediaPlayers )
			{
				delete player;
			}
			mMediaPlayers.clear();
		}

		std::unique_ptr<AudioDevice> mAudioDevice;
		std::unique_ptr<SoundRenderPassStreamSource> mStreamSource;
		std::unique_ptr<SoundWave>    mSoundWave;
		AudioHandle mSoundHandle = ERROR_AUDIO_HANDLE;

		void initAudio(RenderPassData& pass)
		{
			if (mAudioDevice.get() == nullptr)
			{
				mAudioDevice = std::unique_ptr<AudioDevice>(AudioDevice::Create());
				mStreamSource = std::make_unique<SoundRenderPassStreamSource>();
				mStreamSource->mRenderer = &mRenderer;
				mSoundWave = std::make_unique<SoundWave>();
				mSoundWave->setupStream(mStreamSource.get());
			}

			mStreamSource->mRenderPass = &pass;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}


		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			if (bPause == false)
			{
				mTimePrev = mTime;
				mTime += deltaTime;
				for( auto player: mMediaPlayers )
				{
					player->update(deltaTime);
				}
			}

			if (mAudioDevice)
			{
				mAudioDevice->update(deltaTime);
			}
		}

		int   mFrameCount = 0;
		float mTime = 0;
		float mTimePrev = 0;

		bool  bPause = false;
		Vector4 mMouse = Vector4::Zero();
		uint8   mKeyboardData[256 * 3];


		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();

			{
				GPU_PROFILE("Media Render");
				for (auto player : mMediaPlayers)
				{
					player->render(commandList);
				}
			}

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
			inputParam.iDate = Vector4(now.getYear(), now.getMonth(), now.getDay(), double(now.getTicks() % DateTime::TicksPerDay) / double(DateTime::TicksPerSecond));
			mRenderer.updateInput(inputParam, mKeyboardData);

			if ( mStreamSource )
			{
				GPU_PROFILE("Sound");
				mStreamSource->checkGenerateRequiredStreamingSample(commandList);
			}

			PooledRenderTargetRef rtImage = mRenderer.render(commandList, mRenderPassList, screenSize);

			PooledRenderTargetRef rt;
			if (mRenderer.bUseMultisample)
			{
				RenderTargetDesc desc;
				desc.format = ETexture::RGB10A2;
				desc.numSamples = 8;
				desc.size = screenSize;
				desc.debugName = "Canvas";
				rt = GRenderTargetPool.fetchElement(desc);
				mRenderer.mFrameBuffer->setTexture(0, *rt->texture);
				RHISetFrameBuffer(commandList, mRenderer.mFrameBuffer);
			}
			else
			{
				RHISetFrameBuffer(commandList, nullptr);
			}

			if (mFrameCount == 0)
			{
				RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.0, 0.0, 0.0, 0.0), 1);
			}

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

			if (mRenderer.bUseMultisample)
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
			++mFrameCount;
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
			auto IsVaild = [](uint32 value) -> bool
			{
				return 0 <= value && value < 256;
			};
			auto code = msg.getCode();
			if (IsVaild(code))
			{
				if (msg.isDown())
				{
					if (mKeyboardData[code + 0 * 256] != 255)
					{
						mKeyboardData[code + 0 * 256] = 255;
						mKeyboardData[code + 1 * 256] = 255;
						mKeyboardData[code + 2 * 256] = 255 - mKeyboardData[code + 2 * 256];
					}
				}
				else
				{
					mKeyboardData[code + 0 * 256] = 0;
					mKeyboardData[code + 1 * 256] = 0;
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

		bool setupRenderResource(ERenderSystem systemName) override
		{
			ShaderHelper::Get().init();

			mRenderer.initializeRHI();


			loadProject("Test");
			//loadProject("ShaderArt");
			return true;
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			for (auto& pass : mRenderPassList)
			{
				pass->shader.releaseRHI();
			}

			mRenderer.releaseRHI();
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::OpenGL;
		}


	protected:
	};

	REGISTER_STAGE_ENTRY("Shadertoy", TestStage, EExecGroup::GraphicsTest, "Render");

}//namespace Shadertoy