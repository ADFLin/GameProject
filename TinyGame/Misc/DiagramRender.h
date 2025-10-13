#pragma once

#ifndef DiagramRender_H_87A0F6DC_7EFA_496C_9C8E_0BED09BB7ACD
#define DiagramRender_H_87A0F6DC_7EFA_496C_9C8E_0BED09BB7ACD

#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"

namespace Render
{

	struct Diagram
	{

		Vector2 min = Vector2(-2, -2);
		Vector2 max = Vector2(2, 2);
		Vector2 border = Vector2(0.1, 0.1);

		void setup(RHICommandList& commandList, Vector2 renderPos, Vector2 renderSize)
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			RHISetViewport(commandList, renderPos.x, screenSize.y - (renderPos.y + renderSize.y), renderSize.x, renderSize.y);
			Matrix4 matProj = AdjustProjectionMatrixForRHI(OrthoMatrix(min.x - border.x, max.x + border.x, min.y - border.y, max.y + border.y, -1, 1));
			RHISetFixedShaderPipelineState(commandList, matProj);
		}

		void drawGird(RHICommandList& commandList, Vector2 basePoint, Vector2 size)
		{
			if (size.x == 0 || size.y == 0)
				return;

			static TArray<Vector2> buffer;
			buffer.clear();

			float v;
			v = basePoint.x;
			while (v >= min.x)
			{
				buffer.emplace_back(v, min.y);
				buffer.emplace_back(v, max.y);
				v -= size.x;
			}

			v = basePoint.x;
			while (v <= max.x)
			{
				buffer.emplace_back(v, min.y);
				buffer.emplace_back(v, max.y);
				v += size.x;
			}

			v = basePoint.y;
			while (v >= min.y)
			{
				buffer.emplace_back(min.x, v);
				buffer.emplace_back(max.x, v);
				v -= size.y;
			}

			v = basePoint.y;
			while (v <= max.y)
			{
				buffer.emplace_back(min.x, v);
				buffer.emplace_back(max.x, v);
				v += size.y;
			}

			if (!buffer.empty())
			{
				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &buffer[0], buffer.size(), LinearColor(0, 0, 1));
			}

			{
				Vector2 const lines[] =
				{
					Vector2(min.x,min.y), Vector2(min.x,max.y),
					Vector2(max.x,min.y), Vector2(max.x,max.y),
					Vector2(min.x,min.y) , Vector2(max.x,min.y),
					Vector2(min.x,max.y) , Vector2(max.x,max.y),
				};
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineList, &lines[0], ARRAY_SIZE(lines), LinearColor(1, 0, 0));
			}

		}

		void drawCurve(RHICommandList& commandList, TArray< Vector2 > const& points, LinearColor const& color)
		{
			TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineStrip, points.data(), points.size(), color);
		}
	};

}



#endif // DiagramRender_H_87A0F6DC_7EFA_496C_9C8E_0BED09BB7ACD
