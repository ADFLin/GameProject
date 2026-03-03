#include "Tonemap.h"

#include "RHI/GpuProfiler.h"
#include "RHI/ShaderManager.h"

namespace Render
{
	IMPLEMENT_SHADER_PROGRAM(TonemapProgram);

	void FTonemap::Render(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, RHITexture2D* bloomTexture /*= nullptr*/)
	{
		GPU_PROFILE("Tonemap");
		auto TonemapProg = ShaderManager::Get().getGlobalShaderT<TonemapProgram>();

		sceneRenderTargets.swapFrameTexture();
		RHISetFrameBuffer(commandList, sceneRenderTargets.getFrameBuffer());
		RHISetViewport(commandList, 0, 0, sceneRenderTargets.mSize.x, sceneRenderTargets.mSize.y);
		RHISetShaderProgram(commandList, TonemapProg->getRHI());

		PostProcessContext context;
		context.mInputTexture[0] = &sceneRenderTargets.getPrevFrameTexture();
		TonemapProg->setParameters(commandList, context);

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
