#ifndef Renderable_h__a93aa29a_4bda_4917_b753_cddb53d7cf00
#define Renderable_h__a93aa29a_4bda_4917_b753_cddb53d7cf00


struct Transform
{
	Vec2f offset;
	Vec2f scale;
	float rotation;
};


class IRenderable
{
public:
	virtual RenderProxy* createRenderProxy() = 0;
	virtual Transform    getTransform() = 0;
};

class RenderProxy
{
public:


	IRenderable* mRenderable;
};

class RenderCommand
{
public:
	virtual void exec() = 0;
};

#endif // Renderable_h__a93aa29a_4bda_4917_b753_cddb53d7cf00

