#ifndef ObjectRenderer_h__
#define ObjectRenderer_h__

#include "Renderer.h"
#include "Object.h"


class PrimitiveDrawer;

class IObjectRenderer : public IRenderer
{
public:
	IObjectRenderer();

	virtual void render(PrimitiveDrawer& drawer, RenderPass pass, LevelObject* object) = 0;
	virtual void renderGroup(PrimitiveDrawer& drawer, RenderPass pass , int numObj , LevelObject* object);

	int    getOrder(){ return mRenderOrder; }
	static LevelObject* NextObject( LevelObject* obj ){ return obj->renderLink; }

protected:
	int mRenderOrder;
};

#define DEF_OBJ_RENDERER( CLASS , RENDERER )\
	static RENDERER g##CLASS##Renderer;\
	IObjectRenderer* CLASS::getRenderer(){ return &g##CLASS##Renderer; }

#endif // ObjectRenderer_h__
