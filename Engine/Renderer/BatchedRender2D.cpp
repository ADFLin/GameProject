#include "BatchedRender2D.h"
#include "RHI/DrawUtility.h"
#include "ConsoleSystem.h"
#include "CoreShare.h"
#include "ProfileSystem.h"
#include "Core/ScopeGuard.h"

//#TODO : Heap Corrupted when show console frame draw Round Rect

#define USE_NEW_LINE_IDNEX 1
#define USE_BATCHED_GROUP 1
#define USE_SHAPE_CACHE 1

namespace Render
{

	constexpr int MaxTexVerticesCount = 4 * 2048;
	constexpr int MaxIndicesCount = 4 * 2048 * 3;
	constexpr uint32 TexVertexFormat = (Meta::IsSameType< ShapePaintArgs::Color4Type, Color4f >::Value) ? RTVF_XY_CA_T2 : RTVF_XY_CA8_T2;

#if CORE_SHARE_CODE

	CORE_API ShapeVertexCache& BatchedRender::GetShapeCache()
	{
		static ShapeVertexCache StaticInstance;
		return StaticInstance;
	}

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
					Math::SinCos(Math::PI / 4, ts, tc);
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

	void ShapeVertexBuilder::buildPolygonLine(Vector2 v[], int numV, float lineWidth)
	{
		float halfWidth = 0.5 * lineWidth;
#if USE_POLYGON_LINE_NEW
		halfWidth = 1.0f;
		mBuffer.resize(2 * numV);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = mTransform.transformPosition(p);
			++pVertices;
		};

		int index = 0;
		for (int i = 0; i < numV; ++i)
		{
			Vector2 e1 = v[(i + 1) % numV] - v[i];
			if (UNLIKELY(e1.normalize() < FLOAT_DIV_ZERO_EPSILON))
			{
				e1 = v[(i + 2) % numV] - v[i];
				e1.normalize();
			}
			Vector2 e2 = Math::GetNormal(v[(i + numV - 1) % numV] - v[i]);
			if (UNLIKELY(e2.normalize() < FLOAT_DIV_ZERO_EPSILON))
			{
				e2 = v[(i + numV - 2) % numV] - v[i];
				e2.normalize();
			}
			Vector2 offsetDir = e1 + e2;
			Vector2 offset;
			if (LIKELY(offsetDir.normalize() > FLOAT_DIV_ZERO_EPSILON))
			{
				float s = Math::Abs(e1.cross(offsetDir));
				if (LIKELY(s > FLOAT_DIV_ZERO_EPSILON))
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
			EmitVertex(v[i]);
			EmitVertex(offset);
		}
#else
		mBuffer.resize(4 * numV);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = mTransform.transformPosition(p);
			++pVertices;
		};

		Vector2 offset[4] = 
		{ 
			Vector2(-halfWidth,-halfWidth) , 
			Vector2(-halfWidth,halfWidth)  , 
			Vector2(halfWidth,halfWidth),
			Vector2(halfWidth,-halfWidth) 
		};

		int baseIndex;
		TexVertex* pVertices = fetchVertex(4 * numV, baseIndex);
		uint32* pIndices = fetchIndex(4 * 6 * numV);

		for (int i = 0; i < numV; ++i)
		{
			EmitVertex(v[i] + offset[0]);
			EmitVertex(v[i] + offset[1]);
			EmitVertex(v[i] + offset[2]);
			EmitVertex(v[i] + offset[3]);
		}
#endif
	}

	RenderBatchedElement& RenderBatchedElementList::addRect(ShapePaintArgs const& paintArgs, Vector2 const& min, Vector2 const& max)
	{
		TRenderBatchedElement<RectPayload>* element = addElement< RectPayload >();
		element->type = RenderBatchedElement::Rect;
		element->payload.paintArgs = paintArgs;
		element->payload.min = min;
		element->payload.max = max;
		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addRoundRect(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius)
	{
		TRenderBatchedElement<RoundRectPayload>* element = addElement< RoundRectPayload >();
		element->type = RenderBatchedElement::RoundRect;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.rectSize = rectSize;
		element->payload.circleRadius = circleRadius;
		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addCircle(ShapePaintArgs const& paintArgs, Vector2 const& pos, float radius)
	{
		TRenderBatchedElement<CirclePayload>* element = addElement< CirclePayload >();
		element->type = RenderBatchedElement::Circle;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.radius = radius;
		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addEllipse(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& size)
	{
		TRenderBatchedElement<EllipsePayload>* element = addElement< EllipsePayload >();
		element->type = RenderBatchedElement::Ellipse;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.size = size;
		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addTextureRect(Color4Type const& color, Vector2 const& min, Vector2 const& max, Vector2 const& uvMin, Vector2 const& uvMax)
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

	RenderBatchedElement& RenderBatchedElementList::addLine(Color4Type const& color, Vector2 const& p1, Vector2 const& p2, int width)
	{
		TRenderBatchedElement<LinePayload>* element = addElement< LinePayload >();
		element->type = RenderBatchedElement::Line;
		element->payload.color = color;
		element->payload.p1 = p1;
		element->payload.p2 = p2;
		element->payload.width = width;
		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addText(Color4Type const& color, TArray< FontVertex > const& vertices)
	{
		TRenderBatchedElement<TextPayload>* element = addElement< TextPayload >();
		element->type = RenderBatchedElement::Text;
		element->payload.color = color;
		element->payload.vertices = (FontVertex*)mAllocator.alloc(vertices.size() * sizeof(FontVertex));
		FMemory::Copy(element->payload.vertices, vertices.data(), vertices.size() * sizeof(FontVertex));
		element->payload.verticesCount = vertices.size();

		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad)
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

	void RenderBatchedElementList::releaseElements()
	{
		mElements.clear();
	}

	BatchedRender::BatchedRender()
		: mWidth(1)
		, mHeight(1)
	{
		mCachedPositionList.reserve(1024);
	}

	void BatchedRender::initializeRHI()
	{
		mTexVertexBuffer.initialize(MaxTexVerticesCount, BCF_CpuAccessWrite | BCF_UsageVertex);
		mIndexBuffer.initialize(MaxIndicesCount, BCF_CpuAccessWrite | BCF_UsageIndex);
	}

	void BatchedRender::releaseRHI()
	{
		mTexVertexBuffer.release();
		mIndexBuffer.release();
	}

	void BatchedRender::emitPolygon(Vector2 v[], int numV, Color4Type const& color)
	{
		assert(numV > 2);
		int baseIndex;
		TexVertex* pVertices = fetchVertex(numV, baseIndex);
		int numTriangle = (numV - 2);
		uint32* pIndices = fetchIndex(3 * numTriangle);

		for (int i = 0; i < numV; ++i)
		{
			TexVertex& vtx = pVertices[i];
			pVertices->pos = v[i];
			pVertices->color = color;
			++pVertices;
		}

		pIndices = FillPolygon(pIndices, baseIndex, numTriangle);
	}

	void BatchedRender::emitPolygon(Vector2 v[], int numV, ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			PROFILE_ENTRY("EmitPolygon");
			emitPolygon(v, numV, paintArgs.brushColor);
		}
		if (paintArgs.bUsePen)
		{
			PROFILE_ENTRY("EmitPolygonLine");
			emitPolygonLine(v, numV, paintArgs.penColor, paintArgs.penWidth);
		}
	}

	void BatchedRender::emitPolygon(ShapeCachedData& cachedData, RenderTransform2D const& xForm, ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			PROFILE_ENTRY("EmitCachedPolygon");
			assert(cachedData.posList.size() > 2);
			int baseIndex;
			TexVertex* pVertices = fetchVertex(cachedData.posList.size(), baseIndex);
			int numTriangle = (cachedData.posList.size() - 2);
			uint32* pIndices = fetchIndex(3 * numTriangle);

			for (int i = 0; i < cachedData.posList.size(); ++i)
			{
				TexVertex& vtx = pVertices[i];
				pVertices->pos = xForm.transformPosition(cachedData.posList[i]);
				pVertices->color = paintArgs.brushColor;
				++pVertices;
			}
			
			pIndices = FillPolygon(pIndices, baseIndex, numTriangle);
		}
		if (paintArgs.bUsePen)
		{
			PROFILE_ENTRY("EmitCachedPolygonLine");

			ShapeCachedData* lineData = GetShapeCache().getShapeLine(cachedData, paintArgs.penWidth);

#if USE_POLYGON_LINE_NEW

			int baseIndex;
			TexVertex* pVertices = fetchVertex(2 * cachedData.posList.size(), baseIndex);
			uint32* pIndices = fetchIndex(3 * 2 * cachedData.posList.size());

			float halfWidth = 0.5 * float(paintArgs.penWidth);
			if ( xForm.isUniformScale())
			{
				for (int i = 0; i < lineData->posList.size(); i += 2)
				{
					Vector2 pos = xForm.transformPosition(lineData->posList[i]);
					Vector2 offset = halfWidth * xForm.transformVector(lineData->posList[i + 1]);
					pVertices[i].pos = pos + offset;
					pVertices[i].color = paintArgs.penColor;
					pVertices[i + 1].pos = pos - offset;
					pVertices[i + 1].color = paintArgs.penColor;
				}
			}
			else
			{
				//TODO
				for (int i = 0; i < lineData->posList.size(); i += 2)
				{
					Vector2 pos = xForm.transformPosition(lineData->posList[i]);
					Vector2 offset = halfWidth * xForm.transformVector(lineData->posList[i + 1]);
					pVertices[i].pos = pos + offset;
					pVertices[i].color = paintArgs.penColor;
					pVertices[i + 1].pos = pos - offset;
					pVertices[i + 1].color = paintArgs.penColor;
				}
			}
#else
			int baseIndex;
			TexVertex* pVertices = fetchVertex(4 * cachedData.posList.size(), baseIndex);
			uint32* pIndices = fetchIndex(4 * 6 * cachedData.posList.size());
			for (int i = 0; i < lineData->posList.size(); i += 2)
			{
				pVertices[i].pos = xForm.transformPosition(lineData->posList[i]);
				pVertices[i].color = paintArgs.penColor;
			}
#endif

			FillPolygonLine(pIndices, baseIndex, cachedData.posList.size());
		}
	}

	void BatchedRender::emitRect(Vector2 v[], ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			int baseIndex;
			TexVertex* pVertices = fetchVertex(4, baseIndex);
			uint32* pIndices = fetchIndex(3 * 2);

			pVertices[0] = { v[0] , paintArgs.brushColor };
			pVertices[1] = { v[1] , paintArgs.brushColor };
			pVertices[2] = { v[2] , paintArgs.brushColor };
			pVertices[3] = { v[3] , paintArgs.brushColor };

			FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
		}

		if (paintArgs.bUsePen)
		{
			float halfWidth = 0.5 * float(paintArgs.penWidth);
			int baseIndex;
			TexVertex* pVertices = fetchVertex(4 * 2, baseIndex);
			uint32* pIndices = fetchIndex(3 * 2 * 4);

			for (int i = 0; i < 4; ++i)
			{
				Vector2 e1 = Math::GetNormal( v[(i + 1) % 4] - v[i] );
				Vector2 e2 = Math::GetNormal( v[(i + 3) % 4] - v[i] );
				Vector2 offset = halfWidth * (e1 + e2);
				pVertices[0] = { v[i] + offset , paintArgs.penColor };
				pVertices[1] = { v[i] - offset , paintArgs.penColor };
				pVertices += 2;
			}


			pIndices = FillQuad(pIndices, baseIndex, 0, 1, 3, 2);
			pIndices = FillQuad(pIndices, baseIndex, 2, 3, 5, 4);
			pIndices = FillQuad(pIndices, baseIndex, 4, 5, 7, 6);
			pIndices = FillQuad(pIndices, baseIndex, 6, 7, 1, 0);
		}
	}

	BatchedRender::TexVertex* BatchedRender::fetchVertex(int size, int& baseIndex)
	{
		if (!mTexVertexBuffer.canFetch(size))
		{
			flushDrawCommand(false);
		}

		baseIndex = mTexVertexBuffer.usedCount;
		return mTexVertexBuffer.fetch(size);

	}

	uint32* BatchedRender::fetchIndex(int size)
	{
		if (!mIndexBuffer.canFetch(size))
		{
			flushDrawCommand(false);
		}
		return mIndexBuffer.fetch(size);
	}

	void BatchedRender::emitPolygonLine(Vector2 v[], int numV, Color4Type const& color, int lineWidth)
	{
		float halfWidth = 0.5 * float(lineWidth);
#if USE_POLYGON_LINE_NEW
		int baseIndex;
		TexVertex* pVertices = fetchVertex(numV * 2, baseIndex);
		uint32* pIndices = fetchIndex(3 * 2 * numV);

		int index = 0;
		for (int i = 0; i < numV; ++i)
		{
			Vector2 e1 = v[(i + 1) % numV] - v[i];
			if (UNLIKELY(e1.normalize() < FLOAT_DIV_ZERO_EPSILON))
			{
				e1 = v[(i + 2) % numV] - v[i];
				e1.normalize();
			}
			Vector2 e2 = Math::GetNormal(v[(i + numV - 1) % numV] - v[i]);
			if (UNLIKELY(e2.normalize() < FLOAT_DIV_ZERO_EPSILON))
			{
				e2 = v[(i + numV - 2) % numV] - v[i];
				e2.normalize();
			}
			Vector2 offsetDir = e1 + e2;
			Vector2 offset;
			if (LIKELY(offsetDir.normalize() > FLOAT_DIV_ZERO_EPSILON))
			{
				float s = Math::Abs(e1.cross(offsetDir));
				if (LIKELY(s > FLOAT_DIV_ZERO_EPSILON))
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
#else

		Vector2 offset[4] = { Vector2(-halfWidth,-halfWidth) , Vector2(-halfWidth,halfWidth)  , Vector2(halfWidth,halfWidth), Vector2(halfWidth,-halfWidth) };

		int baseIndex;
		TexVertex* pVertices = fetchVertex(4 * numV, baseIndex);
		uint32* pIndices = fetchIndex(4 * 6 * numV);

		for (int i = 0; i < numV; ++i)
		{
			pVertices[0] = { v[i] + offset[0] , color };
			pVertices[1] = { v[i] + offset[1] , color };
			pVertices[2] = { v[i] + offset[2] , color };
			pVertices[3] = { v[i] + offset[3] , color };
			pVertices += 4;
		}
#endif
		FillPolygonLine(pIndices, baseIndex, numV);
	}

	void BatchedRender::beginRender(RHICommandList& commandList)
	{
		mCommandList = &commandList;
		setupInputState(commandList);
		CHECK(mGroups.size() == 0);
	}

	void BatchedRender::render(RenderState const& renderState, RenderBatchedElementList& elementList)
	{
#if USE_BATCHED_GROUP
		RenderGroup group;
		group.elements = std::move(elementList.mElements);
		group.state = renderState;
		mGroups.push_back(std::move(group));
#else
		bool bLocked = mTexVertexBuffer.lock() && mIndexBuffer.lock();
		if (!bLocked)
		{
			mTexVertexBuffer.tryUnlock();
			mIndexBuffer.tryUnlock();
			return;
		}

		updateRenderState(*mCommandList, renderState);
		emitElements(elementList.mElements);
		elementList.releaseElements();
		flushDrawCommand(true);
		elementList.mAllocator.clearFrame();
#endif
	}

	void BatchedRender::flushGroup()
	{
		if (mGroups.empty())
			return;

		ON_SCOPE_EXIT
		{
			mGroups.clear();
		};

		bool bLocked = mTexVertexBuffer.lock() && mIndexBuffer.lock();
		if (!bLocked)
		{
			mTexVertexBuffer.tryUnlock();
			mIndexBuffer.tryUnlock();
			return;
		}

		mIndexRender = 0;
		for (int index = 0; index < mGroups.size(); ++index)
		{
			mIndexEmit = index;
			RenderGroup& group = mGroups[index];
			group.indexStart = mIndexBuffer.usedCount;
			emitElements(group.elements);
			group.indexCount = mIndexBuffer.usedCount - group.indexStart;
		}

		flushDrawCommand(true);
		mGroups.clear();
	}

	void BatchedRender::flushDrawCommand(bool bEndRender)
	{
		PROFILE_ENTRY("flushDrawCommand");

		mTexVertexBuffer.unlock();
		mIndexBuffer.unlock();

#if USE_BATCHED_GROUP

		for (; mIndexRender < mIndexEmit; ++mIndexRender)
		{
			RenderGroup& group = mGroups[mIndexRender];
			updateRenderState(*mCommandList, group.state);
			RHIDrawIndexedPrimitive(*mCommandList, EPrimitive::TriangleList, group.indexStart, group.indexCount);
		}

		{
			RenderGroup& group = mGroups[mIndexEmit];
			int count = mIndexBuffer.usedCount - group.indexStart;
			if (count)
			{
				updateRenderState(*mCommandList, group.state);
				RHIDrawIndexedPrimitive(*mCommandList, EPrimitive::TriangleList, group.indexStart, mIndexBuffer.usedCount - group.indexStart);
			}
			group.indexStart = 0;
		}
#else
		RHIDrawIndexedPrimitive(*mCommandList, EPrimitive::TriangleList, 0, mIndexBuffer.usedCount);
#endif

		if (bEndRender)
		{

		}
		else
		{
#if USE_BATCHED_GROUP
			mGroups[mIndexEmit].indexStart = 0;
#endif
			mTexVertexBuffer.lock();
			mIndexBuffer.lock();
		}
	}

	void BatchedRender::emitElements(TArray<RenderBatchedElement* > const& elements)
	{
		for (RenderBatchedElement* element : elements)
		{
			switch (element->type)
			{
			case RenderBatchedElement::Rect:
				{
					RenderBatchedElementList::RectPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::RectPayload >(element);
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
					RenderBatchedElementList::PolygonPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::PolygonPayload >(element);
					emitPolygon(payload.posList, payload.posCount, payload.paintArgs);
				}
				break;
			case RenderBatchedElement::Circle:
				{
					PROFILE_ENTRY("Emit Circle");
					RenderBatchedElementList::CirclePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::CirclePayload >(element);
#if USE_SHAPE_CACHE
					ShapeCachedData* cachedData = GetShapeCache().getCircle(calcCircleSemgmentNum(payload.radius));
					RenderTransform2D xForm = RenderTransform2D(Vector2(payload.radius, payload.radius), payload.pos) * element->transform;
					emitPolygon(*cachedData, xForm, payload.paintArgs);
#else
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					builder.buildCircle(payload.pos, payload.radius, calcCircleSemgmentNum(payload.radius));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
#endif
				}
				break;
			case RenderBatchedElement::Ellipse:
				{
					RenderBatchedElementList::EllipsePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::EllipsePayload >(element);
#if USE_SHAPE_CACHE
					float yFactor = float(payload.size.y) / payload.size.x;
					ShapeCachedData* cachedData = GetShapeCache().getEllipse(yFactor, calcCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
					RenderTransform2D xForm = RenderTransform2D(Vector2(payload.size.x, payload.size.x), payload.pos) * element->transform;
					emitPolygon(*cachedData, xForm, payload.paintArgs);
#else
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					float yFactor = float(payload.size.y) / payload.size.x;
					builder.buildEllipse(payload.pos, payload.size.x, yFactor, calcCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
#endif
				}
				break;
			case RenderBatchedElement::RoundRect:
				{
					PROFILE_ENTRY("Emit RoundRect");

					RenderBatchedElementList::RoundRectPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::RoundRectPayload >(element);
#if USE_SHAPE_CACHE
					ShapeCachedData* cachedData = GetShapeCache().getRoundRect(payload.rectSize, payload.circleRadius);
					RenderTransform2D xForm = RenderTransform2D::Translate(payload.pos) * element->transform;
					emitPolygon(*cachedData, xForm, payload.paintArgs);
#else
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					builder.buildRoundRect(payload.pos, payload.rectSize, payload.circleRadius);
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
#endif
				}
				break;
			case RenderBatchedElement::TextureRect:
				{
					RenderBatchedElementList::TextureRectPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::TextureRectPayload >(element);
					int baseIndex;
					TexVertex* pVertices = fetchVertex(4, baseIndex);
					uint32* pIndices = fetchIndex(6);

					pVertices[0] = { element->transform.transformPosition(payload.min) , payload.color , payload.uvMin };
					pVertices[1] = { element->transform.transformPosition(Vector2(payload.min.x, payload.max.y)), payload.color , Vector2(payload.uvMin.x , payload.uvMax.y) };
					pVertices[2] = { element->transform.transformPosition(payload.max), payload.color , payload.uvMax };
					pVertices[3] = { element->transform.transformPosition(Vector2(payload.max.x, payload.min.y)), payload.color , Vector2(payload.uvMax.x , payload.uvMin.y) };

					FillQuad(pIndices, baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 3);
				}
				break;
			case RenderBatchedElement::Line:
				{
					RenderBatchedElementList::LinePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::LinePayload >(element);

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
					TexVertex* pVertices = fetchVertex(3 * 2, baseIndex);
					uint32* pIndices = fetchIndex(4 * 3);

					pVertices[0] = { positions[0] + offset[index2] , payload.color };
					pVertices[1] = { positions[0] + offset[index3] , payload.color };
					pVertices[2] = { positions[0] + offset[index1] , payload.color };
					pVertices[3] = { positions[1] + offset[index0] , payload.color };
					pVertices[4] = { positions[1] + offset[index1] , payload.color };
					pVertices[5] = { positions[1] + offset[index3] , payload.color };

					pIndices = FillTriangle(pIndices, baseIndex, 0, 1, 2);
					pIndices = FillTriangle(pIndices, baseIndex, 3, 4, 5);
					pIndices = FillQuad(pIndices, baseIndex, 2, 1, 5, 4);
#else
					int baseIndex;
					TexVertex* pVertices = fetchVertex(4 * 2, baseIndex);
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
					RenderBatchedElementList::LineStripPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::LineStripPayload >(element);

					int lineCount = payload.posCount - 1;

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
					TexVertex* pVertices = fetchVertex(3 * 2 * lineCount, baseIndex);
					uint32* pIndices = fetchIndex(4 * 3 * lineCount);
#else
					TexVertex* pVertices = fetchVertex(4 * 2, baseIndex);
					uint32* pIndices = fetchIndex(4 * 6 * lineCount);
#endif

					for (int i = 0; i < lineCount; ++i)
					{
						Vector2 positions[2];
						positions[0] = payload.posList[i];
						positions[1] = payload.posList[i + 1];
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
					PROFILE_ENTRY("Text Vertices");
					RenderBatchedElementList::TextPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::TextPayload >(element);

#if USE_NEW_LINE_IDNEX
					int numChar = payload.verticesCount / 4;
					FontVertex* pSrcVertices = payload.vertices;
#else
					int numChar = payload.vertices.size() / 4;
					FontVertex* pSrcVertices = payload.vertices.data();
#endif
					int baseIndex;
					TexVertex* pVertices = fetchVertex(4 * numChar, baseIndex);
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
					RenderBatchedElementList::GradientRectPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::GradientRectPayload >(element);

					int baseIndex;
					TexVertex* pVertices = fetchVertex(4, baseIndex);
					uint32* pIndices = fetchIndex(6);

					pVertices[0] = { element->transform.transformPosition(payload.posLT) , payload.colorLT };
					pVertices[1] = { element->transform.transformPosition(Vector2(payload.posLT.x, payload.posRB.y)), payload.bHGrad ? payload.colorLT : payload.colorRB };
					pVertices[2] = { element->transform.transformPosition(payload.posRB), payload.colorRB };
					pVertices[3] = { element->transform.transformPosition(Vector2(payload.posRB.x, payload.posLT.y)),  payload.bHGrad ? payload.colorRB : payload.colorLT };

					pIndices = FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
				}
			}
		}
	}

	void BatchedRender::flush()
	{
#if USE_BATCHED_GROUP
		flushGroup();
#endif
	}

	void BatchedRender::commitRenderState(RHICommandList& commandList, RenderState const& state)
	{
		RHISetViewport(commandList, 0, 0, mWidth, mHeight);
		RHISetDepthStencilState(commandList, GraphicsDepthState::GetRHI());
		RHISetBlendState(commandList, GetBlendState(state.blendMode));
		RHISetRasterizerState(commandList, GetRasterizerState(state.bEnableScissor, state.bEnableMultiSample));
		if (state.bEnableScissor)
		{
			auto const& rect = state.scissorRect;
			RHISetScissorRect(commandList, rect.pos.x, mHeight - rect.pos.y - rect.size.y, rect.size.x, rect.size.y);
		}

		uint32 const attributeMask = BIT(EVertex::ATTRIBUTE_COLOR) | BIT(EVertex::ATTRIBUTE_TEXCOORD);

		SimplePipelineProgram* program = SimplePipelineProgram::Get(attributeMask, state.texture != nullptr);
		RHISetShaderProgram(commandList, program->getRHI());
		program->setParameters(commandList, mBaseTransform, LinearColor(1, 1, 1, 1), state.texture, state.sampler);

		setupInputState(commandList);

	}

	void BatchedRender::setupInputState(RHICommandList& commandList)
	{
		InputStreamInfo inputStream;
		inputStream.buffer = &*mTexVertexBuffer;
		inputStream.offset = 0;
		inputStream.stride = TRenderRT<TexVertexFormat>::GetVertexSize();
		RHISetInputStream(commandList, &TStaticRenderRTInputLayout<TexVertexFormat>::GetRHI(), &inputStream, 1);
		RHISetIndexBuffer(commandList, &*mIndexBuffer);
	}

	void BatchedRender::updateRenderState(RHICommandList& commandList, RenderState const& state)
	{
		uint32 const attributeMask = BIT(EVertex::ATTRIBUTE_COLOR) | BIT(EVertex::ATTRIBUTE_TEXCOORD);

		RHISetBlendState(commandList, GetBlendState(state.blendMode));
		RHISetRasterizerState(commandList, GetRasterizerState(state.bEnableScissor, state.bEnableMultiSample));
		if (state.bEnableScissor)
		{
			auto const& rect = state.scissorRect;
			RHISetScissorRect(commandList, rect.pos.x, mHeight - rect.pos.y - rect.size.y, rect.size.x, rect.size.y);
		}

		SimplePipelineProgram* program = SimplePipelineProgram::Get(attributeMask, state.texture != nullptr);
		RHISetShaderProgram(commandList, program->getRHI());
		if (state.texture)
		{
			program->setTextureParam(commandList, state.texture, state.sampler);
		}
	}

	RHIBlendState& GraphicsDefinition::GetBlendState(ESimpleBlendMode blendMode)
	{
		switch (blendMode)
		{
		case ESimpleBlendMode::Translucent:
			return TStaticBlendState< CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI();
		case ESimpleBlendMode::Add:
			return TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI();
		case ESimpleBlendMode::Multiply:
			return TStaticBlendState< CWM_RGBA, EBlend::DestColor, EBlend::Zero >::GetRHI();
		case ESimpleBlendMode::None:
			return TStaticBlendState<>::GetRHI();
		}
		NEVER_REACH("GetBlendState");
		return TStaticBlendState<>::GetRHI();
	}

	Render::RHIRasterizerState& GraphicsDefinition::GetRasterizerState(bool bEnableScissor, bool bEnableMultiSample)
	{
		if (bEnableMultiSample)
		{
			if (bEnableScissor)
			{
				return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true, true >::GetRHI();
			}
			else
			{
				return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false, true >::GetRHI();
			}

		}
		else if (bEnableScissor)
		{
			return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI();
		}

		return TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI();
	}


	template< typename TFunc >
	ShapeCachedData* ShapeVertexCache::findOrCache(CacheKey const& key, TFunc& func)
	{
		ShapeCachedData* cachedData;
		auto iter = mCachedMap.find(key);
		if (iter == mCachedMap.end())
		{
			ShapeCachedData data;
			func(data);
			data.id = mNextId++;
			auto iter = mCachedMap.emplace(key, std::move(data));
			cachedData = &iter.first->second;
		}
		else
		{
			cachedData = &iter->second;
		}
		cachedData->frame = mFrame;
		return cachedData;

	}

	template< typename TFunc >
	TArrayView< Vector2 > ShapeVertexCache::getOrBuild(FrameAllocator& allocator, RenderTransform2D const& xForm, CacheKey const& key, TFunc& func)
	{
		ShapeCachedData* cachedData = findOrCache(key, func);
		return cachedData->build(allocator, xForm);
	}

	ShapeCachedData* ShapeVertexCache::getCircle(int numSegment)
	{
		CacheKey key;
		key.type = CacheKey::eCircle;
		key.v4 = Vector4(numSegment, 0, 0, 0);
		return findOrCache(key,
			[&](ShapeCachedData& data)
			{
				ShapeVertexBuilder builder(data.posList);
				builder.mTransform = RenderTransform2D::Identity();
				builder.buildCircle(Vector2::Zero(), 1.0f, numSegment);
			}
		);
	}

	ShapeCachedData* ShapeVertexCache::getEllipse(float yFactor, int numSegment)
	{
		CacheKey key;
		key.type = CacheKey::eEllipse;
		key.v4 = Vector4(numSegment, yFactor, 0, 0);
		return findOrCache( key,
			[&](ShapeCachedData& data)
			{
				ShapeVertexBuilder builder(data.posList);
				builder.mTransform = RenderTransform2D::Identity();
				builder.buildEllipse(Vector2::Zero(), 1.0f, yFactor, numSegment);
			}
		);
	}

	ShapeCachedData* ShapeVertexCache::getRoundRect(Vector2 const& rectSize, Vector2 const& circleRadius)
	{
		CacheKey key;
		key.type = CacheKey::eRoundRect;
		key.v4 = Vector4(rectSize, circleRadius);
		return findOrCache(key,
			[&](ShapeCachedData& data)
			{
				ShapeVertexBuilder builder(data.posList);
				builder.mTransform = RenderTransform2D::Identity();
				builder.buildRoundRect(Vector2::Zero(), rectSize, circleRadius);
			}
		);
	}

	ShapeCachedData* ShapeVertexCache::getShapeLine(ShapeCachedData& cacheData, float width)
	{
		CacheKey key;
		key.type = CacheKey::eShapLine;
		key.ptr = (uintptr_t)&cacheData;
		key.v2  = Vector2(width, 0);
		return findOrCache(key,
			[&](ShapeCachedData& data)
			{
				ShapeVertexBuilder builder(data.posList);
				builder.mTransform = RenderTransform2D::Identity();
				builder.buildPolygonLine(cacheData.posList.data(), cacheData.posList.size(), width);
			}
		);
	}


}// namespace Render