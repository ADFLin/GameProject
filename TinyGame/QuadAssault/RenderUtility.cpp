#include "RenderUtility.h"

#include "Texture.h"
#include "Base.h"

#include <cassert>

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"

using namespace Render;




struct Vertex
{
	Vec2f pos;
	Vec2f uv;
};

void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size)
{
	Vertex vertices[4] =
	{
		{ mStack.get().transformPosition(pos), Vector2(0,0) },
		{ mStack.get().transformPosition(pos + Vec2f(size.x,0)), Vector2(1,0) },
		{ mStack.get().transformPosition(pos + size), Vector2(1,1) },
		{ mStack.get().transformPosition(pos + Vec2f(0, size.y)), Vector2(0,1) },
	};

	TRenderRT< RTVF_XY_T2 >::Draw(*mCommandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
}

void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size, float rot, Vec2f const& pivot)
{
	Vec2f offset = size.mul(pivot);
	mStack.push();
	mStack.translate(pos + offset);
	mStack.rotate(rot);
	mStack.translate(-offset);
	drawRect(Vec2f::Zero(), size);
	mStack.pop();
}

void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& pivot)
{
	Vec2f offset = size.mul(pivot);
	drawRect(pos + offset, size);
}


void PrimitiveDrawer::drawRect(Vec2f const& pos, Vec2f const& size, Vec2f const& uvMin, Vec2f const& uvMax)
{
	Vertex vertices[4] =
	{
		{ mStack.get().transformPosition(pos), uvMin },
		{ mStack.get().transformPosition(pos + Vec2f(size.x,0)), Vec2f(uvMax.x,uvMin.y) },
		{ mStack.get().transformPosition(pos + size), uvMax },
		{ mStack.get().transformPosition(pos + Vec2f(0, size.y)), Vec2f(uvMin.x,uvMax.y) },
	};
	TRenderRT< RTVF_XY_T2 >::Draw(*mCommandList, EPrimitive::Quad, vertices, ARRAY_SIZE(vertices));
}

void PrimitiveDrawer::drawRectLine(Vec2f const& pos, Vec2f const size)
{
	Vertex vertices[4] =
	{
		{ mStack.get().transformPosition(pos), Vec2f(0,0) },
		{ mStack.get().transformPosition(pos + Vec2f(size.x,0)), Vec2f(1,0) },
	    { mStack.get().transformPosition(pos + size), Vector2(1,1) },
	    { mStack.get().transformPosition(pos + Vec2f(0, size.y)), Vec2f(0,1) },
	};
	TRenderRT< RTVF_XY_T2 >::Draw(*mCommandList, EPrimitive::LineLoop, vertices, ARRAY_SIZE(vertices));
}
