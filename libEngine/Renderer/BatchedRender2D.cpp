#include "BatchedRender2D.h"
#include "RHI/DrawUtility.h"


namespace Render
{

	void ShapeVertexBuilder::buildRect(Vector2 const& p1, Vector2 const& p2)
	{
		mBuffer.resize(4);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = mTransform.transformPosition(p);
			++pVertices;
		};

		EmitVertex(p1);
		EmitVertex(Vector2(p2.x, p1.y));
		EmitVertex(p2);
		EmitVertex(Vector2(p1.x, p2.y));
	}

	void ShapeVertexBuilder::buildCircle(Vector2 const& pos, float r, int numSegment)
	{
		mBuffer.resize(numSegment);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = p;
			++pVertices;
		};

		float theta = 2 * Math::PI / float(numSegment);
		Matrix2 rotation = Matrix2::Rotate(theta);

		Vector2 offset = Vector2(r, 0);
		for (int i = 0; i < numSegment; ++i)
		{
			EmitVertex(pos + offset);
			//apply the rotation matrix
			offset = rotation.leftMul(offset);
		}
	}

	void ShapeVertexBuilder::buildEllipse(Vector2 const& pos, float r, float yFactor, int numSegment)
	{
		mBuffer.resize(numSegment);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = mTransform.transformPosition(p);
			++pVertices;
		};

		float theta = 2 * Math::PI / float(numSegment);
		Matrix2 rotation = Matrix2::Rotate(theta);

		Vector2 offset = Vector2(r, 0);
		for (int i = 0; i < numSegment; ++i)
		{
			EmitVertex(pos + offset.mul(Vector2(1, yFactor)));
			//apply the rotation matrix
			offset = rotation.leftMul(offset);
		}
	}

	void ShapeVertexBuilder::buildRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius)
	{
		int numSegment = calcCircleSemgmentNum((circleRadius.x + circleRadius.y) / 2);
		float yFactor = float(circleRadius.y) / circleRadius.x;
		int num = numSegment / 4;

		mBuffer.clear();
		auto EmitVertex = [&](Vector2 const& p)
		{
			mBuffer.push_back(mTransform.transformPosition(p));
		};


		double theta = 2 * Math::PI / float(numSegment);
		double c = cos(theta);//precalculate the sine and cosine
		double s = sin(theta);

		Vector2 cvn[4] =
		{
			{ float(pos.x + rectSize.x - circleRadius.x) , float(pos.y + rectSize.y - circleRadius.y) },
			{ float(pos.x + circleRadius.x)              , float(pos.y + rectSize.y - circleRadius.y) },
			{ float(pos.x + circleRadius.x)              , float(pos.y + circleRadius.y) },
			{ float(pos.x + rectSize.x - circleRadius.x), float(pos.y + circleRadius.y) },
		};

		Vector2 v;
		double x, y;
		Vector2 cv = cvn[0];
		switch (0)
		{
		case 0: x = circleRadius.x; y = 0; break;
		case 1: x = 0; y = circleRadius.x; break;
		case 2: x = -circleRadius.x; y = 0; break;
		case 3: x = 0; y = -circleRadius.x; break;
		}

		for (int side = 0; side < 4; ++side)
		{
			v[0] = cv.x + x;
			v[1] = cv.y + yFactor * y;
			EmitVertex(v);

			for (int i = 0; i < num; ++i)
			{
				if (i == num / 2)
				{
					float tc, ts;
					Math::SinCos(theta * i, ts, tc);
					float x0, y0;
					switch (side)
					{
					case 0: x0 = circleRadius.x; y0 = 0; break;
					case 1: x0 = 0; y0 = circleRadius.x; break;
					case 2: x0 = -circleRadius.x; y0 = 0; break;
					case 3: x0 = 0; y0 = -circleRadius.x; break;
					}

					x = tc * x0 - ts * y0;
					y = ts * x0 + tc * y0;
				}
				else
				{
					double x0 = x;
					x = c * x0 - s * y;
					y = s * x0 + c * y;
				}

				v[0] = cv.x + x;
				v[1] = cv.y + yFactor * y;
				EmitVertex(v);
			}

			int next = (side == 3) ? 0 : side + 1;
			switch (next)
			{
			case 0: x = circleRadius.x; y = 0; break;
			case 1: x = 0; y = circleRadius.x; break;
			case 2: x = -circleRadius.x; y = 0; break;
			case 3: x = 0; y = -circleRadius.x; break;
			}

			cv = cvn[next];
			v[0] = cv.x + x;
			v[1] = cv.y + yFactor * y;
			EmitVertex(v);
		}
	}

	RenderBachedElement& RenderBachedElementList::addRect(ShapePaintArgs const& paintArgs, Vector2 const& min, Vector2 const& max)
	{
		TRenderBachedElement<RectPayload>* element = addElement< RectPayload >();
		element->type = RenderBachedElement::Rect;
		element->payload.paintArgs = paintArgs;
		element->payload.min = min;
		element->payload.max = max;
		return *element;
	}

	RenderBachedElement& RenderBachedElementList::addRoundRect(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius)
	{
		TRenderBachedElement<RoundRectPayload>* element = addElement< RoundRectPayload >();
		element->type = RenderBachedElement::RoundRect;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.rectSize = rectSize;
		element->payload.circleRadius = circleRadius;
		return *element;
	}

	RenderBachedElement& RenderBachedElementList::addCircle(ShapePaintArgs const& paintArgs, Vector2 const& pos, float radius)
	{
		TRenderBachedElement<CirclePayload>* element = addElement< CirclePayload >();
		element->type = RenderBachedElement::Circle;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.radius = radius;
		return *element;
	}

	RenderBachedElement& RenderBachedElementList::addEllipse(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& size)
	{
		TRenderBachedElement<EllipsePayload>* element = addElement< EllipsePayload >();
		element->type = RenderBachedElement::Ellipse;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.size = size;
		return *element;
	}

	RenderBachedElement& RenderBachedElementList::addTextureRect(Color4f const& color, Vector2 const& min, Vector2 const& max, Vector2 const& uvMin, Vector2 const& uvMax)
	{
		TRenderBachedElement<TextureRectPayload>* element = addElement< TextureRectPayload >();
		element->type = RenderBachedElement::TextureRect;
		element->payload.color = color;
		element->payload.min = min;
		element->payload.max = max;
		element->payload.uvMin = uvMin;
		element->payload.uvMax = uvMax;
		return *element;
	}

	RenderBachedElement& RenderBachedElementList::addLine(Color4f const& color, Vector2 const& p1, Vector2 const& p2, int width)
	{
		TRenderBachedElement<LinePayload>* element = addElement< LinePayload >();
		element->type = RenderBachedElement::Line;
		element->payload.color = color;
		element->payload.p1 = p1;
		element->payload.p2 = p2;
		element->payload.width = width;
		return *element;
	}

	RenderBachedElement& RenderBachedElementList::addText( Color4f const& color, std::vector< FontVertex >&& vertices)
	{
		TRenderBachedElement<TextPayload>* element = addElement< TextPayload >();
		element->type = RenderBachedElement::Text;
		element->payload.color = color;
		element->payload.vertices = std::move(vertices);
		return *element;
	}

	void RenderBachedElementList::releaseElements()
	{
		for (auto element : mElements)
		{
			switch (element->type)
			{
			case RenderBachedElement::Rect:
				TypeDataHelper::Destruct< TRenderBachedElement<RectPayload> >(element);
				break;
			case RenderBachedElement::Circle:
				TypeDataHelper::Destruct< TRenderBachedElement<CirclePayload> >(element);
				break;
			case RenderBachedElement::Polygon:
				TypeDataHelper::Destruct< TRenderBachedElement<PolygonPayload> >(element);
				break;
			case RenderBachedElement::Ellipse:
				TypeDataHelper::Destruct< TRenderBachedElement<EllipsePayload> >(element);
				break;
			case RenderBachedElement::RoundRect:
				TypeDataHelper::Destruct< TRenderBachedElement<RoundRectPayload> >(element);
				break;
			case RenderBachedElement::TextureRect:
				TypeDataHelper::Destruct< TRenderBachedElement<TextureRectPayload> >(element);
				break;
			case RenderBachedElement::Line:
				TypeDataHelper::Destruct< TRenderBachedElement<LinePayload> >(element);
				break;
			case RenderBachedElement::Text:
				TypeDataHelper::Destruct< TRenderBachedElement<TextPayload> >(element);
				break;
			}
		}

		mElements.clear();
		mAllocator.clearFrame();
	}

	void BatchedRender::render(RHICommandList& commandList, RenderBachedElementList& elementList)
	{
		for (RenderBachedElement* element : elementList.mElements)
		{
			switch (element->type)
			{
			case RenderBachedElement::Rect:
				{
					RenderBachedElementList::RectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::RectPayload >(element);
					Vector2 vertices[4];
					vertices[0] = element->transform.transformPosition(payload.min);
					vertices[1] = element->transform.transformPosition(Vector2(payload.min.x, payload.max.y));
					vertices[2] = element->transform.transformPosition(payload.max);
					vertices[3] = element->transform.transformPosition(Vector2(payload.max.x, payload.min.y));
					emitRect(vertices, payload.paintArgs);
				}
				break;
			case RenderBachedElement::Polygon:
				{
					RenderBachedElementList::PolygonPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::PolygonPayload >(element);
					emitPolygon(payload.posList.data(), payload.posList.size(), payload.paintArgs);
				}
				break;
			case RenderBachedElement::Circle:
				{
					RenderBachedElementList::CirclePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::CirclePayload >(element);
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.buildCircle(payload.pos, payload.radius, calcCircleSemgmentNum(payload.radius));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBachedElement::Ellipse:
				{
					RenderBachedElementList::EllipsePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::EllipsePayload >(element);
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					float yFactor = float(payload.size.y) / payload.size.x;
					builder.buildEllipse(payload.pos, payload.size.x, yFactor, calcCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBachedElement::RoundRect:
				{
					RenderBachedElementList::RoundRectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::RoundRectPayload >(element);
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					builder.buildRoundRect(payload.pos, payload.rectSize, payload.circleRadius);
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBachedElement::TextureRect:
				{
					RenderBachedElementList::TextureRectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::TextureRectPayload >(element);
					int baseIndex = mTexVerteices.size();
					mTexVerteices.resize(baseIndex + 4);
					TexVertex* pVertices = &mTexVerteices[baseIndex];
					pVertices[0] = { element->transform.transformPosition(payload.min) , payload.color , payload.uvMin };
					pVertices[1] = { element->transform.transformPosition(Vector2(payload.min.x, payload.max.y)), payload.color , Vector2(payload.uvMin.x , payload.uvMax.y) };
					pVertices[2] = { element->transform.transformPosition(payload.max), payload.color , payload.uvMax };
					pVertices[3] = { element->transform.transformPosition(Vector2(payload.max.x, payload.min.y)), payload.color , Vector2(payload.uvMax.x , payload.uvMin.y) };

					uint32* pIndices = fetchIndexBuffer(6);
					FillQuad(pIndices, baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 3);
				}
				break;
			case RenderBachedElement::Line:
				{
					RenderBachedElementList::LinePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::LinePayload >(element);

					Vector2 positions[2];
					positions[0] = element->transform.transformPosition(payload.p1);
					positions[1] = element->transform.transformPosition(payload.p2);

					float halfWidth = 0.5 * float(payload.width);
					Vector2 offset[4] = { Vector2(-halfWidth,-halfWidth) , Vector2(-halfWidth,halfWidth)  , Vector2(halfWidth,halfWidth), Vector2(halfWidth,-halfWidth) };

					int baseIndex;
					BaseVertex* pVertices = fetchBaseBuffer(4 * 2, baseIndex);
					for (int i = 0; i < 2; ++i)
					{
						pVertices[0] = { positions[i] + offset[0] , payload.color };
						pVertices[1] = { positions[i] + offset[1] , payload.color };
						pVertices[2] = { positions[i] + offset[2] , payload.color };
						pVertices[3] = { positions[i] + offset[3] , payload.color };
						pVertices += 4;
					}

					uint32* pIndices = fetchIndexBuffer(4 * 6);
					EmitLineShapeIndices(pIndices, baseIndex, baseIndex + 4);
				}
				break;
			case RenderBachedElement::Text:
				{
					RenderBachedElementList::TextPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::TextPayload >(element);

					int numChar = payload.vertices.size() / 4;
					int baseIndex;
					TexVertex* pVertices = fetchTexBuffer(4 * numChar, baseIndex);
					FontVertex* pSrcVertices = payload.vertices.data();
					uint32* pIndices = fetchIndexBuffer(numChar * 6);
					for (int i = 0; i < numChar; ++i)
					{
						pVertices[0] = { element->transform.transformPosition(pSrcVertices[0].pos) , payload.color , pSrcVertices[0].uv };
						pVertices[1] = { element->transform.transformPosition(pSrcVertices[1].pos) , payload.color , pSrcVertices[1].uv };
						pVertices[2] = { element->transform.transformPosition(pSrcVertices[2].pos) , payload.color , pSrcVertices[2].uv };
						pVertices[3] = { element->transform.transformPosition(pSrcVertices[3].pos) , payload.color , pSrcVertices[3].uv };

						pVertices += 4;
						pSrcVertices += 4;

						FillQuad(pIndices, baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 3);
						pIndices += 6;
						baseIndex += 4;
					}
				}
				break;
			}
		}

		flushDrawCommond(commandList);
	}

	void BatchedRender::flushDrawCommond(RHICommandList& commandList)
	{
		//CHECK(mTexVerteices.size() || mBaseVertices.size());
		if (!mBaseVertices.empty())
		{
			TRenderRT< RTVF_XY_CA >::DrawIndexed(commandList, EPrimitive::TriangleList, mBaseVertices.data(), mBaseVertices.size(), mBaseIndices.data(), mBaseIndices.size());
			mBaseVertices.clear();
			mBaseIndices.clear();
		}
		else if (!mTexVerteices.empty())
		{
			TRenderRT< RTVF_XY_CA_T2 >::DrawIndexed(commandList, EPrimitive::TriangleList, mTexVerteices.data(), mTexVerteices.size(), mBaseIndices.data(), mBaseIndices.size());
			mTexVerteices.clear();
			mBaseIndices.clear();
		}
	}

	void BatchedRender::emitPolygon(Vector2 v[], int numV, Color4f const& color)
	{
		assert(numV > 2);
		int baseIndex;
		BaseVertex* pVertices = fetchBaseBuffer(numV, baseIndex);
		for (int i = 0; i < numV; ++i)
		{
			BaseVertex& vtx = mBaseVertices[baseIndex + i];
			pVertices->pos = v[i];
			pVertices->color = color;
			++pVertices;
		}

		int numTriangle = (numV - 2);
		uint32* pIndices = fetchIndexBuffer(3 * numTriangle);
		for (int i = 0; i < numTriangle; ++i)
		{
			pIndices[0] = baseIndex;
			pIndices[1] = baseIndex + i + 1;
			pIndices[2] = baseIndex + i + 2;
			pIndices += 3;
		}
	}

	void BatchedRender::emitPolygon(Vector2 v[], int numV, ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			emitPolygon(v, numV, paintArgs.brushColor);
		}
		if (paintArgs.bUsePen)
		{
			emitPolygonLine(v, numV, paintArgs.penColor, paintArgs.penWidth);
		}
	}

	void BatchedRender::emitRect(Vector2 v[], ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			int baseIndex;
			BaseVertex* pVertices = fetchBaseBuffer(4, baseIndex);
			pVertices[0] = { v[0] , paintArgs.brushColor };
			pVertices[1] = { v[1] , paintArgs.brushColor };
			pVertices[2] = { v[2] , paintArgs.brushColor };
			pVertices[3] = { v[3] , paintArgs.brushColor };

			uint32* pIndices = fetchIndexBuffer(3 * 2);
			FillQuad(pIndices, baseIndex + 0 , baseIndex + 1 , baseIndex + 2 , baseIndex + 3);
		}

		if (paintArgs.bUsePen)
		{
			float halfWidth = 0.5 * float(paintArgs.penWidth);
			int baseIndex;
			BaseVertex* pVertices = fetchBaseBuffer(4 * 2, baseIndex);
			for (int i = 0; i < 4; ++i)
			{
				Vector2 e1 = Math::GetNormal( v[(i + 1) % 4] - v[i] );
				Vector2 e2 = Math::GetNormal( v[(i + 3) % 4] - v[i] );
				Vector2 offset = halfWidth * (e1 + e2);
				pVertices[0] = { v[i] + offset , paintArgs.penColor };
				pVertices[1] = { v[i] - offset , paintArgs.penColor };
				pVertices += 2;
			}

			uint32* pIndices = fetchIndexBuffer(3 * 2 * 4);
			FillQuad(pIndices, baseIndex + 0, baseIndex + 1, baseIndex + 3, baseIndex + 2); 
			pIndices += 6;
			FillQuad(pIndices, baseIndex + 2, baseIndex + 3, baseIndex + 5, baseIndex + 4); 
			pIndices += 6;
			FillQuad(pIndices, baseIndex + 4, baseIndex + 5, baseIndex + 7, baseIndex + 6);
			pIndices += 6;
			FillQuad(pIndices, baseIndex + 6, baseIndex + 7, baseIndex + 1, baseIndex + 0);
		}
	}

	void BatchedRender::emitPolygonLine(Vector2 v[], int numV, Color4ub const& color, int lineWidth)
	{
		float halfWidth = 0.5 * float(lineWidth);
#if 1
		int baseIndex;
		BaseVertex* pVertices = fetchBaseBuffer(numV * 2, baseIndex);
		int index = 0;
		for (int i = 0; i < numV; ++i)
		{
			Vector2 e1 = v[(i + 1) % numV] - v[i];
			if (e1.normalize() < 1e-5)
			{
				e1 = v[(i + 2) % numV] - v[i];
				e1.normalize();
			}
			Vector2 e2 = Math::GetNormal(v[(i + numV - 1) % numV] - v[i]);
			if (e2.normalize() < 1e-5)
			{
				e2 = v[(i + numV - 2) % numV] - v[i];
				e2.normalize();
			}
			Vector2 offsetDir = e1 + e2;
			Vector2 offset;
			if (offsetDir.normalize() > 1e-5)
			{
				float s = Math::Abs(e1.cross(offsetDir));
				offset = (halfWidth / s) * offsetDir;
			}
			else
			{
				offset = Vector2(e1.y, -e2.x) * halfWidth;
			}
			
			pVertices[0] = { v[i] + offset , color };
			pVertices[1] = { v[i] - offset , color };
			pVertices += 2;
		}

		int baseIndexStart = baseIndex;
		uint32* pIndices = fetchIndexBuffer(3 * 2 * numV);
		for (int i = 0; i < numV - 1; ++i)
		{
			FillQuad(pIndices, baseIndex + 0, baseIndex + 1, baseIndex + 3, baseIndex + 2);
			pIndices += 6;
			baseIndex += 2;
		}
		FillQuad(pIndices, baseIndex , baseIndex + 1, baseIndexStart + 1, baseIndexStart + 0);

#else

	
		Vector2 offset[4] = { Vector2(-halfWidth,-halfWidth) , Vector2(-halfWidth,halfWidth)  , Vector2(halfWidth,halfWidth), Vector2(halfWidth,-halfWidth) };

		int baseIndex;
		BaseVertex* pVertices = fetchBaseBuffer(4 * numV, baseIndex);
		for (int i = 0; i < numV; ++i)
		{
			pVertices[0] = { v[i] + offset[0] , color };
			pVertices[1] = { v[i] + offset[1] , color };
			pVertices[2] = { v[i] + offset[2] , color };
			pVertices[3] = { v[i] + offset[3] , color };
			pVertices += 4;
		}

		int* pIndices = fetchIndexBuffer(4 * 6 * numV);
		int indexCur = baseIndex;
		for (int i = 0; i < numV - 1; ++i)
		{
			EmitLineShapeIndices(pIndices, indexCur, indexCur + 4);
			indexCur += 4;
		}
		EmitLineShapeIndices(pIndices, indexCur, baseIndex);
#endif
	}

}// namespace Render