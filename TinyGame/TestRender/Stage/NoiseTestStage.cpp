#include "TestRenderStageBase.h"

#include "RHI/Scene.h"
#include "RHI/SceneRenderer.h"

#include "Random.h"
#include "DataCacheInterface.h"
#include "DataSteamBuffer.h"




namespace Render
{
	uint8 DefaultPerm[256] =
	{
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
		140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
		247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
		57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
		74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
		60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
		65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,200,
		196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
		52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
		207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
		119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
		129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
		218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
		81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
		184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
		222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};

	template< int bRepeat = false >
	class TPerlinNoise
	{
	public:

		TPerlinNoise()
		{
			for( int i = 0; i < 256; ++i )
			{
				mValue[i] = mValue[256 + i] = DefaultPerm[i];

				float s, c;
				Math::SinCos(2 * Math::PI * i / 256, s, c);
				mDir[i] = Vector2(c, s);
			}
		}

		template< class T >
		T getValue(T x)
		{
			auto& p = mValue;

			int xi = Math::FloorToInt(x);
			x -= xi; xi &= 0xff;
			if( bRepeat && repeat )
			{
				xi %= repeat;
			}

			T u = Fade(x);
			int a, b;
			a = p[xi];
			b = p[inc(xi)];
			return 	Lerp(u, Grad<T>(a, x),
					        Grad<T>(b, x - 1) );
		}

		template< class T >
		float getValue(T x, T y)
		{
			auto& p = mValue;

			int xi = Math::FloorToInt(x);
			int yi = Math::FloorToInt(y);

			x -= xi; xi &= 0xff;
			y -= yi; yi &= 0xff;

			if( bRepeat && repeat )
			{
				xi %= repeat;
				yi %= repeat;
			}

			T u = Fade(x);
			T v = Fade(y);

			int aa, ab, ba , bb;
			aa = p[p[xi] + yi];
			ba = p[p[inc(xi)] + yi];
			ab = p[p[xi] + inc(yi)];
			bb = p[p[inc(xi)] + inc(yi)];

			return Lerp(
				v,
				Lerp(u, Grad<T>(aa, x, y),
					    Grad<T>(ba, x - 1, y)),
				Lerp(u, Grad<T>(ab, x, y - 1),
					    Grad<T>(bb, x - 1, y - 1))
			);
		}

		template< class T >
		T getValue(T x , T y , T z)
		{
			auto& p = mValue;
			int xi = Math::FloorToInt(x);
			int yi = Math::FloorToInt(y);
			int zi = Math::FloorToInt(z);

			x -= xi; xi &= 0xff;
			y -= yi; yi &= 0xff;
			z -= zi; zi &= 0xff;


			if( bRepeat && repeat )
			{
				xi %= repeat;
				yi %= repeat;
				zi %= repeat;
			}
			
			T u = Fade(x);
			T v = Fade(y);
			T w = Fade(z);

			int aaa, aba, aab, abb, baa, bba, bab, bbb;
			aaa = p[p[p[xi] + yi] + zi];
			aba = p[p[p[xi] + inc(yi)] + zi];
			aab = p[p[p[xi] + yi] + inc(zi)];
			abb = p[p[p[xi] + inc(yi)] + inc(zi)];
			baa = p[p[p[inc(xi)] + yi] + zi];
			bba = p[p[p[inc(xi)] + inc(yi)] + zi];
			bab = p[p[p[inc(xi)] + yi] + inc(zi)];
			bbb = p[p[p[inc(xi)] + inc(yi)] + inc(zi)];

			return Lerp(
				w,
				Lerp(v, Lerp(u, Grad(aaa, x, y, z),
							    Grad(baa, x-1, y, z)),
					    Lerp(u, Grad(aba, x, y-1, z),
						        Grad(bba, x-1, y-1, z))),
				Lerp(v, Lerp(u, Grad(aab, x, y, z-1),
							    Grad(bab, x-1, y, z-1)),
					    Lerp(u, Grad(abb, x, y-1, z-1),
						        Grad(bbb, x-1, y-1, z-1)))
			);
		}

		int   repeat = 0;
		int   inc(int x) const 
		{ 
			++x;
			if( bRepeat && repeat )
				x %= repeat;
			return x;
		}

		template < class T >
		T Grad(int hash, T x)
		{
			return Grad<T>(hash, x, 0);
		}

		template < class T >
		T Grad(int hash, T x, T y)
		{
#if 0
			auto const& dir = mDir[hash];
			return dir.dot(Vector2(x, y));
#else
			return Grad<T>(hash, x, y, 0);
#endif
		}

		template < class T >
		static T Grad(int hash, T x, T y, T z)
		{
#if 0
			int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
			T u = h < 8 ? x : y;              // INTO 12 GRADIENT DIRECTIONS.
			T v = h < 4 ? y : h == 12 || h == 14 ? x : z;
			return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
#else
			switch( hash & 0xF )
			{
			case 0x0: return  x + y;
			case 0x1: return -x + y;
			case 0x2: return  x - y;
			case 0x3: return -x - y;
			case 0x4: return  x + z;
			case 0x5: return -x + z;
			case 0x6: return  x - z;
			case 0x7: return -x - z;
			case 0x8: return  y + z;
			case 0x9: return -y + z;
			case 0xA: return  y - z;
			case 0xB: return -y - z;
			case 0xC: return  y + x;
			case 0xD: return -y + z;
			case 0xE: return  y - x;
			case 0xF: return -y - z;
			}
			return 0;
#endif

		}
		template < class T >
		static T Fade(T x)
		{
			T x2 = x * x;
			return x2 * x * (6.0 * x2 - 15.0 * x + 10.0);
		}
		template < class T >
		static T Lerp(T t , T a, T b)
		{
			return a + t * (b - a);
		}

		uint32  mValue[512];
		Vector2 mDir[256];

	};
	struct NoiseShaderParamsData
	{
		float time;
		Vector4 FBMFactor;
		RHITexture2DRef randTexture;
		RHITexture2DRef noiseTexture;
		RHITexture3DRef volumeTexture;
		RHITexture3DRef NoiseVolumeTexture;
		

		NoiseShaderParamsData()
		{
			FBMFactor = Vector4(100, 1.0, 0.5, 5);
		
		}
	};

	struct V3
	{
		V3() = default;
		float x, y, z;
	};

	struct Foo
	{
		int a;
		int c;
		Vector3 v;
	};
	static_assert(std::is_standard_layout<Foo>::value, "aa");
	static_assert(std::is_trivially_constructible<Foo>::value, "cc");
	static_assert(std::is_trivial<Foo>::value, "bb");

	class NoiseShaderProgramBase : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(NoiseShaderProgramBase, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/NoiseTest";
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


		virtual void bindParameters(ShaderParameterMap& parameterMap) override
		{
			parameterMap.bind(mParamTime, SHADER_PARAM(Time));
			parameterMap.bind(mParamFBMFactor, SHADER_PARAM(FBMFactor));
			parameterMap.bind(mParamRandTexture, SHADER_PARAM(RandTexture));
			parameterMap.bind(mParamNoiseTexture, SHADER_PARAM(NoiseTexture));
			parameterMap.bind(mParamVolumeTexture, SHADER_PARAM(VolumeTexture));
			parameterMap.bind(mParamNoiseVolumeTexture, SHADER_PARAM(NoiseVolumeTexture));
			
		}

		void setParameters(NoiseShaderParamsData& data)
		{
			if( mParamTime.isBound() )
			{
				setParam(mParamTime, data.time);
			}
			if( mParamFBMFactor.isBound() )
			{
				setParam(mParamFBMFactor, data.FBMFactor);
			}
			if( mParamRandTexture.isBound() )
			{
				setTexture(mParamRandTexture, *data.randTexture, TStaticSamplerState<Sampler::ePoint>::GetRHI());
			}
			if( mParamNoiseTexture.isBound() )
			{
				setTexture(mParamNoiseTexture, *data.noiseTexture, TStaticSamplerState<Sampler::eBilinear>::GetRHI());
			}
			if( mParamVolumeTexture.isBound() )
			{
				setTexture(mParamVolumeTexture, *data.volumeTexture, TStaticSamplerState<Sampler::eBilinear>::GetRHI());
			}
			if( mParamNoiseVolumeTexture.isBound() )
			{
				setTexture(mParamNoiseVolumeTexture, *data.NoiseVolumeTexture, TStaticSamplerState<Sampler::eBilinear>::GetRHI());
			}

		}


		ShaderParameter mParamTime;
		ShaderParameter mParamRandTexture;
		ShaderParameter mParamNoiseTexture;
		ShaderParameter mParamVolumeTexture;
		ShaderParameter mParamNoiseVolumeTexture;
		ShaderParameter mParamFBMFactor;
	};

	class NoiseShaderTestProgram : public NoiseShaderProgramBase
	{
		typedef NoiseShaderProgramBase BaseClass;
		DECLARE_SHADER_PROGRAM(NoiseShaderTestProgram, Global);
	public:
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(TestPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(NoiseShaderTestProgram);

	template< bool bUseRandTexture , bool bUseFBMFactor = true >
	class TNoiseShaderProgram : public NoiseShaderProgramBase
	{
		typedef NoiseShaderProgramBase BaseClass;
		DECLARE_SHADER_PROGRAM(TNoiseShaderProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(USE_RAND_TEXTURE), bUseRandTexture);
			option.addDefine(SHADER_PARAM(USE_FBM_FACTOR), bUseFBMFactor);
		}
	};

#define IMPLEMENT_NOISE_SHADER( T1 , T2 )\
	typedef TNoiseShaderProgram< T1 , T2 > NoiseShaderProgram##T1##T2;\
	IMPLEMENT_SHADER_PROGRAM( NoiseShaderProgram##T1##T2 );

	IMPLEMENT_NOISE_SHADER(false, false);
	IMPLEMENT_NOISE_SHADER(false, true);
	IMPLEMENT_NOISE_SHADER(true, false);
	IMPLEMENT_NOISE_SHADER(true, true);


	class CloudRenderTestProgram : public NoiseShaderProgramBase
	{
		typedef NoiseShaderProgramBase BaseClass;
		DECLARE_SHADER_PROGRAM(CloudRenderTestProgram, Global);
	public:
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(CloudRenderPS) },
			};
			return entries;
		}

		virtual void bindParameters(ShaderParameterMap& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			parameterMap.bind(mParamVolumeMin, SHADER_PARAM(VolumeMin));
			parameterMap.bind(mParamVolumeSize, SHADER_PARAM(VolumeSize));
			parameterMap.bind(mParamScatterFactor, SHADER_PARAM(ScatterFactor));
		}

		void setParameters(ViewInfo& view, NoiseShaderParamsData& data, Vector3 volumeCenter, Vector3 volumeSize ,  float densityFactor , float phaseG )
		{
			BaseClass::setParameters(data);
			view.setupShader(*this);
			setParam(mParamVolumeMin, volumeCenter - 0.5 * volumeSize);
			setParam(mParamVolumeSize, volumeSize);
			setParam(mParamScatterFactor, Vector4( densityFactor , phaseG , 0 , 0 ) );
		}

		ShaderParameter mParamVolumeMin;
		ShaderParameter mParamVolumeSize;
		ShaderParameter mParamScatterFactor;
	};

	IMPLEMENT_SHADER_PROGRAM(CloudRenderTestProgram);

	class NoiseTestStage : public TestRenderStageBase

	{
		typedef TestRenderStageBase BaseClass;
	public:
		NoiseTestStage() {}

		NoiseShaderParamsData mData;

		NoiseShaderProgramBase* mProgNoise;
		NoiseShaderProgramBase* mProgNoiseUseTexture;
		NoiseShaderProgramBase* mProgNoiseTest;
		CloudRenderTestProgram* mProgCloudRender;

		TStructuredStorageBuffer< TiledLightInfo > mLightsBuffer;


		OpenGLFrameBuffer  mFrameBuffer;
		RHITextureDepthRef mDepthBuffer;
		RHITexture2DRef    mScreenBuffer;
		float mDensityFactor = 1.0;
		float mPhaseG = 0.3;
		std::vector< LightInfo > mLights;
		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;

			if( !InitGlobalRHIResource() )
				return false;

			if( !ShaderHelper::Get().init() )
				return false;

			mViewFrustum.bUseReverse = false;

			IntVector2 screenSize = ::Global::GetDrawEngine().getScreenSize();
			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());

			VERIFY_RETURN_FALSE(SharedAssetData::createCommonShader());

			VERIFY_RETURN_FALSE(mDepthBuffer = RHICreateTextureDepth(Texture::eDepth32F, screenSize.x, screenSize.y));
			VERIFY_RETURN_FALSE(mScreenBuffer = RHICreateTexture2D(Texture::eFloatRGBA, screenSize.x, screenSize.y, 1, TCF_DefalutValue | TCF_RenderTarget));
			VERIFY_RETURN_FALSE(mFrameBuffer.create());
			mFrameBuffer.addTexture(*mScreenBuffer);
			mFrameBuffer.setDepth(*mDepthBuffer);

			mProgNoise = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true,false> >(true);
			mProgNoiseUseTexture = ShaderManager::Get().getGlobalShaderT< TNoiseShaderProgram<true,true> >(true);
			mProgNoiseTest = ShaderManager::Get().getGlobalShaderT< NoiseShaderTestProgram >(true);
			mProgCloudRender = ShaderManager::Get().getGlobalShaderT< CloudRenderTestProgram >(true);

			{
				Random::Well512 rand;
				int randSize = 512;
				std::vector< float > randomData(randSize * randSize);
				for( float& v : randomData )
				{
					v = double(rand.rand()) / double(std::numeric_limits<uint32>::max());
				}
				mData.randTexture = RHICreateTexture2D(Texture::eR32F, randSize, randSize, 1, TCF_DefalutValue, randomData.data(), 1);
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
				if( !::Global::DataCache().loadT(cacheKey, data))
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
				mData.noiseTexture = RHICreateTexture2D(Texture::eR32F, textureSize, textureSize, 1, TCF_DefalutValue, data.data(), 1);
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
				mData.volumeTexture = RHICreateTexture3D(Texture::eR32F, textureSize, textureSize, textureSize , TCF_DefalutValue, data.data() );
			}


			{

				int textureSize = 128;
				int noiseSize = 24;
				int cellSize = 32;
				DataCacheKey cacheKey;
				cacheKey.typeName = "VOLUME-NOISE";
				cacheKey.version = "2F7B7D13-7DF5-454B-809E-4D0FD4D0DDFD";
				cacheKey.keySuffix.add( textureSize , "Perlin", noiseSize , cellSize);

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
				mData.NoiseVolumeTexture = RHICreateTexture3D(Texture::eR32F, textureSize, textureSize, textureSize, TCF_DefalutValue, data.data());
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
				light.pos = Vector3(0, 0, 0);
				light.color = Vector3(1, 1, 1);
				light.radius = 50;
				light.intensity = 2;
			}


			VERIFY_RETURN_FALSE(mLightsBuffer.initializeResource(mLights.size()));
			updateLightToBuffer();

			ShaderManager::Get().registerShaderAssets(::Global::GetAssetManager());

			::Global::GUI().cleanupWidget();

			auto devFrame = WidgetUtility::CreateDevFrame();
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.x, 0, 20);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.y, 0, 3);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.z, 0, 2);
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mData.FBMFactor.w, 0, 20);

			devFrame->addText("DensityFactor");
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mDensityFactor, 0, 40, 2);
			devFrame->addText("PhaseG");
			WidgetPropery::Bind(devFrame->addSlider(UI_ANY), mPhaseG, -1, 1);
			restart();

			return true;
		}

		virtual void onEnd()
		{
			ShaderManager::Get().unregisterShaderAssets(::Global::GetAssetManager());
			::Global::GetDrawEngine().shutdownRHI(true);
			BaseClass::onEnd();
		}

		void updateLightToBuffer()
		{
			TiledLightInfo* pLightInfo = mLightsBuffer.lock();
			for( int i = 0; i < mLights.size(); ++i )
			{
				pLightInfo[i].color = mLights[i].color;
				pLightInfo[i].pos = mLights[i].pos;
				pLightInfo[i].radius = mLights[i].radius;
				pLightInfo[i].intensity = mLights[i].intensity;
			}
			mLightsBuffer.unlock();
		}
		void drawNoiseImage(IntVector2 const& pos, IntVector2 const& size, NoiseShaderProgramBase& shader)
		{
			GPU_PROFILE("drawNoiseImage");
			RHISetViewport(pos.x, pos.y, size.x, size.y);
			GL_BIND_LOCK_OBJECT(shader);
			shader.setParameters(mData);
			DrawUtility::ScreenRectShader();
		}


		void restart()
		{
			mData.time = 0;
		}
		void tick() {}
		void updateFrame(int frame) {}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);
			mData.time += float(time) / 1000.0f;
		}

		void onRender(float dFrame)
		{
			initializeRenderState();

			{
				mFrameBuffer.setDepth(*mDepthBuffer);
				GL_BIND_LOCK_OBJECT(mFrameBuffer);

				glClearDepth( mViewFrustum.bUseReverse ? 0 : 1 );
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				GPU_PROFILE("Scene");
				RHISetupFixedPipelineState(mView.worldToView, mView.viewToClip);
				DrawUtility::AixsLine();

				RHISetupFixedPipelineState(Matrix4::Scale(1.5) * Matrix4::Translate(2,2,2) * mView.worldToView, mView.viewToClip);
				mSimpleMeshs[SimpleMeshId::Doughnut].draw(LinearColor(1, 0.5, 0));

				{
					GPU_PROFILE("LightPoints");
					//FIXME
					RHISetupFixedPipelineState(mView.worldToView, mView.viewToClip);
					drawLightPoints(mView, MakeView(mLights));
				}
			}

			{
				mFrameBuffer.removeDepthBuffer();
				GL_BIND_LOCK_OBJECT(mFrameBuffer);
				GPU_PROFILE("Cloud");
				RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetBlendState(TStaticBlendState<CWM_RGB, Blend::eOne, Blend::eSrcAlpha >::GetRHI());
				RHISetDepthStencilState(StaticDepthDisableState::GetRHI());

				GL_BIND_LOCK_OBJECT(*mProgCloudRender);
				mProgCloudRender->setParameters(mView , mData, Vector3(0, 0, 0), Vector3(10, 10, 10), mDensityFactor , mPhaseG );
				mProgCloudRender->setStructuredBufferT< TiledLightInfo >(*mLightsBuffer.getRHI());
				mProgCloudRender->setParam(SHADER_PARAM(TiledLightNum), (int)mLights.size());
				mProgCloudRender->setTexture(SHADER_PARAM(DepthBuffer), *mDepthBuffer);
				DrawUtility::ScreenRectShader();
			}

			{
				GPU_PROFILE("CopyToScreen");
				ShaderHelper::Get().copyTextureToBuffer(*mScreenBuffer);
			}


			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetRasterizerState(TStaticRasterizerState<ECullMode::None>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
			auto gameWindow = ::Global::GetDrawEngine().getWindow();


			TArrayView< NoiseShaderProgramBase* const > noiseShaders = { mProgNoise , mProgNoiseUseTexture , mProgNoiseTest };
			int imageSize = Math::Min<int>( 150 , ( gameWindow.getWidth() - 5 * (noiseShaders.size() + 1) ) / noiseShaders.size() );
			for( int i = 0; i < noiseShaders.size(); ++i )
			{
				drawNoiseImage(IntVector2( 5 + (imageSize + 5 ) * i , 5 ), IntVector2(imageSize, imageSize), *noiseShaders[i]);

			}

			if ( 0 )
			{
				RHISetViewport(0, 0, screenSize.x, screenSize.y);
				OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
				MatrixSaveScope matScope(matProj);
				DrawUtility::DrawTexture(*mData.noiseTexture, IntVector2(10, 10), IntVector2(512, 512));
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
			default:
				break;
			}
			return BaseClass::onKey(key,isDown);
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



	REGISTER_STAGE2("Noise Test", NoiseTestStage, EStageGroup::FeatureDev, 1);
}