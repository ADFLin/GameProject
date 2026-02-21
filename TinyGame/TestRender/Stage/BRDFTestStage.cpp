#include "BRDFTestStage.h"

#include "RHI/Scene.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"

#include "Renderer/MeshBuild.h"

#include "DataCacheInterface.h"
#include "DataStreamBuffer.h"
#include "ProfileSystem.h"

#include "RHI/D3D11Command.h"


namespace Render
{

	REGISTER_STAGE_ENTRY("BRDF Test", BRDFTestStage, EExecGroup::FeatureDev, 1, "Render|Test" );

	IMPLEMENT_SHADER_PROGRAM(LightProbeVisualizeProgram);


	bool BRDFTestStage::onInit()
	{
		if( !BaseClass::onInit() )
			return false;

		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Use Tonemap"), bEnableTonemap);
		FWidgetProperty::Bind(frame->addSlider(UI_ANY), SkyboxShowIndex , 0 , (int)ESkyboxShow::Count - 1);
		FWidgetProperty::Bind(frame->addCheckBox(UI_ANY, "Use Shader Blit"), mbUseShaderBlit);

		return true;
	}

	bool BRDFTestStage::setupRenderResource(ERenderSystem systemName)
	{
		VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));

		VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
		VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());

		char const* HDRImagePath = "Texture/HDR/A.hdr";
		{
			TIME_SCOPE("HDR Texture");
			VERIFY_RETURN_FALSE(mHDRImage = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Texture/HDR/A.hdr", TextureLoadOption().HDR()));
		}
		{
			TIME_SCOPE("IBL");
			VERIFY_RETURN_FALSE(mBuilder.loadOrBuildResource(::Global::DataCache(), HDRImagePath, *mHDRImage, mIBLResource));
		}

		{
			TIME_SCOPE("BRDF Texture");
			VERIFY_RETURN_FALSE(mRockTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Texture/rocks.jpg", TextureLoadOption().SRGB()));
			VERIFY_RETURN_FALSE(mNormalTexture = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), "Texture/N.png", TextureLoadOption()));
		}

		GTextureShowManager.registerTexture("HDR", mHDRImage);
		GTextureShowManager.registerTexture("Rock", mRockTexture);
		GTextureShowManager.registerTexture("Normal", mNormalTexture);
		GTextureShowManager.registerTexture("BRDF", IBLResource::SharedBRDFTexture);
		GTextureShowManager.registerTexture("SkyBox", mIBLResource.texture);

		{
			TIME_SCOPE("BRDF Shader");
			mProgVisualize = ShaderManager::Get().getGlobalShaderT< LightProbeVisualizeProgram >(true);
			mProgTonemap = ShaderManager::Get().getGlobalShaderT< TonemapProgram >(true);
		}

		VERIFY_RETURN_FALSE(mParamBuffer.initializeResource(1));
		{
			auto* pParams = mParamBuffer.lock();
			*pParams = mParams;
			mParamBuffer.unlock();
		}
		Vec2i screenSize = ::Global::GetScreenSize();

		VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());

		return true;
	}

	void BRDFTestStage::preShutdownRenderSystem(bool bReInit /*= false*/)
	{
		mSceneRenderTargets.releaseRHI();
		mProgVisualize = nullptr;
		mProgTonemap = nullptr;
		mHDRImage.release();
		mRockTexture.release();
		mNormalTexture.release();
		mIBLResource.releaseRHI();
		mBuilder.releaseRHI();
		mParamBuffer.releaseResource();

		BaseClass::preShutdownRenderSystem(bReInit);
	}

	bool BRDFTestStage::isRenderSystemSupported(ERenderSystem systemName)
	{
		switch (systemName)
		{
		case ERenderSystem::OpenGL:
		case ERenderSystem::D3D11:
			return true;
		case ERenderSystem::D3D12:
			break;
		case ERenderSystem::Vulkan:
			break;
		default:
			break;
		}
		return false;
	}

	void BRDFTestStage::onRender(float dFrame)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();

		initializeRenderState();

		mSceneRenderTargets.prepare(screenSize);

		{
			GPU_PROFILE("Scene");
			mSceneRenderTargets.attachDepthTexture();
			RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
			RHIClearRenderTargets(commandList, EClearBits::All , &LinearColor(0.2, 0.2, 0.2, 1), 1);

			drawSkyBox(commandList, mView, *mHDRImage, mIBLResource, SkyboxShowIndex);

			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

			{
				RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
				DrawUtility::AixsLine(commandList);
			}

			{
				GPU_PROFILE("LightProbe Visualize");

				RHISetShaderProgram(commandList, mProgVisualize->getRHI());
				mProgVisualize->setStructuredUniformBufferT< LightProbeVisualizeParams >(commandList, *mParamBuffer.getRHI());
				mProgVisualize->setTexture(commandList, SHADER_PARAM(NormalTexture), mNormalTexture);
				mView.setupShader(commandList, *mProgVisualize);
				mProgVisualize->setParameters(commandList, mIBLResource);
				RHISetInputStream(commandList, nullptr , nullptr , 0 );
				RHIDrawPrimitiveInstanced(commandList, EPrimitive::TriangleList, 0, 6, mParams.gridNum.x * mParams.gridNum.y);
			}
		}

		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetFrameBuffer(commandList, nullptr);
		RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);


		if (bEnableTonemap)
		{
			GPU_PROFILE("Tonemap");

			mSceneRenderTargets.swapFrameTexture();
			PostProcessContext context;
			context.mInputTexture[0] = &mSceneRenderTargets.getPrevFrameTexture();

			RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());
			RHISetShaderProgram(commandList, mProgTonemap->getRHI());

			mProgTonemap->setParameters(commandList, context);
			DrawUtility::ScreenRect(commandList);
		}


		RHISetFrameBuffer(commandList, nullptr);

		{
			GPU_PROFILE("Blit To Screen");
			if ( true || mbUseShaderBlit )
			{
				ShaderHelper::Get().copyTextureToBuffer(commandList, mSceneRenderTargets.getFrameTexture());
			}
			else if (GRHISystem->getName() == RHISystemName::OpenGL)
			{
				OpenGLCast::To(mSceneRenderTargets.getFrameBuffer())->blitToBackBuffer();
			}
		}

#if 0
		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		g.beginRender();
		if (IBLResource::SharedBRDFTexture.isValid())
		{
			g.setBrush(Color3f(1, 1, 1));
			g.drawTexture(*IBLResource::SharedBRDFTexture, IntVector2(10, 10), IntVector2(512, 512));
		}
		g.endRender();

		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(commandList, *mHDRImage, IntVector2(10, 10), IntVector2(512, 512));
		}

		if ( IBLResource::SharedBRDFTexture.isValid()  )
		{
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			OrthoMatrix matProj(0, screenSize.x, 0, screenSize.y, -1, 1);
			MatrixSaveScope matScope(matProj);
			DrawUtility::DrawTexture(commandList, *IBLResource::SharedBRDFTexture , IntVector2(10, 10), IntVector2(512, 512));
		}
#endif

		GRenderTargetPool.freeAllUsedElements();
	}

}