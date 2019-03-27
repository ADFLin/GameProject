#include "BRDFTestStage.h"

#include "RHI/Scene.h"

#include "DataCacheInterface.h"
#include "DataSteamBuffer.h"


namespace Render
{

	TGlobalRHIResource< RHITexture2D > IBLResource::SharedBRDFTexture;


	class LightProbeVisualizeProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(LightProbeVisualizeProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/LightProbeVisualize";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			mParamIBL.bindParameters(parameterMap);
		}

		void setParameters(IBLResource const& IBL)
		{
			mParamIBL.setParameters(*this, IBL);
		}

		IBLShaderParameters mParamIBL;
	};

	struct PostProcessContext
	{
	public:
		RHITexture2D* getTexture(int slot = 0) const { return mInputTexture[slot]; }


		RHITexture2DRef mInputTexture[4];
	};


	struct PostProcessParameters
	{
		void bindParameters(ShaderParameterMap& parameterMap)
		{
			for( int i = 0; i < MaxInputNum; ++i )
			{
				FixString<128> name;
				parameterMap.bind(mParamTextureInput[i], name.format("TextureInput%d", i));
			}
		}

		void setParameters(ShaderProgram& shader , PostProcessContext const& context)
		{
			for( int i = 0; i < MaxInputNum; ++i )
			{
				if ( !mParamTextureInput[i].isBound() )
					break;
				if ( context.getTexture(i) )
					shader.setTexture(mParamTextureInput[i], *context.getTexture(i));
			}
		}

		static int const MaxInputNum = 4;
		
		ShaderParameter mParamTextureInput[MaxInputNum];


	};

	class TonemapProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(TonemapProgram, Global);


		static char const* GetShaderFileName()
		{
			return "Shader/Tonemap";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			mParamPostProcess.bindParameters(parameterMap);
		}
		void setParameters(PostProcessContext const& context)
		{
			mParamPostProcess.setParameters(*this , context);
		}
		PostProcessParameters mParamPostProcess;

	};


	class SkyBoxProgram : public GlobalShaderProgram
	{
	public:
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(SkyBoxProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/SkyBox";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(MainVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};
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



	void BRDFTestStage::buildIBLResource(RHITexture2D& EnvTexture, IBLResource& resource )
	{
		OpenGLFrameBuffer frameBuffer;
		frameBuffer.create();

		UpdateCubeTexture(frameBuffer , *resource.texture, *mProgEquirectangularToCube, 0 , [&]()
		{
			mProgEquirectangularToCube->setTexture(SHADER_PARAM(Texture), EnvTexture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		});

		//IrradianceTexture
		UpdateCubeTexture(frameBuffer , *resource.irradianceTexture, *mProgIrradianceGen, 0 , [&]()
		{
			mProgIrradianceGen->setTexture(SHADER_PARAM(CubeTexture), resource.texture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
		});

		for( int level = 0; level < IBLResource::NumPerFilteredLevel; ++level )
		{
			UpdateCubeTexture(frameBuffer, *resource.perfilteredTexture, *mProgPrefilterdGen, level, [&]()
			{
				mProgPrefilterdGen->setParam(SHADER_PARAM(Roughness), float(level) / (IBLResource::NumPerFilteredLevel-1 ) );
				mProgPrefilterdGen->setTexture(SHADER_PARAM(CubeTexture), resource.texture, TStaticSamplerState<Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp >::GetRHI());
			});
		}


		if ( !resource.SharedBRDFTexture.isValid() )
		{
			resource.SharedBRDFTexture.Initialize(RHICreateTexture2D(Texture::eFloatRGBA, 512, 512, 1));

			frameBuffer.setTexture(0, *resource.SharedBRDFTexture.getRHI());
			RHISetViewport(0, 0, resource.SharedBRDFTexture.getRHI()->getSizeX(), resource.SharedBRDFTexture.getRHI()->getSizeY());
			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA >::GetRHI());

			GL_BIND_LOCK_OBJECT(frameBuffer);
			GL_BIND_LOCK_OBJECT(*mProgPreIntegrateBRDFGen);
			DrawUtility::ScreenRectShader();
		}

	}

	bool BRDFTestStage::onInit()
	{
		::Global::GetDrawEngine().changeScreenSize(1024, 768);
		if( !BaseClass::onInit() )
			return false;

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		ShaderHelper::Get().init();

		

		VERIFY_RETURN_FALSE(mHDRImage = RHIUtility::LoadTexture2DFromFile("Texture/HDR/C.hdr", TextureLoadOption().HDR().ReverseH()));
		VERIFY_RETURN_FALSE(mRockTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg", TextureLoadOption().SRGB().ReverseH()));
		VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile("Texture/N.png", TextureLoadOption()));

		VERIFY_RETURN_FALSE(MeshBuild::SkyBox(mSkyBox));

		mProgVisualize = ShaderManager::Get().getGlobalShaderT< LightProbeVisualizeProgram >(true);
		mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >(true);
		mProgSkyBox = ShaderManager::Get().getGlobalShaderT< SkyBoxProgram >(true);
		

		VERIFY_RETURN_FALSE(mIBLResource.initializeRHI());
		mProgEquirectangularToCube = ShaderManager::Get().getGlobalShaderT< EquirectangularToCubeProgram >(true);
		mProgIrradianceGen = ShaderManager::Get().getGlobalShaderT< IrradianceGenProgram >(true);
		mProgPrefilterdGen = ShaderManager::Get().getGlobalShaderT< PrefilteredGenProgram >(true);
		mProgPreIntegrateBRDFGen = ShaderManager::Get().getGlobalShaderT< PreIntegrateBRDFGenProgram >(true);
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
				mProgVisualize->setTexture(SHADER_PARAM(IrradianceTexture), mIBLResource.irradianceTexture);
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

		if ( IBLResource::SharedBRDFTexture.getRHI() )
		{
			RHISetViewport(0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(*IBLResource::SharedBRDFTexture.getRHI(), IntVector2(10, 10), IntVector2(512, 512));
		}
	}



}