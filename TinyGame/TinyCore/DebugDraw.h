#pragma once
#ifndef DebugDraw_H_B5E1CE88_5AE6_4258_B002_5C83D29A054B
#define DebugDraw_H_B5E1CE88_5AE6_4258_B002_5C83D29A054B

#include "Math/Vector2.h"
#include "Core/Color.h"

using Math::Vector2;


TINY_API void DrawDebugLine(Vector2 const& sStart, Vector2 const& posEnd, Color3ub const& color, float thickneess = 0.0f);
TINY_API void DrawDebugPoint(Vector2 const& pos, Color3ub const& color, float size = 0.0f);
TINY_API void DrawDebugText(Vector2 const& pos, char const* text, Color3ub const& color);

class IGraphics2D;
TINY_API void DrawDebugCommit(IGraphics2D& g);

TINY_API void DrawDebugClear();

#endif // DebugDraw_H_B5E1CE88_5AE6_4258_B002_5C83D29A054B

