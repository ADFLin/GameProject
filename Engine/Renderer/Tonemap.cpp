#include "Tonemap.h"

#include "RHI/GpuProfiler.h"
#include "RHI/ShaderManager.h"

namespace Render
{
	IMPLEMENT_SHADER_PROGRAM(TonemapProgram);

	void FTonemap::Render(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, RHITexture2D* bloomTexture /*= nullptr*/, float exposure /*= 1.0f*/)
	{
		GPU_PROFILE("Tonemap");
		TonemapProgram::PermutationDomain domain;
		domain.set<TonemapProgram::UseBloom>(bloomTexture != nullptr);
		domain.set<TonemapProgram::UseACES>(false); // Default to false for legacy callers
		auto TonemapProg = ShaderManager::Get().getGlobalShaderT<TonemapProgram>(domain);

		sceneRenderTargets.swapFrameTexture();
		RHISetFrameBuffer(commandList, sceneRenderTargets.getFrameBuffer());
		RHISetViewport(commandList, 0, 0, sceneRenderTargets.mSize.x, sceneRenderTargets.mSize.y);
		RHISetShaderProgram(commandList, TonemapProg->getRHI());

		PostProcessContext context;
		context.mInputTexture[0] = &sceneRenderTargets.getPrevFrameTexture();
		TonemapProg->setParameters(commandList, context);
		TonemapProg->setExposure(commandList, exposure);

		if (bloomTexture)
		{
			TonemapProg->setBloomTexture(commandList, *bloomTexture);
		}
		else
		{
			TonemapProg->setBloomTexture(commandList, *GBlackTexture2D);
		}

		DrawUtility::ScreenRect(commandList);
	}
}
