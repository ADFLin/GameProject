#include "NoiseTestStage.h"

namespace Render
{
	REGISTER_STAGE2("Noise Test", NoiseTestStage, EStageGroup::FeatureDev, 1);


#define IMPLEMENT_NOISE_SHADER( T1 , T2 )\
	typedef TNoiseShaderProgram< T1 , T2 > NoiseShaderProgram##T1##T2;\
	IMPLEMENT_SHADER_PROGRAM( NoiseShaderProgram##T1##T2 );

	IMPLEMENT_NOISE_SHADER(false, false);
	IMPLEMENT_NOISE_SHADER(false, true);
	IMPLEMENT_NOISE_SHADER(true, false);
	IMPLEMENT_NOISE_SHADER(true, true);

	IMPLEMENT_SHADER_PROGRAM(SmokeRenderProgram);
	IMPLEMENT_SHADER_PROGRAM(SmokeBlendProgram);

	bool NoiseTestStage::onInit()
	{
		::Global::GetDrawEngine().changeScreenSize(1024, 768);

		if( !BaseClass::onInit() )
			return false;

		if( !ShaderHelper::Get().init() )
			return false;

		mViewFrustum.bUseReverse = false;

		IntVector2 screenSize = ::Global::GetDrawEngine().getScreenSize();
		VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
		VERIFY_RETURN_FALSE(SharedAssetData::createCommonShader());

		int numSamples = 8;
		VERIFY_RETURN_FALSE(mDepthBuffer = RHICreateTextureDepth(Texture::eDepth32F, screenSize.x, screenSize.y, 1, numSamples));
		VERIFY_RETURN_FALSE(mScreenBuffer = RHICreateTexture2D(Texture::eFloatRGBA, screenSize.x, screenSize.y, 1, numSamples, TCF_DefalutValue | TCF_RenderTarget));
		VERIFY_RETURN_FALSE(mFrameBuffer.create());
		mFrameBuffer.addTexture(*mScreenBuffer);
		mFrameBuffer.setDepth(*mDepthBuffer);

		for( int i = 0; i < 2; ++i )
		{
			VERIFY_RETURN_FALSE(mSmokeFrameTextures[i] = RHICreateTexture2D(Texture::eFloatRGBA, screenSize.x, screenSize.y, 1, 1, TCF_DefalutValue | TCF_RenderTarget));
		}
		VERIFY_RETURN_FALSE(mSmokeDepthTexture = RHICreateTexture2D(Texture::eR32F, screenSize.x, screenSize.y, 1, 1, TCF_DefalutValue | TCF_RenderTarget));
		VERIFY_RETURN_FALSE(mSmokeFrameBuffer.create());
		mSmokeFrameBuffer.addTexture(*mSmokeFrameTextures[0]);
		mSmokeFrameBuffer.addTexture(*mSmokeDepthTexture);

		mGrassTexture = RHIUtility::LoadTexture2DFromFile("Texture/Grass.png", TextureLoadOption().SRGB().MipLevel(10).ReverseH());
		MeshBuild::Plane(mGrassPlane, Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 0, 1), Vector2(1, float(mGrassTexture->getSizeY()) / mGrassTexture->getSizeX()), 1);
		ShaderManager::Get().loadFileSimple(mProgGrass, "Shader/Game/Grass");

		mProgNoise = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true, false> >(true);
		mProgNoiseUseTexture = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true, true> >(true);
		mProgNoiseTest = ShaderManager::Get().getGlobalShaderT< NoiseShaderTestProgram >(true);
		mProgSmokeRender = ShaderManager::Get().getGlobalShaderT< SmokeRenderProgram >(true);
		mProgSmokeBlend = ShaderManager::Get().getGlobalShaderT< SmokeBlendProgram >(true);

		{
			Random::Well512 rand;
			int randSize = 512;
			std::vector< float > randomData(randSize * randSize);
			for( float& v : randomData )
			{
				v = double(rand.rand()) / double(std::numeric_limits<uint32>::max());
			}
			mData.randTexture = RHICreateTexture2D(Texture::eR32F, randSize, randSize, 1, 1, TCF_DefalutValue, randomData.data(), 1);
		}

		{
			TPerlinNoise<true> noise;
			int textureSize = 256;
			int noiseSize = 24;
			int cellSize = 32;
			noise.repeat = noiseSize;

			DataCacheKey cacheKey;
			cacheKey.typeName = "NOISE-TEX";
			cacheKey.version = "66CEB0D6-4DBC-4967-A1CF-00F2221476CB";
			cacheKey.keySuffix.addFormat("%d-%d-%d", textureSize, noiseSize, cellSize);
			std::vector< float > data;
			if( !::Global::DataCache().loadT(cacheKey, data) )
			{
				data.resize(textureSize * textureSize);
				for( int j = 0; j < textureSize; ++j )
				{
					for( int i = 0; i < textureSize; ++i )
					{
						float nv = noise.getValue(noiseSize * i / float(textureSize - 1), noiseSize * j / float(textureSize - 1));
						data[i + j * textureSize] = 0.5 + 0.5 * nv;
					}
				}
				::Global::DataCache().saveT(cacheKey, data);
			}
			mData.noiseTexture = RHICreateTexture2D(Texture::eR32F, textureSize, textureSize, 1, 1, TCF_DefalutValue, data.data(), 1);
		}


		{

			int textureSize = 128;
			DataCacheKey cacheKey;
			cacheKey.typeName = "VOLUME-NOISE";
			cacheKey.version = "2F7B7D13-7DF5-454B-809E-4D0FD4D0DDFD";
			cacheKey.keySuffix.addFormat("%d", textureSize);

			std::vector< float > data;
			if( !::Global::DataCache().loadT(cacheKey, data) )
			{
				data.resize(textureSize * textureSize * textureSize);
				for( int k = 0; k < textureSize; ++k )
				{
					for( int j = 0; j < textureSize; ++j )
					{
						for( int i = 0; i < textureSize; ++i )
						{
							float value = ((Vector3(i, j, k) + Vector3(0.5)) / float(textureSize) - Vector3(0.5)).length();
							if( value < 0.5 )
							{
								data[i + (j + k * textureSize) * textureSize] = 0.5 - value;

							}
							else
							{
								data[i + (j + k * textureSize) * textureSize] = 0;
							}
						}
					}
				}
				::Global::DataCache().saveT(cacheKey, data);
			}
			mData.volumeTexture = RHICreateTexture3D(Texture::eR32F, textureSize, textureSize, textureSize, 1 , 1, TCF_DefalutValue, data.data());
		}


		{

			int textureSize = 128;
			int noiseSize = 24;
			int cellSize = 32;
			DataCacheKey cacheKey;
			cacheKey.typeName = "VOLUME-NOISE";
			cacheKey.version = "2F7B7D13-7DF5-454B-809E-4D0FD4D0DDFD";
			cacheKey.keySuffix.add(textureSize, "Perlin", noiseSize, cellSize);

			TPerlinNoise<true> noise;
			noise.repeat = noiseSize;

			std::vector< float > data;
			if( !::Global::DataCache().loadT(cacheKey, data) )
			{
				data.resize(textureSize * textureSize * textureSize);
				for( int k = 0; k < textureSize; ++k )
				{
					for( int j = 0; j < textureSize; ++j )
					{
						for( int i = 0; i < textureSize; ++i )
						{
							float nv = noise.getValue(noiseSize * i / float(textureSize - 1), noiseSize * j / float(textureSize - 1), noiseSize * k / float(textureSize - 1));
							data[i + (j + k * textureSize) * textureSize] = 0.5 + 0.5 * nv;
						}
					}
				}
				::Global::DataCache().saveT(cacheKey, data);
			}
			mData.NoiseVolumeTexture = RHICreateTexture3D(Texture::eR32F, textureSize, textureSize, textureSize, 1 , 1, TCF_DefalutValue, data.data());
		}


		mLights.resize(4);
		{
			auto& light = mLights[0];
			light.type = LightType::Point;
			light.pos = Vector3(5, 5, 5);
			light.color = Vector3(1, 0, 0);
			light.radius = 50;
			light.intensity = 20;
		}

		{
			auto& light = mLights[1];
			light.type = LightType::Point;
			light.pos = Vector3(5, -5, 5);
			light.color = Vector3(0, 1, 0);
			light.radius = 50;
			light.intensity = 20;
		}

		{
			auto& light = mLights[2];
			light.type = LightType::Point;
			light.pos = Vector3(-5, 5, 5);
			light.color = Vector3(0, 0, 1);
			light.radius = 50;
			light.intensity = 20;
		}
		{
			auto& light = mLights[3];
			light.type = LightType::Point;
			light.pos = Vector3(0, 0, -5);
			light.color = Vector3(1, 1, 1);
			light.radius = 50;
			light.intensity = 2;
		}


		VERIFY_RETURN_FALSE(mLightsBuffer.initializeResource(mLights.size(), BCF_DefalutValue | BCF_UsageDynamic));
		updateLightToBuffer();

		ShaderManager::Get().registerShaderAssets(::Global::GetAssetManager());

		::Global::GUI().cleanupWidget();

		auto devFrame = WidgetUtility::CreateDevFrame();
		devFrame->addText("FBM Shift");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.x, 0, 20);
		devFrame->addText("FBM Scale");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.y, 0, 3);
		devFrame->addText("FBM Rotate Angle");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.z, 0, 2);
		devFrame->addText("FBM Octaves");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.w, 0, 20);

		devFrame->addText("StepSize");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.stepSize, 0.001, 10, 2);

		devFrame->addText("DensityFactor");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.densityFactor, 0, 40, 2);
		devFrame->addText("PhaseG");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.phaseG, -1, 1);
		devFrame->addText("ScatterCoefficient");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.scatterCoefficient, 0, 10);
		devFrame->addText("Albedo");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.albedo, 0, 1);

		devFrame->addText("LightInstensity");
		WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mLights[0].intensity, 0, 200 , 2 , [&]( float value )
		{
			for( int i = 1; i < mLights.size(); ++i )
			{
				mLights[i].intensity = value;
			}
			updateLightToBuffer();
		});

		restart();

		return true;
	}



	void NoiseTestStage::onRender(float dFrame)
	{
		initializeRenderState();

		indexFrameTexture = 1 - indexFrameTexture;

		{

			GPU_PROFILE("Scene");

			{
				//mFrameBuffer.setDepth(*mDepthBuffer);
				GL_BIND_LOCK_OBJECT(mFrameBuffer);

				glClearDepth(mViewFrustum.bUseReverse ? 0 : 1);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				RHISetupFixedPipelineState(mView.worldToView, mView.viewToClip);
				DrawUtility::AixsLine();

				RHISetupFixedPipelineState(Matrix4::Scale(1.5) * Matrix4::Translate(2, 2, 2) * mView.worldToView, mView.viewToClip);
				mSimpleMeshs[SimpleMeshId::Doughnut].draw(LinearColor(1, 0.5, 0));

				RHISetupFixedPipelineState(Matrix4::Scale(1) * Matrix4::Translate(7, 2, -2) * mView.worldToView, mView.viewToClip);
				mSimpleMeshs[SimpleMeshId::Box].draw(LinearColor(0.25, 0.5, 1));


				{
					RHISetBlendState(TStaticAlphaToCoverageBlendState<>::GetRHI());
					//RHITexture2D const* textures[] = { mGrassTexture.get() };
					//RHISetupFixedPipelineState(Matrix4::Scale(1) * Matrix4::Translate(0, 0, 12) * mView.worldToView, mView.viewToClip , 1 , textures);
					GL_BIND_LOCK_OBJECT(mProgGrass);
					mView.setupShader(mProgGrass);
					mProgGrass.setTexture(SHADER_PARAM(Texture), *mGrassTexture , TStaticSamplerState< Sampler::eTrilinear > ::GetRHI());

					for( int n = 0 ; n < 100 ; ++n )
					{
						for( int i = 0; i < 10; ++i )
						{
							mProgGrass.setParam(SHADER_PARAM(Primitive.localToWorld), Matrix4::Rotate(Vector3(0, 0, 1), 2 * PI * i / 10) * Matrix4::Scale(1) * Matrix4::Translate( n / 10 , n % 10 , 12));
							mGrassPlane.drawShader();
						}
					}
				}

				{
					GPU_PROFILE("LightPoints");
					//FIXME
					RHISetupFixedPipelineState(mView.worldToView, mView.viewToClip);
					drawLightPoints(mView, MakeView(mLights));

				}
			}


			{
				GPU_PROFILE("CopyToScreen");
				mFrameBuffer.blitToBackBuffer();
				//ShaderHelper::Get().copyTextureToBuffer(*mScreenBuffer);
				return;
			}

			{
				mSmokeFrameBuffer.setTexture(0, *mSmokeFrameTextures[indexFrameTexture]);		
				//mFrameBuffer.removeDepthBuffer();
				GL_BIND_LOCK_OBJECT(mSmokeFrameBuffer);
				glClearColor(0, 0, 0, 1);
				GLfloat clearValueA[] = { 0 ,0, 0, 1 };
				GLfloat clearValueB[] = { 0 ,0, 0, 1 };
				glClearBufferfv(GL_COLOR, 0, clearValueA);
				glClearBufferfv(GL_COLOR, 1, clearValueB);

				GPU_PROFILE("SmokeRender");
				RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());

				GL_BIND_LOCK_OBJECT(*mProgSmokeRender);
				mProgSmokeRender->setParameters(mView, mData, Vector3(0, 0, 0), Vector3(20, 20, 20), mSmokeParams);
				mProgSmokeRender->setStructuredBufferT< TiledLightInfo >(*mLightsBuffer.getRHI());
				mProgSmokeRender->setParam(SHADER_PARAM(TiledLightNum), (int)mLights.size());
				mProgSmokeRender->setTexture(SHADER_PARAM(SceneDepthTexture), *mDepthBuffer);
				DrawUtility::ScreenRectShader();
			}

			{
				GL_BIND_LOCK_OBJECT(mFrameBuffer);
				GPU_PROFILE("SmokeBlend");
				RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(TStaticBlendState<CWM_RGB, Blend::eOne, Blend::eSrcAlpha >::GetRHI());
				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());

				GL_BIND_LOCK_OBJECT(*mProgSmokeBlend);
				mProgSmokeBlend->setParameters(mView, *mSmokeFrameTextures[indexFrameTexture], *mSmokeFrameTextures[1 - indexFrameTexture]);
				mProgSmokeBlend->setTexture(SHADER_PARAM(DepthTexture), *mSmokeDepthTexture);
				DrawUtility::ScreenRectShader();
			}
		}


		RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(TStaticRasterizerState<ECullMode::None>::GetRHI());
		RHISetBlendState(TStaticBlendState<>::GetRHI());
		Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
		auto gameWindow = ::Global::GetDrawEngine().getWindow();


		TArrayView< NoiseShaderProgramBase* const > noiseShaders = { mProgNoise , mProgNoiseUseTexture , mProgNoiseTest };
		int imageSize = Math::Min<int>(150, (gameWindow.getWidth() - 5 * (noiseShaders.size() + 1)) / noiseShaders.size());
		for( int i = 0; i < noiseShaders.size(); ++i )
		{
			drawNoiseImage(IntVector2(5 + (imageSize + 5) * i, 5), IntVector2(imageSize, imageSize), *noiseShaders[i]);

		}

		if( 0 )
		{
			RHISetViewport(0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(*mSmokeDepthTexture, IntVector2(10, 10), IntVector2(512, 512));
		}
	}

	void NoiseTestStage::onEnd()
	{
		ShaderManager::Get().unregisterShaderAssets(::Global::GetAssetManager());
		BaseClass::onEnd();
	}

}//namespace Render

