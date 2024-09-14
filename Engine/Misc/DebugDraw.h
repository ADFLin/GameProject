#pragma once
#ifndef DebugDraw_H_A4AB8279_2BAA_409E_8AEE_1580322C3D9A
#define DebugDraw_H_A4AB8279_2BAA_409E_8AEE_1580322C3D9A


#include "Math/Vector2.h"
#include "Core/Color.h"
#include "CoreShare.h"

using Math::Vector2;

void DrawDebugLine(Vector2 const& sStart, Vector2 const& posEnd, Color3ub const& color, float thickneess = 0.0f);
void DrawDebugPoint(Vector2 const& pos, Color3ub const& color, float size = 0.0f);
void DrawDebugText(Vector2 const& pos, char const* text, Color3ub const& color);
void DrawDebugClear();

class IDebugDrawBatcher
{
public:
	virtual void addPoint(Vector2 const& pos, Color3ub const& color, float size) = 0;
	virtual void addLine(Vector2 const& posStart, Vector2 const& posEnd, Color3ub const& color, float thickneess) = 0;
	virtual void addText(Vector2 const& pos, char const* text, Color3ub const& color) = 0;
	virtual void clear() = 0;

	static CORE_API void Set(IDebugDrawBatcher* debugDraw);
};

#endif // DebugDraw_H_A4AB8279_2BAA_409E_8AEE_1580322C3D9A
