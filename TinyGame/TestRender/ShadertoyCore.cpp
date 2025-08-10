#include "ShadertoyCore.h"

#include "RHI/DrawUtility.h"
#include "RHI/ShaderManager.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIUtility.h"

#include "Serialize/FileStream.h"

#include "RenderDebug.h"
#include "Misc/Format.h"


namespace Shadertoy
{
	IMPLEMENT_SHADER(CubeOnePassVS, EShader::Vertex, SHADER_ENTRY(CubeOnePassVS));

	void RenderPassShader::bindParameters(ShaderParameterMap const& parameterMap)
	{
		mParamChannelResolution.bind(parameterMap, "iChannelResolution");
		if (passInfo)
		{
			bindInputParameters(passInfo->inputs, parameterMap);
		}
	}

	void RenderPassShader::bindInputParameters(TArray<RenderInput> const& inputs, ShaderParameterMap const& parameterMap)
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

	RHISamplerState& RenderPassShader::GetSamplerState(RenderInput const& input)
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

	RHITextureBase* RenderPassShader::GetInputTexture(RenderInput const& input, TArray< RHITextureRef > const& inputResources)
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

	void RenderPassShader::setInputParameters(RHICommandList& commandList, TArray<RenderInput> const& inputs, TArray< RHITextureRef > const& inputResources)
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

	void RenderPassShader::setSoundParameters(RHICommandList& commandList, float timeOffset, int64 sampleOffset)
	{
		setParam(commandList, SHADER_PARAM(iTimeOffset), timeOffset);
		setParam(commandList, SHADER_PARAM(iSampleOffset), (int32)sampleOffset);
	}

	void RenderPassShader::setCubeParameters(RHICommandList& commandList, IntVector2 const& viewportSize, int subPassIndex /*= INDEX_NONE*/)
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
	}

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

	void Renderer::setRenderTarget(RHICommandList& commandList, RHITexture2D& target)
	{
		mFrameBuffer->setTexture(0, target);
		RHISetFrameBuffer(commandList, mFrameBuffer);
	}

	void Renderer::setPixelShader(RHICommandList& commandList, RenderPassShader& shader)
	{
		GraphicsShaderStateDesc state;
		state.vertex = mScreenVS->getRHI();
		state.pixel = shader.getRHI();
		RHISetGraphicsShaderBoundState(commandList, state);
	}

	void Renderer::updateInput(InputParam const& inputParam, uint8 keyboardData[])
	{
		mInputBuffer.updateBuffer(inputParam);
		RHIUpdateTexture(*mTexKeyboard, 0, 0, 256, 3, keyboardData);
	}

	void Renderer::setInputParameters(RHICommandList& commandList, RenderPassData& pass)
	{
		pass.shader.setInputParameters(commandList, pass.info->inputs, mInputResources);
		SetStructuredUniformBuffer(commandList, pass.shader, mInputBuffer);
	}

	PooledRenderTargetRef Renderer::render(RHICommandList& commandList, TArray< std::unique_ptr<RenderPassData> > const& renderPassList, IntVector2 const& screenSize)
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

	void Renderer::renderImage(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers)
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

	void Renderer::renderCubeOnePass(RHICommandList& commandList, RenderPassData& pass, IntVector2 const& viewportSize, TArrayView< PooledRenderTargetRef > outputBuffers)
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

	void Renderer::renderSound(RHICommandList& commandList, RenderPassData& pass, WaveFormatInfo const& format, int64 samplePos, int sampleCount, AudioStreamSample& outData)
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

	bool Renderer::initializeRHI()
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

	void Renderer::releaseRHI()
	{
		mFrameBuffer.release();
		mScreenVS = nullptr;
		mTexSound.release();
		mTexKeyboard.release();
		mInputBuffer.releaseResource();
	}

	void Renderer::readSampleData(int16* pOutData, int numSamples)
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

	void Renderer::addInputResource(RenderInput const& input, RHITextureBase& texture)
	{
		if (mInputResources.size() < input.resId + 1)
		{
			mInputResources.resize(input.resId + 1);
		}
		mInputResources[input.resId] = &texture;
		GTextureShowManager.registerTexture(InlineString<>::Make("Res%d", input.resId).c_str(), &texture);
	}

	bool Renderer::compileShader(RenderPassData& pass, TArrayView<uint8 const> codeTemplate, RenderPassInfo* commonInfo)
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

}//namespace Shadertoy