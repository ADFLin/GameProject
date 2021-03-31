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

class PrimitiveDrawer 
{
public:
	void drawRect(Vec2f const& pos, Vec2f const& size);

	void drawSprite(Vec2f const& pos, Vec2f const& size, Texture* tex);
	void drawSprite(Vec2f const& pos, Vec2f const& size, float rot, Texture* tex);
	void drawSprite(Vec2f const& pos, Vec2f const& size, float rot)
	{
		drawSprite(pos, size, rot, size / 2);
	}
	void drawSprite(Vec2f const& pos, Vec2f const& size, float rot, Vec2f const& pivot);
	void drawRectLine(Vec2f const& pos, Vec2f const size);

	Render::TransformStack2D& getStack() { return mStack; }


	Math::Matrix4 mBaseTransform;
	Render::RHICommandList* mCommandList;
	Render::TransformStack2D mStack;
};


void drawRect( Vec2f const& pos , Vec2f const& size );
void drawSprite( Vec2f const& pos , Vec2f const& size , Texture* tex);
void drawSprite( Vec2f const& pos , Vec2f const& size , float rot, Texture* tex);
void drawSprite(Vec2f const& pos, Vec2f const& size, float rot );
void drawRectLine( Vec2f const& pos , Vec2f const size );

#endif // RenderUtility_h__