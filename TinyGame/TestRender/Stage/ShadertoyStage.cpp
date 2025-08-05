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

	class CubeOnePassVS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(CubeOnePassVS, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/ShadertoyTemplate";
		}
	};
	IMPLEMENT_SHADER(CubeOnePassVS, EShader::Vertex, SHADER_ENTRY(CubeOnePassVS));

	class RenderPassShader : public Shader
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamChannelResolution.bind(parameterMap, "iChannelResolution");
			if (passInfo)
			{
				bindInputParameters(passInfo->inputs, parameterMap);
			}
		}

		RenderPassInfo const* passInfo = nullptr;
		void bindInputParameters(TArray<RenderInput> const& inputs, ShaderParameterMap const& parameterMap)
		{
			mParamInputs.resize(inputs.size());
			mParamInputSamplers.resize(inputs.size());
			int index = 0;
			for (auto const& input : inputs)
			{
				mParamInputs[index].bind(parameterMap, InlineString<>::Make("iChannel%d", input.channel));
				++index;
			}
		}

		static RHISamplerState& GetSamplerState(RenderInput const& input)
		{
			if (input.type == EInputType::Keyboard)
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

		static RHITextureBase* GetInputTexture(RenderInput const& input, TArray< RHITextureRef > const& inputResources)
		{
			if (inputResources.isValidIndex(input.resId) && inputResources[input.resId].isValid())
			{
				return inputResources[input.resId];
			}

			switch (input.type)
			{
			case EInputType::Buffer:
				return GBlackTexture2D;
			case EInputType::Texture:
				return GBlackTexture2D;
			case EInputType::CubeMap:
				return GBlackTextureCube;
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

		void setInputParameters(RHICommandList& commandList, TArray<RenderInput> const& inputs, TArray< RHITextureRef > const& inputResources)
		{
			Vector3 channelSize[4] = { Vector3(0,0,0) , Vector3(0,0,0), Vector3(0,0,0) , Vector3(0,0,0) };
			int index = 0;
			for (auto const& input : inputs)
			{
				RHITextureBase* texture = GetInputTexture(input, inputResources);
				if (texture)
				{
					setTexture(commandList, mParamInputs[index], *texture, mParamInputSamplers[index], GetSamplerState(input));
					channelSize[input.channel] = Vector3(texture->getDesc().dimension);
				}
				++index;
			}

			if (mParamChannelResolution.isBound())
			{
				setParam(commandList, mParamChannelResolution, channelSize, ARRAY_SIZE(channelSize));
			}
		}

		void setSoundParameters(RHICommandList& commandList, float timeOffset, int64 sampleOffset)
		{
			setParam(commandList, SHADER_PARAM(iTimeOffset), timeOffset);
			setParam(commandList, SHADER_PARAM(iSampleOffset), (int32)sampleOffset);
		}

		void setCubeParameters(RHICommandList& commandList, IntVector2 const& viewportSize, int subPassIndex = INDEX_NONE)
		{
			if (subPassIndex == INDEX_NONE)
			{
				Vector3 const* faceDirs = ETexture::GetFaceDirArray();
				Vector3 const* faceUpDirs = ETexture::GetFaceUpArray();

				static Vector3 const faceRightDirs[ETexture::FaceCount] =
				{
					faceDirs[0].cross(faceUpDirs[0]),
					faceDirs[1].cross(faceUpDirs[1]),
					faceDirs[2].cross(faceUpDirs[2]),
					faceDirs[3].cross(faceUpDirs[3]),
					faceDirs[4].cross(faceUpDirs[4]),
					faceDirs[5].cross(faceUpDirs[5]),
				};

				setParam(commandList, SHADER_PARAM(iFaceDirs), faceDirs, ETexture::FaceCount);
				setParam(commandList, SHADER_PARAM(iFaceVDirs), faceUpDirs, ETexture::FaceCount);
				setParam(commandList, SHADER_PARAM(iFaceUDirs), faceRightDirs, ETexture::FaceCount);
			}
			else
			{
				Vector3 faceDir = ETexture::GetFaceDir(ETexture::Face(subPassIndex));
				Vector3 faceUpDir = ETexture::GetFaceUpDir(ETexture::Face(subPassIndex));
				setParam(commandList, SHADER_PARAM(iFaceDir), faceDir);
				setParam(commandList, SHADER_PARAM(iFaceVDir), faceUpDir);
				setParam(commandList, SHADER_PARAM(iFaceUDir), faceDir.cross(faceUpDir));
				setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
				setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
			}

			setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
			setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
		};

		ShaderParameter mParamChannelResolution;
		TArray< ShaderParameter > mParamInputs;
		TArray< ShaderParameter > mParamInputSamplers;
		TArray< ShaderParameter > mParamOutputs;
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
		case 1:  format = ETexture::R8; break;
		case 2:  format = ETexture::RG8; break;
		case 3:  format = ETexture::RGB8; break;
		case 4:  format = ETexture::RGBA8; break;
		case 11: format = ETexture::R32F; break;
		case 12: format = ETexture::RG32F; break;
		case 13: format = ETexture::RGB32F; break;
		case 14: format = ETexture::RGBA32F; break;
		}

		TextureDesc desc = TextureDesc::Type3D(format, header.sizeX, header.sizeY, header.sizeZ);
		if (bGenMipmap)
		{
			desc.numMipLevel = RHIUtility::CalcMipmapLevel(Math::Min(header.sizeX, Math::Min(header.sizeY, header.sizeZ)));
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

		void updateInput(InputParam const& inputParam, uint8 keyboardData[])
		{
			mInputBuffer.updateBuffer(inputParam);
			RHIUpdateTexture(*mTexKeyboard, 0, 0, 256, 3, keyboardData);
		}

		void setInputParameters(RHICommandList& commandList, RenderPassData& pass)
		{
			pass.shader.setInputParameters(commandList, pass.info->inputs, mInputResources);
			SetStructuredUniformBuffer(commandList, pass.shader, mInputBuffer);
		}

		static char const* GetOutputName(int index)
		{
			static char const* OutputNames[] = { "OutTexture", "OutTexture1" , "OutTexture2" , "OutTexture3" };
			return OutputNames[index];
		}
		static constexpr int GROUP_SIZE = 8;

		PooledRenderTargetRef render(RHICommandList& commandList, TArray< std::unique_ptr<RenderPassData> > const& renderPassList, IntVector2 const& screenSize)
		{
			GPU_PROFILE(bAllowComputeShader ? "RenderPassCS" : "RenderPass");

			PooledRenderTargetRef outImage;
			int const CubeMapSize = 1024;

			for (auto& pass : renderPassList)
			{
				if (pass->info->passType == EPassType::Sound)
				{
					continue;
				}

				bool bUseComputeShader = pass->shader.getType() == EShader::Compute;
				bool bRenderCubeMap = pass->info->passType == EPassType::CubeMap;

				bool bRenderCubeMapOnePass = bRenderCubeMap && bAllowRenderCubeOnePass;

				TArray< PooledRenderTargetRef, TInlineAllocator<4> > outputBuffers;
				HashString debugName;
				int index;
				IntVector2 viewportSize = bRenderCubeMap ? IntVector2(CubeMapSize, CubeMapSize) : screenSize;

				RenderTargetDesc desc;
				desc.format = ETexture::RGBA32F;
				desc.numSamples = 1;
				if (bUseMultisample && pass->info->passType == EPassType::Image)
				{
					desc.numSamples = 8;
				}
				desc.size = viewportSize;
				desc.type = bRenderCubeMap ? ETexture::TypeCube : ETexture::Type2D;

				if (bUseComputeShader)
				{
					desc.creationFlags |= TCF_CreateUAV;
				}

				index = 0;
				for (auto const& output : pass->info->outputs)
				{
					if (index == 0)
					{
						switch (pass->info->passType)
						{
						case EPassType::Buffer:
							{
								desc.debugName = InlineString<>::Make("Buffer%c", AlphaSeq[pass->info->typeIndex]);
							}
							break;
						case EPassType::CubeMap:
							{
								desc.debugName = InlineString<>::Make("CubeMap%c", AlphaSeq[pass->info->typeIndex]);
							}
							break;
						case EPassType::Image:
							{
								desc.debugName = "Image";
							}
							break;
						}

						if (bRenderCubeMapOnePass)
						{
							debugName = InlineString<>::Make("CubeMap%c OnePass", AlphaSeq[pass->info->typeIndex]);
						}
						else
						{
							debugName = desc.debugName;
						}
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
								desc.debugName = InlineString<>::Make("Image(%d)", output.channel);
							}
							break;
						}
					}

					PooledRenderTargetRef rt;
					rt = GRenderTargetPool.fetchElement(desc);
					outputBuffers.push_back(rt);
					++index;
				}

				GPU_PROFILE(debugName.c_str());

				if (bRenderCubeMapOnePass)
				{
					renderCubeOnePass(commandList, *pass, viewportSize, outputBuffers);
				}
				else
				{
					renderImage(commandList, *pass, viewportSize, outputBuffers);
				}
				if (pass->info->passType == EPassType::Image)
				{
					outImage = outputBuffers[0];
					if (outImage->desc.numSamples > 1)
					{
						outImage->resolve(commandList);
					}
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
						
						if (buffer->desc.numSamples > 1)
						{
							buffer->resolve(commandList);
						}
						++index;

						mInputResources[output.resId] = buffer->resolvedTexture;
					}
				}
			}

			return outImage;
		}

		void renderImage(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers)
		{
			int index;
			auto& passShader = pass.shader;

			auto SetPassParameters = [&](RHICommandList& commandList, int subPassIndex)
			{
				if (pass.info->passType == EPassType::CubeMap)
				{
					passShader.setCubeParameters(commandList, viewportSize, subPassIndex);
				}
			};

			int subPassCount = pass.info->passType == EPassType::CubeMap ? 6 : 1;

			if (passShader.getType() == EShader::Compute)
			{
				RHISetComputeShader(commandList, pass.shader.getRHI());
				setInputParameters(commandList, pass);

				for (int subPassIndex = 0; subPassIndex < subPassCount; ++subPassIndex)
				{
					SetPassParameters(commandList, subPassIndex);

					if (subPassCount == 1)
					{
						index = 0;
						for (auto const& output : pass.info->outputs)
						{
							passShader.setRWTexture(commandList, GetOutputName(output.channel), *outputBuffers[index]->resolvedTexture, 0, EAccessOp::WriteOnly);
							++index;
						}
					}
					else
					{
						index = 0;
						for (auto const& output : pass.info->outputs)
						{
							passShader.setRWSubTexture(commandList, GetOutputName(output.channel), *outputBuffers[index]->resolvedTexture, subPassIndex, 0, EAccessOp::WriteOnly);
							++index;
						}
					}

					RHIDispatchCompute(commandList, Math::AlignCount(viewportSize.x, GROUP_SIZE), Math::AlignCount(viewportSize.y, GROUP_SIZE), 1);
				}

				index = 0;
				for (auto const& output : pass.info->outputs)
				{
					passShader.clearRWTexture(commandList, GetOutputName(output.channel));
					++index;
				}
			}
			else
			{
				RHISetViewport(commandList, 0, 0, viewportSize.x, viewportSize.y);

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				setPixelShader(commandList, passShader);
				setInputParameters(commandList, pass);

				for (int subPassIndex = 0; subPassIndex < subPassCount; ++subPassIndex)
				{
					index = 0;
					for (auto const& output : pass.info->outputs)
					{
						mFrameBuffer->setTexture(output.channel, *outputBuffers[index]->texture, subPassIndex);
						++index;
					}

					RHISetFrameBuffer(commandList, mFrameBuffer);
					SetPassParameters(commandList, subPassIndex);
					DrawUtility::ScreenRect(commandList);
					RHISetFrameBuffer(commandList, nullptr);
				}
			}
		}


		void renderCubeOnePass(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers)
		{
			int index;
			auto& passShader = pass.shader;

			auto SetCubeParameters = [&](RHICommandList& commandList, IntVector2 const& viewportSize)
			{				
				Vector3 const* faceDirs = ETexture::GetFaceDirArray();
				Vector3 const* faceUpDirs = ETexture::GetFaceUpArray();

				static Vector3 const faceRightDirs[ETexture::FaceCount] =
				{
					faceDirs[0].cross(faceUpDirs[0]),
					faceDirs[1].cross(faceUpDirs[1]),
					faceDirs[2].cross(faceUpDirs[2]),
					faceDirs[3].cross(faceUpDirs[3]),
					faceDirs[4].cross(faceUpDirs[4]),
					faceDirs[5].cross(faceUpDirs[5]),
				};

				passShader.setParam(commandList, SHADER_PARAM(iFaceDirs), faceDirs, ETexture::FaceCount);
				passShader.setParam(commandList, SHADER_PARAM(iFaceVDirs), faceUpDirs, ETexture::FaceCount);
				passShader.setParam(commandList, SHADER_PARAM(iFaceUDirs), faceRightDirs, ETexture::FaceCount);

				passShader.setParam(commandList, SHADER_PARAM(iOrigin), Vector3(0, 0, 0));
				passShader.setParam(commandList, SHADER_PARAM(iViewportSize), Vector2(viewportSize));
			};
			
			if (passShader.getType() == EShader::Compute)
			{
				RHISetComputeShader(commandList, pass.shader.getRHI());
				setInputParameters(commandList, pass);
				passShader.setCubeParameters(commandList, viewportSize);

				index = 0;
				for (auto const& output : pass.info->outputs)
				{
					passShader.setRWTexture(commandList, GetOutputName(output.channel), *outputBuffers[index]->resolvedTexture, 0, EAccessOp::WriteOnly);
					++index;
				}

				RHIDispatchCompute(commandList, Math::AlignCount(viewportSize.x, GROUP_SIZE), Math::AlignCount(viewportSize.y, GROUP_SIZE), 6);

				index = 0;
				for (auto const& output : pass.info->outputs)
				{
					passShader.clearRWTexture(commandList, GetOutputName(output.channel));
					++index;
				}
			}
			else
			{
				RHISetViewport(commandList, 0, 0, viewportSize.x, viewportSize.y);

				index = 0;
				for (auto const& output : pass.info->outputs)
				{
					mFrameBuffer->setTextureArray(output.channel, static_cast<RHITextureCube&>(*outputBuffers[index]->texture));
					++index;
				}

				RHISetFrameBuffer(commandList, mFrameBuffer);

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				GraphicsShaderStateDesc state;
				state.vertex = mCubeOnePassVS->getRHI();
				state.pixel = passShader.getRHI();
				RHISetGraphicsShaderBoundState(commandList, state);

				setInputParameters(commandList, pass);
				passShader.setCubeParameters(commandList, viewportSize);

				DrawUtility::ScreenRect(commandList, ETexture::FaceCount);
				RHISetFrameBuffer(commandList, nullptr);
			}
		}

		void renderSound(RHICommandList& commandList, RenderPassData& pass, WaveFormatInfo const& format, int64 samplePos, int sampleCount, AudioStreamSample& outData)
		{
			auto& passShader = pass.shader;

			int blockFrame = SoundTextureSize * SoundTextureSize;

			int numBlocks = Math::AlignCount(sampleCount, blockFrame);

			if (passShader.getType() == EShader::Compute)
			{
				RHISetComputeShader(commandList, pass.shader.getRHI());
				setInputParameters(commandList, pass);

				passShader.setRWTexture(commandList, GetOutputName(0), *mTexSound, 0, EAccessOp::WriteOnly);

				int16* pData = (int16*)outData.data;
				for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock)
				{
					int64 sampleOffset = samplePos + indexBlock * blockFrame;
					float timeOffset = float(sampleOffset) / format.sampleRate;
					passShader.setSoundParameters(commandList, timeOffset, sampleOffset);

					int dispatchCount = Math::AlignCount(SoundTextureSize, GROUP_SIZE);
					RHIDispatchCompute(commandList, dispatchCount, dispatchCount, 1);
					RHIFlushCommand(commandList);

					int numRead = Math::Min(sampleCount - indexBlock * blockFrame, blockFrame);
					readSampleData(pData, numRead);
					pData += numRead * format.numChannels;
				}

				passShader.clearRWTexture(commandList, GetOutputName(0));
			}
			else
			{
				setRenderTarget(commandList, *mTexSound);

				RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 0), 1);
				RHISetViewport(commandList, 0, 0, SoundTextureSize, SoundTextureSize);

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				setPixelShader(commandList, passShader);
				setInputParameters(commandList, pass);

				int16* pData = (int16*)outData.data;
				for (int indexBlock = 0; indexBlock < numBlocks; ++indexBlock)
				{
					int64 sampleOffset = samplePos + indexBlock * blockFrame;
					float timeOffset = float(sampleOffset) / format.sampleRate;
					passShader.setSoundParameters(commandList, timeOffset, sampleOffset);

					DrawUtility::ScreenRect(commandList);
					RHIFlushCommand(commandList);

					int numRead = Math::Min(sampleCount - indexBlock * blockFrame, blockFrame);
					readSampleData(pData, numRead);
					pData += numRead * format.numChannels;
				}
			}

		}

		bool initializeRHI()
		{
			mFrameBuffer = RHICreateFrameBuffer();

			ScreenVS::PermutationDomain domainVector;
			domainVector.set<ScreenVS::UseTexCoord>(false);
			mScreenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector);
			mCubeOnePassVS = ShaderManager::Get().getGlobalShaderT<CubeOnePassVS>();

			mTexSound = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, SoundTextureSize, SoundTextureSize).AddFlags(TCF_AllowCPUAccess | TCF_CreateUAV | TCF_RenderTarget));
			GTextureShowManager.registerTexture("SoundTexture", mTexSound);
			mTexKeyboard = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R8, 256, 3));
			GTextureShowManager.registerTexture("Keyborad", mTexKeyboard);
			mInputBuffer.initializeResource(1);
			return true;
		}

		void releaseRHI()
		{
			mFrameBuffer.release();
			mScreenVS = nullptr;
			mTexSound.release();
			mTexKeyboard.release();
			mInputBuffer.releaseResource();
		}

		static TVector2<int16> Decode(uint8 const* pData)
		{
			int16 left = int16(MaxInt16 *(-1.0 + 2.0*(pData[0] + 256.0*pData[1]) / 65535.0));
			int16 right = int16(MaxInt16 *(-1.0 + 2.0*(pData[2] + 256.0*pData[3]) / 65535.0));
			return { left, right };
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

		void addInputResource(RenderInput const& input, RHITextureBase& texture)
		{
			if (mInputResources.size() < input.resId + 1)
			{
				mInputResources.resize(input.resId + 1);
			}
			mInputResources[input.resId] = &texture;
			GTextureShowManager.registerTexture(InlineString<>::Make("Res%d", input.resId).c_str(), &texture);
		}

		bool canUseComputeShader(RenderPassData const& pass)
		{
			return bAllowComputeShader /*&& pass.info->passType != EPassType::CubeMap*/;
		}

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

			ShaderCompileOption option;

			std::string code;
			Text::Format((char const*)codeTemplate.data(), { StringView(channelCode), commonInfo ? StringView(commonInfo->code) : StringView(), StringView(pass.info->code) }, code);
			option.addCode(code.c_str());

			bool bUseComputeShader = canUseComputeShader(pass);
			if (bUseComputeShader)
			{
				if (pass.info->code.find("discard") != std::string::npos ||
					(commonInfo && commonInfo->code.find("discard") != std::string::npos))
				{
					option.addDefine(SHADER_PARAM(COMPUTE_DISCARD_SIM), 1);
				}
				option.addDefine(SHADER_PARAM(GROUP_SIZE), GROUP_SIZE);
			}

			switch (pass.info->passType)
			{
			case EPassType::Buffer:
				option.addDefine(SHADER_PARAM(RENDER_BUFFER), 1);
				break;
			case EPassType::Sound:
				option.addDefine(SHADER_PARAM(RENDER_SOUND), 1);
				option.addDefine(SHADER_PARAM(SOUND_TEXTURE_SIZE), SoundTextureSize);
				break;
			case EPassType::CubeMap:
				option.addDefine(SHADER_PARAM(RENDER_CUBE), 1);
				if (bAllowRenderCubeOnePass)
				{
					option.addDefine(SHADER_PARAM(RENDER_CUBE_ONE_PASS), 1);
				}
				break;
			case EPassType::Image:
				option.addDefine(SHADER_PARAM(RENDER_IMAGE), 1);
				break;
			}

			ShaderEntryInfo entry = bUseComputeShader ? ShaderEntryInfo{ EShader::Compute, "MainCS" } : ShaderEntryInfo{ EShader::Pixel, "MainPS" };
			
			pass.shader.passInfo = pass.info;
			if (!ShaderManager::Get().loadFile(pass.shader, nullptr, entry, option))
				return false;

			return true;
		}


		bool bAllowComputeShader = false;
		bool bAllowRenderCubeOnePass = true;
		bool bUseMultisample = false;


		TArray< PooledRenderTargetRef > mOutputBuffers;
		TArray< RHITextureRef > mInputResources;

		static constexpr int SoundSampleRate = 44100;
		static constexpr int SoundTextureSize = 256;

		ScreenVS* mScreenVS;
		CubeOnePassVS* mCubeOnePassVS;
		RHIFrameBufferRef mFrameBuffer;
		TStructuredBuffer< InputParam > mInputBuffer;
		RHITexture2DRef mTexKeyboard;
		RHITexture2DRef mTexSound;
	};

	class N12ConvertPS : public GlobalShader
	{
		DECLARE_SHADER(N12ConvertPS, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/MediaShaders";
		}
	};

	IMPLEMENT_SHADER(N12ConvertPS, EShader::Pixel, SHADER_ENTRY(N12ConvertPS));

	class MediaPlayer : public IMFSourceReaderCallback
	{
	public:
		bool load(char const* path)
		{
			static MediaFoundationScope MFScope;

			path = "Shadertoy/Assets/dd.mp4";
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

			commitPosition(mCurrentTime);
			mHaveSample = false;
			mbReadSampleRequested = false;
			return true;
		}
		static constexpr int64 SecondTimeUnit = 10000000LL;

		double mDuration = 0.0f;
		double mPlayRate = 2.0f;
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

			ScreenVS::PermutationDomain domainVector;
			domainVector.set<ScreenVS::UseTexCoord>(true);
			auto screenVS = ShaderManager::Get().getGlobalShaderT<ScreenVS>(domainVector, true);
			auto n12ConvertPS = ShaderManager::Get().getGlobalShaderT<N12ConvertPS>(true);

			GraphicsShaderStateDesc stateDesc;
			stateDesc.vertex = screenVS->getRHI();
			stateDesc.pixel = n12ConvertPS->getRHI();
			RHISetGraphicsShaderBoundState(commandList, stateDesc);
			n12ConvertPS->setParam(commandList, SHADER_PARAM(UVScale), Vector2(float(mFrameSize.x) / mAligendFrameSize.x, float(mFrameSize.y) / mAligendFrameSize.y));
			n12ConvertPS->setParam(commandList, SHADER_PARAM(SampleWidth), float(mAligendFrameSize.x));
			n12ConvertPS->setTexture(commandList, SHADER_PARAM(Texture), *mTexSample, SHADER_SAMPLER(Texture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());

			Matrix4 const colorTransform = Matrix4(
				1.16438356164f, 0.000000000000f, 1.792652263418f, 0.000000f,
				1.16438356164f, -0.213237021569f, -0.533004040142f, 0.000000f,
				1.16438356164f, 2.112419281991f, 0.000000000000f, 0.000000f,
				0.000000f, 0.000000f, 0.000000f, 0.000000f
			) * Matrix4(
				1.0f, 0.0f, 0.0f, -0.06274509803921568627f,
				0.0f, 1.0f, 0.0f, -0.5019607843137254902f,
				0.0f, 0.0f, 1.0f, -0.5019607843137254902f,
				0.0f, 0.0f, 0.0f, 1.000000f
			);
			n12ConvertPS->setParam(commandList, SHADER_PARAM(ColorTransform), colorTransform);

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
					input.type = EInputType::Vedio;
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
					case EInputType::Vedio:
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
			std::vector<uint8> codeTemplate;
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
			case EInputType::Vedio:
				{
					MediaPlayer* mediaPlayer = new MediaPlayer;
					mediaPlayer->load(loadPath.c_str());
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

			mTimePrev = mTime;
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