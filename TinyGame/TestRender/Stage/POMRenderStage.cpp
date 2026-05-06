#include "Stage/TestRenderStageBase.h"

#include "RHI/Material.h"
#include "Renderer/BasePassRendering.h"
#include "Renderer/SceneLighting.h"
#include "Renderer/MeshBuild.h"
#include "Renderer/RenderTargetPool.h"

namespace Render
{
	class POMRenderStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			mCamera.lookAt(Vector3(1.8f, -4.5f, 2.0f), Vector3(0, 0, 0), Vector3(0, 0, 1));
			::Global::GUI().cleanupWidget();
			auto frame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frame->addSlider(UI_ANY, "Parallax Scale"), mParallaxScale, 0.0f, 0.12f);
			FWidgetProperty::Bind(frame->addSlider(UI_ANY, "Parallax Direction"), mParallaxDirection, -1.0f, 1.0f);
			FWidgetProperty::Bind(frame->addSlider(UI_ANY, "Reference Depth"), mReferenceDepth, 0.0f, 1.0f);

			return true;
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
			VERIFY_RETURN_FALSE(mMaterial.loadFile("POMTitle"));

			VERIFY_RETURN_FALSE(mBaseTexture = RHIUtility::LoadTexture2DFromFile(
				::Global::DataCache(), "Texture/rocks.png", TextureLoadOption().SRGB().FlipV()));
			VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile(
				::Global::DataCache(), "Texture/rocks_NM_height.tga", TextureLoadOption().FlipV()));

			mMaterial.setParameter(SHADER_PARAM(BaseTexture), *mBaseTexture);
			mMaterial.setParameter(SHADER_PARAM(NormalTexture), *mNormalTexture);
			mMaterial.setParameter(SHADER_PARAM(DispFactor), Vector3(-1, 1, mReferenceDepth));
			mMaterial.setParameter(SHADER_PARAM(TileUVScale), Vector3(1, 1, 1));
			mMaterial.setParameter(SHADER_PARAM(ParallaxScale), mParallaxScale);
			mMaterial.setParameter(SHADER_PARAM(ParallaxDirection), mParallaxDirection);

			VERIFY_RETURN_FALSE(mGBufferFrameBuffer = RHICreateFrameBuffer());
			VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());

			GTextureShowManager.registerTexture("POM Base", mBaseTexture);
			GTextureShowManager.registerTexture("POM Normal", mNormalTexture);

			return true;
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mSceneRenderTargets.releaseRHI();
			mGBufferFrameBuffer.release();
			mMaterial.releaseRHI();
			mBaseTexture.release();
			mNormalTexture.release();
			GRenderTargetPool.releaseRHI();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();
			mMaterial.setParameter(SHADER_PARAM(DispFactor), Vector3(-1, 1, mReferenceDepth));
			mMaterial.setParameter(SHADER_PARAM(ParallaxScale), mParallaxScale);
			mMaterial.setParameter(SHADER_PARAM(ParallaxDirection), mParallaxDirection);

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			IntVector2 screenSize = ::Global::GetScreenSize();

			mSceneRenderTargets.prepare(screenSize, 1);
			mGBufferFrameBuffer->setTexture(0, mSceneRenderTargets.getFrameTexture());
			mSceneRenderTargets.getGBuffer().attachToBuffer(*mGBufferFrameBuffer);
			mGBufferFrameBuffer->setDepth(mSceneRenderTargets.getDepthTexture());

			{
				GPU_PROFILE("POM Base Pass");

				RHISetFrameBuffer(commandList, mGBufferFrameBuffer);
				LinearColor clearColors[EGBuffer::Count + 1] =
				{
					LinearColor(0, 0, 0, 0),
					LinearColor(0, 0, 0, 0),
					LinearColor(0, 0, 0, 0),
					LinearColor(0, 0, 0, 0),
					LinearColor(0, 0, 0, 0),
				};
				RHIClearRenderTargets(commandList, EClearBits::All, clearColors, EGBuffer::Count + 1);
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

				DeferredBasePassProgram* shaderProgram = mMaterial.getShaderT<DeferredBasePassProgram>(nullptr);

				if (shaderProgram)
				{
					RHISetShaderProgram(commandList, shaderProgram->getRHI());
					mView.setupShader(commandList, *shaderProgram);
					mMaterial.setupShader(commandList, *shaderProgram);

					shaderProgram->setSampler(commandList, SHADER_SAMPLER(BaseTexture), TStaticSamplerState<ESampler::Bilinear>::GetRHI());
					shaderProgram->setSampler(commandList, SHADER_SAMPLER(NormalTexture), TStaticSamplerState<ESampler::Bilinear>::GetRHI());

					Matrix4 localToWorld = Matrix4::Scale(Vector3(2.5f, 2.5f, 2.5f))/* *
					                       Matrix4::Rotate(Vector3(0, 0, 1), Math::DegToRad(35.0f))*/;
					Matrix4 worldToLocal;
					float det;
					localToWorld.inverseAffine(worldToLocal, det);
					shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), localToWorld);
					shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);

					mSimpleMeshs[SimpleMeshId::Box].draw(commandList, LinearColor(1, 1, 1, 1));
				}
			}

			{
				GPU_PROFILE("POM Lighting");

				mSceneRenderTargets.detachDepthTexture();
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::One>::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

				LightInfo light;
				light.type = ELightType::Directional;
				light.color = Vector3(1.0f, 0.95f, 0.85f);
				light.dir = Vector3(-0.8f, 0.2f, -0.45f).getNormal();
				light.intensity = 4.0f;
				light.radius = 20.0f;
				light.bCastShadow = false;

				DeferredLightingProgram::PermutationDomain permutationVector;
				permutationVector.set<DeferredLightingProgram::UseLightType>((int)light.type);
				permutationVector.set<DeferredLightingProgram::HaveBoundShape>(false);
				DeferredLightingProgram* lightingProgram = ShaderManager::Get().getGlobalShaderT<DeferredLightingProgram>(permutationVector);
				if (lightingProgram)
				{
					RHISetShaderProgram(commandList, lightingProgram->getRHI());
					mView.setupShader(commandList, *lightingProgram);
					lightingProgram->setParamters(commandList, mSceneRenderTargets);
					light.setupShaderGlobalParam(commandList, *lightingProgram);
					DrawUtility::ScreenRect(commandList);
					lightingProgram->clearTextures(commandList);
				}
			}


			GTextureShowManager.registerTexture("FrameTexture", &mSceneRenderTargets.getFrameTexture());

			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());
			GTextureShowManager.registerRenderTarget(GRenderTargetPool);
		}

		MaterialMaster mMaterial;
		float mParallaxScale = 0.04f;
		float mParallaxDirection = 1.0f;
		float mReferenceDepth = 0.0f;
		RHITexture2DRef mBaseTexture;
		RHITexture2DRef mNormalTexture;
		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef mGBufferFrameBuffer;
	};

	REGISTER_STAGE_ENTRY("POM Render", POMRenderStage, EExecGroup::FeatureDev, "Render|Test");
}
