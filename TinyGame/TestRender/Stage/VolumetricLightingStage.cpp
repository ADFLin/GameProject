#include "Stage/TestRenderStageBase.h"
#include "Renderer/VolumetricLighting.h"
#include "Renderer/SceneView.h"
#include "RHI/RHICommand.h"
#include "Renderer/RenderTargetPool.h"
#include "RHI/RHIGlobalResource.h"
#include "Renderer/BasePassRendering.h"
#include "Renderer/ShadowDepthRendering.h"
#include "Renderer/MeshBuild.h"
#include "Editor.h"

namespace Render
{
	class VolumetricLightingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		
		bool onInit() override
		{
			if( !BaseClass::onInit() )
				return false;

			mCamera.lookAt(Vector3(150, 150, 150), Vector3(0, 0, 0), Vector3(0, 0, 1));

			auto AddSpotLight = [this](Vector3 const& pos, Vector3 const& dir, Vector3 const& color)
			{
				LightInfo light;
				light.type = ELightType::Spot;
				light.bCastShadow = false;
				light.dir = dir.getNormal();
				light.intensity = 1500 * 5;
				light.radius = 300;
				light.spotAngle = Vector3(0, 44, 0);
				light.pos = pos;
				light.color = color;
				mLights.push_back(light);
			};

			// Add RGB Spot Lights
			AddSpotLight(Vector3(-60, 0, 100), Vector3(0.5, 0, -1), Vector3(1, 0, 0));  // Red
			AddSpotLight(Vector3(30, 52, 100), Vector3(-0.25, -0.43, -1), Vector3(0, 1, 0)); // Green
			AddSpotLight(Vector3(30, -52, 100), Vector3(-0.25, 0.43, -1), Vector3(0, 0, 1)); // Blue

			::Global::GUI().cleanupWidget();

			if (::Global::Editor())
			{
				DetailViewConfig config;
				config.name = "Lights Control";
				mDetailView = ::Global::Editor()->createDetailView(config);
				for(auto& light : mLights)
					mDetailView->addStruct(light);
			}



			return true;
		}

		void onEnd() override
		{
			mResources.releaseRHI();
			if (mDetailView)
			{
				mDetailView->release();
			}
			BaseClass::onEnd();
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			IntVector2 screenSize = ::Global::GetScreenSize();

			// Match UE5 settings: GridPixelSize=8, GridSizeZ=128
			mResources.prepare(screenSize, 8, 128);

			//Testing Logic
			{
				GPU_PROFILE("VolumetricLighting");
				RHISetFrameBuffer(commandList, nullptr);

				// Transitions for Initialization
				{
					RHIResource* uavs[] = { mResources.mVolumeBufferA.get(), mResources.mVolumeBufferB.get(), mResources.mScatteringBuffer.get() };
					RHIResourceTransition(commandList, uavs, EResourceTransition::UAV);
				}

				// 1. Clear Buffers
				{
					GPU_PROFILE("ClearBuffer");
					// Inject a subtle base density: Scattering = 0.02, Extinction = 0.001
					mProgClearBuffer->clearTexture(commandList, *mResources.mVolumeBufferA, Vector4(0.02, 0.02, 0.02, 0.0005));
					// Set gFactor = 0.5 for balanced forward scattering
					mProgClearBuffer->clearTexture(commandList, *mResources.mVolumeBufferB, Vector4(0, 0, 0, 0.5));
					mProgClearBuffer->clearTexture(commandList, *mResources.mScatteringBuffer, Vector4(0, 0, 0, 0));
					mProgClearBuffer->clearTexture(commandList, *mResources.mScatteringRawBuffer, Vector4(0, 0, 0, 0));
				}

				// 2. Upload Light Data
				{
					TiledLightInfo* pInfo = mResources.mTiledLightBuffer.lock();
					if (pInfo)
					{
						for (auto const& light : mLights)
						{
							pInfo->setValue(light);
							++pInfo;
						}
						mResources.mTiledLightBuffer.unlock();
					}
				}

				// 3. Dispatch Injection // 2. Compute Scattering
				{
					GPU_PROFILE("LightScattering");

					// Transition VolumeBuffers and History back to SRV so they can be sampled
					{
						RHIResource* srvs[] = { mResources.mVolumeBufferA.get(), mResources.mVolumeBufferB.get(), mResources.mHistoryBuffer.get() };
						RHIResourceTransition(commandList, srvs, EResourceTransition::SRV);
					}

					RHISetShaderProgram(commandList, mProgLightScattering->getRHI());
					mProgLightScattering->setParameters(commandList, mView, mResources, (int)mLights.size());
					
					int nx = (mResources.mScatteringBuffer->getSizeX() + 7) / 8;
					int ny = (mResources.mScatteringBuffer->getSizeY() + 7) / 8;
					int nz = mResources.mScatteringBuffer->getSizeZ();
					RHIDispatchCompute(commandList, nx, ny, nz);
					
					// Force unbind UAV to prevent hazard in next pass
					mProgLightScattering->unbindParameters(commandList, mResources);
				}

				// Copy current scattering to history for temporal accumulation next frame
				{
					// DISABLED: Ghosting investigation
					// RHIResource* copyDest[] = { mResources.mHistoryBuffer.get() };
					// RHIResourceTransition(commandList, copyDest, EResourceTransition::CopyDest);
					// RHIResource* copySrc[] = { mResources.mScatteringBuffer.get() };
					// RHIResourceTransition(commandList, copySrc, EResourceTransition::CopySrc);
					// RHICopyResource(commandList, *mResources.mHistoryBuffer, *mResources.mScatteringBuffer);
				}

				// Transition for Integration
				{
					RHIResource* barriers[] = { mResources.mScatteringBuffer.get() };
					RHIResourceTransition(commandList, barriers, EResourceTransition::UAV);

					// New: Transition VolumeBufferA to SRV for reading in Integration shader
					// Actually, we are using ScatteringRawBuffer now
					RHIResource* srvTrans[] = { mResources.mVolumeBufferA.get(), mResources.mScatteringRawBuffer.get() };
					RHIResourceTransition(commandList, srvTrans, EResourceTransition::SRV);
				}

				// 4. Dispatch Integration Compute Shader
				{
					GPU_PROFILE("VolumetricIntegration");
					RHISetShaderProgram(commandList, mProgIntegration->getRHI());
					mProgIntegration->setParameters(commandList, mResources);

					int nx = (mResources.mScatteringBuffer->getSizeX() + 8 - 1) / 8; // GroupSizeX
					int ny = (mResources.mScatteringBuffer->getSizeY() + 8 - 1) / 8; // GroupSizeY
					RHIDispatchCompute(commandList, nx, ny, 1);
				}

				// Transition for Visualization
				{
					RHIResource* srvs[] = { mResources.mScatteringBuffer.get() };
					RHIResourceTransition(commandList, srvs, EResourceTransition::SRV);
				}
			}

			// Visualization
			{
				GPU_PROFILE("Visualization");
				RHISetFrameBuffer(commandList, mSceneFrameBuffer);
				RHIClearRenderTargets(commandList, EClearBits::All, nullptr, 0, 1.0f);
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

				// Draw Scene Geometry (Plane and Box)
				{
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<true, ECompareFunc::LessEqual>::GetRHI());
					
					// Ground Plane
					RHISetFixedShaderPipelineState(commandList, Matrix4::Scale(400, 400, 1) * AdjustProjectionMatrixForRHI(mView.worldToClip), LinearColor(0.2, 0.4, 0.2));
					mSimpleMeshs[SimpleMeshId::Plane].draw(commandList);

					// Red Box
					RHISetFixedShaderPipelineState(commandList, Matrix4::Translate(0, 0, 5) * Matrix4::Scale(10, 10, 10) * AdjustProjectionMatrixForRHI(mView.worldToClip), LinearColor(0.7, 0.1, 0.1));
					mSimpleMeshs[SimpleMeshId::Box].draw(commandList);
				}

				// Composite Volumetric Lighting
				{
					// Switch to color-only framebuffer to avoid D3D11 hazrad with depth texture
					RHISetFrameBuffer(commandList, mCompositeFrameBuffer);

					// Final = 1 * Scattering + Transmittance * SceneColor
					RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::One, EBlend::SrcAlpha>::GetRHI());
					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

					RHISetShaderProgram(commandList, mProgVisualization->getRHI());
					mProgVisualization->setParameters(commandList, mView, mResources, *mSceneDepthTexture);
					DrawUtility::ScreenRect(commandList);
				}

				// Final Copy to screen
				{
					RHISetFrameBuffer(commandList, nullptr);
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
					ShaderHelper::Get().copyTextureToBuffer(commandList, *mSceneColorTexture);
				}
			}
			
			GTextureShowManager.registerTexture("Volume_Scat_Ext", mResources.mVolumeBufferA.get());
			GTextureShowManager.registerTexture("Volume_Emit_Phase", mResources.mVolumeBufferB.get());
			GTextureShowManager.registerTexture("Scattering_Trans", mResources.mScatteringBuffer.get());
			GTextureShowManager.registerRenderTarget(GRenderTargetPool);
		}

		TArray< LightInfo > mLights;
		VolumetricLightingResources mResources;
		
		ClearBufferProgram* mProgClearBuffer;
		LightScatteringProgram* mProgLightScattering;
		VolumetricIntegrationProgram* mProgIntegration;
		VolumetricVisualizationProgram* mProgVisualization;

		IEditorDetailView* mDetailView = nullptr;
		RHITexture2DRef   mSceneColorTexture;
		RHITexture2DRef   mSceneDepthTexture;
		RHIFrameBufferRef mSceneFrameBuffer;
		RHIFrameBufferRef mCompositeFrameBuffer;

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));

			if (!loadCommonShader() || !createSimpleMesh())
				return false;

			mProgClearBuffer = ShaderManager::Get().getGlobalShaderT< ClearBufferProgram >(true);
			mProgLightScattering = ShaderManager::Get().getGlobalShaderT< LightScatteringProgram >(true);
			mProgIntegration = ShaderManager::Get().getGlobalShaderT< VolumetricIntegrationProgram >(true);
			mProgVisualization = ShaderManager::Get().getGlobalShaderT< VolumetricVisualizationProgram >(true);

			if (!mResources.prepare(IntVector2(1280, 720), 8, 64))
				return false;

			IntVector2 screenSize = ::Global::GetScreenSize();
			// Resources are already created in mResources.prepare() above
			
			mSceneColorTexture = RHICreateTexture2D(ETexture::RGBA8, screenSize.x, screenSize.y, 1, 1, TCF_CreateSRV | TCF_RenderTarget);
			mSceneDepthTexture = RHICreateTexture2D(ETexture::Depth32F, screenSize.x, screenSize.y, 1, 1, TCF_CreateSRV);
			mSceneFrameBuffer = RHICreateFrameBuffer();
			mSceneFrameBuffer->setTexture(0, *mSceneColorTexture);
			mSceneFrameBuffer->setDepth(*mSceneDepthTexture);

			mCompositeFrameBuffer = RHICreateFrameBuffer();
			mCompositeFrameBuffer->setTexture(0, *mSceneColorTexture);

			return true;
		}

	};

	REGISTER_STAGE_ENTRY("Volumetric Lighting", VolumetricLightingStage, EExecGroup::FeatureDev, "Render");

}//namespace Render
