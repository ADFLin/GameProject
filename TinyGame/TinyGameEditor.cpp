#include "TinyGameApp.h"

#if TINY_WITH_EDITOR

#include "GameGlobal.h"
#include "DrawEngine.h"

#include "RHI/RHICommand.h"
#include "RHI/D3D11Command.h"

#include "Module/ModuleManager.h"

using namespace Render;

bool TinyGameApp::initializeEditor()
{
	VERIFY_RETURN_FALSE(mEditor = IEditor::Create());
	return true;
}

bool TinyGameApp::initializeEditorRender()
{
	char const* RHIModuleName = "D3D11RHI.dll";

	FCString::IsConstSegment(RHIModuleName);

	ModuleManager::Get().loadModule(RHIModuleName);
	::Global::GetDrawEngine().lockSystem(ERenderSystem::D3D11);

	mEditor->initializeRender();
	mEditor->addGameViewport(this);
	return true;
}

void TinyGameApp::finalizeEditor()
{
	if (mEditor)
	{
		mEditor->release();
		mEditor = nullptr;
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