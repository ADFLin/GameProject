#include "BRDFTestStage.h"

#include "RHI/Scene.h"

#include "DataCacheInterface.h"
#include "DataSteamBuffer.h"


namespace Render
{

	TGlobalRHIResource< RHITexture2D > IBLResource::SharedBRDFTexture;


	void IBLResource::fillData(ImageBaseLightingData& outData)
	{
		outData.envMapSize = texture->getSize();
		outData.irradianceSize = irradianceTexture->getSize();
		outData.perFilteredSize = perfilteredTexture->getSize();
		GetTextureData(*texture, Texture::eFloatRGBA, 0, outData.envMap);
		GetTextureData(*irradianceTexture, Texture::eFloatRGBA, 0, outData.irradiance);
		for( int level = 0; level < IBLResource::NumPerFilteredLevel; ++level )
		{
			GetTextureData(*perfilteredTexture, Texture::eFloatRGBA, level, outData.perFiltered[level]);
		}
	}

	bool IBLResource::initializeRHI(ImageBaseLightingData* IBLData)
	{
		if( IBLData )
		{
			void* data[6];
			GetCubeMapData(IBLData->envMap, Texture::eFloatRGBA, IBLData->envMapSize, 0, data);
			VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(Texture::eFloatRGBA, IBLData->envMapSize, 1, TCF_DefalutValue, data));
			GetCubeMapData(IBLData->irradiance, Texture::eFloatRGBA, IBLData->irradianceSize, 0, data);
			VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(Texture::eFloatRGBA, IBLData->irradianceSize, 1, TCF_DefalutValue, data));

			VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(Texture::eFloatRGBA, 256, NumPerFilteredLevel));
			for( int level = 0; level < IBLResource::NumPerFilteredLevel; ++level )
			{
				GetCubeMapData(IBLData->perFiltered[level], Texture::eFloatRGBA, IBLData->perFilteredSize, level, data);
				for( int face = 0; face < Texture::FaceCount; ++face )
				{
					int size = Math::Max(1, IBLData->perFilteredSize >> level);
					perfilteredTexture->update(Texture::Face(face), 0, 0, size, size, Texture::eFloatRGBA, data[face], level);
				}
			}
		}
		else
		{
			VERIFY_RETURN_FALSE(texture = RHICreateTextureCube(Texture::eFloatRGBA, 1024));
			VERIFY_RETURN_FALSE(irradianceTexture = RHICreateTextureCube(Texture::eFloatRGBA, 256));
			VERIFY_RETURN_FALSE(perfilteredTexture = RHICreateTextureCube(Texture::eFloatRGBA, 256, NumPerFilteredLevel));
		}
		return true;
	}

	IMPLEMENT_SHADER_PROGRAM(LightProbeVisualizeProgram);
	IMPLEMENT_SHADER_PROGRAM(TonemapProgram);
	IMPLEMENT_SHADER_PROGRAM(SkyBoxProgram);

	class EquirectangularToCubeProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(EquirectangularToCubeProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(EquirectangularToCubePS) },
			};
			return entries;
		}
	};
	

	class IrradianceGenProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(IrradianceGenProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(IrradianceGemPS) },
			};
			return entries;
		}
	};

	class PrefilteredGenProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(PrefilteredGenProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(PrefilteredGenPS) },
			};
			return entries;
		}
	};

	class PreIntegrateBRDFGenProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(PreIntegrateBRDFGenProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/EnvLightTextureGen";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(PreIntegrateBRDFGenPS) },
			};
			return entries;
		}
	};

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
		VERIFY_RETURN_FALSE(mHDRImage = RHIUtility::LoadTexture2DFromFile("Texture/HDR/A.hdr", TextureLoadOption().HDR().ReverseH()));
		VERIFY_RETURN_FALSE(mBuilder.loadOrBuildResource(::Global::DataCache() , HDRImagePath, *mHDRImage, mIBLResource));

		VERIFY_RETURN_FALSE(mRockTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg", TextureLoadOption().SRGB().ReverseH()));
		VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile("Texture/N.png", TextureLoadOption()));

		reigsterTexture("HDR", *mHDRImage);
		reigsterTexture("Rock", *mRockTexture);
		reigsterTexture("Normal", *mNormalTexture);

		VERIFY_RETURN_FALSE(MeshBuild::SkyBox(mSkyBox));

		mProgVisualize = ShaderManager::Get().getGlobalShaderT< LightProbeVisualizeProgram >(true);
		mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >(true);
		mProgSkyBox = ShaderManager::Get().getGlobalShaderT< SkyBoxProgram >(true);
		
		mParamBuffer.initializeResource(1);
		{
			auto* pParams = mParamBuffer.lock();
			*pParams = mParams;
			mParamBuffer.unlock();
		}
		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();

		mSceneRenderTargets.initializeRHI(screenSize);

		ShaderManager::Get().registerShaderAssets(::Global::GetAssetManager());

		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		WidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Use Tonemap"), bEnableTonemap);
		WidgetPropery::Bind(frame->addSlider(UI_ANY), SkyboxShowIndex , 0 , (int)ESkyboxShow::Count - 1);


		return true;
	}

	void BRDFTestStage::onRender(float dFrame)
	{
		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();

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
				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());

				GL_BIND_LOCK_OBJECT(mProgSkyBox);
				mProgSkyBox->setTexture(SHADER_PARAM(Texture), *mHDRImage);
				switch( SkyboxShowIndex )
				{
				case ESkyboxShow::Normal:
					mProgSkyBox->setTexture(SHADER_PARAM(CubeTexture), mIBLResource.texture);
					mProgSkyBox->setParam(SHADER_PARAM(CubeLevel), float(0));
					break;
				case ESkyboxShow::Irradiance:
					mProgSkyBox->setTexture(SHADER_PARAM(CubeTexture), mIBLResource.irradianceTexture );
					mProgSkyBox->setParam(SHADER_PARAM(CubeLevel), float(0));
					break;
				default:
					mProgSkyBox->setTexture(SHADER_PARAM(CubeTexture), mIBLResource.perfilteredTexture , 
											TStaticSamplerState< Sampler::eTrilinear , Sampler::eClamp , Sampler::eClamp , Sampler ::eClamp > ::GetRHI() );
					mProgSkyBox->setParam(SHADER_PARAM(CubeLevel), float(SkyboxShowIndex - ESkyboxShow::Prefiltered_0));
				}
				
				mView.setupShader(*mProgSkyBox);
				mSkyBox.drawShader();
			}

			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			{
				RHISetupFixedPipelineState(mView.worldToView, mView.viewToClip);
				DrawUtility::AixsLine();
			}
			{
				GPU_PROFILE("LightProbe Visualize");

				GL_BIND_LOCK_OBJECT(*mProgVisualize);
				mProgVisualize->setStructuredBufferT< LightProbeVisualizeParams >(*mParamBuffer.getRHI());
				mProgVisualize->setTexture(SHADER_PARAM(NormalTexture), mNormalTexture);
				mView.setupShader(*mProgVisualize);
				mProgVisualize->setParameters(mIBLResource);
				RHIDrawPrimitiveInstanced(PrimitiveType::Quad, 0, 4, mParams.gridNum.x * mParams.gridNum.y);
			}

		}


		RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
		{
			RHISetViewport(0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(*mHDRImage, IntVector2(10, 10), IntVector2(512, 512));
		}

		if( bEnableTonemap )
		{
			GPU_PROFILE("Tonemap");

			mSceneRenderTargets.swapFrameTexture();
			PostProcessContext context;
			context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();

			GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());

			GL_BIND_LOCK_OBJECT(mProgTonemap);

			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGB >::GetRHI());
			mProgTonemap->setParameters(context);
			DrawUtility::ScreenRectShader();
		}

		{
			ShaderHelper::Get().copyTextureToBuffer(mSceneRenderTargets.getFrameTexture());
		}

		if ( IBLResource::SharedBRDFTexture.isValid() && 0 )
		{
			RHISetViewport(0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(*IBLResource::SharedBRDFTexture , IntVector2(10, 10), IntVector2(512, 512));
		}
	}



	bool IBLResourceBuilder::initializeShaderProgram()
	{
		if( mProgEquirectangularToCube != nullptr )
			return true;

		VERIFY_RETURN_FALSE(mProgEquirectangularToCube = ShaderManager::Get().getGlobalShaderT< EquirectangularToCubeProgram >(true));
		VERIFY_RETURN_FALSE(mProgIrradianceGen = ShaderManager::Get().getGlobalShaderT< IrradianceGenProgram >(true));
		VERIFY_RETURN_FALSE(mProgPrefilterdGen = ShaderManager::Get().getGlobalShaderT< PrefilteredGenProgram >(true));
		VERIFY_RETURN_FALSE(mProgPreIntegrateBRDFGen = ShaderManager::Get().getGlobalShaderT< PreIntegrateBRDFGenProgram >(true));
		return true;
	}


	bool IBLResourceBuilder::loadOrBuildResource(DataCacheInterface& dataCache, char const* path, RHITexture2D& HDRImage, IBLResource& resource)
	{
		auto const GetBRDFCacheKey = []()->DataCacheKey
		{
			DataCacheKey cacheKey;
			cacheKey.typeName = "IBL-BRDF-LUT";
			cacheKey.version = "7A38AC9D-1EFC-4537-898E-BC8552AD7758";
			cacheKey.keySuffix;
			return cacheKey;
		};

		{
			DataCacheKey cacheKey;
			cacheKey.typeName = "IBL";
			cacheKey.version = "7A38AC9D-1EFC-4537-898E-BC8552AD7758";
			cacheKey.keySuffix.add(path);

			auto LoadFun = [&resource](IStreamSerializer& serializer) -> bool
			{
				ImageBaseLightingData IBLData;
				serializer >> IBLData;
				if( !resource.initializeRHI(&IBLData) )
					return false;

				return true;
			};

			if( !dataCache.loadDelegate(cacheKey, LoadFun) )
			{
				resource.initializeRHI(nullptr);
				if( !buildIBLResource(HDRImage, resource) )
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
				auto LoadFun = [this](IStreamSerializer& serializer) -> bool
				{
					std::vector< uint8 > data;
					serializer >> data;
					if( !IBLResource::InitializeBRDFTexture(data.data()) )
						return false;

					return true;
				};

				if( !dataCache.loadDelegate(GetBRDFCacheKey(), LoadFun) )
				{
					return false;
				}
			}
		}

		return true;
	}


	template< class Func >
	void RenderCubeTexture(OpenGLFrameBuffer& frameBuffer, RHITextureCube& cubeTexture, ShaderProgram& updateShader, int level, Func shaderSetup)
	{
		int size = cubeTexture.getSize() >> level;

		RHISetViewport(0, 0, size, size);
		RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
		RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
		RHISetBlendState(TStaticBlendState< CWM_RGBA >::GetRHI());
		for( int i = 0; i < Texture::FaceCount; ++i )
		{
			frameBuffer.setTexture(0, cubeTexture, Texture::Face(i), level);
			GL_BIND_LOCK_OBJECT(frameBuffer);
			GL_BIND_LOCK_OBJECT(updateShader);
			updateShader.setParam(SHADER_PARAM(FaceDir), Texture::GetFaceDir(Texture::Face(i)));
			updateShader.setParam(SHADER_PARAM(FaceUpDir), Texture::GetFaceUpDir(Texture::Face(i)));
			shaderSetup();
			DrawUtility::ScreenRectShader();
		}
	}

	bool IBLResourceBuilder::buildIBLResource(RHITexture2D& envTexture, IBLResource& resource)
	{
		if( initializeShaderProgram() )
			return false;

		OpenGLFrameBuffer frameBuffer;
		frameBuffer.create();

		RenderCubeTexture(frameBuffer, *resource.texture, *mProgEquirectangularToCube, 0, [&]()
		{
			mProgEquirectangularToCube->setTexture(SHADER_PARAM(Texture), envTexture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		});

		//IrradianceTexture
		RenderCubeTexture(frameBuffer, *resource.irradianceTexture, *mProgIrradianceGen, 0, [&]()
		{
			mProgIrradianceGen->setTexture(SHADER_PARAM(CubeTexture), resource.texture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		});

		for( int level = 0; level < IBLResource::NumPerFilteredLevel; ++level )
		{
			RenderCubeTexture(frameBuffer, *resource.perfilteredTexture, *mProgPrefilterdGen, level, [&]()
			{
				mProgPrefilterdGen->setParam(SHADER_PARAM(Roughness), float(level) / (IBLResource::NumPerFilteredLevel - 1));
				mProgPrefilterdGen->setTexture(SHADER_PARAM(CubeTexture), resource.texture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
			});
		}

		if( !resource.SharedBRDFTexture.isValid() )
		{
			IBLResource::InitializeBRDFTexture(nullptr);

			frameBuffer.setTexture(0, *resource.SharedBRDFTexture);
			RHISetViewport(0, 0, resource.SharedBRDFTexture->getSizeX(), resource.SharedBRDFTexture->getSizeY());
			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA >::GetRHI());

			GL_BIND_LOCK_OBJECT(frameBuffer);
			GL_BIND_LOCK_OBJECT(*mProgPreIntegrateBRDFGen);
			DrawUtility::ScreenRectShader();
		}
	}

}