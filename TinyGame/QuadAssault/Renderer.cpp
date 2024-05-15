#include "Renderer.h"

struct RendererList
{
	IRenderer* head = nullptr;
	bool bInitialized = false;

	void add(IRenderer* renderer)
	{
		renderer->mLink = head;
		head = renderer;
	}

	void initRenderers()
	{
		if (bInitialized)
			return;

		IRenderer* link = head;
		while (link)
		{
			link->init();
			link = link->mLink;
		}
		bInitialized = true;
	}

	static RendererList& Get()
	{
		static RendererList StaticList;
		return StaticList;
	}
};

IRenderer::IRenderer()
{
	RendererList::Get().add(this);
}

void IRenderer::cleanup()
{

}

void IRenderer::Initialize()
{
	RendererList::Get().initRenderers();
}
