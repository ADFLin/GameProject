#include "TestRenderStageBase.h"
#include "Renderer/MeshBuild.h"
#include "RHI/RHICommand.h"
#include "Editor.h"

#include "Image/ImageProcessing.h"
#include "Image/ImageData.h"
#include "BitUtility.h"



namespace Render
{
	class FFTShader : public GlobalShader
	{
	public:
		using BaseClass = GlobalShader;
		SHADER_PERMUTATION_TYPE_BOOL(IsInverse, SHADER_PARAM(FFT_INVERSE));
		SHADER_PERMUTATION_TYPE_BOOL(IsOnePass, SHADER_PARAM(FFT_ONE_PASS));
		SHADER_PERMUTATION_TYPE_INT_SELECTIONS(Size, SHADER_PARAM(FFT_SIZE), 1024, 512, 256);
		using PermutationDomain = TShaderPermutationDomain< IsInverse , IsOnePass , Size >;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(FFT_LOG_SIZE), FBitUtility::CountTrailingZeros(domain.get<Size>()));
		}
		static char const* GetShaderFileName()
		{
			return "Shader/FFT";
		}
	};
	class FFTHCS : public FFTShader
	{
		using BaseClass = FFTShader;
		DECLARE_SHADER(FFTShader, Global);
	};
	class FFTVCS : public FFTShader
	{
		using BaseClass = FFTShader;
		DECLARE_SHADER(FFTShader, Global);
	};
	IMPLEMENT_SHADER(FFTHCS, EShader::Compute, SHADER_ENTRY(MainFFTH));
	IMPLEMENT_SHADER(FFTVCS, EShader::Compute, SHADER_ENTRY(MainFFTV));

	class FillValueCS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(FillValueCS, Global);

	public:
		static constexpr int GroupSize = 8;
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(GROUP_SIZE), GroupSize);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/FFT";
		}
	};	
	
	class ScaleValueCS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(ScaleValueCS, Global);

	public:
		static constexpr int GroupSize = 8;
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(GROUP_SIZE), GroupSize);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/FFT";
		}
	};

	class NormailizeCS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(NormailizeCS, Global);

	public:
		static constexpr int GroupSize = 8;
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(GROUP_SIZE), GroupSize);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/FFT";
		}
	};
	IMPLEMENT_SHADER(FillValueCS, EShader::Compute, SHADER_ENTRY(FillValueCS));
	IMPLEMENT_SHADER(ScaleValueCS, EShader::Compute, SHADER_ENTRY(ScaleValueCS));
	IMPLEMENT_SHADER(NormailizeCS, EShader::Compute, SHADER_ENTRY(NormailizeCS));

	class WaveRenderProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(WaveRenderProgram, Global);
	public:
		static constexpr int GroupSize = 8;
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(GROUP_SIZE), GroupSize);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaveRender";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};

			return entries;
		}

		//DEFINE_SHADER_PARAM()
	};

	class WaveGenerateCS : public GlobalShader
	{
		using BaseClass = GlobalShader;
		DECLARE_SHADER(WaveGenerateCS, Global);
	public:
		static constexpr int GroupSize = 8;
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(GROUP_SIZE), GroupSize);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Game/WaveRender";
		}
	};

	IMPLEMENT_SHADER_PROGRAM(WaveRenderProgram);
	IMPLEMENT_SHADER(WaveGenerateCS, EShader::Compute, SHADER_ENTRY(GenWaveCS));

	struct WaveParameters
	{
		DECLARE_BUFFER_STRUCT(WaveParams);

		Vector2 direction = Vector2(1,0);
		float wavelength = 10.0f;
		float steepness = 0.5;

		REFLECT_STRUCT_BEGIN(WaveParameters)
			REF_PROPERTY(direction)
			REF_PROPERTY(wavelength)
			REF_PROPERTY(steepness, Slider(0, 1))
		REFLECT_STRUCT_END()
	};

	class WaveRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		WaveRenderingStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			mWaveDataList.resize(3);

			for (int i = 1; i < 3; ++i)
			{
				auto& prevWave = mWaveDataList[i - 1];
				auto& wave = mWaveDataList[i];
				wave.steepness = 0.81 * prevWave.steepness;
				wave.wavelength = 0.81 * prevWave.wavelength;
			}

			if (::Global::Editor())
			{
				DetailViewConfig config;
				config.name = "Wave Params";
				mDetailView = ::Global::Editor()->createDetailView(config);
				mDetailView->addStruct(mWaveParams, "Wave");
				mDetailView->addValue(mTests, "test");
				mDetailView->addValue(bOnepassFFT, "FFT OnePass");

				{
					auto handle = mDetailView->addValue(mWaveDataList, "WaveDataList");
					mDetailView->addCallback(handle, [this](char const* name)
					{
						updateWaveParams();
					});
				}
			}

			return true;
		}

		TArray<int> mTests = { 1 , 2, 3 };
		TArray<WaveParameters> mWaveDataList;

		float mTileLength = 10;
		RHITexture2DRef mTextureDisp;
		RHITexture2DRef mTextureDispDiff;
		IEditorDetailView* mDetailView = nullptr;

		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{

		}


		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(FMeshBuild::Tile(mPlane, 100, 1, false));
			VERIFY_RETURN_FALSE(mWaveRenderProgram = ShaderManager::Get().getGlobalShaderT< WaveRenderProgram >());
			VERIFY_RETURN_FALSE(mWaveGenerateCS = ShaderManager::Get().getGlobalShaderT< WaveGenerateCS >());

			int const textureSize = 1024;
			mTextureDisp = RHICreateTexture2D(TextureDesc::Type2D(ETexture::FloatRGBA, textureSize, textureSize).Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue));

			mTextureDispDiff = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, textureSize, textureSize).Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue));



			Color4ub colors[16];
			for (int i = 0; i < ARRAY_SIZE(colors); ++i)
			{
				colors[i] = Color4ub(255, 255, 255, 0);
			}

			auto ToIndex = [](int x, int y) { return 4 * y + x; };
			colors[ToIndex(1, 1)] = Color4ub(255, 0, 0, 255);
			colors[ToIndex(2, 1)] = Color4ub(0, 255, 0, 255);
			colors[ToIndex(1, 2)] = Color4ub(255, 255, 0, 255);
			colors[ToIndex(2, 2)] = Color4ub(0, 0, 255, 255);

			for (int i = 0; i < ARRAY_SIZE(colors); ++i)
			{
				colors[i].r = Math::FloorToInt(float(colors[i].r) * float(colors[i].a / 255.0));
				colors[i].g = Math::FloorToInt(float(colors[i].g) * float(colors[i].a / 255.0));
				colors[i].b = Math::FloorToInt(float(colors[i].b) * float(colors[i].a / 255.0));
			}
			mTextureTest = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, 4, 4), colors);

			VERIFY_RETURN_FALSE(mFFTTestTex = RHIUtility::LoadTexture2DFromFile("texture/ref2.ppm"));


			for (int i = 0; i < 2; ++i)
			{
				mFFTTextures[i] = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RG32F, mFFTTestTex->getSizeX(), mFFTTestTex->getSizeY()).Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue));
			}

			mFFTResultTex = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RG32F, mFFTTestTex->getSizeX(), mFFTTestTex->getSizeY()).Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue));
			mIFFTResultTex = RHICreateTexture2D(TextureDesc::Type2D(ETexture::R32F, mFFTTestTex->getSizeX(), mFFTTestTex->getSizeY()).Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue));
			VERIFY_RETURN_FALSE(mFillValueCS = ShaderManager::Get().getGlobalShaderT<FillValueCS>());
			VERIFY_RETURN_FALSE(mNormalizeCS = ShaderManager::Get().getGlobalShaderT<NormailizeCS>());
			VERIFY_RETURN_FALSE(mScaleValueCS = ShaderManager::Get().getGlobalShaderT<ScaleValueCS>());


			GTextureShowManager.registerTexture("FFTTest", mFFTTestTex);
			GTextureShowManager.registerTexture("FFTTexture0", mFFTTextures[0]);
			GTextureShowManager.registerTexture("FFTTexture1", mFFTTextures[1]);
			GTextureShowManager.registerTexture("FFT Result", mFFTResultTex);
			GTextureShowManager.registerTexture("IFFT Result", mIFFTResultTex);
			GTextureShowManager.registerTexture("TexDisp", mTextureDisp);
			GTextureShowManager.registerTexture("TexDispDiff", mTextureDispDiff);
			GTextureShowManager.registerTexture("Test", mTextureTest);

			updateWaveParams();
			return true;
		}
		RHITexture2DRef mFFTTestTex;
		RHITexture2DRef mFFTTextures[2];
		RHITexture2DRef mFFTResultTex;
		RHITexture2DRef mIFFTResultTex;
		RHITexture2DRef mTextureTest;
		FillValueCS* mFillValueCS = nullptr;
		ScaleValueCS* mScaleValueCS = nullptr;
		FFTHCS* mFFTHCS = nullptr;
		FFTVCS* mFFTVCS = nullptr;
		NormailizeCS* mNormalizeCS = nullptr;

		bool bOnepassFFT = true;

		void doFFT2D(RHICommandList& commandList, bool bInverse, int& inoutReadIndex)
		{
			IntVector2 textureSize;
			textureSize.x = mFFTTextures[0]->getSizeX();
			textureSize.y = mFFTTextures[0]->getSizeY();

			FFTShader::PermutationDomain permutationVector;
			permutationVector.set< FFTShader::IsInverse >(bInverse);
			permutationVector.set< FFTShader::IsOnePass >(bOnepassFFT);

			permutationVector.set< FFTShader::Size >(textureSize.x);
			mFFTHCS = ShaderManager::Get().getGlobalShaderT<FFTHCS>(permutationVector);

			int numStep = FBitUtility::CountTrailingZeros(textureSize.x);

			RHISetComputeShader(commandList, mFFTHCS->getRHI());
			if (bOnepassFFT)
			{
				mFFTHCS->setRWTexture(commandList, SHADER_PARAM(TexOut), *mFFTTextures[inoutReadIndex], AO_READ_AND_WRITE);
				RHIDispatchCompute(commandList, 1, textureSize.y, 1);
				mFFTHCS->clearRWTexture(commandList, SHADER_PARAM(TexOut));
			}
			else
			{
				for (int step = 0; step < numStep; ++step)
				{
					int stride = textureSize.x >> (step + 1);
					mFFTHCS->setTexture(commandList, SHADER_PARAM(TexIn), mFFTTextures[inoutReadIndex]);
					mFFTHCS->setRWTexture(commandList, SHADER_PARAM(TexOut), *mFFTTextures[1 - inoutReadIndex], AO_WRITE_ONLY);
					mFFTHCS->setParam(commandList, SHADER_PARAM(FFTStride), stride);
					RHIDispatchCompute(commandList, 1, textureSize.y, 1);

					mFFTHCS->clearTexture(commandList, SHADER_PARAM(TexIn));
					mFFTHCS->clearRWTexture(commandList, SHADER_PARAM(TexOut));
					inoutReadIndex = 1 - inoutReadIndex;
				}
			}

			permutationVector.set< FFTShader::Size >(textureSize.y);
			mFFTVCS = ShaderManager::Get().getGlobalShaderT<FFTVCS>(permutationVector);
			RHISetComputeShader(commandList, mFFTVCS->getRHI());
			if (bOnepassFFT)
			{
				mFFTVCS->setRWTexture(commandList, SHADER_PARAM(TexOut), *mFFTTextures[inoutReadIndex], AO_READ_AND_WRITE);
				RHIDispatchCompute(commandList, 1, textureSize.x, 1);
				mFFTVCS->clearRWTexture(commandList, SHADER_PARAM(TexOut));
			}
			else
			{
				for (int step = 0; step < numStep; ++step)
				{
					int stride = textureSize.y >> (step + 1);
					mFFTVCS->setTexture(commandList, SHADER_PARAM(TexIn), mFFTTextures[inoutReadIndex]);
					mFFTVCS->setRWTexture(commandList, SHADER_PARAM(TexOut), *mFFTTextures[1 - inoutReadIndex], AO_WRITE_ONLY);
					mFFTVCS->setParam(commandList, SHADER_PARAM(FFTStride), stride);
					RHIDispatchCompute(commandList, 1, textureSize.x, 1);

					mFFTVCS->clearTexture(commandList, SHADER_PARAM(TexIn));
					mFFTVCS->clearRWTexture(commandList, SHADER_PARAM(TexOut));
					inoutReadIndex = 1 - inoutReadIndex;
				}
			}

			if (!bOnepassFFT)
			{
				RHISetComputeShader(commandList, mScaleValueCS->getRHI());
				mScaleValueCS->setRWTexture(commandList, SHADER_PARAM(TexDest), *mFFTTextures[inoutReadIndex], AO_READ_AND_WRITE);
				mScaleValueCS->setParam(commandList, SHADER_PARAM(Scale), Vector2(1.0 / textureSize.x, 1.0 / textureSize.y));
				RHIDispatchCompute(commandList, textureSize.x / ScaleValueCS::GroupSize, textureSize.y / ScaleValueCS::GroupSize, 1);
				mScaleValueCS->clearRWTexture(commandList, SHADER_PARAM(TexDest));
			}
		}

		void TestFFT(RHICommandList& commandList)
		{
			GPU_PROFILE("TestFFT");
			RHIClearSRVResource(commandList, mFFTTextures[0]);
			RHIClearSRVResource(commandList, mFFTTextures[1]);

			RHISetComputeShader(commandList, mFillValueCS->getRHI());
			mFillValueCS->setTexture(commandList, SHADER_PARAM(TexSource), mFFTTestTex);
			mFillValueCS->setRWTexture(commandList, SHADER_PARAM(TexDest), *mFFTTextures[0] , AO_WRITE_ONLY);
			RHIDispatchCompute(commandList, mFFTTestTex->getSizeX() / FillValueCS::GroupSize, mFFTTestTex->getSizeY() / FillValueCS::GroupSize, 1);
			mFillValueCS->clearRWTexture(commandList, SHADER_PARAM(TexDest));

			int readIndex = 0;
			{
				GPU_PROFILE("FFT");
				doFFT2D(commandList, false, readIndex);
			}

			RHIClearSRVResource(commandList, mFFTResultTex);
			RHISetComputeShader(commandList, mNormalizeCS->getRHI());
			mNormalizeCS->setTexture(commandList, SHADER_PARAM(TexIn), *mFFTTextures[readIndex]);
			mNormalizeCS->setRWTexture(commandList, SHADER_PARAM(TexResult), *mFFTResultTex, AO_WRITE_ONLY);
			mNormalizeCS->setParam(commandList, SHADER_PARAM(Size), mFFTTestTex->getSizeX());
			RHIDispatchCompute(commandList, mFFTTestTex->getSizeX() / NormailizeCS::GroupSize, mFFTTestTex->getSizeY() / NormailizeCS::GroupSize, 1);
			mNormalizeCS->clearTexture(commandList, SHADER_PARAM(TexIn));
			mNormalizeCS->clearRWTexture(commandList, SHADER_PARAM(TexResult));

			{
				GPU_PROFILE("IFFT");
				doFFT2D(commandList, true , readIndex);
			}

			ShaderHelper::Get().copyTexture(commandList, *mIFFTResultTex, *mFFTTextures[readIndex], true);
		}

		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			BaseClass::preShutdownRenderSystem(bReInit);
			mPlane.releaseRHIResource();
			mWaveRenderProgram = nullptr;
			mWaveGenerateCS = nullptr;
			mTextureDisp.release();
		}

		void generateWave(RHICommandList& commandList)
		{
			GPU_PROFILE("GenerateWave");
			CHECK(mTextureDisp->getSizeX() == mTextureDisp->getSizeY() && (mTextureDisp->getSizeX() % WaveGenerateCS::GroupSize == 0));

			RHIClearSRVResource(commandList, mTextureDisp);
			RHIClearSRVResource(commandList, mTextureDispDiff);
			RHISetComputeShader(commandList, mWaveGenerateCS->getRHI());
			mWaveGenerateCS->setRWTexture(commandList, SHADER_PARAM(TexDisp), *mTextureDisp, EAccessOperator::AO_WRITE_ONLY);
			mWaveGenerateCS->setRWTexture(commandList, SHADER_PARAM(TexDispDiff), *mTextureDispDiff, EAccessOperator::AO_WRITE_ONLY);
			mView.setupShader(commandList, *mWaveGenerateCS);

			float lengthScale = mTileLength / mTextureDisp->getSizeX();
			mWaveGenerateCS->setParam(commandList, SHADER_PARAM(LengthScale), lengthScale);
			mWaveGenerateCS->setParam(commandList, SHADER_PARAM(NumWaveParams), (int)mWaveDataList.size());
			SetStructuredStorageBuffer(commandList, *mWaveGenerateCS, mWaveParamsBuffer);

			RHIDispatchCompute(commandList, mTextureDisp->getSizeX() / WaveGenerateCS::GroupSize, mTextureDisp->getSizeX() / WaveGenerateCS::GroupSize, 1);
			mWaveGenerateCS->clearRWTexture(commandList, SHADER_PARAM(TexDisp));
			mWaveGenerateCS->clearRWTexture(commandList, SHADER_PARAM(TexDispDiff));
		}

		void updateWaveParams()
		{
			float totalSteepness = 0;
			float maxSteepness = 1.0f;
			for (int i = 0; i < 3; ++i)
			{
				auto& wave = mWaveDataList[i];
				//wave.direction.normalize();
				totalSteepness += wave.steepness;
			}
			float steepnessScale = Math::Min(totalSteepness, maxSteepness) / totalSteepness;
			for (int i = 0; i < 3; ++i)
			{
				auto& wave = mWaveDataList[i];
				wave.steepness *= steepnessScale;
			}

			if (mWaveDataList.size() > mWaveParamsBuffer.getElementNum())
			{
				mWaveParamsBuffer.releaseResource();
				mWaveParamsBuffer.initializeResource(mWaveDataList.size(), EStructuredBufferType::Buffer);
			}
			mWaveParamsBuffer.updateBuffer(mWaveDataList);
		}

		Mesh mPlane;
		WaveRenderProgram* mWaveRenderProgram;
		WaveGenerateCS* mWaveGenerateCS;
		TStructuredBuffer<WaveParameters>  mWaveParamsBuffer;
		WaveParameters mWaveParams;

		void onEnd() override
		{
			if (mDetailView)
			{
				mDetailView->release();
			}
			BaseClass::onEnd();
		}

		void restart() override
		{
			BaseClass::restart();

		}

		void tick() override
		{
			BaseClass::tick();
		}

		void updateFrame(int frame) override
		{

			
		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);
		}

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			initializeRenderState();

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			generateWave(commandList);

			TestFFT(commandList);


			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(mView.worldToClip));
			DrawUtility::AixsLine(commandList, 10);

#if 0
			RHISetRasterizerState(commandList, GetStaticRasterizerState(ECullMode::Back, EFillMode::Wireframe));
#endif

			RHISetShaderProgram(commandList, mWaveRenderProgram->getRHI());
			mView.setupShader(commandList, *mWaveRenderProgram);
			mWaveRenderProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), Matrix4::Scale(mTileLength, mTileLength, 1));
			mWaveRenderProgram->setTexture(commandList, SHADER_PARAM(TexDisp), mTextureDisp, SHADER_SAMPLER(TexDisp), TStaticSamplerState<ESampler::Bilinear>::GetRHI());
			mWaveRenderProgram->setTexture(commandList, SHADER_PARAM(TexDispDiff), mTextureDispDiff, SHADER_SAMPLER(TexDispDiff), TStaticSamplerState<ESampler::Bilinear>::GetRHI());

			mPlane.draw(commandList);


#if 0
			RHISetShaderProgram(commandList, nullptr);
			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA,
				EBlend::One, EBlend::OneMinusSrcAlpha, EBlend::Add,
				EBlend::One, EBlend::OneMinusSrcAlpha, EBlend::Add >::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1));
			ShaderHelper::Get().copyTextureToBuffer(commandList, *mTextureTest);
#endif

		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				default:
					break;
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
	protected:
	};

	REGISTER_STAGE_ENTRY("Wave Rendering", WaveRenderingStage, EExecGroup::FeatureDev);
}