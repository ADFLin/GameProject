#include "NoiseTestStage.h"


#include "Renderer/MeshBuild.h"

#include "Image/ImageData.h"
#include "Math/GeometryAlgo2D.h"
#include "DataStructure/Grid2D.h"
#include "PerlinNoise.h"

namespace Geom2D
{
	template<>
	struct PolyProperty< std::vector< Vector2 > >
	{
		typedef std::vector< Vector2 > PolyType;
		static void  Setup(PolyType& p, int size) { p.resize(size); }
		static int   Size(PolyType const& p) { return p.size(); }
		static Vector2 const& Vertex(PolyType const& p, int idx) { return p[idx]; }
		static void  UpdateVertex(PolyType& p, int idx, Vector2 const& value) { p[idx] = value; }
	};
}

namespace Render
{
	REGISTER_STAGE_ENTRY("Noise Test", NoiseTestStage, EExecGroup::FeatureDev, 1, "Render|Shader");


#define IMPLEMENT_NOISE_SHADER( T1 , T2 )\
	typedef TNoiseShaderProgram< T1 , T2 > NoiseShaderProgram##T1##T2;\
	IMPLEMENT_SHADER_PROGRAM( NoiseShaderProgram##T1##T2 );

	IMPLEMENT_NOISE_SHADER(false, false);
	IMPLEMENT_NOISE_SHADER(false, true);
	IMPLEMENT_NOISE_SHADER(true, false);
	IMPLEMENT_NOISE_SHADER(true, true);

	IMPLEMENT_SHADER_PROGRAM(SmokeRenderProgram);
	IMPLEMENT_SHADER_PROGRAM(SmokeBlendProgram);


	class PointToRectOutlineProgram : public GlobalShaderProgram
	{
		typedef GlobalShaderProgram BaseClass;
		DECLARE_SHADER_PROGRAM(PointToRectOutlineProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/Game/PointToRectOutline";
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Geometry  , SHADER_ENTRY(MainGS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		virtual void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamHalfSize.bind(parameterMap, SHADER_PARAM(HalfSize));
			mParamViewProjectMatrix.bind(parameterMap, SHADER_PARAM(ViewProjectMatrix));
			mParamColor.bind(parameterMap, SHADER_PARAM(Color));
		}

		void setParameters(RHICommandList& commandList, float halfSize , Matrix4 const& viewProjectMatrix , LinearColor const& color )
		{
			setParam(commandList, mParamHalfSize, halfSize);
			setParam(commandList, mParamViewProjectMatrix, viewProjectMatrix);
			setParam(commandList, mParamColor, color);
		}

		ShaderParameter mParamHalfSize;
		ShaderParameter mParamViewProjectMatrix;
		ShaderParameter mParamColor;
	};

	IMPLEMENT_SHADER_PROGRAM(PointToRectOutlineProgram);

	template < class TFunc >
	IntVector2 CalcMaxRect(uint8 const* pPixel, int rowStride, IntVector2 const& maxSize , TFunc&& Func)
	{
		assert(Func(*pPixel) == false);
		enum
		{
			XBlocked = BIT(0),
			YBlocked = BIT(1),
			CornerBlocked = BIT(2),
		};

		uint8 const* pData;
		uint state = 0;
		int  len = 1;
		int maxLen = Math::Min(maxSize.x, maxSize.y) - 1;
		for( ; len < maxLen; ++len )
		{
			pData = pPixel + len;
			for( int i = len ; i ; --i, pData += rowStride )
			{
				if( Func(*pData) )
				{
					state |= XBlocked;
					break;
				}
			}

			pData = pPixel + len * rowStride;
			for( int i = len; i ; --i, pData += 1 )
			{
				if( Func(*pData) )
				{
					state |= YBlocked;
					break;
				}
			}

			if( Func(*pData) )
			{
				state |= CornerBlocked;
				break;
			}

			if ( state != 0 )
				break;
		}

		if( state == 0 )
		{
			if( maxSize.x == maxSize.y )
			{
				return maxSize;
			}
			else if( maxSize.x > maxSize.y )
			{
				state = BIT(1);
			}
			else
			{
				state = BIT(0);
			}
		}

		if( ( state & (XBlocked | YBlocked) ) == (XBlocked | YBlocked)  )
			return IntVector2(len , len);

		IntVector2 result;
		if ( !( state & YBlocked ) )
		{
			result.x = len;
			for( int sizeY = len + 1; sizeY < maxSize.y; ++sizeY )
			{
				pData = pPixel + sizeY * rowStride;
				for( int i = len; i; --i, pData += 1 )
				{
					if( Func(*pData) )
					{
						goto EndY;
					}
				}
			}
		EndY:
			result.y = sizeY;

		}
		else
		{
			result.y = len;
			for( int sizeX = len + 1; sizeX < maxSize.x; ++sizeX )
			{
				pData = pPixel + sizeX;
				for( int i = len; i; --i, pData += rowStride )
				{
					if( Func(*pData) )
					{
						goto EndX;
					}
				}
			}
		EndX:
			result.x = sizeX;
		}

		return result;
	}

	int const gGrassNum = 5000;
	bool NoiseTestStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;


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

		for (int n = 0; n < gGrassNum; ++n)
		{
			for (int i = 0; i < 10; ++i)
			{
				mInstancedMesh.addInstance(Vector3(n / 100, n % 100, 12), Vector3(1, 1, 1), Quaternion::Rotate(Vector3(0, 0, 1), 2 * PI * i / 10), Vector4(0, 0, 0, 0));
			}
		}

		::Global::GUI().cleanupWidget();

		auto devFrame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "Use Instanced"), mbDrawInstaced);
		FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "Use OptMesh"), mbUseOptMesh);

		devFrame->addText("FBM Shift");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.x, 0, 20);
		devFrame->addText("FBM Scale");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.y, 0, 3);
		devFrame->addText("FBM Rotate Angle");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.z, 0, 2);
		devFrame->addText("FBM Octaves");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.w, 0, 20);

		devFrame->addText("StepSize");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.stepSize, 0.001, 10, 2);

		devFrame->addText("DensityFactor");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.densityFactor, 0, 40, 2);
		devFrame->addText("PhaseG");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.phaseG, -1, 1);
		devFrame->addText("ScatterCoefficient");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.scatterCoefficient, 0, 10);
		devFrame->addText("Albedo");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mSmokeParams.albedo, 0, 1);

		devFrame->addText("LightInstensity");
		FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mLights[0].intensity, 0, 200, 2, [&](float value)
		{
			for (int i = 1; i < mLights.size(); ++i)
			{
				mLights[i].intensity = value;
			}
			updateLightToBuffer();
		});
		restart();

		return true;
	}

	bool NoiseTestStage::setupRenderSystem(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(BaseClass::setupRenderSystem(systemName));

		mViewFrustum.bUseReverse = false;

		IntVector2 screenSize = ::Global::GetScreenSize();
		VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
		VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());

		int numSamples = 1;

		VERIFY_RETURN_FALSE(mDepthBuffer = RHICreateTextureDepth(ETexture::Depth32F, screenSize.x, screenSize.y, 1, numSamples, TCF_CreateSRV));
		VERIFY_RETURN_FALSE(mScreenBuffer = RHICreateTexture2D(ETexture::FloatRGBA, screenSize.x, screenSize.y, 1, numSamples, TCF_CreateSRV | TCF_RenderTarget));
		VERIFY_RETURN_FALSE(mFrameBuffer = RHICreateFrameBuffer());
		mFrameBuffer->addTexture(*mScreenBuffer);
		mFrameBuffer->setDepth(*mDepthBuffer);

		if( numSamples != 1 )
		{
			VERIFY_RETURN_FALSE(mResolvedDepthBuffer = RHICreateTextureDepth(ETexture::Depth32F, screenSize.x, screenSize.y, 1, 1, TCF_CreateSRV));
		}
		else
		{
			mResolvedDepthBuffer = mDepthBuffer;
		}
		mResolveFrameBuffer = RHICreateFrameBuffer();
		mResolveFrameBuffer->setDepth(*mResolvedDepthBuffer);

		for( int i = 0; i < 2; ++i )
		{
			VERIFY_RETURN_FALSE(mSmokeFrameTextures[i] = RHICreateTexture2D(ETexture::FloatRGBA, screenSize.x, screenSize.y, 1, 1, TCF_DefalutValue | TCF_RenderTarget));
		}
		VERIFY_RETURN_FALSE(mSmokeDepthTexture = RHICreateTexture2D(ETexture::R32F, screenSize.x, screenSize.y, 1, 1, TCF_DefalutValue | TCF_RenderTarget));
		VERIFY_RETURN_FALSE(mSmokeFrameBuffer = RHICreateFrameBuffer() );
		mSmokeFrameBuffer->addTexture(*mSmokeFrameTextures[0]);
		mSmokeFrameBuffer->addTexture(*mSmokeDepthTexture);

		{
			char const* grassTexturePath = "Texture/Grass.png";
			ImageData imageData;

			VERIFY_RETURN_FALSE(imageData.load(grassTexturePath, false, true));


			TGrid2D< IntVector2 > pixels;

			if ( imageData.numComponent == 4 || imageData.numComponent == 1 )
			{
				uint8 const* alphaPtr = (uint8*)(imageData.data) + 3;

				std::vector< uint8 > imageFlags;
				imageFlags.resize(imageData.width * imageData.height, 0);

				for( int j = 0; j < imageData.height; ++j )
				{
					for( int i = 0; i < imageData.width; ++i )
					{
						uint8 alpha = alphaPtr[imageData.numComponent * ((imageData.width * j) + i)];
						if( alpha == 0 )
						{
							static int const xOffset[] = { 1, -1 , 0 , 0 };
							static int const yOffset[] = { 0 , 0, 1, -1 };
							for( int dir = 0; dir < 4; ++dir )
							{
								int nx = i + xOffset[dir];
								int ny = j + yOffset[dir];
								if ( 0 <= nx && nx < imageData.width &&
									 0 <= ny && ny < imageData.height )
								{
									if( alphaPtr[imageData.numComponent * ((imageData.width * ny) + nx)] )
									{
										mImagePixels.push_back(Vector2(i, j));
										break;
									}
								}
							}
						}
					}
				}
				std::vector< int > hullIndices;
				hullIndices.resize( mImagePixels.size() );
				int numIndices = Geom2D::QuickHull(mImagePixels, hullIndices.data());
				int xxx = 1;
			}
			else
			{



			}

			VERIFY_RETURN_FALSE( mGrassTexture = RHIUtility::CreateTexture2D(imageData, TextureLoadOption().SRGB().AutoMipMap().FlipV()) );
		}

		FMeshBuild::Plane(mGrassMesh, Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 0, 1), Vector2(1, float(mGrassTexture->getSizeY()) / mGrassTexture->getSizeX()), 1);
		mInstancedMesh.setupMesh(mGrassMesh);

		ShaderManager::Get().loadFileSimple(mProgGrass, "Shader/Game/Grass");
		ShaderManager::Get().loadFileSimple(mProgGrassInstanced, "Shader/Game/Grass", "#define USE_INSTANCED 1\n");
		mProgNoise = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true, false> >(true);
		mProgNoiseUseTexture = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true, true> >(true);
		mProgNoiseTest = ShaderManager::Get().getGlobalShaderT< NoiseShaderTestProgram >(true);
		mProgSmokeRender = ShaderManager::Get().getGlobalShaderT< SmokeRenderProgram >(true);
		mProgSmokeBlend = ShaderManager::Get().getGlobalShaderT< SmokeBlendProgram >(true);
		mProgResolveDepth = ShaderManager::Get().getGlobalShaderT< ResovleDepthProgram >(true);
#if 0
		mProgPointToRectOutline = ShaderManager::Get().getGlobalShaderT< PointToRectOutlineProgram >(true);
#endif

		{
			Random::Well512 rand;
			int randSize = 512;
			std::vector< float > randomData(randSize * randSize);
			for( float& v : randomData )
			{
				v = double(rand.rand()) / double(std::numeric_limits<uint32>::max());
			}
			mData.randTexture = RHICreateTexture2D(ETexture::R32F, randSize, randSize, 1, 1, TCF_DefalutValue, randomData.data(), 1);
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
			mData.noiseTexture = RHICreateTexture2D(ETexture::R32F, textureSize, textureSize, 1, 1, TCF_DefalutValue, data.data(), 1);
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
			mData.volumeTexture = RHICreateTexture3D(ETexture::R32F, textureSize, textureSize, textureSize, 1 , 1, TCF_DefalutValue, data.data());
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
			mData.noiseVolumeTexture = RHICreateTexture3D(ETexture::R32F, textureSize, textureSize, textureSize, 1 , 1, TCF_DefalutValue, data.data());
		}



		VERIFY_RETURN_FALSE(mLightsBuffer.initializeResource(mLights.size(), EStructuredBufferType::Buffer));
		updateLightToBuffer();

		return true;
	}

	void NoiseTestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mData.releaseRHI();
		mLightsBuffer.releaseResources();
		mSmokeFrameTextures[0].release();
		mSmokeFrameTextures[1].release();
		mSmokeDepthTexture.release();
		mSmokeFrameBuffer.release();
		mResolveFrameBuffer.release();
		mFrameBuffer.release();
		mDepthBuffer.release();
		mResolvedDepthBuffer.release();
		mScreenBuffer.release();
		mGrassTexture.release();
		mGrassMesh.releaseRHIResource();
		mGrassMeshOpt.releaseRHIResource();
		mProgGrass.releaseRHI();
		mProgGrassInstanced.releaseRHI();
		mInstancedMesh.releaseRHI();

		BaseClass::preShutdownRenderSystem(bReInit);
	}

	void NoiseTestStage::onRender(float dFrame)
	{
		initializeRenderState();
		Vec2i screenSize = ::Global::GetScreenSize();
		RHICommandList& commandList = RHICommandList::GetImmediateList();
		indexFrameTexture = 1 - indexFrameTexture;

		TextureShowManager::registerTexture("SomkeTexture", mSmokeFrameTextures[indexFrameTexture]);
		{

			GPU_PROFILE("Scene");

			{
				//RHISetFrameBuffer(commandList, nullptr);

				mFrameBuffer->setDepth(*mDepthBuffer);
				RHISetFrameBuffer(commandList, mFrameBuffer);
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.2, 0.2, 0.2, 1), 1, mViewFrustum.bUseReverse ? 0 : 1);

				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

#if 1
				RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
				DrawUtility::AixsLine(commandList);

				RHISetFixedShaderPipelineState(commandList, Matrix4::Scale(1.5) * Matrix4::Translate(2, 2, 2) * mView.worldToClip);
				mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList, LinearColor(1, 0.5, 0));

				RHISetFixedShaderPipelineState(commandList, Matrix4::Scale(1) * Matrix4::Translate(7, 2, -2) * mView.worldToClip);
				mSimpleMeshs[SimpleMeshId::Box].draw(commandList, LinearColor(0.25, 0.5, 1));
#endif


				{
					GPU_PROFILE("DrawGrass");
					RHISetBlendState(commandList, TStaticAlphaToCoverageBlendState<>::GetRHI());
					auto& progGrass = (mbDrawInstaced) ? mProgGrassInstanced : mProgGrass;
					RHISetShaderProgram(commandList, progGrass.getRHI());
					mView.setupShader(commandList, progGrass);
					progGrass.setTexture(commandList, SHADER_PARAM(Texture), *mGrassTexture , SHADER_PARAM(TextureSampler) , TStaticSamplerState< ESampler::Trilinear > ::GetRHI());

					Mesh& meshUsed = (mbUseOptMesh) ? mGrassMeshOpt : mGrassMesh;
					if( mbDrawInstaced )
					{
						progGrass.setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Identity());

						mInstancedMesh.changeMesh(meshUsed);
						mInstancedMesh.draw(commandList);
					}
					else
					{

						for( int n = 0; n < gGrassNum; ++n )
						{
							Vector3 pos = Vector3(n / 100, n % 100, 12);
							for( int i = 0; i < 10; ++i )
							{
								progGrass.setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Rotate(Vector3(0, 0, 1), 2 * PI * i / 10) * Matrix4::Scale(1) * Matrix4::Translate(pos));
								meshUsed.draw(commandList);
							}
						}
					}
				}

				{
					GPU_PROFILE("LightPoints");
					//FIXME
					RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
					drawLightPoints(commandList, mView, MakeView(mLights));

				}
			}

			{
				GPU_PROFILE("CopyToScreen");
				RHISetFrameBuffer(commandList, nullptr);
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

				//OpenGLCast::To( mFrameBuffer )->blitToBackBuffer();
				ShaderHelper::Get().copyTextureToBuffer(commandList , *mScreenBuffer);
			}

			if( mDepthBuffer->getNumSamples() != 1 )
			{
				RHISetFrameBuffer(commandList, mResolveFrameBuffer);

				GPU_PROFILE("ResolveDepth");
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState< true , ECompareFunc::Always >::GetRHI());

				RHISetShaderProgram(commandList, mProgResolveDepth->getRHI());
				mProgResolveDepth->setTexture(commandList, SHADER_PARAM(UnsolvedDepthTexture), *mDepthBuffer);
				DrawUtility::ScreenRect(commandList, mDepthBuffer->getSizeX() , mDepthBuffer->getSizeY());
			}

			{
				mSmokeFrameBuffer->setTexture(0, *mSmokeFrameTextures[indexFrameTexture]);		
				//mFrameBuffer.removeDepthBuffer();
				RHISetFrameBuffer(commandList, mSmokeFrameBuffer);

				LinearColor clearColors[] = { LinearColor(0 ,0, 0, 1), LinearColor(0 ,0, 0, 1) };
				RHIClearRenderTargets(commandList, EClearBits::Color, clearColors, 2);

				GPU_PROFILE("SmokeRender");
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

				RHISetShaderProgram(commandList, mProgSmokeRender->getRHI());
				mProgSmokeRender->setParameters(commandList, mView, mData, Vector3(0, 0, 0), Vector3(20, 20, 20), mSmokeParams);
				mProgSmokeRender->setStructuredStorageBufferT< TiledLightInfo >(commandList, *mLightsBuffer.getRHI());
				mProgSmokeRender->setParam(commandList, SHADER_PARAM(TiledLightNum), (int)mLights.size());
				auto& sampler = TStaticSamplerState<ESampler::Bilinear , ESampler::Clamp , ESampler::Clamp>::GetRHI();
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgSmokeRender, SceneDepthTexture, *mResolvedDepthBuffer, sampler);
				DrawUtility::ScreenRect(commandList);

				CLEAR_SHADER_TEXTURE(commandList, *mProgSmokeRender, SceneDepthTexture);

				RHISetFrameBuffer(commandList, nullptr);
			}

			{
				RHISetFrameBuffer(commandList, nullptr);

				//GL_BIND_LOCK_OBJECT(mFrameBuffer);
				GPU_PROFILE("SmokeBlend");
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<CWM_RGB, EBlend::One, EBlend::SrcAlpha >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

				RHISetShaderProgram(commandList, mProgSmokeBlend->getRHI());
				mProgSmokeBlend->setParameters(commandList, mView, *mSmokeFrameTextures[indexFrameTexture], *mSmokeFrameTextures[1 - indexFrameTexture]);
				auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp>::GetRHI();
				SET_SHADER_TEXTURE_AND_SAMPLER(commandList, *mProgSmokeBlend, SceneDepthTexture, *mSmokeDepthTexture, sampler);
				DrawUtility::ScreenRect(commandList);

				CLEAR_SHADER_TEXTURE(commandList, *mProgSmokeBlend, SceneDepthTexture);
				CLEAR_SHADER_TEXTURE(commandList, *mProgSmokeBlend, FrameTexture);
				CLEAR_SHADER_TEXTURE(commandList, *mProgSmokeBlend, HistroyTexture);
			}
		}

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());



		TArrayView< NoiseShaderProgramBase* const > noiseShaders = { mProgNoise , mProgNoiseUseTexture , mProgNoiseTest };
		int imageSize = Math::Min<int>(150, (screenSize.x - 5 * (noiseShaders.size() + 1)) / noiseShaders.size());
		for( int i = 0; i < noiseShaders.size(); ++i )
		{
			drawNoiseImage(commandList, IntVector2(5 + (imageSize + 5) * i, 5), IntVector2(imageSize, imageSize), *noiseShaders[i]);

		}


#if 0
		if( 1 )
		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			//OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			OrthoMatrix matProj(200, 400, 200, 400, -1, 1);
			RHISetShaderProgram(commandList, mProgPointToRectOutline->getRHIResource());
			mProgPointToRectOutline->setParameters(commandList, 0.5, AdjProjectionMatrixForRHI( matProj ), LinearColor(1, 1, 0, 1));
			TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::Points, mImagePixels.data(), mImagePixels.size());
		}
#endif

		RHISetShaderProgram(commandList, nullptr);



		if( 0 )
		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, 0, screenSize.y, -1, 1));
			DrawUtility::DrawTexture(commandList, porjectMatrix, *mSmokeDepthTexture, IntVector2(10, 10), IntVector2(512, 512));
		}
	}

	void NoiseTestStage::onEnd()
	{
		BaseClass::onEnd();
	}

}//namespace Render

