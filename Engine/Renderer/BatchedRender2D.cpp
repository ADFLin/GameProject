#include "BatchedRender2D.h"
#include "RHI/DrawUtility.h"
#include "ConsoleSystem.h"
#include "CoreShare.h"

//#TODO : Heap Corrupted when show console frame draw Round Rect

#define USE_NEW_LINE_IDNEX 1

namespace Render
{
	constexpr int MaxBaseVerticesCount = 2048;
	constexpr int MaxTexVerticesCount = 2048;
	constexpr int MaxIndicesCount = 2048 * 3;

	CORE_API extern TConsoleVariable< bool > CVarUseBufferResource;

#if CORE_SHARE_CODE
	CORE_API TConsoleVariable< bool > CVarUseBufferResource{ true , "g.BatchedRenderUseBuffer", CVF_TOGGLEABLE };
#endif

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
			*pVertices = mTransform.transformPosition(p);
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

	RenderBatchedElement& RenderBachedElementList::addRect(ShapePaintArgs const& paintArgs, Vector2 const& min, Vector2 const& max)
	{
		TRenderBatchedElement<RectPayload>* element = addElement< RectPayload >();
		element->type = RenderBatchedElement::Rect;
		element->payload.paintArgs = paintArgs;
		element->payload.min = min;
		element->payload.max = max;
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addRoundRect(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius)
	{
		TRenderBatchedElement<RoundRectPayload>* element = addElement< RoundRectPayload >();
		element->type = RenderBatchedElement::RoundRect;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.rectSize = rectSize;
		element->payload.circleRadius = circleRadius;
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addCircle(ShapePaintArgs const& paintArgs, Vector2 const& pos, float radius)
	{
		TRenderBatchedElement<CirclePayload>* element = addElement< CirclePayload >();
		element->type = RenderBatchedElement::Circle;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.radius = radius;
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addEllipse(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& size)
	{
		TRenderBatchedElement<EllipsePayload>* element = addElement< EllipsePayload >();
		element->type = RenderBatchedElement::Ellipse;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.size = size;
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addTextureRect(Color4Type const& color, Vector2 const& min, Vector2 const& max, Vector2 const& uvMin, Vector2 const& uvMax)
	{
		TRenderBatchedElement<TextureRectPayload>* element = addElement< TextureRectPayload >();
		element->type = RenderBatchedElement::TextureRect;
		element->payload.color = color;
		element->payload.min = min;
		element->payload.max = max;
		element->payload.uvMin = uvMin;
		element->payload.uvMax = uvMax;
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addLine(Color4Type const& color, Vector2 const& p1, Vector2 const& p2, int width)
	{
		TRenderBatchedElement<LinePayload>* element = addElement< LinePayload >();
		element->type = RenderBatchedElement::Line;
		element->payload.color = color;
		element->payload.p1 = p1;
		element->payload.p2 = p2;
		element->payload.width = width;
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addText(Color4Type const& color, TArray< FontVertex >&& vertices)
	{
		TRenderBatchedElement<TextPayload>* element = addElement< TextPayload >();
		element->type = RenderBatchedElement::Text;
		element->payload.color = color;
#if BATCHED_ELEMENT_NO_DESTRUCT
		element->payload.vertices = (FontVertex*)mAllocator.alloc(vertices.size() * sizeof(FontVertex));
		FMemory::Copy(element->payload.vertices, vertices.data(), vertices.size() * sizeof(FontVertex));
		element->payload.verticesCount = vertices.size();
#else
		element->payload.vertices = std::move(vertices);
#endif
		return *element;
	}

	RenderBatchedElement& RenderBachedElementList::addGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad)
	{
		TRenderBatchedElement<GradientRectPayload>* element = addElement< GradientRectPayload >();
		element->type = RenderBatchedElement::GradientRect;
		element->payload.posLT = posLT;
		element->payload.posRB = posRB;
		element->payload.colorLT = colorLT;
		element->payload.colorRB = colorRB;
		element->payload.bHGrad = bHGrad;
		return *element;
	}

	void RenderBachedElementList::releaseElements()
	{

#if BATCHED_ELEMENT_NO_DESTRUCT

#else
		for (auto element : mDestructElements)
		{
			switch (element->type)
			{
			case RenderBatchedElement::Rect:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<RectPayload> >(element);
				break;
			case RenderBatchedElement::Circle:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<CirclePayload> >(element);
				break;
			case RenderBatchedElement::Polygon:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<PolygonPayload> >(element);
				break;
			case RenderBatchedElement::Ellipse:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<EllipsePayload> >(element);
				break;
			case RenderBatchedElement::RoundRect:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<RoundRectPayload> >(element);
				break;
			case RenderBatchedElement::TextureRect:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<TextureRectPayload> >(element);
				break;
			case RenderBatchedElement::Line:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<LinePayload> >(element);
				break;
			case RenderBatchedElement::Text:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<TextPayload> >(element);
				break;
			case RenderBatchedElement::LineStrip:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<LineStripPayload> >(element);
				break;
			case RenderBatchedElement::GradientRect:
				FTypeMemoryOp::Destruct< TRenderBatchedElement<GradientRectPayload> >(element);
				break;
			}
		}

		mDestructElements.clear();
#endif
		mElements.clear();
		mAllocator.clearFrame();
	}

	BatchedRender::BatchedRender()
	{
		mCachedPositionList.reserve(1024);
		mBaseVertices.reserve(1024);
		mBaseIndices.reserve(1024 * 3);
		mTexVerteices.reserve(1024);
	}

	void BatchedRender::initializeRHI()
	{
		mBaseVertexBuffer.initialize(MaxBaseVerticesCount, BCF_CpuAccessWrite | BCF_UsageVertex); 
		mTexVertexBuffer.initialize(MaxTexVerticesCount, BCF_CpuAccessWrite | BCF_UsageVertex);
		mIndexBuffer.initialize(MaxIndicesCount, BCF_CpuAccessWrite | BCF_UsageIndex);
	}

	void BatchedRender::releaseRHI()
	{
		mBaseVertexBuffer.release();
		mTexVertexBuffer.release();
		mIndexBuffer.release();
	}

	void BatchedRender::render(RHICommandList& commandList, RenderBachedElementList& elementList)
	{
		mCommandList = &commandList;
		if (CVarUseBufferResource)
		{
			bUseBuffer = mBaseVertexBuffer.lock() &&
			mTexVertexBuffer.lock() &&
			mIndexBuffer.lock();
		}
		else
		{
			bUseBuffer = false;
		}
		for (RenderBatchedElement* element : elementList.mElements)
		{
			switch (element->type)
			{
			case RenderBatchedElement::Rect:
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
			case RenderBatchedElement::Polygon:
				{
					RenderBachedElementList::PolygonPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::PolygonPayload >(element);
#if BATCHED_ELEMENT_NO_DESTRUCT
					emitPolygon(payload.posList, payload.posCount, payload.paintArgs);
#else
					emitPolygon(payload.posList.data(), payload.posList.size(), payload.paintArgs);
#endif
				}
				break;
			case RenderBatchedElement::Circle:
				{
					RenderBachedElementList::CirclePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::CirclePayload >(element);
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					builder.buildCircle(payload.pos, payload.radius, calcCircleSemgmentNum(payload.radius));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBatchedElement::Ellipse:
				{
					RenderBachedElementList::EllipsePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::EllipsePayload >(element);
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					float yFactor = float(payload.size.y) / payload.size.x;
					builder.buildEllipse(payload.pos, payload.size.x, yFactor, calcCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBatchedElement::RoundRect:
				{
					RenderBachedElementList::RoundRectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::RoundRectPayload >(element);
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					builder.buildRoundRect(payload.pos, payload.rectSize, payload.circleRadius);
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBatchedElement::TextureRect:
				{
					RenderBachedElementList::TextureRectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::TextureRectPayload >(element);
					int baseIndex;
					TexVertex* pVertices = fetchTexVertex(4, baseIndex);
					pVertices[0] = { element->transform.transformPosition(payload.min) , payload.color , payload.uvMin };
					pVertices[1] = { element->transform.transformPosition(Vector2(payload.min.x, payload.max.y)), payload.color , Vector2(payload.uvMin.x , payload.uvMax.y) };
					pVertices[2] = { element->transform.transformPosition(payload.max), payload.color , payload.uvMax };
					pVertices[3] = { element->transform.transformPosition(Vector2(payload.max.x, payload.min.y)), payload.color , Vector2(payload.uvMax.x , payload.uvMin.y) };

					uint32* pIndices = fetchIndex(6);
					FillQuad(pIndices, baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 3);
				}
				break;
			case RenderBatchedElement::Line:
				{
					RenderBachedElementList::LinePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::LinePayload >(element);

					Vector2 positions[2];
					positions[0] = element->transform.transformPosition(payload.p1);
					positions[1] = element->transform.transformPosition(payload.p2);

					float halfWidth = 0.5 * float(payload.width);
					Vector2 offset[4] = 
					{ 
						Vector2(-halfWidth,-halfWidth) , 
						Vector2(-halfWidth,halfWidth)  , 
						Vector2(halfWidth,halfWidth), 
						Vector2(halfWidth,-halfWidth) 
					};

#if USE_NEW_LINE_IDNEX
					Vector2 dir = positions[1] - positions[0];
					int index0 = dir.x < 0 ? (dir.y < 0 ? 0 : 1) : (dir.y < 0 ? 3 : 2);
					int index1 = (index0 + 1) % 4;
					int index2 = (index0 + 2) % 4;
					int index3 = (index0 + 3) % 4;

					int baseIndex;
					BaseVertex* pVertices = fetchBaseVertex(3 * 2, baseIndex);
					pVertices[0] = { positions[0] + offset[index2] , payload.color };
					pVertices[1] = { positions[0] + offset[index3] , payload.color };
					pVertices[2] = { positions[0] + offset[index1] , payload.color };
					pVertices[3] = { positions[1] + offset[index0] , payload.color };
					pVertices[4] = { positions[1] + offset[index1] , payload.color };
					pVertices[5] = { positions[1] + offset[index3] , payload.color };

					uint32* pIndices = fetchIndex(4 * 3);
					pIndices = FillTriangle(pIndices, baseIndex, 0, 1, 2);
					pIndices = FillTriangle(pIndices, baseIndex, 3, 4, 5);
					pIndices = FillQuad(pIndices, baseIndex, 2, 1, 5, 4);
#else
					int baseIndex;
					BaseVertex* pVertices = fetchBaseVertex(4 * 2, baseIndex);
					for (int i = 0; i < 2; ++i)
					{
						pVertices[0] = { positions[i] + offset[0] , payload.color };
						pVertices[1] = { positions[i] + offset[1] , payload.color };
						pVertices[2] = { positions[i] + offset[2] , payload.color };
						pVertices[3] = { positions[i] + offset[3] , payload.color };
						pVertices += 4;
					}

					uint32* pIndices = fetchIndex(4 * 6);
					FillLineShapeIndices(pIndices, baseIndex, baseIndex + 4);
#endif
				}
				break;
			case RenderBatchedElement::LineStrip:
				{
					RenderBachedElementList::LineStripPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::LineStripPayload >(element);
#if BATCHED_ELEMENT_NO_DESTRUCT
					int lineCount = payload.posCount - 1;
#else
					int lineCount = payload.posList.size() - 1;
#endif
					float halfWidth = 0.5 * float(payload.width);
					Vector2 offset[4] = 
					{ 
						Vector2(-halfWidth,-halfWidth) , 
						Vector2(-halfWidth,halfWidth)  , 
						Vector2(halfWidth,halfWidth), 
						Vector2(halfWidth,-halfWidth) 
					};

#if USE_NEW_LINE_IDNEX
					int baseIndex;
					BaseVertex* pVertices = fetchBaseVertex(3 * 2 * lineCount, baseIndex);
					uint32* pIndices = fetchIndex(4 * 3 * lineCount);
#else
					BaseVertex* pVertices = fetchBaseVertex(4 * 2, baseIndex);
					uint32* pIndices = fetchIndex(4 * 6 * lineCount);
#endif

					for( int i = 0 ; i < lineCount; ++i )
					{
						Vector2 positions[2];
						positions[0] = payload.posList[i];
						positions[1] = payload.posList[i+1];
#if USE_NEW_LINE_IDNEX
						Vector2 dir = positions[1] - positions[0];
						int index0 = dir.x < 0 ? (dir.y < 0 ? 0 : 1) : (dir.y < 0 ? 3 : 2);
						int index1 = (index0 + 1) % 4;
						int index2 = (index0 + 2) % 4;
						int index3 = (index0 + 3) % 4;

						pVertices[0] = { positions[0] + offset[index2] , payload.color };
						pVertices[1] = { positions[0] + offset[index3] , payload.color };
						pVertices[2] = { positions[0] + offset[index1] , payload.color };
						pVertices[3] = { positions[1] + offset[index0] , payload.color };
						pVertices[4] = { positions[1] + offset[index1] , payload.color };
						pVertices[5] = { positions[1] + offset[index3] , payload.color };
						pVertices += 6;

						pIndices = FillTriangle(pIndices, baseIndex, 0, 1, 2);
						pIndices = FillTriangle(pIndices, baseIndex, 3, 4, 5);
						pIndices = FillQuad(pIndices, baseIndex, 2, 1, 5, 4);
						baseIndex += 6;
#else
						for (int i = 0; i < 2; ++i)
						{
							pVertices[0] = { positions[i] + offset[0] , payload.color };
							pVertices[1] = { positions[i] + offset[1] , payload.color };
							pVertices[2] = { positions[i] + offset[2] , payload.color };
							pVertices[3] = { positions[i] + offset[3] , payload.color };
							pVertices += 4;
						}

						pIndices = FillLineShapeIndices(pIndices, baseIndex, baseIndex + 4);
						baseIndex += 8;
#endif
					}
				}
				break;
			case RenderBatchedElement::Text:
				{
					RenderBachedElementList::TextPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::TextPayload >(element);

#if USE_NEW_LINE_IDNEX
					int numChar = payload.verticesCount / 4;
					FontVertex* pSrcVertices = payload.vertices;
#else
					int numChar = payload.vertices.size() / 4;
					FontVertex* pSrcVertices = payload.vertices.data();
#endif
					int baseIndex;
					TexVertex* pVertices = fetchTexVertex(4 * numChar, baseIndex);
					uint32* pIndices = fetchIndex(numChar * 6);
					for (int i = 0; i < numChar; ++i)
					{
						pVertices[0] = { element->transform.transformPosition(pSrcVertices[0].pos) , payload.color , pSrcVertices[0].uv };
						pVertices[1] = { element->transform.transformPosition(pSrcVertices[1].pos) , payload.color , pSrcVertices[1].uv };
						pVertices[2] = { element->transform.transformPosition(pSrcVertices[2].pos) , payload.color , pSrcVertices[2].uv };
						pVertices[3] = { element->transform.transformPosition(pSrcVertices[3].pos) , payload.color , pSrcVertices[3].uv };

						pVertices += 4;
						pSrcVertices += 4;

						pIndices = FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
						baseIndex += 4;
					}
				}
				break;
			case RenderBatchedElement::GradientRect:
				{
					RenderBachedElementList::GradientRectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::GradientRectPayload >(element);

					int baseIndex;
					BaseVertex* pVertices = fetchBaseVertex(4, baseIndex);
					pVertices[0] = { element->transform.transformPosition(payload.posLT) , payload.colorLT };
					pVertices[1] = { element->transform.transformPosition(Vector2(payload.posLT.x, payload.posRB.y)), payload.bHGrad ? payload.colorLT : payload.colorRB };
					pVertices[2] = { element->transform.transformPosition(payload.posRB), payload.colorRB };
					pVertices[3] = { element->transform.transformPosition(Vector2(payload.posRB.x, payload.posLT.y)),  payload.bHGrad ? payload.colorRB : payload.colorLT };

					uint32* pIndices = fetchIndex(6);
					pIndices = FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
				}
			}
		}

		if (bUseBuffer)
		{
			flushDrawCommandForBuffer(true);
		}
		else
		{
			flushDrawCommand();
		}
	}

	void BatchedRender::emitPolygon(Vector2 v[], int numV, Color4Type const& color)
	{
		assert(numV > 2);
		int baseIndex;
		BaseVertex* pVertices = fetchBaseVertex(numV, baseIndex);
		for (int i = 0; i < numV; ++i)
		{
			BaseVertex& vtx = pVertices[i];
			pVertices->pos = v[i];
			pVertices->color = color;
			++pVertices;
		}

		int numTriangle = (numV - 2);
		uint32* pIndices = fetchIndex(3 * numTriangle);
		for (int i = 0; i < numTriangle; ++i)
		{
			pIndices = FillTriangle(pIndices, baseIndex, 0, i + 1, i + 2);
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
			BaseVertex* pVertices = fetchBaseVertex(4, baseIndex);
			pVertices[0] = { v[0] , paintArgs.brushColor };
			pVertices[1] = { v[1] , paintArgs.brushColor };
			pVertices[2] = { v[2] , paintArgs.brushColor };
			pVertices[3] = { v[3] , paintArgs.brushColor };

			uint32* pIndices = fetchIndex(3 * 2);
			FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
		}

		if (paintArgs.bUsePen)
		{
			float halfWidth = 0.5 * float(paintArgs.penWidth);
			int baseIndex;
			BaseVertex* pVertices = fetchBaseVertex(4 * 2, baseIndex);
			for (int i = 0; i < 4; ++i)
			{
				Vector2 e1 = Math::GetNormal( v[(i + 1) % 4] - v[i] );
				Vector2 e2 = Math::GetNormal( v[(i + 3) % 4] - v[i] );
				Vector2 offset = halfWidth * (e1 + e2);
				pVertices[0] = { v[i] + offset , paintArgs.penColor };
				pVertices[1] = { v[i] - offset , paintArgs.penColor };
				pVertices += 2;
			}

			uint32* pIndices = fetchIndex(3 * 2 * 4);
			pIndices = FillQuad(pIndices, baseIndex, 0, 1, 3, 2);
			pIndices = FillQuad(pIndices, baseIndex, 2, 3, 5, 4);
			pIndices = FillQuad(pIndices, baseIndex, 4, 5, 7, 6);
			pIndices = FillQuad(pIndices, baseIndex, 6, 7, 1, 0);
		}
	}

	BatchedRender::BaseVertex* BatchedRender::fetchBaseVertex(int size, int& baseIndex)
	{
		if (bUseBuffer)
		{
			if (!mBaseVertexBuffer.canFetch(size))
			{
				flushDrawCommandForBuffer(false);
			}

			baseIndex = mBaseVertexBuffer.usedCount;
			return mBaseVertexBuffer.fetch(size);
		}
		else
		{
			baseIndex = mBaseVertices.size();
			mBaseVertices.resize(baseIndex + size);
			return &mBaseVertices[baseIndex];
		}

	}

	BatchedRender::TexVertex* BatchedRender::fetchTexVertex(int size, int& baseIndex)
	{
		if (bUseBuffer)
		{
			if (!mTexVertexBuffer.canFetch(size))
			{
				flushDrawCommandForBuffer(false);
			}

			baseIndex = mTexVertexBuffer.usedCount;
			return mTexVertexBuffer.fetch(size);
		}
		else
		{
			baseIndex = mTexVerteices.size();
			mTexVerteices.resize(baseIndex + size);
			return &mTexVerteices[baseIndex];
		}
	}

	uint32* BatchedRender::fetchIndex(int size)
	{
		if (bUseBuffer)
		{
			if (!mIndexBuffer.canFetch(size))
			{
				flushDrawCommandForBuffer(false);
			}
			return mIndexBuffer.fetch(size);
		}
		else
		{
			int index = mBaseIndices.size();
			mBaseIndices.resize(index + size);
			return &mBaseIndices[index];
		}
	}

	void BatchedRender::emitPolygonLine(Vector2 v[], int numV, Color4Type const& color, int lineWidth)
	{
		float halfWidth = 0.5 * float(lineWidth);
#if 1
		int baseIndex;
		BaseVertex* pVertices = fetchBaseVertex(numV * 2, baseIndex);
		int index = 0;
		for (int i = 0; i < numV; ++i)
		{
			Vector2 e1 = v[(i + 1) % numV] - v[i];
			if (e1.normalize() < FLOAT_DIV_ZERO_EPSILON)
			{
				e1 = v[(i + 2) % numV] - v[i];
				e1.normalize();
			}
			Vector2 e2 = Math::GetNormal(v[(i + numV - 1) % numV] - v[i]);
			if (e2.normalize() < FLOAT_DIV_ZERO_EPSILON)
			{
				e2 = v[(i + numV - 2) % numV] - v[i];
				e2.normalize();
			}
			Vector2 offsetDir = e1 + e2;
			Vector2 offset;
			if (offsetDir.normalize() > FLOAT_DIV_ZERO_EPSILON)
			{
				float s = Math::Abs(e1.cross(offsetDir));
				if (s > FLOAT_DIV_ZERO_EPSILON)
				{
					offset = (halfWidth / s) * offsetDir;
				}
				else
				{
					offset = halfWidth * offsetDir;
				}

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
		uint32* pIndices = fetchIndex(3 * 2 * numV);
		for (int i = 0; i < numV - 1; ++i)
		{
			pIndices = FillQuad(pIndices, baseIndex, 0, 1, 3, 2);
			baseIndex += 2;
		}
		FillQuad(pIndices, baseIndex , baseIndex + 1, baseIndexStart + 1, baseIndexStart + 0);

#else

		Vector2 offset[4] = { Vector2(-halfWidth,-halfWidth) , Vector2(-halfWidth,halfWidth)  , Vector2(halfWidth,halfWidth), Vector2(halfWidth,-halfWidth) };

		int baseIndex;
		BaseVertex* pVertices = fetchBaseVertex(4 * numV, baseIndex);
		for (int i = 0; i < numV; ++i)
		{
			pVertices[0] = { v[i] + offset[0] , color };
			pVertices[1] = { v[i] + offset[1] , color };
			pVertices[2] = { v[i] + offset[2] , color };
			pVertices[3] = { v[i] + offset[3] , color };
			pVertices += 4;
		}

		uint32* pIndices = fetchIndex(4 * 6 * numV);
		int indexCur = baseIndex;
		for (int i = 0; i < numV - 1; ++i)
		{
			pIndices = FillLineShapeIndices(pIndices, indexCur, indexCur + 4);
			indexCur += 4;
		}
		FillLineShapeIndices(pIndices, indexCur, baseIndex);
#endif
	}

	void BatchedRender::flushDrawCommand()
	{
		//CHECK(mTexVerteices.size() || mBaseVertices.size());
		if constexpr (Meta::IsSameType< Color4Type, Color4f >::Value)
		{
			if (!mBaseVertices.empty())
			{
				TRenderRT< RTVF_XY_CA >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, mBaseVertices.data(), mBaseVertices.size(), mBaseIndices.data(), mBaseIndices.size());
				mBaseVertices.clear();
				mBaseIndices.clear();
			}
			else if (!mTexVerteices.empty())
			{
				TRenderRT< RTVF_XY_CA_T2 >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, mTexVerteices.data(), mTexVerteices.size(), mBaseIndices.data(), mBaseIndices.size());
				mTexVerteices.clear();
				mBaseIndices.clear();
			}
		}
		else
		{
			if (!mBaseVertices.empty())
			{
				TRenderRT< RTVF_XY_CA8 >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, mBaseVertices.data(), mBaseVertices.size(), mBaseIndices.data(), mBaseIndices.size());
				mBaseVertices.clear();
				mBaseIndices.clear();
			}
			else if (!mTexVerteices.empty())
			{
				TRenderRT< RTVF_XY_CA8_T2 >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, mTexVerteices.data(), mTexVerteices.size(), mBaseIndices.data(), mBaseIndices.size());
				mTexVerteices.clear();
				mBaseIndices.clear();
			}
		}
	}

	void BatchedRender::flushDrawCommandForBuffer(bool bEndRender)
	{
		if (bEndRender)
		{
			mBaseVertexBuffer.unlock();
			mTexVertexBuffer.unlock();
		}
		else
		{
			if (mBaseVertexBuffer.usedCount)
			{
				mBaseVertexBuffer.unlock();
			}
			else if (mTexVertexBuffer.usedCount)
			{
				mTexVertexBuffer.unlock();
			}
		}

		mIndexBuffer.unlock();

		if constexpr (Meta::IsSameType< Color4Type, Color4f >::Value)
		{
			if (mBaseVertexBuffer.usedCount)
			{
				TRenderRT< RTVF_XY_CA >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, *mBaseVertexBuffer, mBaseVertexBuffer.usedCount, *mIndexBuffer, mIndexBuffer.usedCount);
			}
			else if (mTexVertexBuffer.usedCount)
			{
				TRenderRT< RTVF_XY_CA_T2 >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, *mTexVertexBuffer, mTexVertexBuffer.usedCount, *mIndexBuffer, mIndexBuffer.usedCount);
			}
		}
		else
		{
			if (mBaseVertexBuffer.usedCount)
			{
				TRenderRT< RTVF_XY_CA8 >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, *mBaseVertexBuffer, mBaseVertexBuffer.usedCount, *mIndexBuffer, mIndexBuffer.usedCount);
			}
			else if (mTexVertexBuffer.usedCount)
			{
				TRenderRT< RTVF_XY_CA8_T2 >::DrawIndexed(*mCommandList, EPrimitive::TriangleList, *mTexVertexBuffer, mTexVertexBuffer.usedCount, *mIndexBuffer, mIndexBuffer.usedCount);
			}
		}

		if (bEndRender == false)
		{
			if (mBaseVertexBuffer.usedCount)
			{
				mBaseVertexBuffer.lock();
			}
			else if (mTexVertexBuffer.usedCount)
			{
				mTexVertexBuffer.lock();
			}
			mIndexBuffer.lock();
		}
	}

}// namespace Render