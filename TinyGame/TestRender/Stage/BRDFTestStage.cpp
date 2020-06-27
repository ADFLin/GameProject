#include "BRDFTestStage.h"

#include "RHI/Scene.h"

#include "DataCacheInterface.h"
#include "DataStreamBuffer.h"
#include "ProfileSystem.h"



namespace Render
{

	TGlobalRHIResource< RHITexture2D > IBLResource::SharedBRDFTexture;


	void IBLResource::fillData(ImageBaseLightingData& outData)
	{
		outData.envMapSize = texture->getSize();
		outData.irradianceSize = irradianceTexture->getSize();
		outData.perFilteredSize = perfilteredTexture->getSize();
		ReadTextureData(*texture, Texture::eFloatRGBA, 0, outData.envMap);
		ReadTextureData(*irradianceTexture, Texture::eFloatRGBA, 0, outData.irradiance);
		for( int level = 0; level < IBLResource::NumPerFilteredLevel; ++level )
		{
			ReadTextureData(*perfilteredTexture, Texture::eFloatRGBA, level, outData.perFiltered[level]);
		}
	}

	bool IBLResource::initializeRHI(ImageBaseLightingData& IBLData)
	{
		void* data[6];
		{
			TIME_SCOPE("Create EnvMap Texture");
			GetCubeMapData(IBLData.envMap, Texture::eFloatRGBA, IBLData.envMapSize, 0, data);
			VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(Texture::eFloatRGBA, IBLData.envMapSize, 1, TCF_DefalutValue, data));
		}
		{
			TIME_SCOPE("Create Irradiance Texture");
			GetCubeMapData(IBLData.irradiance, Texture::eFloatRGBA, IBLData.irradianceSize, 0, data);
			VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(Texture::eFloatRGBA, IBLData.irradianceSize, 1, TCF_DefalutValue, data));
		}

		{
			TIME_SCOPE("Create Perfiltered Texture");
			VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(Texture::eFloatRGBA, 256, NumPerFilteredLevel));
			for (int level = 0; level < IBLResource::NumPerFilteredLevel; ++level)
			{
				GetCubeMapData(IBLData.perFiltered[level], Texture::eFloatRGBA, IBLData.perFilteredSize, level, data);
				for (int face = 0; face < Texture::FaceCount; ++face)
				{
					int size = Math::Max(1, IBLData.perFilteredSize >> level);
					perfilteredTexture->update(Texture::Face(face), 0, 0, size, size, Texture::eFloatRGBA, data[face], level);
				}
			}
		}
		return true;
	}

	bool IBLResource::initializeRHI(IBLBuildSetting const& setting)
	{
		VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(Texture::eFloatRGBA, setting.envSize));
		VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(Texture::eFloatRGBA, setting.irradianceSize));
		VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(Texture::eFloatRGBA, setting.perfilteredSize , NumPerFilteredLevel));
		return true;
	}

	IMPLEMENT_SHADER_PROGRAM(LightProbeVisualizeProgram);
	IMPLEMENT_SHADER_PROGRAM(TonemapProgram);
	IMPLEMENT_SHADER_PROGRAM(SkyBoxProgram);

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
			mParamRoughness.bind(parameterMap, SHADER_PARAM(Roughness));
			mParamCubeTexture.bind(parameterMap, SHADER_PARAM(CubeTexture));
			mParamPrefilterSampleCount.bind(parameterMap, SHADER_PARAM(PrefilterSampleCount));
		}

		void setParameters(RHICommandList& commandList, int level, RHITextureCube& texture, int sampleCount)
		{
			setParam(commandList, mParamRoughness, float(level) / (IBLResource::NumPerFilteredLevel - 1));
			setTexture(commandList, mParamCubeTexture, texture, mParamCubeTexture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
			setParam(commandList, SHADER_PARAM(PrefilterSampleCount), sampleCount);
		}

		ShaderParameter mParamRoughness;
		ShaderParameter mParamCubeTexture;
		ShaderParameter mParamPrefilterSampleCount;

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


	REGISTER_STAGE2("BRDF Test", BRDFTestStage, EStageGroup::FeatureDev, 1);


	bool BRDFTestStage::onInit()
	{
		::Global::GetDrawEngine().changeScreenSize(1024, 768);
		if( !BaseClass::onInit() )
			return false;

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		ShaderHelper::Get().init();


		char const* HDRImagePath = "Texture/HDR/A.hdr";
		{
			TIME_SCOPE("HDR Texture");
			VERIFY_RETURN_FALSE(mHDRImage = RHIUtility::LoadTexture2DFromFile(::Global::DataCache() , "Texture/HDR/A.hdr", TextureLoadOption().HDR().ReverseH()));
		}
		{
			TIME_SCOPE("IBL");
			VERIFY_RETURN_FALSE(mBuilder.loadOrBuildResource(::Global::DataCache(), HDRImagePath, *mHDRImage, mIBLResource));
		}

		{
			TIME_SCOPE("BRDF Texture");
			VERIFY_RETURN_FALSE(mRockTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Texture/rocks.jpg", TextureLoadOption().SRGB().ReverseH()));
			VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Texture/N.png", TextureLoadOption()));
		}

		registerTexture("HDR", *mHDRImage);
		registerTexture("Rock", *mRockTexture);
		registerTexture("Normal", *mNormalTexture);
		registerTexture("BRDF", *IBLResource::SharedBRDFTexture);

		VERIFY_RETURN_FALSE(MeshBuild::SkyBox(mSkyBox));

		{
			TIME_SCOPE("BRDF Shader");
			mProgVisualize = ShaderManager::Get().getGlobalShaderT< LightProbeVisualizeProgram >(true);
			mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >(true);
			mProgSkyBox = ShaderManager::Get().getGlobalShaderT< SkyBoxProgram >(true);
		}
		
		mParamBuffer.initializeResource(1);
		{
			auto* pParams = mParamBuffer.lock();
			*pParams = mParams;
			mParamBuffer.unlock();
		}
		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();

		mSceneRenderTargets.initializeRHI(screenSize);

		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Use Tonemap"), bEnableTonemap);
		FWidgetPropery::Bind(frame->addSlider(UI_ANY), SkyboxShowIndex , 0 , (int)ESkyboxShow::Count - 1);


		return true;
	}

	void BRDFTestStage::onRender(float dFrame)
	{
		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		initializeRenderState();
		{
			GPU_PROFILE("Scene");
			mSceneRenderTargets.attachDepthTexture();
			GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());


			glClearColor(0.2, 0.2, 0.2, 1);
			glClearDepth(mViewFrustum.bUseReverse ? 0 : 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			{
				GPU_PROFILE("SkyBox");
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

				RHISetShaderProgram(commandList, mProgSkyBox->getRHIResource());
				mProgSkyBox->setTexture(commandList, SHADER_PARAM(Texture), *mHDRImage);
				switch( SkyboxShowIndex )
				{
				case ESkyboxShow::Normal:
					mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.texture);
					mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(0));
					break;
				case ESkyboxShow::Irradiance:
					mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.irradianceTexture );
					mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(0));
					break;
				default:
					mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.perfilteredTexture , SHADER_PARAM(CubeTextureSampler),
											TStaticSamplerState< Sampler::eTrilinear , Sampler::eClamp , Sampler::eClamp , Sampler ::eClamp > ::GetRHI() );
					mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(SkyboxShowIndex - ESkyboxShow::Prefiltered_0));
				}
				
				mView.setupShader(commandList, *mProgSkyBox);
				mSkyBox.draw(commandList);
			}

			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

			{
				RHISetupFixedPipelineState(commandList, mView.worldToClip);
				DrawUtility::AixsLine(commandList);
			}

			{
				GPU_PROFILE("LightProbe Visualize");

				RHISetShaderProgram(commandList, mProgVisualize->getRHIResource());
				mProgVisualize->setStructuredUniformBufferT< LightProbeVisualizeParams >(commandList, *mParamBuffer.getRHI());
				mProgVisualize->setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture);
				mView.setupShader(commandList, *mProgVisualize);
				mProgVisualize->setParameters(commandList, mIBLResource);
				RHISetInputStream(commandList, nullptr , nullptr , 0 );
				RHIDrawPrimitiveInstanced(commandList, EPrimitive::Quad, 0, 4, mParams.gridNum.x * mParams.gridNum.y);
			}
		}


		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(commandList, *mHDRImage, IntVector2(10, 10), IntVector2(512, 512));
		}

		if( bEnableTonemap )
		{
			GPU_PROFILE("Tonemap");

			mSceneRenderTargets.swapFrameTexture();
			PostProcessContext context;
			context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();

			GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());

			RHISetShaderProgram(commandList, mProgTonemap->getRHIResource());
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGB >::GetRHI());
			mProgTonemap->setParameters(commandList, context);
			DrawUtility::ScreenRect(commandList);
		}

		{
			ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());
		}

		if ( IBLResource::SharedBRDFTexture.isValid() && 0 )
		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(commandList, *IBLResource::SharedBRDFTexture , IntVector2(10, 10), IntVector2(512, 512));
		}
	}



	bool IBLResourceBuilder::initializeShaderProgram()
	{
		if( mEquirectangularToCubePS != nullptr )
			return true;

#if USE_SEPARATE_SHADER
		VERIFY_RETURN_FALSE(mScreenVS = ShaderManager::Get().getGlobalShaderT< ScreenVS >(true));
		VERIFY_RETURN_FALSE(mEquirectangularToCubePS = ShaderManager::Get().getGlobalShaderT< EquirectangularToCubePS >(true));
		VERIFY_RETURN_FALSE(mIrradianceGenPS = ShaderManager::Get().getGlobalShaderT< IrradianceGenPS >(true));
		VERIFY_RETURN_FALSE(mPrefilteredGenPS = ShaderManager::Get().getGlobalShaderT< PrefilteredGenPS >(true));
		VERIFY_RETURN_FALSE(mPreIntegrateBRDFGenPS = ShaderManager::Get().getGlobalShaderT< PreIntegrateBRDFGenPS >(true));
#else
		VERIFY_RETURN_FALSE(mProgEquirectangularToCube = ShaderManager::Get().getGlobalShaderT< EquirectangularToCubeProgram >(true));
		VERIFY_RETURN_FALSE(mProgIrradianceGen = ShaderManager::Get().getGlobalShaderT< IrradianceGenProgram >(true));
		VERIFY_RETURN_FALSE(mProgPrefilteredGen = ShaderManager::Get().getGlobalShaderT< PrefilteredGenProgram >(true));
		VERIFY_RETURN_FALSE(mProgPreIntegrateBRDFGen = ShaderManager::Get().getGlobalShaderT< PreIntegrateBRDFGenProgram >(true));
#endif	

		return true;
	}


	bool IBLResourceBuilder::loadOrBuildResource(DataCacheInterface& dataCache, char const* path, RHITexture2D& HDRImage, IBLResource& resource , IBLBuildSetting const& setting )
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
			cacheKey.keySuffix.add(path , setting.envSize , setting.irradianceSize , setting.perfilteredSize,
				setting.irradianceSampleCount[0] , setting.irradianceSampleCount[1] , setting.prefilterSampleCount );

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

			if( true || !dataCache.loadDelegate(cacheKey, LoadFunc) )
			{
				resource.initializeRHI(setting);
				if( !buildIBLResource(HDRImage, resource, setting) )
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
				if( !dataCache.loadDelegate(GetBRDFCacheKey(), LoadFunc) )
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
		for( int i = 0; i < Texture::FaceCount; ++i )
		{
			frameBuffer->setTexture(0, cubeTexture, Texture::Face(i), level);
			GL_BIND_LOCK_OBJECT(frameBuffer);

			RHISetShaderProgram(commandList, updateShader.getRHIResource());
			updateShader.setParam(commandList, SHADER_PARAM(FaceDir), Texture::GetFaceDir(Texture::Face(i)));
			updateShader.setParam(commandList, SHADER_PARAM(FaceUpDir), Texture::GetFaceUpDir(Texture::Face(i)));
			shaderSetup(commandList);
			DrawUtility::ScreenRect(commandList);
		}
	}

	template< class TFunc >
	void IBLResourceBuilder::renderCubeTexture(RHICommandList& commandList, RHIFrameBufferRef& frameBuffer, RHITextureCube& cubeTexture, GlobalShader& shaderPS ,int level, TFunc&& shaderSetup)
	{
		int size = cubeTexture.getSize() >> level;

		RHISetViewport(commandList, 0, 0, size, size);
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());

		for (int i = 0; i < Texture::FaceCount; ++i)
		{
			frameBuffer->setTexture(0, cubeTexture, Texture::Face(i), level);

			RHISetFrameBuffer(commandList, frameBuffer);

			GraphicShaderBoundState boundState;
			boundState.vertexShader = mScreenVS->getRHIResource();
			boundState.pixelShader = shaderPS.getRHIResource();
			RHISetGraphicsShaderBoundState(commandList, boundState);

			shaderPS.setParam(commandList, SHADER_PARAM(FaceDir), Texture::GetFaceDir(Texture::Face(i)));
			shaderPS.setParam(commandList, SHADER_PARAM(FaceUpDir), Texture::GetFaceUpDir(Texture::Face(i)));

			shaderSetup(commandList);
			DrawUtility::ScreenRect(commandList);

			RHISetFrameBuffer(commandList, nullptr);
		}
	}


	bool IBLResourceBuilder::buildIBLResource( RHITexture2D& envTexture, IBLResource& resource, IBLBuildSetting const& setting )
	{
		if( !initializeShaderProgram() )
			return false;

		RHICommandList& commandList = RHICommandList::GetImmediateList();


		RHIFrameBufferRef frameBuffer = RHICreateFrameBuffer();

#if USE_SEPARATE_SHADER
		renderCubeTexture(commandList, frameBuffer, *resource.texture, *mEquirectangularToCubePS, 0, [&](RHICommandList& commandList)
		{
			mEquirectangularToCubePS->setTexture(commandList,
				SHADER_PARAM(Texture), envTexture,
				SHADER_PARAM(TextureSampler), TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		});
#else
		RenderCubeTexture( commandList, frameBuffer, *resource.texture, *mProgEquirectangularToCube, 0, [&](RHICommandList& commandList)
		{
			mProgEquirectangularToCube->setTexture(commandList,
				SHADER_PARAM(Texture), envTexture, 
				SHADER_PARAM(TextureSampler) ,TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		});
#endif

#if USE_SEPARATE_SHADER
		//IrradianceTexture
		renderCubeTexture( commandList, frameBuffer, *resource.irradianceTexture, *mIrradianceGenPS, 0, [&](RHICommandList& commandList)
		{
			mIrradianceGenPS->setTexture(commandList, SHADER_PARAM(CubeTexture), resource.texture, SHADER_PARAM(CubeTextureSampler) , TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
			mIrradianceGenPS->setParam(commandList, SHADER_PARAM(IrrianceSampleCount), setting.irradianceSampleCount[0], setting.irradianceSampleCount[1]);
		});
#else
		RenderCubeTexture(commandList, frameBuffer, *resource.irradianceTexture, *mProgIrradianceGen, 0, [&](RHICommandList& commandList)
		{
			mProgIrradianceGen->setTexture(commandList, SHADER_PARAM(CubeTexture), resource.texture, SHADER_PARAM(CubeTextureSampler), TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
			mProgIrradianceGen->setParam(commandList, SHADER_PARAM(IrrianceSampleCount), setting.irradianceSampleCount[0], setting.irradianceSampleCount[1]);
		});

#endif

		for( int level = 0; level < IBLResource::NumPerFilteredLevel; ++level )
		{
#if USE_SEPARATE_SHADER
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

		if( !resource.SharedBRDFTexture.isValid() )
		{
			IBLResource::InitializeBRDFTexture(nullptr);
			frameBuffer->setTexture(0, *resource.SharedBRDFTexture);
			RHISetFrameBuffer(commandList, frameBuffer);


			RHISetViewport(commandList, 0, 0, resource.SharedBRDFTexture->getSizeX(), resource.SharedBRDFTexture->getSizeY());
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());

#if USE_SEPARATE_SHADER
			GraphicShaderBoundState boundState;
			boundState.vertexShader = mScreenVS->getRHIResource();
			boundState.pixelShader = mPreIntegrateBRDFGenPS->getRHIResource();
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

}