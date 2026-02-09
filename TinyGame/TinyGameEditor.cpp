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
	ERenderSystem renderSystem = ERenderSystem::D3D11;
	char const* moduleName = "D3D11RHI.dll";
	switch (renderSystem)
	{
	case ERenderSystem::OpenGL:
		moduleName = "OpenGLRHI.dll";
		break;
	case ERenderSystem::D3D11:
		moduleName = "D3D11RHI.dll";
		break;
	case ERenderSystem::D3D12:
		moduleName = "D3D12RHI.dll";
		break;
	case ERenderSystem::Vulkan:
		moduleName = "VulkanRHI.dll";
		break;
	}
	if (!ModuleManager::Get().loadModule(moduleName))
	{
		return false;
	}

	RenderSystemConfigs configs;
	configs.bDebugMode = true;
	configs.bVSyncEnable = true;
	configs.numSamples = 1;
	if (!::Global::GetDrawEngine().lockSystem(renderSystem, configs))
	{
		return false;
	}

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

TVector2<int> TinyGameApp::getInitialSize()
{
	return TVector2<int>(mGameWindow.getWidth(), mGameWindow.getHeight());
}

void TinyGameApp::resizeViewport(int w, int h)
{
	::Global::GetDrawEngine().changeScreenSize(w, h, true);
}

void TinyGameApp::renderViewport(IEditorRenderContext& context)
{
	if (::Global::GetDrawEngine().isRHIEnabled())
	{
		if (GRHISystem->getName() == RHISystemName::D3D11)
		{
			static_cast<D3D11System*>(GRHISystem)->mSwapChain->getBackBufferTexture();
			context.setRenderTexture(*static_cast<D3D11System*>(GRHISystem)->mSwapChain->getBackBufferTexture());
		}
		else
		{


		}
	}
	else
	{
		context.copyToRenderTarget(::Global::GetDrawEngine().getBufferDC());
	}
}

void TinyGameApp::onViewportMouseEvent(MouseMsg const& msg)
{
	handleMouseEvent(msg);
}

#endif