#include "BRDFTestStage.h"

#include "TestRenderStageBase.h"
#include "ProfileSystem.h"
#include "RHI/MeshImportor.h"

namespace Render
{
	class EnvTestProgram : public ShaderProgram
	{
		using BaseClass = ShaderProgram;
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamIBL.bindParameters(parameterMap);
		}
	public:
		IBLShaderParameters mParamIBL;
	};

	class FBXImportTestStage : public BRDFTestStage
	{
		using BaseClass = BRDFTestStage;
	public:
		FBXImportTestStage()
		{


		}

		EnvTestProgram mTestShader;

		Mesh mMesh;
		RHITexture2DRef mDiffuseTexture;
		RHITexture2DRef mNormalTexture;
		RHITexture2DRef mMetalTexture;
		RHITexture2DRef mRoughnessTexture;
		bool mbUseMipMap = true;

		float mSkyLightInstensity = 1.0;


		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			HighResClock mClock;
			{
				TIME_SCOPE("EnvLightingTest Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mTestShader, "Shader/Game/EnvLightingTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), nullptr));
			}
			{
				TIME_SCOPE("Mesh Texture");
				VERIFY_RETURN_FALSE(mDiffuseTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_A.tga" , TextureLoadOption().SRGB().MipLevel(10).ReverseH()));
				VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_N.tga", TextureLoadOption().MipLevel(10).ReverseH()));
				VERIFY_RETURN_FALSE(mMetalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_M.tga", TextureLoadOption().MipLevel(10).ReverseH()));
				VERIFY_RETURN_FALSE(mRoughnessTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_R.tga", TextureLoadOption().MipLevel(10).ReverseH()));
			}
			{
				TIME_SCOPE("FBX Mesh");
				char const* filePath = "Mesh/Cerberus/Cerberus_LP.FBX";
				BuildMesh(mMesh, filePath, [filePath](Mesh& mesh) ->bool
				{
					IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImprotor("FBX");
					VERIFY_RETURN_FALSE(importer->importFromFile(filePath, mesh, nullptr));
					return true;
				});
			}

			auto frame = ::Global::GUI().findTopWidget< DevFrame >();
			FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Use MinpMap"), mbUseMipMap);
			FWidgetPropery::Bind(frame->addSlider(UI_ANY), mSkyLightInstensity , 0 , 10 );
			FWidgetPropery::Bind(frame->addCheckBox(UI_ANY, "Use Shader Blit"), mbUseShaderBlit);
 			return true;
		}

		void onEnd() override
		{
			mTestShader.releaseRHI();
			BaseClass::onEnd();
		}

		void onRender(float dFrame) override
		{

			Vec2i screenSize = ::Global::GetDrawEngine().getScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			{
				GPU_PROFILE("Scene");
				mSceneRenderTargets.attachDepthTexture();
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());	
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth | EClearBits::Stencil,
					&LinearColor(0.2, 0.2, 0.2, 1), 1, mViewFrustum.bUseReverse ? 0 : 1, 0);

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
						mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.irradianceTexture);
						mProgSkyBox->setParam(commandList, SHADER_PARAM(CubeLevel), float(0));
						break;
					default:
						mProgSkyBox->setTexture(commandList, SHADER_PARAM(CubeTexture), mIBLResource.perfilteredTexture, SHADER_PARAM(CubeTextureSampler),
												TStaticSamplerState< Sampler::eTrilinear, Sampler::eClamp, Sampler::eClamp, Sampler::eClamp > ::GetRHI());
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
					RHISetShaderProgram(commandList, mTestShader.getRHIResource());
					mView.setupShader(commandList, mTestShader);

					auto& samplerState = (mbUseMipMap) ? TStaticSamplerState<Sampler::eTrilinear>::GetRHI() : TStaticSamplerState<Sampler::eBilinear>::GetRHI();
					mTestShader.setTexture(commandList, SHADER_PARAM(DiffuseTexture), mDiffuseTexture , SHADER_PARAM(DiffuseTextureSampler), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture, SHADER_PARAM(NormalTextureSampler), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(MetalTexture), mMetalTexture, SHADER_PARAM(MetalTextureSampler), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(RoughnessTexture), mRoughnessTexture, SHADER_PARAM(RoughnessTextureSampler), samplerState);
					mTestShader.setParam(commandList, SHADER_PARAM(SkyLightInstensity), mSkyLightInstensity);
					mTestShader.mParamIBL.setParameters(commandList, mTestShader, mIBLResource);
					mMesh.draw(commandList);
				}
				if ( 0 )
				{
					GPU_PROFILE("LightProbe Visualize");

					RHISetShaderProgram(commandList, mProgVisualize->getRHIResource());
					mProgVisualize->setStructuredUniformBufferT< LightProbeVisualizeParams >(commandList, *mParamBuffer.getRHI());
					mProgVisualize->setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture);
					mView.setupShader(commandList, *mProgVisualize);
					mProgVisualize->setParameters(commandList, mIBLResource);
					RHIDrawPrimitiveInstanced(commandList, EPrimitive::Quad, 0, 4, mParams.gridNum.x * mParams.gridNum.y);
				}

				RHISetFrameBuffer(commandList, nullptr);
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

				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());

				RHISetShaderProgram(commandList, mProgTonemap->getRHIResource());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGB >::GetRHI());
				mProgTonemap->setParameters(commandList, context);
				DrawUtility::ScreenRect(commandList);

				RHISetFrameBuffer(commandList, nullptr);
			}

			{
				GPU_PROFILE("Blit To Screen");
				if( mbUseShaderBlit )
				{
					ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());
				}
				else
				{
					OpenGLCast::To( mSceneRenderTargets.getFrameBuffer() )->blitToBackBuffer();
				}
			}
		}

	};

	REGISTER_STAGE2("FBX Import Test", FBXImportTestStage, EStageGroup::FeatureDev, 1);

}