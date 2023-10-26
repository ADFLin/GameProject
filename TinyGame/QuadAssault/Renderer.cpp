#include "Renderer.h"

static IRenderer* gLink;
static bool bInitialized  = false;

struct RendererInitHelper
{
	RendererInitHelper(){ gLink = NULL; }
};

IRenderer::IRenderer()
{
	static RendererInitHelper helper;
	mLink = gLink;
	gLink = this;
}

void IRenderer::cleanup()
{

}

void IRenderer::Initialize()
{
	if ( bInitialized )
		return;

	IRenderer* link = gLink;
	while( link )
	{
		link->init();
		link = link->mLink;
	}
	bInitialized = true;
}
