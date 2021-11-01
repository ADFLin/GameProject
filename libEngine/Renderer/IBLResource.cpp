#include "IBLResource.h"

#include "RHI/GlobalShader.h"
#include "RHI/DrawUtility.h"
#include "RHI/RHICommand.h"
#include "RHI/ShaderManager.h"

#include "DataCacheInterface.h"
#include "ProfileSystem.h"

//#TODO : remove
#include "RHI/D3D11Command.h"
#include "RHI/OpenGLCommon.h"


namespace Render
{
	class EquirectangularToCubeProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(EquirectangularToCubeProgram, Global);

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(EquirectangularToCubePS) },
			};
			return entries;
		}

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
	};

	class EquirectangularToCubePS : public GlobalShader
	{
	public:
		using BaseClass = GlobalShader;
		DECLARE_SHADER(EquirectangularToCubePS, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
	};


	class IrradianceGenProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(IrradianceGenProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(IrradianceGenPS) },
			};
			return entries;
		}
	};


	class IrradianceGenPS : public GlobalShader
	{
	public:
		using BaseClass = GlobalShader;
		DECLARE_SHADER(IrradianceGenPS, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
	};

	template< class TShader >
	class TPrefilteredGen : public TShader
	{
	public:
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, Roughness);
			BIND_TEXTURE_PARAM(parameterMap, CubeTexture);
			BIND_SHADER_PARAM(parameterMap, PrefilterSampleCount);
		}

		void setParameters(RHICommandList& commandList, int level, RHITextureCube& texture, int sampleCount)
		{
			SET_SHADER_PARAM(commandList, *this, Roughness, float(level) / (IBLResource::NumPerFilteredLevel - 1));

			auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI();
			SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *this, CubeTexture, texture, sampler);
			SET_SHADER_PARAM(commandList, *this, PrefilterSampleCount, sampleCount);
		}

		DEFINE_SHADER_PARAM(Roughness);
		DEFINE_TEXTURE_PARAM(CubeTexture);
		DEFINE_SHADER_PARAM(PrefilterSampleCount);

	};

	class PrefilteredGenProgram : public TPrefilteredGen< GlobalShaderProgram >
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(PrefilteredGenProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(PrefilteredGenPS) },
			};
			return entries;
		}
	};

	class PrefilteredGenPS : public TPrefilteredGen < GlobalShader >
	{
	public:
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PrefilteredGenPS, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
	};

	class PreIntegrateBRDFGenProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(PreIntegrateBRDFGenProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(PreIntegrateBRDFGenPS) },
			};
			return entries;
		}
	};


	class PreIntegrateBRDFGenPS : public GlobalShader
	{
	public:
		using BaseClass = GlobalShader;
		DECLARE_SHADER(PreIntegrateBRDFGenPS, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
	};

	IMPLEMENT_SHADER(EquirectangularToCubePS, EShader::Pixel, SHADER_ENTRY(EquirectangularToCubePS));
	IMPLEMENT_SHADER(IrradianceGenPS, EShader::Pixel, SHADER_ENTRY(IrradianceGenPS));
	IMPLEMENT_SHADER(PrefilteredGenPS, EShader::Pixel, SHADER_ENTRY(PrefilteredGenPS));
	IMPLEMENT_SHADER(PreIntegrateBRDFGenPS, EShader::Pixel, SHADER_ENTRY(PreIntegrateBRDFGenPS));

	IMPLEMENT_SHADER_PROGRAM(EquirectangularToCubeProgram);
	IMPLEMENT_SHADER_PROGRAM(IrradianceGenProgram);
	IMPLEMENT_SHADER_PROGRAM(PrefilteredGenProgram);
	IMPLEMENT_SHADER_PROGRAM(PreIntegrateBRDFGenProgram);

	TGlobalRenderResource< RHITexture2D > IBLResource::SharedBRDFTexture;

	int GetFormatClientSize(ETexture::Format format)
	{
		int formatSize = ETexture::GetFormatSize(format);
		if (GRHISystem->getName() == RHISystemName::OpenGL && ETexture::GetComponentType(format) == CVT_Half)
		{
			formatSize *= 2;
		}
		return formatSize;
	}

	void IBLResource::fillData(ImageBaseLightingData& outData)
	{
		outData.envMapSize = texture->getSize();
		outData.irradianceSize = irradianceTexture->getSize();
		outData.perFilteredSize = perfilteredTexture->getSize();
		ReadTextureData(*texture, ETexture::FloatRGBA, 0, outData.envMap);
		ReadTextureData(*irradianceTexture, ETexture::FloatRGBA, 0, outData.irradiance);
		for (int level = 0; level < IBLResource::NumPerFilteredLevel; ++level)
		{
			ReadTextureData(*perfilteredTexture, ETexture::FloatRGBA, level, outData.perFiltered[level]);
		}
	}

	void IBLResource::GetCubeMapData(std::vector< uint8 >& data, ETexture::Format format, int size, int level, void* outData[])
	{
		int formatSize = GetFormatClientSize(format);
		int textureSize = Math::Max(size >> level, 1);
		int faceDataSize = textureSize * textureSize * formatSize;

		for (int i = 0; i < ETexture::FaceCount; ++i)
			outData[i] = &data[i * faceDataSize];
	}

	void IBLResource::ReadTextureData(RHITextureCube& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
	{
		int formatSize = GetFormatClientSize(format);
		int textureSize = Math::Max(texture.getSize() >> level, 1);
		int faceDataSize = textureSize * textureSize * formatSize;
		outData.resize(ETexture::FaceCount * faceDataSize);
		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			OpenGLCast::To(&texture)->bind();
			for (int i = 0; i < ETexture::FaceCount; ++i)
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), &outData[faceDataSize*i]);
			}
			OpenGLCast::To(&texture)->unbind();
		}
		else if ( GRHISystem->getName() == RHISystemName::D3D11 )
		{
			TComPtr<ID3D11Texture2D> stagingTexture;
			static_cast<D3D11System*>(GRHISystem)->createStagingTexture(D3D11Cast::GetResource(texture), stagingTexture);
			static_cast<D3D11System*>(GRHISystem)->mDeviceContext->CopyResource(stagingTexture, D3D11Cast::GetResource(texture));

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			static_cast<D3D11System*>(GRHISystem)->mDeviceContext->Map(stagingTexture, level, D3D11_MAP_READ, 0, &mappedResource);
			memcpy(outData.data(), mappedResource.pData, outData.size());
			static_cast<D3D11System*>(GRHISystem)->mDeviceContext->Unmap(stagingTexture, level);
		}
	}

	void IBLResource::ReadTextureData(RHITexture2D& texture, ETexture::Format format, int level, std::vector< uint8 >& outData)
	{
		int formatSize = GetFormatClientSize(format);
		int dataSize = Math::Max(texture.getSizeX() >> level, 1) * Math::Max(texture.getSizeY() >> level, 1) * formatSize;
		outData.resize(dataSize);
		if (GRHISystem->getName() == RHISystemName::OpenGL)
		{
			OpenGLCast::To(&texture)->bind();
			glGetTexImage(GL_TEXTURE_2D, level, OpenGLTranslate::BaseFormat(format), OpenGLTranslate::TextureComponentType(format), &outData[0]);
			OpenGLCast::To(&texture)->unbind();
		}
		else if (GRHISystem->getName() == RHISystemName::D3D11)
		{
			TComPtr<ID3D11Texture2D> stagingTexture;
			static_cast<D3D11System*>(GRHISystem)->createStagingTexture(D3D11Cast::GetResource(texture), stagingTexture);
			static_cast<D3D11System*>(GRHISystem)->mDeviceContext->CopyResource(stagingTexture, D3D11Cast::GetResource(texture));

			D3D11_MAPPED_SUBRESOURCE mappedResource;
			static_cast<D3D11System*>(GRHISystem)->mDeviceContext->Map(stagingTexture, level, D3D11_MAP_READ, 0, &mappedResource);
			memcpy(outData.data(), mappedResource.pData, outData.size());
			static_cast<D3D11System*>(GRHISystem)->mDeviceContext->Unmap(stagingTexture, level);
		}
	}

	bool IBLResource::initializeRHI(ImageBaseLightingData& IBLData)
	{
		void* data[6];
		{
			TIME_SCOPE("Create EnvMap Texture");
			GetCubeMapData(IBLData.envMap, ETexture::FloatRGBA, IBLData.envMapSize, 0, data);
			VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(ETexture::FloatRGBA, IBLData.envMapSize, 1, TCF_DefalutValue, data));
		}
		{
			TIME_SCOPE("Create Irradiance Texture");
			GetCubeMapData(IBLData.irradiance, ETexture::FloatRGBA, IBLData.irradianceSize, 0, data);
			VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(ETexture::FloatRGBA, IBLData.irradianceSize, 1, TCF_DefalutValue, data));
		}

		{
			TIME_SCOPE("Create Perfiltered Texture");
			VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(ETexture::FloatRGBA, IBLData.perFilteredSize, NumPerFilteredLevel));
			for (int level = 0; level < IBLResource::NumPerFilteredLevel; ++level)
			{
				GetCubeMapData(IBLData.perFiltered[level], ETexture::FloatRGBA, IBLData.perFilteredSize, level, data);
				for (int face = 0; face < ETexture::FaceCount; ++face)
				{
					int size = Math::Max(1, IBLData.perFilteredSize >> level);
					perfilteredTexture->update(ETexture::Face(face), 0, 0, size, size, ETexture::FloatRGBA, data[face], level);
				}
			}
		}
		return true;
	}

	bool IBLResource::initializeRHI(IBLBuildSetting const& setting)
	{
		VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(ETexture::FloatRGBA, setting.envSize, 0, TCF_DefalutValue | TCF_RenderTarget));
		VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(ETexture::FloatRGBA, setting.irradianceSize, 0, TCF_DefalutValue | TCF_RenderTarget));
		VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(ETexture::FloatRGBA, setting.perfilteredSize, NumPerFilteredLevel, TCF_DefalutValue | TCF_RenderTarget));
		return true;
	}

	bool IBLResource::InitializeBRDFTexture(void* data)
	{
		uint32 flags = TCF_DefalutValue;
		if (data == nullptr)
		{
			flags |= TCF_RenderTarget;
		}
		SharedBRDFTexture.initialize(RHICreateTexture2D(ETexture::FloatRGBA, 512, 512, 0, 1, flags, data));
		return SharedBRDFTexture.isValid();
	}

	bool IBLResourceBuilder::initializeShaderProgram()
	{
		if (mEquirectangularToCubePS != nullptr)
			return true;

#//if USE_SEPARATE_SHADER
		VERIFY_RETURN_FALSE(mScreenVS = ShaderManager::Get().getGlobalShaderT< ScreenVS >(true));
		VERIFY_RETURN_FALSE(mEquirectangularToCubePS = ShaderManager::Get().getGlobalShaderT< EquirectangularToCubePS >(true));
		VERIFY_RETURN_FALSE(mIrradianceGenPS = ShaderManager::Get().getGlobalShaderT< IrradianceGenPS >(true));
		VERIFY_RETURN_FALSE(mPrefilteredGenPS = ShaderManager::Get().getGlobalShaderT< PrefilteredGenPS >(true));
		VERIFY_RETURN_FALSE(mPreIntegrateBRDFGenPS = ShaderManager::Get().getGlobalShaderT< PreIntegrateBRDFGenPS >(true));
//#else
		VERIFY_RETURN_FALSE(mProgEquirectangularToCube = ShaderManager::Get().getGlobalShaderT< EquirectangularToCubeProgram >(true));
		VERIFY_RETURN_FALSE(mProgIrradianceGen = ShaderManager::Get().getGlobalShaderT< IrradianceGenProgram >(true));
		VERIFY_RETURN_FALSE(mProgPrefilteredGen = ShaderManager::Get().getGlobalShaderT< PrefilteredGenProgram >(true));
		VERIFY_RETURN_FALSE(mProgPreIntegrateBRDFGen = ShaderManager::Get().getGlobalShaderT< PreIntegrateBRDFGenProgram >(true));
//#endif	

		return true;
	}


	void IBLResourceBuilder::releaseRHI()
	{
		mScreenVS = nullptr;
		mEquirectangularToCubePS = nullptr;
		mIrradianceGenPS = nullptr;
		mPrefilteredGenPS = nullptr;
		mPreIntegrateBRDFGenPS = nullptr;
		mProgEquirectangularToCube = nullptr;
		mProgIrradianceGen = nullptr;
		mProgPrefilteredGen = nullptr;
		mProgPreIntegrateBRDFGen = nullptr;
	}

	bool IBLResourceBuilder::loadOrBuildResource(DataCacheInterface& dataCache, char const* path, RHITexture2D& HDRImage, IBLResource& resource, IBLBuildSetting const& setting)
	{
		auto const GetBRDFCacheKey = [&setting]()->DataCacheKey
		{
			DataCacheKey cacheKey;
			cacheKey.typeName = "IBL-BRDF-LUT";
			cacheKey.version = "7A38AC9D-1EFC-4537-898E-BC8552AD7758";
			cacheKey.keySuffix.add(setting.BRDFSampleCount);
			return cacheKey;
		};

		{
			DataCacheKey cacheKey;
			cacheKey.typeName = "IBL";
			cacheKey.version = "7A38AC9D-1EFC-4537-898E-BC8552AD7758";
			cacheKey.keySuffix.add(path, setting.envSize, setting.irradianceSize, setting.perfilteredSize,
				setting.irradianceSampleCount[0], setting.irradianceSampleCount[1], setting.prefilterSampleCount);

			auto LoadFunc = [&resource](IStreamSerializer& serializer) -> bool
			{
				ImageBaseLightingData IBLData;
				{
					TIME_SCOPE("Load ImageBaseLighting Data");
					serializer >> IBLData;
				}
				{
					TIME_SCOPE("Init IBLResource RHI");
					if (!resource.initializeRHI(IBLData))
						return false;
				}

				return true;
			};

			if (!dataCache.loadDelegate(cacheKey, LoadFunc))
			{
				if (!resource.initializeRHI(setting))
					return false;

				if (!buildIBLResource(HDRImage, resource, setting))
				{
					return false;
				}

				dataCache.saveDelegate(cacheKey, [&resource](IStreamSerializer& serializer) -> bool
				{
					ImageBaseLightingData IBLData;
					resource.fillData(IBLData);
					serializer << IBLData;
					return true;
				});


				dataCache.saveDelegate(GetBRDFCacheKey(), [this](IStreamSerializer& serializer) -> bool
				{
					std::vector< uint8 > data;
					IBLResource::FillSharedBRDFData(data);
					serializer << data;
					return true;
				});
			}
			else
			{
				auto LoadFunc = [this](IStreamSerializer& serializer) -> bool
				{
					std::vector< uint8 > data;
					{
						TIME_SCOPE("Serialize BRDF Data");
						serializer >> data;
					}
					{
						TIME_SCOPE("Initialize BRDF Texture");
						if (!IBLResource::InitializeBRDFTexture(data.data()))
							return false;
					}

					return true;
				};

				TIME_SCOPE("Load BRDF Data");
				if (!dataCache.loadDelegate(GetBRDFCacheKey(), LoadFunc))
				{
					return false;
				}
			}
		}

		return true;
	}


	template< class TFunc >
	void RenderCubeTexture(RHICommandList& commandList, RHIFrameBufferRef& frameBuffer, RHITextureCube& cubeTexture, ShaderProgram& updateShader, int level, TFunc&& shaderSetup)
	{
		int size = cubeTexture.getSize() >> level;

		RHISetViewport(commandList, 0, 0, size, size);
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());
		for (int i = 0; i < ETexture::FaceCount; ++i)
		{
			frameBuffer->setTexture(0, cubeTexture, ETexture::Face(i), level);
			RHISetFrameBuffer(commandList, frameBuffer);
			RHISetShaderProgram(commandList, updateShader.getRHIResource());
			updateShader.setParam(commandList, SHADER_PARAM(FaceDir), ETexture::GetFaceDir(ETexture::Face(i)));
			updateShader.setParam(commandList, SHADER_PARAM(FaceUpDir), ETexture::GetFaceUpDir(ETexture::Face(i)));
			shaderSetup(commandList);
			DrawUtility::ScreenRect(commandList);
			RHISetFrameBuffer(commandList, nullptr);
		}
	}

	template< class TFunc >
	void IBLResourceBuilder::renderCubeTexture(RHICommandList& commandList, RHIFrameBufferRef& frameBuffer, RHITextureCube& cubeTexture, GlobalShader& shaderPS, int level, TFunc&& shaderSetup)
	{
		int size = cubeTexture.getSize() >> level;

		RHISetViewport(commandList, 0, 0, size, size);
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());

		for (int i = 0; i < ETexture::FaceCount; ++i)
		{
			frameBuffer->setTexture(0, cubeTexture, ETexture::Face(i), level);

			RHISetFrameBuffer(commandList, frameBuffer);

			GraphicsShaderStateDesc boundState;
			boundState.vertex = mScreenVS->getRHIResource();
			boundState.pixel = shaderPS.getRHIResource();
			RHISetGraphicsShaderBoundState(commandList, boundState);

			shaderPS.setParam(commandList, SHADER_PARAM(FaceDir), ETexture::GetFaceDir(ETexture::Face(i)));
			shaderPS.setParam(commandList, SHADER_PARAM(FaceUpDir), ETexture::GetFaceUpDir(ETexture::Face(i)));

			shaderSetup(commandList);
			DrawUtility::ScreenRect(commandList);

			RHISetFrameBuffer(commandList, nullptr);
		}
	}


	bool IBLResourceBuilder::buildIBLResource(RHITexture2D& envTexture, IBLResource& resource, IBLBuildSetting const& setting)
	{
		if (!initializeShaderProgram())
			return false;

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		RHIFrameBufferRef frameBuffer = RHICreateFrameBuffer();



#if USE_SEPARATE_SHADER && 0
		renderCubeTexture(commandList, frameBuffer, *resource.texture, *mEquirectangularToCubePS, 0, [&](RHICommandList& commandList)
		{
			mEquirectangularToCubePS->setTexture(commandList,
				SHADER_PARAM(Texture), envTexture,
				SHADER_PARAM(TextureSampler), TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI());
		});
#else
		RenderCubeTexture(commandList, frameBuffer, *resource.texture, *mProgEquirectangularToCube, 0, [&](RHICommandList& commandList)
		{
			mProgEquirectangularToCube->setTexture(commandList,
				SHADER_PARAM(Texture), envTexture,
				SHADER_PARAM(TextureSampler), TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI());
		});
#endif



#if USE_SEPARATE_SHADER && 0
		//IrradianceTexture
		renderCubeTexture(commandList, frameBuffer, *resource.irradianceTexture, *mIrradianceGenPS, 0, [&](RHICommandList& commandList)
		{
			mIrradianceGenPS->setTexture(commandList, SHADER_PARAM(CubeTexture), resource.texture, SHADER_SAMPLER(CubeTexture), TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI());
			mIrradianceGenPS->setParam(commandList, SHADER_PARAM(IrrianceSampleCount), setting.irradianceSampleCount[0], setting.irradianceSampleCount[1]);
		});
#else
		RenderCubeTexture(commandList, frameBuffer, *resource.irradianceTexture, *mProgIrradianceGen, 0, [&](RHICommandList& commandList)
		{
			mProgIrradianceGen->setTexture(commandList, SHADER_PARAM(CubeTexture), resource.texture, SHADER_SAMPLER(CubeTexture), TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp >::GetRHI());
			mProgIrradianceGen->setParam(commandList, SHADER_PARAM(IrrianceSampleCount), setting.irradianceSampleCount[0], setting.irradianceSampleCount[1]);
		});

#endif


		for (int level = 0; level < IBLResource::NumPerFilteredLevel; ++level)
		{
#if USE_SEPARATE_SHADER && 0
			renderCubeTexture(commandList, frameBuffer, *resource.perfilteredTexture, *mPrefilteredGenPS, level, [&](RHICommandList& commandList)
			{
				mPrefilteredGenPS->setParameters(commandList, level, *resource.texture, setting.prefilterSampleCount);
			});
#else
			RenderCubeTexture(commandList, frameBuffer, *resource.perfilteredTexture, *mProgPrefilteredGen, level, [&](RHICommandList& commandList)
			{
				mProgPrefilteredGen->setParameters(commandList, level, *resource.texture, setting.prefilterSampleCount);
			});
#endif
		}

		if (!resource.SharedBRDFTexture.isValid())
		{
			IBLResource::InitializeBRDFTexture(nullptr);
			frameBuffer->setTexture(0, *resource.SharedBRDFTexture);
			RHISetFrameBuffer(commandList, frameBuffer);


			RHISetViewport(commandList, 0, 0, resource.SharedBRDFTexture->getSizeX(), resource.SharedBRDFTexture->getSizeY());
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());

#if USE_SEPARATE_SHADER  && 0
			GraphicsShaderStateDesc boundState;
			boundState.vertex = mScreenVS->getRHIResource();
			boundState.pixel = mPreIntegrateBRDFGenPS->getRHIResource();
			RHISetGraphicsShaderBoundState(commandList, boundState);
			mPreIntegrateBRDFGenPS->setParam(commandList, SHADER_PARAM(BRDFSampleCount), setting.BRDFSampleCount);
#else
			RHISetShaderProgram(commandList, mProgPreIntegrateBRDFGen->getRHIResource());
			mProgPreIntegrateBRDFGen->setParam(commandList, SHADER_PARAM(BRDFSampleCount), setting.BRDFSampleCount);
#endif

			DrawUtility::ScreenRect(commandList);
		}

		return true;
	}

}//namespace Render