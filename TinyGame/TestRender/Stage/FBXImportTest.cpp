#include "BRDFTestStage.h"

#include "TestRenderStageBase.h"
#include "ProfileSystem.h"
#include "Renderer/MeshImportor.h"

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

	class PongProgram : public ShaderProgram
	{
	public:


	};

	class FBXImportTestStage : public BRDFTestStage
	{
		using BaseClass = BRDFTestStage;
	public:
		FBXImportTestStage()
		{


		}

		EnvTestProgram mTestShader;
		ShaderProgram  mPongShader;
		ShaderProgram  mClipCoordShader;
		Mesh mMesh;
		TArray<Mesh> mCharMeshes;
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

			auto frame = ::Global::GUI().findTopWidget< DevFrame >();
			FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Use MinpMap"), mbUseMipMap);
			FWidgetProperty::Bind(frame->addSlider(UI_ANY), mSkyLightInstensity , 0 , 10 );
 			return true;
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			BaseClass::setupRenderResource(systemName);

			{
				TIME_SCOPE("EnvLightingTest Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mTestShader, "Shader/Game/EnvLightingTest", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), nullptr));
			}
			{
				TIME_SCOPE("Pong Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mPongShader, "Shader/Game/PongShader", SHADER_ENTRY(MainVS), SHADER_ENTRY(MainPS), nullptr));
			}

			{
				TIME_SCOPE("ClipCoord Shader");
				VERIFY_RETURN_FALSE(ShaderManager::Get().loadFile(mClipCoordShader, "Shader/Game/ClipCoord", SHADER_ENTRY(ScreenVS), SHADER_ENTRY(MainPS), nullptr));
			}
			{
				TIME_SCOPE("Mesh Texture");
				VERIFY_RETURN_FALSE(mDiffuseTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_A.tga", TextureLoadOption().SRGB().AutoMipMap()));
				VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_N.tga", TextureLoadOption().AutoMipMap()));
				VERIFY_RETURN_FALSE(mMetalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_M.tga", TextureLoadOption().AutoMipMap()));
				VERIFY_RETURN_FALSE(mRoughnessTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Mesh/Cerberus/Cerberus_R.tga", TextureLoadOption().AutoMipMap()));

				GTextureShowManager.registerTexture("CD", mDiffuseTexture);
				GTextureShowManager.registerTexture("CN", mNormalTexture);
				GTextureShowManager.registerTexture("CM", mMetalTexture);
				GTextureShowManager.registerTexture("CR", mRoughnessTexture);
			}
			{
				TIME_SCOPE("FBX Mesh");
				auto LoadFBXMesh = [this](Mesh& mesh, char const* filePath)
				{
					return BuildMeshFromFile(mesh, filePath, [](Mesh& mesh, char const* filePath) -> bool
					{
						IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImproter("FBX");
						VERIFY_RETURN_FALSE(importer->importFromFile(filePath, mesh, nullptr));
						return true;
					});
				};

				auto LoadFBXMeshes = [this](TArray<Mesh>& meshes, char const* filePath) -> bool
				{
					return BuildMultiMeshFromFile(meshes, filePath, [](TArray<Mesh>& meshes, char const* filePath) -> bool
					{
						IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImproter("FBX");
						VERIFY_RETURN_FALSE(importer->importMultiFromFile(filePath, meshes, nullptr));
						return true;
					});
				};

				LoadFBXMeshes(mCharMeshes, "Mesh/Tracer/tracer.FBX");
				LoadFBXMesh(mMesh, "Mesh/Cerberus/Cerberus_LP.FBX");
			}

			return true;
		}


		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mTestShader.releaseRHI();

			mDiffuseTexture.release();
			mNormalTexture.release();
			mMetalTexture.release();
			mRoughnessTexture.release();
			mMesh.releaseRHIResource();
			for (auto& mesh : mCharMeshes)
			{
				mesh.releaseRHIResource();
			}

			BaseClass::preShutdownRenderSystem();
		}

		void onEnd() override
		{
			mTestShader.releaseRHI();
			BaseClass::onEnd();
		}

		void onRender(float dFrame) override
		{

			Vec2i screenSize = ::Global::GetScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			initializeRenderState();

			GRenderTargetPool.freeAllUsedElements();
			mSceneRenderTargets.prepare(screenSize);


#if 1
			//RHISetFrameBuffer(commandList, nullptr);
			RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
			RHISetShaderProgram(commandList, mClipCoordShader.getRHI());
			DrawUtility::ScreenRect(commandList);
			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());
			return;
#endif



			{
				GPU_PROFILE("Scene");
				mSceneRenderTargets.attachDepthTexture();
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());	
				RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth | EClearBits::Stencil,
					&LinearColor(0.2, 0.2, 0.2, 1), 1);

				drawSkyBox(commandList, mView, *mHDRImage, mIBLResource, SkyboxShowIndex);

				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
				{
					RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
					DrawUtility::AixsLine(commandList);
				}
				if(0)
				{
					RHISetShaderProgram(commandList, mTestShader.getRHI());
					mView.setupShader(commandList, mTestShader);

					auto& samplerState = (mbUseMipMap) ? TStaticSamplerState<ESampler::Trilinear>::GetRHI() : TStaticSamplerState<ESampler::Bilinear>::GetRHI();
					mTestShader.setTexture(commandList, SHADER_PARAM(DiffuseTexture), mDiffuseTexture , SHADER_SAMPLER(DiffuseTexture), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture, SHADER_SAMPLER(NormalTexture), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(MetalTexture), mMetalTexture, SHADER_SAMPLER(MetalTexture), samplerState);
					mTestShader.setTexture(commandList, SHADER_PARAM(RoughnessTexture), mRoughnessTexture, SHADER_SAMPLER(RoughnessTexture), samplerState);
					mTestShader.setParam(commandList, SHADER_PARAM(SkyLightInstensity), mSkyLightInstensity);
					mTestShader.mParamIBL.setParameters(commandList, mTestShader, mIBLResource);
					mMesh.draw(commandList);
				}

				{
					RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
#if 0
					RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
#else
					RHISetShaderProgram(commandList, mPongShader.getRHI());
					mView.setupShader(commandList, mPongShader);
#endif
					for (auto& mesh : mCharMeshes)
					{
						mesh.draw(commandList);
					}
				}
				if ( 0 )
				{
					GPU_PROFILE("LightProbe Visualize");

					RHISetShaderProgram(commandList, mProgVisualize->getRHI());
					mProgVisualize->setStructuredUniformBufferT< LightProbeVisualizeParams >(commandList, *mParamBuffer.getRHI());
					mProgVisualize->setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture);
					mView.setupShader(commandList, *mProgVisualize);
					mProgVisualize->setParameters(commandList, mIBLResource);
					RHIDrawPrimitiveInstanced(commandList, EPrimitive::Quad, 0, 4, mParams.gridNum.x * mParams.gridNum.y);
				}

				RHISetFrameBuffer(commandList, nullptr);
			}

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			if( bEnableTonemap )
			{
				GPU_PROFILE("Tonemap");

				mSceneRenderTargets.swapFrameTexture();
				PostProcessContext context;
				context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();

				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());

				RHISetShaderProgram(commandList, mProgTonemap->getRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGB >::GetRHI());
				mProgTonemap->setParameters(commandList, context);
				DrawUtility::ScreenRect(commandList);

				RHISetFrameBuffer(commandList, nullptr);
			}


			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());
		}


	};

	REGISTER_STAGE_ENTRY("FBX Import Test", FBXImportTestStage, EExecGroup::FeatureDev, 1, "Render|Test");

}