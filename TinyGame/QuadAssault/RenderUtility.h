#ifndef RenderUtility_h__
#define RenderUtility_h__

#include "MathCore.h"
#include "Renderer/BatchedRender2D.h"
#include "Renderer/RenderTransform2D.h"
#include "Math/Matrix4.h"

namespace Render
{
	class RHICommandList;
}

class Texture;

struct PrimitiveMat
{
	Color3f  color = Color3f(1,1,1);
	Texture* baseTex = nullptr;
	Texture* normalTex = nullptr;
};
class PrimitiveDrawer 
{
public:
	virtual void setGlow(Texture* texture , Color3f const& color = Color3f::White()) {}
	virtual void setMaterial(PrimitiveMat const& mat) {}
	virtual void setWriteMask(uint32 masks){}
	virtual void beginBlend(float alpha, Render::ESimpleBlendMode mode = Render::ESimpleBlendMode::Translucent){}
	virtual void endBlend(){}

	void drawRect(Vec2f const& pos, Vec2f const& size);

	void drawRect(Vec2f const& pos, Vec2f const& size, float rot)
	{
		drawRect(pos, size, rot, Vec2f(0.5,0.5));
	}

	void drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& uvMin, Vec2f const& uvMax);
	void drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& pivot);
	void drawRect(Vec2f const& pos, Vec2f const& size, float rot, Vec2f const& pivot);

	Render::TransformStack2D& getStack() { return mStack; }
	Vec2f transformPos(Vec2f const& pos) const
	{
		return mStack.get().transformPosition(pos);
	}

	float time;

	Math::Matrix4 mBaseTransformRHI;
	Render::RHICommandList* mCommandList;
	Render::TransformStack2D mStack;
};


#endif // RenderUtility_h__