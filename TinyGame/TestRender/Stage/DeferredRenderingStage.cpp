#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"
#include "Renderer/RenderTargetPool.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/Material.h"

#include "Renderer/BasePassRendering.h"
#include "Renderer/ShadowDepthRendering.h"

namespace Render
{

	class DeferredRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		DeferredRenderingStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			mCamera.lookAt(Vector3(20, 20, 4), Vector3(10, 10, 0), Vector3(0, 0, 1));
			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			auto frameWidget = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "bShowGBuffer"), bShowGBuffer);
			return true;
		}


		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1280;
			systemConfigs.screenHeight = 720;
			systemConfigs.bDebugMode = true;
		}


		virtual bool setupRenderSystem(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderSystem(systemName));

			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());

			VERIFY_RETURN_FALSE(mMaterial.loadFile("Simple1"));

			VERIFY_RETURN_FALSE(mShadowFrameBuffer = RHICreateFrameBuffer());
			VERIFY_RETURN_FALSE(mGBufferFrameBuffer = RHICreateFrameBuffer());

			VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());

			VERIFY_RETURN_FALSE(mProgLiginting = ShaderManager::Get().getGlobalShaderT< COMMA_SEPARATED(TDeferredLightingProgram< LightType::Point , false >) >());
			return true;
		}


		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mSceneRenderTargets.releaseRHI();
			mShadowFrameBuffer.release();
			mGBufferFrameBuffer.release();
			GRenderTargetPool.releaseRHI();

			mMaterial.releaseRHI();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		MaterialMaster mMaterial;
		DeferredLightingProgram* mProgLiginting;

		bool bShowGBuffer = false;

		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef  mShadowFrameBuffer;
		RHIFrameBufferRef  mGBufferFrameBuffer;




		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void tick() override
		{

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
			initializeRenderState();

			GRenderTargetPool.freeAllUsedElements();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			Vec2i screenSize = ::Global::GetScreenSize();

			mSceneRenderTargets.prepare(screenSize, 1);
#if 0
			{
				RenderTargetDesc desc;
				desc.format = Texture::eDepth16;
				desc.size = IntVector2(256, 256);
				desc.numSamples = 1;
				desc.creationFlags = TCF_CreateSRV;

				PooledRenderTargetRef shadowRT = GRenderTargetPool.fetchElement(desc);


				mShadowFrameBuffer->setDepth(*shadowRT->texture);


				RHISetFrameBuffer(commandList, mShadowFrameBuffer);
				RHIClearRenderTargets(commandList, EClearBits::Depth, nullptr, 0);


				ShadowDepthProgram* progShadow = mMaterial.getShaderT< ShadowDepthProgram >(nullptr);
			}
#endif

			{
				GPU_PROFILE("Base Pass");

				mGBufferFrameBuffer->setTexture(0, mSceneRenderTargets.getFrameTexture());
				mSceneRenderTargets.getGBuffer().attachToBuffer(*mGBufferFrameBuffer);
				mGBufferFrameBuffer->setDepth(mSceneRenderTargets.getDepthTexture());

				RHISetFrameBuffer(commandList, mGBufferFrameBuffer);
				LinearColor clearColors[EGBuffer::Count + 1] = { LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0) };
				RHIClearRenderTargets(commandList, EClearBits::All, clearColors, EGBuffer::Count + 1);
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

				//RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
				DeferredBasePassProgram* progBase = mMaterial.getShaderT< DeferredBasePassProgram >(nullptr);
				RHISetShaderProgram(commandList, progBase->getRHIResource());
				mView.setupShader(commandList, *progBase);

				auto SetupTransform = [&](Matrix4 const& localToWorld)
				{
					Matrix4 worldToLocal;
					float det;
					localToWorld.inverseAffine(worldToLocal, det);
					progBase->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), localToWorld);
					progBase->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);
				};

				{
					SetupTransform(Matrix4::Identity());
					//DrawUtility::AixsLine(commandList);
				}
				for (int i = 0; i < 100; ++i)
				{
					SetupTransform(Matrix4::Scale(0.4) * Matrix4::Translate(Vector3(2.5 * (i / 10), 2.5 * (i % 10), 1)));
					mSimpleMeshs[SimpleMeshId::Doughnut].draw(commandList, LinearColor(1, 0, 0, 1));
				}


			}

			{
				GPU_PROFILE("Lighting");

				mSceneRenderTargets.getFrameBuffer()->removeDepth();
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());

				RHISetShaderProgram(commandList, mProgLiginting->getRHIResource());

				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

				mView.setupShader(commandList, *mProgLiginting);
				mProgLiginting->setParamters(commandList, mSceneRenderTargets);
				LightInfo light;
				light.pos = Vector3(10, 10, 5);
				light.type = LightType::Point;
				light.bCastShadow = false;
				light.dir = Vector3(1, 0, 0);
				light.color = Vector3(1, 1, 1);
				light.intensity = 100;
				light.radius = 20;
				light.setupShaderGlobalParam(commandList, *mProgLiginting);

				DrawUtility::ScreenRect(commandList);

				//mProgLiginting->clearTextures(commandList);
			}


			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());

			if (bShowGBuffer)
			{
				GPU_PROFILE("Draw GBuffer");
				RHISetShaderProgram(commandList, nullptr);
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, 0, screenSize.y, -1, 1));
				mSceneRenderTargets.getGBuffer().drawTextures(commandList, porjectMatrix, screenSize / 4, IntVector2(2, 2));
			}

			TextureShowManager::registerRenderTarget(GRenderTargetPool);
		}



		bool onMouse(MouseMsg const& msg) override
		{
			if (!BaseClass::onMouse(msg))
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
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

	REGISTER_STAGE_ENTRY("Deferred Rendering", DeferredRenderingStage, EExecGroup::FeatureDev, "Render");

}