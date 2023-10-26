#ifndef RenderUtility_h__
#define RenderUtility_h__

#include "MathCore.h"
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
	virtual void beginTranslucent(float alpha){}
	virtual void endTranslucent(){}

	void drawRect(Vec2f const& pos, Vec2f const& size);

	void drawRect(Vec2f const& pos, Vec2f const& size, float rot)
	{
		drawRect(pos, size, rot, Vec2f(0.5,0.5));
	}

	void drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& uvMin, Vec2f const& uvMax);
	void drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& pivot);
	void drawRect(Vec2f const& pos, Vec2f const& size, float rot, Vec2f const& pivot);
	void drawRectLine(Vec2f const& pos, Vec2f const size);

	Render::TransformStack2D& getStack() { return mStack; }

	Math::Matrix4 mBaseTransformRHI;
	Render::RHICommandList* mCommandList;
	Render::TransformStack2D mStack;
};


void drawRect( Vec2f const& pos , Vec2f const& size );
void drawSprite( Vec2f const& pos , Vec2f const& size , Texture* tex);
void drawSprite( Vec2f const& pos , Vec2f const& size , float rot, Texture* tex);
void drawSprite(Vec2f const& pos, Vec2f const& size, float rot );
void drawRectLine( Vec2f const& pos , Vec2f const size );

#endif // RenderUtility_h__