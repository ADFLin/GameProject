#include "TestRenderStageBase.h"
#include "RHI/RHIGraphics2D.h"
#include "Renderer/IBLResource.h"
#include "ProfileSystem.h"

namespace Render
{

	struct ParticleData
	{
		DECLARE_BUFFER_STRUCT(Particles);

		Vector3 pos;
	};


	class FRBasePassProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;

		DECLARE_SHADER_PROGRAM(FRBasePassProgram, Global);

		SHADER_PERMUTATION_TYPE_BOOL(DrawThinkness, SHADER_PARAM(DRAW_THICKNESS));
		using PermutationDomain = TShaderPermutationDomain<DrawThinkness>;

		static char const* GetShaderFileName()
		{
			return "Shader/FluidRendering";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(BasePassVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BasePassPS) },
			};
			return entries;
		}
	};

	class FRFilterProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;

		DECLARE_SHADER_PROGRAM(FRFilterProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/FluidRendering";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(FilterPS) },
			};
			return entries;
		}
	};

	class FRRenderProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(FRRenderProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/FluidRendering";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(RenderPS) },
			};
			return entries;
		}
	};


	IMPLEMENT_SHADER_PROGRAM(FRBasePassProgram);
	IMPLEMENT_SHADER_PROGRAM(FRFilterProgram);
	IMPLEMENT_SHADER_PROGRAM(FRRenderProgram);


	class FluidRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		FluidRenderingStage()
		{


		}

		bool onInit()
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frame->addSlider("Render Radius"), mRenderRadius, 0, 1);
			FWidgetProperty::Bind(frame->addSlider("Filter Radius"), mFilterRadius, 1, 20);

			FWidgetProperty::Bind(frame->addSlider("Blur Depth Falloff"), mBlurDepthFalloff, 0, 2);
			FWidgetProperty::Bind(frame->addSlider("Blur Scale"), mBlurScale, 0, 0.5);
			return true;
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
		{
			BaseClass::configRenderSystem(systenName, systemConfigs);
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}

		bool setupRenderResource(ERenderSystem systemName)
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());
			VERIFY_RETURN_FALSE(generateTestData());

			Vector2 vertices[] = 
			{
				Vector2(-1,-1),
				Vector2(-1, 1),
				Vector2( 1,-1),
				Vector2( 1, 1),
			};

			mQuadVertexBuffer = RHICreateVertexBuffer(sizeof(Vector2) , 4 , BCF_DefalutValue, vertices);
			uint16 indices[] =
			{
				0,1,2,2,1,3
			};
			mQuadIndexBuffer = RHICreateIndexBuffer(ARRAY_SIZE(indices), false , BCF_DefalutValue, indices );

			mProgDepthPass = ShaderManager::Get().getGlobalShaderT<FRBasePassProgram>();
			
			FRBasePassProgram::PermutationDomain pdVector;
			pdVector.set<FRBasePassProgram::DrawThinkness>(true);
			mProgThinknessPass = ShaderManager::Get().getGlobalShaderT<FRBasePassProgram>(pdVector);

			
			mProgFilter = ShaderManager::Get().getGlobalShaderT<FRFilterProgram>();
			mProgRender = ShaderManager::Get().getGlobalShaderT<FRRenderProgram>();

			mFrameBuffer = RHICreateFrameBuffer();

#if 0
			char const* HDRImagePath = "Texture/HDR/A.hdr";
			{
				TIME_SCOPE("HDR Texture");
				VERIFY_RETURN_FALSE(mHDRImage = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Texture/HDR/A.hdr", TextureLoadOption().HDR()));
			}
			{
				TIME_SCOPE("IBL");
				VERIFY_RETURN_FALSE(mBuilder.loadOrBuildResource(::Global::DataCache(), HDRImagePath, *mHDRImage, mIBLResource));
			}
#endif


			return true;
		}
		IBLResourceBuilder mBuilder;
		IBLResource mIBLResource;
		RHITexture2DRef mHDRImage;

		RHIBufferRef mQuadVertexBuffer;
		RHIBufferRef mQuadIndexBuffer;

		RHIFrameBufferRef mFrameBuffer;


		FRBasePassProgram* mProgDepthPass;
		FRBasePassProgram* mProgThinknessPass;
		FRFilterProgram*   mProgFilter;
		FRRenderProgram*   mProgRender;

		bool generateTestData()
		{
			int numSample = 200;
			float length = 10.0f / numSample;

			TArray< ParticleData > datas;
			for (int i = 0; i < numSample; ++i)
			{
				for (int j = 0; j < numSample; ++j)
				{
					float x = i * length;
					float y = j * length;
					float h = 1 + 0.5 * sin( x + y ) * sin( y );

					int nh = Math::FloorToInt( h / length );
					for (int k = 0; k < nh; ++k)
					{
						ParticleData data;
						data.pos = Vector3(x, y, length * k);
						datas.push_back(data);
					}
				}
			}
			VERIFY_RETURN_FALSE(mParticleBuffer.initializeResource(datas.size(), EStructuredBufferType::Buffer));
			VERIFY_RETURN_FALSE(mParticleBuffer.updateBuffer(datas));
			return true;
		}

		float mRenderRadius = 0.2;
		int   mFilterRadius = 10;
		float mBlurDepthFalloff = 1.0f;
		float mBlurScale = 0.1f;


		void onRender(float dFrame)
		{
			GRenderTargetPool.freeAllUsedElements();

			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			initializeRenderState();

			PooledRenderTargetRef RTDepth;
			PooledRenderTargetRef RTDepthBuffer;


			auto DrawParticles  = [&]()
			{
				InputStreamInfo stream;
				stream.buffer = mQuadVertexBuffer;
				stream.offset = 0;
				RHISetInputStream(commandList, &TRenderRT<RTVF_XY>::GetInputLayout(), &stream, 1);
#if 0
				RHISetIndexBuffer(commandList, mQuadIndexBuffer);
				RHIDrawIndexedPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 6, mParticleBuffer.getElementNum());
#else
				RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleStrip, 0, 4, mParticleBuffer.getElementNum());
#endif
			};

			{
				GPU_PROFILE("Fluid Depth Pass");

				RenderTargetDesc descDepth;
				descDepth.format = ETexture::R32F;
				descDepth.size = screenSize;
				descDepth.debugName = "Depth";
				RTDepth = GRenderTargetPool.fetchElement(descDepth);
				mFrameBuffer->setTexture(0, *RTDepth->texture);
				RenderTargetDesc descDepthBuffer;
				descDepthBuffer.format = ETexture::Depth32F;
				descDepthBuffer.size = screenSize;
				descDepthBuffer.debugName = "DepthBuffer";
				RTDepthBuffer = GRenderTargetPool.fetchElement(descDepthBuffer);
				mFrameBuffer->setDepth(*RTDepthBuffer->texture);


				RHISetFrameBuffer(commandList, mFrameBuffer);

				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				RHIClearRenderTargets(commandList, EClearBits::Color |EClearBits::Depth , &LinearColor(0,0,0,0) , 1 );

				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

				RHISetShaderProgram(commandList, mProgDepthPass->getRHI());
				SetStructuredStorageBuffer(commandList, *mProgDepthPass, mParticleBuffer);
				mView.setupShader(commandList, *mProgDepthPass);
				mProgDepthPass->setParam(commandList, SHADER_PARAM(RenderRadius), mRenderRadius);

				DrawParticles();
			}

			PooledRenderTargetRef RTThickness;
			{
				GPU_PROFILE("Fluid Thinkness Pass");

				RenderTargetDesc descThickness;
				descThickness.format = ETexture::R32F;
				descThickness.size = screenSize / 2;
				descThickness.debugName = "Thickness";
				RTThickness = GRenderTargetPool.fetchElement(descThickness);
				mFrameBuffer->setTexture(0, *RTThickness->texture);
				mFrameBuffer->removeDepth();

				RHISetFrameBuffer(commandList, mFrameBuffer);
				RHISetViewport(commandList, 0, 0, screenSize.x / 2 , screenSize.y / 2);
				RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 0), 1);

				RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One , EBlend::One >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

				RHISetShaderProgram(commandList, mProgThinknessPass->getRHI());
				SetStructuredStorageBuffer(commandList, *mProgThinknessPass, mParticleBuffer);
				mView.setupShader(commandList, *mProgThinknessPass);
				mProgThinknessPass->setParam(commandList, SHADER_PARAM(RenderRadius), mRenderRadius);

				DrawParticles();
			}

			PooledRenderTargetRef RTFilteredDepthY;
			PooledRenderTargetRef RTFilteredDepth;
			{
				GPU_PROFILE("Fluid Depth Filter");
				{
					GPU_PROFILE("Fluid Depth Filter - Y");

					RenderTargetDesc descFilteredDepthY;
					descFilteredDepthY.format = ETexture::R32F;
					descFilteredDepthY.size = screenSize;
					descFilteredDepthY.debugName = "FilteredDepth-X";
					RTFilteredDepthY = GRenderTargetPool.fetchElement(descFilteredDepthY);
					mFrameBuffer->setTexture(0, *RTFilteredDepthY->texture);
					mFrameBuffer->removeDepth();

					RHISetFrameBuffer(commandList, mFrameBuffer);
					RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
					RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 0), 1);

					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

					RHISetShaderProgram(commandList, mProgFilter->getRHI());
					mProgFilter->setTexture(commandList, SHADER_PARAM(DepthTexture), RTDepth->texture, SHADER_PARAM(DepthTextureSampler),
						TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());

					mProgFilter->setParam(commandList, SHADER_PARAM(FilterRadius), float(mFilterRadius));
					mProgFilter->setParam(commandList, SHADER_PARAM(BlurParams), Vector4(0, 1.0 / screenSize.y, mBlurScale, mBlurDepthFalloff));
					DrawUtility::ScreenRect(commandList);
				}

				{
					GPU_PROFILE("Fluid Depth Filter - X");

					RenderTargetDesc descFilteredDepth;
					descFilteredDepth.format = ETexture::R32F;
					descFilteredDepth.size = screenSize;
					descFilteredDepth.debugName = "FilteredDepth";
					RTFilteredDepth = GRenderTargetPool.fetchElement(descFilteredDepth);
					mFrameBuffer->setTexture(0, *RTFilteredDepth->texture);

					RHISetFrameBuffer(commandList, mFrameBuffer);
					RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 0), 1);

					RHISetShaderProgram(commandList, mProgFilter->getRHI());
					mProgFilter->setTexture(commandList, SHADER_PARAM(DepthTexture), RTFilteredDepthY->texture, SHADER_PARAM(DepthTextureSampler),
						TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());

					mProgFilter->setParam(commandList, SHADER_PARAM(FilterRadius), float(mFilterRadius));
					mProgFilter->setParam(commandList, SHADER_PARAM(BlurParams), Vector4(1.0 / screenSize.x, 0, mBlurScale, mBlurDepthFalloff));
					DrawUtility::ScreenRect(commandList);
				}
			}



			RHISetFrameBuffer(commandList, nullptr);
			{

				//drawSkyBox(commandList, mView, *mHDRImage, mIBLResource, ESkyboxShow::Normal);
			}

			{
				GPU_PROFILE("Fluid Render");

				RHISetBlendState(commandList, StaticTranslucentBlendState::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<false>::GetRHI());

				RHISetShaderProgram(commandList, mProgRender->getRHI());
				mView.setupShader(commandList, *mProgRender);
				mProgRender->setTexture(commandList, SHADER_PARAM(DepthTexture), RTFilteredDepth->texture, SHADER_PARAM(DepthTextureSampler),
					TStaticSamplerState<>::GetRHI());
				mProgRender->setTexture(commandList, SHADER_PARAM(ThicknessTexture), RTThickness->texture, SHADER_PARAM(ThicknessTextureSampler),
					TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp>::GetRHI());

				DrawUtility::ScreenRect(commandList);
			}
			
			RHISetFixedShaderPipelineState(commandList, AdjProjectionMatrixForRHI(mView.worldToClip));
			DrawUtility::AixsLine(commandList, 10);



			RHIFlushCommand(commandList);

			GTextureShowManager.registerRenderTarget(GRenderTargetPool);


			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.drawText( Vector2(20,20) , InlineString<>::Make("Particle Count = %d", mParticleBuffer.getElementNum()));
			g.endRender();
		}


		TStructuredBuffer<ParticleData> mParticleBuffer;
	};


	REGISTER_STAGE_ENTRY("Fluid Rendering", FluidRenderingStage, EExecGroup::FeatureDev, 1, "Render|Test");



}