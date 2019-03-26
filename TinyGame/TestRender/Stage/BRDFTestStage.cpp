#include "TestRenderStageBase.h"

#include "RHI/Scene.h"
#include "RHI/SceneRenderer.h"

#include "DataCacheInterface.h"
#include "DataSteamBuffer.h"


namespace Render
{

	struct GPU_BUFFER_ALIGN LightProbeVisualizeParams
	{
		DECLARE_BUFFER_STRUCT(LightProbeVisualizeParamsBlock);
		Vector3 xAxis;
		float gridSize;
		Vector3 yAxis;
		float probeRadius;
		Vector3 startPos;
		float dummy2;

		IntVector2 gridNum;
		int dummy[2];

		LightProbeVisualizeParams()
		{
			gridNum = IntVector2(10, 10);
			xAxis = Vector3(1, 0, 0);
			yAxis = Vector3(0, 0, 1);

			gridSize = 1;
			probeRadius = 0.45;
			startPos = -0.5 * Vector3(gridNum.x - 1 , 0 , gridNum.y - 1 ) * gridSize;

		}
	};
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
	IMPLEMENT_SHADER_PROGRAM(IrradianceGenProgram);



	class BRDFTestStage : public TestRenderStageBase
	{
		typedef TestRenderStageBase BaseClass;
	public:
		BRDFTestStage() {}

		LightProbeVisualizeParams mParams;

		LightProbeVisualizeProgram* mProgVisualize;
		TonemapProgram* mProgTonemap;
		SkyBoxProgram* mProgSkyBox;
		IrradianceGenProgram* mProgIrradianceGen;
		TStructuredUniformBuffer< LightProbeVisualizeParams > mParamBuffer;

		SceneRenderTargets mSceneRenderTargets;

		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mHDRImage;
		RHITexture2DRef mRockTexture;
		RHITextureCubeRef mEnvCube;

		Mesh mSkyBox;

		template< class Func >
		void UpdateCubeTexture(RHITextureCube& cubeTexture, ShaderProgram& updateShader , Func shaderSetup )
		{
			OpenGLFrameBuffer frameBuffer;
			frameBuffer.create();
			frameBuffer.addTexture(cubeTexture, Texture::eFaceX);

			int size = cubeTexture.getSize();

			RHISetViewport(0, 0, size, size);
			RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA >::GetRHI());
			for( int i = 0; i < Texture::FaceCount; ++i )
			{
				frameBuffer.setTexture(0 , cubeTexture, Texture::Face( i ) );
				GL_BIND_LOCK_OBJECT(frameBuffer);
				GL_BIND_LOCK_OBJECT(updateShader);
				updateShader.setParam(SHADER_PARAM(FaceDir), Texture::GetFaceDir(Texture::Face(i)));
				updateShader.setParam(SHADER_PARAM(FaceUpDir), Texture::GetFaceUpDir(Texture::Face(i)));
				shaderSetup();
				DrawUtility::ScreenRectShader();
			}
		}

		static void GetTextureData(RHITextureCube& cubeTexture , Texture::Format format , std::vector< uint8 >& outData )
		{
			int formatSize = Texture::GetFormatSize(format);
			int faceDataSize = cubeTexture.getSize() * cubeTexture.getSize() * formatSize;
			outData.resize(6 * faceDataSize);
			OpenGLCast::To(&cubeTexture)->bind();
			for( int i = 0; i < Texture::FaceCount; ++i )
			{
				glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GLConvert::BaseFormat(format), GLConvert::TextureComponentType(format) , &outData[faceDataSize*i]);
			}
			OpenGLCast::To(&cubeTexture)->unbind();
		}

		void CalcIrradianceTexture(RHITextureCube& cubeTexture, RHITexture2D& HDRTexture)
		{
			UpdateCubeTexture(cubeTexture, *mProgIrradianceGen, [&]()
			{
				mProgIrradianceGen->setTexture(SHADER_PARAM(Texture), HDRTexture , TStaticSamplerState<Sampler::eBilinear , Sampler::eClamp , Sampler::eClamp >::GetRHI() );
			});
		}

		virtual bool onInit()
		{
			::Global::GetDrawEngine().changeScreenSize(1024, 768);
			if( !BaseClass::onInit() )
				return false;

			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

			InitGlobalRHIResource();
			ShaderHelper::Get().init();

			mHDRImage = RHIUtility::LoadTexture2DFromFile("Texture/HDR/C.hdr", TextureLoadOption().HDR().ReverseH());
			mRockTexture = RHIUtility::LoadTexture2DFromFile("Texture/rocks.jpg" , TextureLoadOption().SRGB().ReverseH());
			mNormalTexture = RHIUtility::LoadTexture2DFromFile("Texture/N.png", TextureLoadOption());

			mEnvCube = RHICreateTextureCube();
			mEnvCube->create(Texture::eFloatRGBA, 1024);

			VERIFY_RETURN_FALSE(MeshBuild::SkyBox(mSkyBox));

			mProgVisualize = ShaderManager::Get().getGlobalShaderT< LightProbeVisualizeProgram >(true);
			mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >(true);
			mProgSkyBox = ShaderManager::Get().getGlobalShaderT< SkyBoxProgram >(true);
			mProgIrradianceGen = ShaderManager::Get().getGlobalShaderT< IrradianceGenProgram >(true);

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
			WidgetPropery::Bind(frame->addCheckBox(UI_ANY , "Use Tonemap"), bEnableTonemap);


			CalcIrradianceTexture(*mEnvCube, *mHDRImage);
			return true;
		}

		virtual void onEnd()
		{
			ShaderManager::Get().unregisterShaderAssets(::Global::GetAssetManager());
			BaseClass::onEnd();
		}

		void restart()
		{

		}
		void tick() 
		{

		}
		void updateFrame(int frame) 
		{
		}

		virtual void onUpdate(long time)
		{

		}

		bool bEnableTonemap;
		void onRender(float dFrame)
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
					mProgSkyBox->setTexture(SHADER_PARAM(CubeTexture), *mEnvCube);
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
					mProgVisualize->setTexture(SHADER_PARAM(IrradianceCubeTexture), mEnvCube);
					mView.setupShader(*mProgVisualize);
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

			if ( bEnableTonemap )
			{
				GPU_PROFILE("Tonemap");

				mSceneRenderTargets.swapFrameTexture();
				PostProcessContext context;
				context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();

				GL_BIND_LOCK_OBJECT(mSceneRenderTargets.getFrameBuffer());

				GL_BIND_LOCK_OBJECT(mProgTonemap);
				
				RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
				RHISetBlendState(TStaticBlendState< CWM_RGB >::GetRHI() );
				mProgTonemap->setParameters(context);
				DrawUtility::ScreenRectShader();
			}

			{
				ShaderHelper::Get().copyTextureToBuffer(mSceneRenderTargets.getFrameTexture());
			}
		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;
			switch( key )
			{
			case Keyboard::eX:
				CalcIrradianceTexture(*mEnvCube, *mHDRImage);
				break;
			default:
				break;
			}
			return BaseClass::onKey(key, isDown);
		}

		virtual bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch( id )
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}
	protected:
	};

	REGISTER_STAGE2("BRDF Test", BRDFTestStage, EStageGroup::FeatureDev, 1);


}