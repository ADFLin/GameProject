#include "TinyGameApp.h"

#if TINY_WITH_EDITOR

#include "GameGlobal.h"
#include "DrawEngine.h"

#include "RHI/RHICommand.h"
#include "RHI/D3D11Command.h"

#include "Module/ModuleManager.h"
#include "RenderDebug.h"

using namespace Render;

bool TinyGameApp::initializeEditor()
{
	VERIFY_RETURN_FALSE(mEditor = IEditor::Create());
	extern TINY_API IEditor* GEditor;
	GEditor = mEditor;
	return true;
}

bool TinyGameApp::initializeEditorRender()
{
	char const* RHIModuleName = "D3D11RHI.dll";

	FCString::IsConstSegment(RHIModuleName);

	ModuleManager::Get().loadModule(RHIModuleName);

	RenderSystemConfigs configs;
	configs.bDebugMode = true;
	configs.bVSyncEnable = true;
	configs.numSamples = 1;
	::Global::GetDrawEngine().lockSystem(ERenderSystem::D3D11, configs);

	mEditor->initializeRender();
	mEditor->addGameViewport(this);
	mEditor->setTextureShowManager(&GTextureShowManager);
	return true;
}

void TinyGameApp::finalizeEditor()
{
	if (mEditor)
	{
		mEditor->release();
		mEditor = nullptr;
		::Global::GetDrawEngine().unlockSystem();
	}
}

void TinyGameApp::resizeViewport(int w, int h)
{
	::Global::GetDrawEngine().changeScreenSize(w, h, true);
}

void TinyGameApp::renderViewport(IEditorRenderContext& context)
{
	if (::Global::GetDrawEngine().isRHIEnabled())
	{
		static_cast<D3D11System*>(GRHISystem)->mSwapChain->getBackBufferTexture();
		context.setRenderTexture(*static_cast<D3D11System*>(GRHISystem)->mSwapChain->getBackBufferTexture());
	}
	else
	{
		context.copyToRenderTarget((void*) ::Global::GetDrawEngine().getPlatformGraphics().getTargetDC());
	}
}

void TinyGameApp::onViewportMouseEvent(MouseMsg const& msg)
{
	handleMouseEvent(msg);
}

#endif