#include "Tonemap.h"

#include "RHI/GpuProfiler.h"
#include "RHI/ShaderManager.h"

namespace Render
{
	IMPLEMENT_SHADER_PROGRAM(TonemapProgram);

	void FTonemap::Render(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, RHITexture2D* bloomTexture /*= nullptr*/, float exposure /*= 1.0f*/)
	{
		GPU_PROFILE("Tonemap");
		sceneRenderTargets.swapFrameTexture();
		RHIResourceTransition(commandList, { &sceneRenderTargets.getFrameTexture() } , EResourceTransition::RenderTarget);
		RHISetFrameBuffer(commandList, sceneRenderTargets.getFrameBuffer());
		RHISetViewport(commandList, 0, 0, sceneRenderTargets.mSize.x, sceneRenderTargets.mSize.y);
		Render(commandList, sceneRenderTargets.getPrevFrameTexture(), bloomTexture, exposure, false);
	}

	void FTonemap::Render(RHICommandList& commandList, RHITexture2D& inputTexture, RHITexture2D* bloomTexture, float exposure, bool bUseACES)
	{
		GPU_PROFILE("Tonemap");
		TonemapProgram::PermutationDomain domain;
		domain.set<TonemapProgram::UseBloom>(bloomTexture != nullptr);
		domain.set<TonemapProgram::UseACES>(bUseACES);
		auto TonemapProg = ShaderManager::Get().getGlobalShaderT<TonemapProgram>(domain);

		RHISetShaderProgram(commandList, TonemapProg->getRHI());

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());

		PostProcessContext context;
		context.mInputTexture[0] = &inputTexture;
		TonemapProg->setParameters(commandList, context);
		TonemapProg->setExposure(commandList, exposure);

		if (bloomTexture)
		{
			TonemapProg->setBloomTexture(commandList, *bloomTexture);
		}

		DrawUtility::ScreenRect(commandList);
	}
}
