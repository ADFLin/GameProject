#include "DebugDraw.h"

CORE_API extern IDebugDrawBatcher* GDebugDraw;

#if CORE_SHARE_CODE
IDebugDrawBatcher* GDebugDraw = nullptr;
void IDebugDrawBatcher::Set(IDebugDrawBatcher* debugDraw)
{
	GDebugDraw = debugDraw;
}
#endif

void DrawDebugLine(Vector2 const& posStart, Vector2 const& posEnd, Color3ub const& color, float thickneess)
{
	if (GDebugDraw)
		GDebugDraw->addLine(posStart, posEnd, color, thickneess);
}

void DrawDebugPoint(Vector2 const& pos, Color3ub const& color, float size)
{
	if (GDebugDraw)
		GDebugDraw->addPoint(pos, color, size);
}

void DrawDebugText(Vector2 const& pos, char const* text, Color3ub const& color)
{
	if (GDebugDraw)
		GDebugDraw->addText(pos, text, color);
}


void DrawDebugClear()
{
	if (GDebugDraw)
		GDebugDraw->clear();
}

