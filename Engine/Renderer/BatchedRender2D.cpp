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


	static inline int GetCircleSemgmentNum(float r)
	{
#if 1
		constexpr float Error = 0.3f;
		constexpr int SegmentAlign = 4;
		return Math::Max(32, SegmentAlign * ((Math::CeilToInt(Math::PI / Math::ACos(1 - Error / r)) + SegmentAlign - 1) / SegmentAlign));
#else
		return std::max(4 * (r / 2), 32.0f) * 10;
#endif
	}

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
		int numSegment = GetCircleSemgmentNum((circleRadius.x + circleRadius.y) / 2);
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
			{ float(pos.x + rectSize.x - circleRadius.x) , float(pos.y + circleRadius.y) },
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

		for (int i = 0; i < numV; ++i)
		{
			EmitVertex(v[i] + offset[0]);
			EmitVertex(v[i] + offset[1]);
			EmitVertex(v[i] + offset[2]);
			EmitVertex(v[i] + offset[3]);
		}
#endif
	}

	void ShapeVertexBuilder::buildArcLinePos(Vector2 const& center, float r, float startAngle, float sweepAngle, int numSegment)
	{
		mBuffer.resize(numSegment + 1);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = mTransform.transformPosition(p);
			++pVertices;
		};

		float theta = sweepAngle / float(numSegment);
		Matrix2 rotation = Matrix2::Rotate(theta);

		float s,c;
		Math::SinCos(startAngle, s, c);
		Vector2 offset = r * Vector2(c, s);

		for (int i = 0; i < numSegment; ++i)
		{
			EmitVertex(center + offset);
			//apply the rotation matrix
			offset = rotation.leftMul(offset);
		}

		{
			float s, c;
			Math::SinCos(startAngle + sweepAngle, s, c);
			Vector2 offset = r * Vector2(c, s);
			EmitVertex(center + offset);
		}
	}

	RenderBatchedElement& RenderBatchedElementList::addPoint(Vector2 const& pos, Color4Type const& color, float size /*= 0*/)
	{
		TRenderBatchedElement<PointPayload>* element = addElement< PointPayload >();
		element->type = RenderBatchedElement::Point;
		element->payload.pos = pos;
		element->payload.size = size;
		element->payload.color = color;
		return *element;
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

	RenderBatchedElement& RenderBatchedElementList::addArcLine(Color4Type const& color, Vector2 const& center, float radius, float startAngle, float sweepAngle, int width)
	{
		TRenderBatchedElement<ArcLinePlayload>* element = addElement< ArcLinePlayload >();
		element->type = RenderBatchedElement::ArcLine;
		element->payload.color = color;
		element->payload.center = center;
		element->payload.radius = radius;
		element->payload.startAngle = startAngle;
		element->payload.sweepAngle = sweepAngle;
		element->payload.width = width;
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

	RenderBatchedElement& RenderBatchedElementList::addText(Color4Type const& color, TArray< GlyphVertex > const& vertices, bool bRemoveScale, bool bRemoveRotation)
	{
		TRenderBatchedElement<TextPayload>* element = addElement< TextPayload >();
		element->type = RenderBatchedElement::Text;
		element->payload.color = color;
		element->payload.vertices = (GlyphVertex*)mAllocator.alloc(vertices.size() * sizeof(GlyphVertex));
		FMemory::Copy(element->payload.vertices, vertices.data(), vertices.size() * sizeof(GlyphVertex));
		element->payload.verticesCount = vertices.size();
		element->payload.bRemoveScale = bRemoveScale;
		element->payload.bRemoveRotation = bRemoveRotation;
		return *element;
	}


	template< typename CharT >
	RenderBatchedElement& RenderBatchedElementList::addText(Color4Type const& color, Vector2 const& pos, FontDrawer& front, CharT const* str, int charCount, bool bRemoveScale, bool bRemoveRotation)
	{
		CHECK(charCount > 0);

		int verticesCount = 4 * (charCount);
		TRenderBatchedElement<TextPayload>* element = addElement< TextPayload >();
		element->type = RenderBatchedElement::Text;
		element->payload.color = color;
		element->payload.vertices = (GlyphVertex*)mAllocator.alloc(verticesCount * sizeof(GlyphVertex));
		element->payload.verticesCount = verticesCount;
		element->payload.bRemoveScale = bRemoveScale;
		element->payload.bRemoveRotation = bRemoveRotation;
		front.generateVertices(pos, str, element->payload.vertices);

		return *element;
	}

	template< typename CharT >
	RenderBatchedElement& RenderBatchedElementList::addText(Color4Type const& color, Vector2 const& pos, float scale, FontDrawer& front, CharT const* str, int charCount, bool bRemoveScale, bool bRemoveRotation)
	{
		CHECK(charCount > 0);

		int verticesCount = 4 * (charCount);
		TRenderBatchedElement<TextPayload>* element = addElement< TextPayload >();
		element->type = RenderBatchedElement::Text;
		element->payload.color = color;
		element->payload.vertices = (GlyphVertex*)mAllocator.alloc(verticesCount * sizeof(GlyphVertex));
		element->payload.verticesCount = verticesCount;
		element->payload.bRemoveScale = bRemoveScale;
		element->payload.bRemoveRotation = bRemoveRotation;
		front.generateVertices(pos, str, scale, element->payload.vertices);

		return *element;
	}



	RenderBatchedElement& RenderBatchedElementList::addText(TArray< Color4Type > const& colors, TArray< GlyphVertex > const& vertices,  bool bRemoveScale, bool bRemoveRotation)
	{
		TRenderBatchedElement<ColoredTextPayload>* element = addElement< ColoredTextPayload >();
		element->type = RenderBatchedElement::ColoredText;
		element->payload.vertices = (GlyphVertex*)mAllocator.alloc(vertices.size() * sizeof(GlyphVertex));
		FMemory::Copy(element->payload.vertices, vertices.data(), vertices.size() * sizeof(GlyphVertex));
		element->payload.colors = (Color4Type*)mAllocator.alloc(colors.size() * sizeof(Color4Type));
		FMemory::Copy(element->payload.colors, colors.data(), colors.size() * sizeof(Color4Type));
		element->payload.verticesCount = vertices.size();
		element->payload.bRemoveScale = bRemoveScale;
		element->payload.bRemoveRotation = bRemoveRotation;

		return *element;
	}

	template RenderBatchedElement& RenderBatchedElementList::addText<char>(Color4Type const& color, Vector2 const& pos, FontDrawer& front, char const* str, int charCount, bool bRemoveScale, bool bRemoveRotation);
	template RenderBatchedElement& RenderBatchedElementList::addText<wchar_t>(Color4Type const& color, Vector2 const& pos, FontDrawer& front, wchar_t const* str, int charCount, bool bRemoveScale, bool bRemoveRotation);

	template RenderBatchedElement& RenderBatchedElementList::addText<char>(Color4Type const& color, Vector2 const& pos, float scale, FontDrawer& front, char const* str, int charCount, bool bRemoveScale, bool bRemoveRotation);
	template RenderBatchedElement& RenderBatchedElementList::addText<wchar_t>(Color4Type const& color, Vector2 const& pos, float scale, FontDrawer& front, wchar_t const* str, int charCount, bool bRemoveScale, bool bRemoveRotation);

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

	RenderBatchedElement& RenderBatchedElementList::addCustomState(ICustomElementRenderer* renderer, EObjectManageMode mode)
	{
		TRenderBatchedElement<CustomRenderPayload>* element = addElement< CustomRenderPayload >();
		element->type = RenderBatchedElement::CustomState;
		element->payload.renderer = renderer;
		element->payload.manageMode = mode;
		return *element;
	}

	RenderBatchedElement& RenderBatchedElementList::addCustomRender(ICustomElementRenderer* renderer, EObjectManageMode mode, bool bChangeState)
	{
		TRenderBatchedElement<CustomRenderPayload>* element = addElement< CustomRenderPayload >();
		element->type = bChangeState ? RenderBatchedElement::CustomRenderAndState : RenderBatchedElement::CustomRender;
		element->payload.renderer = renderer;
		element->payload.manageMode = mode;
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
		CHECK(numV > 2);
		int numTriangle = (numV - 2);
		FetchedData data = fetchBuffer(numV, 3 * numTriangle);
		uint32 baseIndex = data.base;
		TexVertex* pVertices = data.vertices;
		uint32* pIndices = data.indices;

		for (int i = 0; i < numV; ++i)
		{
			TexVertex& vertex = pVertices[i];
			vertex.pos = v[i];
			vertex.color = color;
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

	void BatchedRender::emitTriangleList(Vector2 v[], int numV, ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			CHECK(numV > 2);
			int numTriangle = numV / 3;
			int numIndices = numTriangle * 3;
			FetchedData data = fetchBuffer(numV, numIndices);
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;

			for (int i = 0; i < numIndices; ++i)
			{
				TexVertex& vertex = pVertices[i];
				vertex.pos = v[i];
				vertex.color = paintArgs.brushColor;
			}

			pIndices = FillTriangleList(pIndices, baseIndex, numTriangle);
		}
	}

	void BatchedRender::emitTriangleStrip(Vector2 v[], int numV, ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			CHECK(numV > 2);
			int numTriangle = numV - 2;
			int numIndices  = numTriangle * 3;
			FetchedData data = fetchBuffer(numV, numIndices);
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;

			for (int i = 0; i < numV; ++i)
			{
				TexVertex& vertex = pVertices[i];
				vertex.pos = v[i];
				vertex.color = paintArgs.brushColor;
			}

			pIndices = FillTriangleStrip(pIndices, baseIndex, numTriangle);
		}
	}

	void BatchedRender::emitPolygon(ShapeCachedData& cachedData, RenderTransform2D const& xForm, ShapePaintArgs const& paintArgs)
	{
		if (paintArgs.bUseBrush)
		{
			PROFILE_ENTRY("EmitCachedPolygon");
			CHECK(cachedData.posList.size() > 2);
			int numTriangle = (cachedData.posList.size() - 2);

			FetchedData data = fetchBuffer(cachedData.posList.size(), 3 * numTriangle);
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;

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

			FetchedData data = fetchBuffer(2 * cachedData.posList.size(), 3 * 2 * cachedData.posList.size());
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;


			float halfWidth = 0.5 * float(paintArgs.penWidth);
			Vector2 scale = Vector2(halfWidth, halfWidth).div( xForm.getScale() );
			if ( xForm.isUniformScale())
			{
				for (int i = 0; i < lineData->posList.size(); i += 2)
				{
					Vector2 pos = xForm.transformPosition(lineData->posList[i]);
					Vector2 offset = scale * xForm.transformVector(lineData->posList[i + 1]);
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
					Vector2 offset = scale * xForm.transformVector(lineData->posList[i + 1]);
					pVertices[i].pos = pos + offset;
					pVertices[i].color = paintArgs.penColor;
					pVertices[i + 1].pos = pos - offset;
					pVertices[i + 1].color = paintArgs.penColor;
				}
			}
#else
			FetchedData data = fetchBuffer(4 * cachedData.posList.size(), 4 * 6 * cachedData.posList.size());
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;

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
			FetchedData data = fetchBuffer(4 , 3 * 2);
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;

			pVertices[0] = { v[0] , paintArgs.brushColor };
			pVertices[1] = { v[1] , paintArgs.brushColor };
			pVertices[2] = { v[2] , paintArgs.brushColor };
			pVertices[3] = { v[3] , paintArgs.brushColor };

			FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
		}

		if (paintArgs.bUsePen)
		{
			float halfWidth = 0.5 * float(paintArgs.penWidth);
			FetchedData data = fetchBuffer(4 * 2, 3 * 2 * 4);
			uint32 baseIndex = data.base;
			TexVertex* pVertices = data.vertices;
			uint32* pIndices = data.indices;

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

	BatchedRender::TexVertex* BatchedRender::fetchVertex(uint32 size, int& baseIndex)
	{
		if (!mTexVertexBuffer.canFetch(size))
		{
			flushDrawCommand();
			if (!tryLockBuffer())
				return nullptr;
		}

		baseIndex = mTexVertexBuffer.usedCount;
		return mTexVertexBuffer.fetch(size);

	}

	uint32* BatchedRender::fetchIndex(uint32 size)
	{
		if (!mIndexBuffer.canFetch(size))
		{
			flushDrawCommand();
			if (!tryLockBuffer())
				return nullptr;
		}
		return mIndexBuffer.fetch(size);
	}

	BatchedRender::FetchedData BatchedRender::fetchBuffer(uint32 vSize, uint32 iSize)
	{
		//CHECK(vSize > 0 && iSize > 0);
		if (!mTexVertexBuffer.canFetch(vSize) ||
			!mIndexBuffer.canFetch(iSize))
		{
			flushDrawCommand();
			if (!tryLockBuffer())
			{
				FetchedData data;
				data.base = 0;
				data.indices = nullptr;
				data.vertices = nullptr;
				return data;
			}
		}
		FetchedData data;
		data.base = mTexVertexBuffer.usedCount;
		data.vertices = mTexVertexBuffer.fetch(vSize);
		data.indices = mIndexBuffer.fetch(iSize);
		return data;
	}

	void BatchedRender::emitPolygonLine(Vector2 v[], int numV, Color4Type const& color, int lineWidth)
	{
		float halfWidth = 0.5 * float(lineWidth);
#if USE_POLYGON_LINE_NEW
		FetchedData data = fetchBuffer(2 * numV, 3 * 2 * numV);
		uint32 baseIndex = data.base;
		TexVertex* pVertices = data.vertices;
		uint32* pIndices = data.indices;

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

		FetchedData data = fetchBuffer(4 * numV, 4 * 6 * numV);
		uint32 baseIndex = data.base;
		TexVertex* pVertices = data.vertices;
		uint32* pIndices = data.indices;

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
		mProgramCur = nullptr;
		RHISetViewport(commandList, 0, 0, mWidth, mHeight);
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
		if (!tryLockBuffer())
			return;

		updateRenderState(*mCommandList, renderState);
		emitElements(elementList.mElements, renderState);
		elementList.releaseElements();
		flushDrawCommand();
		elementList.mAllocator.clearFrame();
#endif
	}

	void BatchedRender::setViewportSize(int width, int height)
	{
		mWidth = width;
		mHeight = height;
		mBaseTransform = AdjustProjectionMatrixForRHI(OrthoMatrix(0, mWidth, mHeight, 0, -1, 1));
	}

	void BatchedRender::flushGroup()
	{
		if (mGroups.empty())
			return;

		ON_SCOPE_EXIT
		{
			mGroups.clear();
		};

		if (!tryLockBuffer())
			return;

		mIndexRender = 0;
		for (int index = 0; index < mGroups.size(); ++index)
		{
			mIndexEmit = index;
			RenderGroup& group = mGroups[index];
			group.indexStart = mIndexBuffer.usedCount;
			emitElements(group.elements, group.state);
			group.indexCount = mIndexBuffer.usedCount - group.indexStart;
		}

		flushDrawCommand();
	}

	void BatchedRender::flushDrawCommand(bool bForceUpdateState)
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
				RHIDrawIndexedPrimitive(*mCommandList, EPrimitive::TriangleList, group.indexStart, count);
			}
			else if (bForceUpdateState)
			{
				updateRenderState(*mCommandList, group.state);
			}
			group.indexStart = 0;
		}
#else
		RHIDrawIndexedPrimitive(*mCommandList, EPrimitive::TriangleList, 0, mIndexBuffer.usedCount);
#endif
	}

	void BatchedRender::emitElements(TArray<RenderBatchedElement*> const& elements, RenderState const& renderState)
	{
		for (RenderBatchedElement* element : elements)
		{
			switch (element->type)
			{
			case RenderBatchedElement::Point:
				{
					RenderBatchedElementList::PointPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::PointPayload >(element);
					FetchedData data = fetchBuffer(4, 6);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;
					uint32* pIndices = data.indices;

					float size = payload.size;
					if (size == 0)
					{
						size = 1;
					}

					pVertices[0] = { payload.pos, payload.color , Vector2::Zero() };
					pVertices[1] = { payload.pos + Vector2(size,0), payload.color , Vector2::Zero() };
					pVertices[2] = { payload.pos + Vector2(size,size), payload.color , Vector2::Zero() };
					pVertices[3] = { payload.pos + Vector2(0,size), payload.color , Vector2::Zero() };

					FillQuad(pIndices, baseIndex, baseIndex + 1, baseIndex + 2, baseIndex + 3);
				}
				break;
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
					RenderBatchedElementList::TrianglePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::TrianglePayload >(element);
					emitPolygon(payload.posList, payload.posCount, payload.paintArgs);
				}
				break;
			case RenderBatchedElement::TriangleList:
				{
					RenderBatchedElementList::TrianglePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::TrianglePayload >(element);
					emitTriangleList(payload.posList, payload.posCount, payload.paintArgs);
				}
				break;
			case RenderBatchedElement::TriangleStrip:
				{
					RenderBatchedElementList::TrianglePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::TrianglePayload >(element);
					emitTriangleStrip(payload.posList, payload.posCount, payload.paintArgs);
				}
				break;
			case RenderBatchedElement::Circle:
				{
					PROFILE_ENTRY("Emit Circle");
					RenderBatchedElementList::CirclePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::CirclePayload >(element);
#if USE_SHAPE_CACHE
					ShapeCachedData* cachedData = GetShapeCache().getCircle(GetCircleSemgmentNum(payload.radius));
					RenderTransform2D xForm = RenderTransform2D(Vector2(payload.radius, payload.radius), payload.pos) * element->transform;
					emitPolygon(*cachedData, xForm, payload.paintArgs);
#else
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					builder.buildCircle(payload.pos, payload.radius, GetCircleSemgmentNum(payload.radius));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
#endif
				}
				break;
			case RenderBatchedElement::Ellipse:
				{
					RenderBatchedElementList::EllipsePayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::EllipsePayload >(element);
#if USE_SHAPE_CACHE
					float yFactor = float(payload.size.y) / payload.size.x;
					ShapeCachedData* cachedData = GetShapeCache().getEllipse(yFactor, GetCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
					RenderTransform2D xForm = RenderTransform2D(Vector2(payload.size.x, payload.size.x), payload.pos) * element->transform;
					emitPolygon(*cachedData, xForm, payload.paintArgs);
#else
					ShapeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					float yFactor = float(payload.size.y) / payload.size.x;
					builder.buildEllipse(payload.pos, payload.size.x, yFactor, GetCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
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

					FetchedData data = fetchBuffer(4, 6);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;
					uint32* pIndices = data.indices;

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

					FetchedData data = fetchBuffer(3 * 2, 4 * 3);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;
					uint32* pIndices = data.indices;

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
					FetchedData data = fetchBuffer(4 * 2, 4 * 6);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;
					uint32* pIndices = data.indices;

					for (int i = 0; i < 2; ++i)
					{
						pVertices[0] = { positions[i] + offset[0] , payload.color };
						pVertices[1] = { positions[i] + offset[1] , payload.color };
						pVertices[2] = { positions[i] + offset[2] , payload.color };
						pVertices[3] = { positions[i] + offset[3] , payload.color };
						pVertices += 4;
					}
					FillLineShapeIndices(pIndices, baseIndex, baseIndex + 4);
#endif
				}
				break;
			case RenderBatchedElement::LineStrip:
				{
					RenderBatchedElementList::LineStripPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::LineStripPayload >(element);
					emitLineStrip( TArrayView< Vector2 const >( payload.posList , payload.posCount ) , payload.color , payload.width );
				}
				break;
			case RenderBatchedElement::ArcLine:
				{
					RenderBatchedElementList::ArcLinePlayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::ArcLinePlayload >(element);

					ShapeVertexBuilder builder(mCachedPositionList);

					int numSegment = Math::CeilToInt(float(GetCircleSemgmentNum(payload.radius)) * payload.sweepAngle / (2 * Math::PI));

#if 1
					builder.mTransform = RenderTransform2D::Scale(payload.radius) * RenderTransform2D(payload.startAngle, payload.center) * element->transform;
					builder.buildArcLinePos(Vector2::Zero(), 1.0f, 0 , payload.sweepAngle, numSegment);
#else
					builder.mTransform = element->transform;
					builder.buildArcLinePos(payload.center, payload.radius, payload.startAngle, payload.sweepAngle, numSegment);
#endif
					emitLineStrip(mCachedPositionList, payload.color, payload.width);
				}
				break;
			case RenderBatchedElement::Text:
				{
					PROFILE_ENTRY("Text Vertices");
					RenderBatchedElementList::TextPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::TextPayload >(element);

					int numChar = payload.verticesCount / 4;
					CHECK(numChar);

					GlyphVertex* pSrcVertices = payload.vertices;
					
					FetchedData data = fetchBuffer(4 * numChar, 6 * numChar);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;
					uint32* pIndices = data.indices;

					if (payload.bRemoveScale || payload.bRemoveRotation)
					{
						RenderTransform2D transform = element->transform;
						if (payload.bRemoveRotation)
						{
							if (payload.bRemoveScale)
								transform.removeScaleAndRotation();
							else
								transform.removeRotation();
						}
						else if (payload.bRemoveScale)
						{
							transform.removeScale();
						}

						Vector2 pos = element->transform.transformPosition(pSrcVertices[0].pos);
						Vector2 v0 = pSrcVertices[0].pos;
						for (int i = 0; i < numChar; ++i)
						{
							pVertices[0] = { pos + transform.transformVector(pSrcVertices[0].pos - v0), payload.color, pSrcVertices[0].uv };
							pVertices[1] = { pos + transform.transformVector(pSrcVertices[1].pos - v0), payload.color, pSrcVertices[1].uv };
							pVertices[2] = { pos + transform.transformVector(pSrcVertices[2].pos - v0), payload.color, pSrcVertices[2].uv };
							pVertices[3] = { pos + transform.transformVector(pSrcVertices[3].pos - v0), payload.color, pSrcVertices[3].uv };

							pVertices += 4;
							pSrcVertices += 4;
						}
					}
					else
					{
						for (int i = 0; i < numChar; ++i)
						{
							pVertices[0] = { element->transform.transformPosition(pSrcVertices[0].pos), payload.color, pSrcVertices[0].uv };
							pVertices[1] = { element->transform.transformPosition(pSrcVertices[1].pos), payload.color, pSrcVertices[1].uv };
							pVertices[2] = { element->transform.transformPosition(pSrcVertices[2].pos), payload.color, pSrcVertices[2].uv };
							pVertices[3] = { element->transform.transformPosition(pSrcVertices[3].pos), payload.color, pSrcVertices[3].uv };

							pVertices += 4;
							pSrcVertices += 4;
						}
					}
					pIndices = FillQuadSequence(pIndices, baseIndex, numChar);
				}
				break;
			case RenderBatchedElement::ColoredText:
				{
					PROFILE_ENTRY("Text Vertices");
					RenderBatchedElementList::ColoredTextPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::ColoredTextPayload >(element);

					int numChar = payload.verticesCount / 4;
					CHECK(numChar);

					GlyphVertex* pSrcVertices = payload.vertices;
					Color4Type* pColors = payload.colors;

					FetchedData data = fetchBuffer(4 * numChar, 6 * numChar);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;

					uint32* pIndices = data.indices;

					if (payload.bRemoveScale || payload.bRemoveRotation)
					{
						RenderTransform2D transform = element->transform;
						if (payload.bRemoveRotation)
						{
							if (payload.bRemoveScale)
								transform.removeScaleAndRotation();
							else
								transform.removeRotation();
						}
						else if (payload.bRemoveScale)
						{
							transform.removeScale();
						}

						Vector2 pos = element->transform.transformPosition(pSrcVertices[0].pos);
						Vector2 v0 = pSrcVertices[0].pos;
						for (int i = 0; i < numChar; ++i)
						{
							pVertices[0] = { pos + transform.transformVector(pSrcVertices[0].pos - v0), pColors[i], pSrcVertices[0].uv };
							pVertices[1] = { pos + transform.transformVector(pSrcVertices[1].pos - v0), pColors[i], pSrcVertices[1].uv };
							pVertices[2] = { pos + transform.transformVector(pSrcVertices[2].pos - v0), pColors[i], pSrcVertices[2].uv };
							pVertices[3] = { pos + transform.transformVector(pSrcVertices[3].pos - v0), pColors[i], pSrcVertices[3].uv };

							pVertices += 4;
							pSrcVertices += 4;
						}
					}
					else
					{
						for (int i = 0; i < numChar; ++i)
						{
							pVertices[0] = { element->transform.transformPosition(pSrcVertices[0].pos), pColors[i], pSrcVertices[0].uv };
							pVertices[1] = { element->transform.transformPosition(pSrcVertices[1].pos), pColors[i], pSrcVertices[1].uv };
							pVertices[2] = { element->transform.transformPosition(pSrcVertices[2].pos), pColors[i], pSrcVertices[2].uv };
							pVertices[3] = { element->transform.transformPosition(pSrcVertices[3].pos), pColors[i], pSrcVertices[3].uv };

							pVertices += 4;
							pSrcVertices += 4;
						}
					}
					pIndices = FillQuadSequence(pIndices, baseIndex, numChar);
				}
				break;
			case RenderBatchedElement::GradientRect:
				{
					RenderBatchedElementList::GradientRectPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::GradientRectPayload >(element);

					FetchedData data = fetchBuffer(4 , 6);
					uint32 baseIndex = data.base;
					TexVertex* pVertices = data.vertices;
					uint32* pIndices = data.indices;

					pVertices[0] = { element->transform.transformPosition(payload.posLT) , payload.colorLT };
					pVertices[1] = { element->transform.transformPosition(Vector2(payload.posLT.x, payload.posRB.y)), payload.bHGrad ? payload.colorLT : payload.colorRB };
					pVertices[2] = { element->transform.transformPosition(payload.posRB), payload.colorRB };
					pVertices[3] = { element->transform.transformPosition(Vector2(payload.posRB.x, payload.posLT.y)),  payload.bHGrad ? payload.colorRB : payload.colorLT };

					pIndices = FillQuad(pIndices, baseIndex, 0, 1, 2, 3);
				}
				break;
			case RenderBatchedElement::CustomState:
				{
					RenderBatchedElementList::CustomRenderPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::CustomRenderPayload >(element);

					flushDrawCommand();
					if (!tryLockBuffer())
					{

					}

					if (payload.renderer)
					{
						mbCustomState = true;
						payload.renderer->render(*mCommandList, mBaseTransform, *element, renderState);
						FObjectManage::Release(payload.renderer, payload.manageMode);
					}
					else
					{
						mbCustomState = false;
						commitRenderState(*mCommandList, renderState);
					}
				}
				break;
			case RenderBatchedElement::CustomRender:
			case RenderBatchedElement::CustomRenderAndState:
				{
					RenderBatchedElementList::CustomRenderPayload& payload = RenderBatchedElementList::GetPayload< RenderBatchedElementList::CustomRenderPayload >(element);

					flushDrawCommand(true);
					if (!tryLockBuffer())
					{

					}

					payload.renderer->render(*mCommandList, mBaseTransform, *element, renderState);
					if (element->type == RenderBatchedElement::CustomRenderAndState)
					{
						commitRenderState(*mCommandList, renderState);
					}
					FObjectManage::Release(payload.renderer, payload.manageMode);
				}
				break;

			}
		}
	}

	void BatchedRender::emitLineStrip(TArrayView< Vector2 const > posList, Color4Type const& color, int width)
	{
		int lineCount = posList.size() - 1;

		float halfWidth = 0.5 * float(width);
		Vector2 offset[4] =
		{
			Vector2(-halfWidth,-halfWidth) ,
			Vector2(-halfWidth,halfWidth)  ,
			Vector2(halfWidth,halfWidth),
			Vector2(halfWidth,-halfWidth)
		};
#if USE_NEW_LINE_IDNEX
		FetchedData data = fetchBuffer(3 * 2 * lineCount, 4 * 3 * lineCount);
#else
		FetchedData data = fetchBuffer(4 * 2 * lineCount, 4 * 6 * lineCount);
#endif
		uint32 baseIndex = data.base;
		TexVertex* pVertices = data.vertices;
		uint32* pIndices = data.indices;

		for (int i = 0; i < lineCount; ++i)
		{
			Vector2 positions[2];
			positions[0] = posList[i];
			positions[1] = posList[i + 1];
#if USE_NEW_LINE_IDNEX
			Vector2 dir = positions[1] - positions[0];
			int index0 = dir.x < 0 ? (dir.y < 0 ? 0 : 1) : (dir.y < 0 ? 3 : 2);
			int index1 = (index0 + 1) % 4;
			int index2 = (index0 + 2) % 4;
			int index3 = (index0 + 3) % 4;

			pVertices[0] = { positions[0] + offset[index2] , color };
			pVertices[1] = { positions[0] + offset[index3] , color };
			pVertices[2] = { positions[0] + offset[index1] , color };
			pVertices[3] = { positions[1] + offset[index0] , color };
			pVertices[4] = { positions[1] + offset[index1] , color };
			pVertices[5] = { positions[1] + offset[index3] , color };
			pVertices += 6;

			pIndices = FillTriangle(pIndices, baseIndex, 0, 1, 2);
			pIndices = FillTriangle(pIndices, baseIndex, 3, 4, 5);
			pIndices = FillQuad(pIndices, baseIndex, 2, 1, 5, 4);
			baseIndex += 6;
#else
			for (int i = 0; i < 2; ++i)
			{
				pVertices[0] = { positions[i] + offset[0] , color };
				pVertices[1] = { positions[i] + offset[1] , color };
				pVertices[2] = { positions[i] + offset[2] , color };
				pVertices[3] = { positions[i] + offset[3] , color };
				pVertices += 4;
			}

			pIndices = FillLineShapeIndices(pIndices, baseIndex, baseIndex + 4);
			baseIndex += 8;
#endif
		}
	}

	void BatchedRender::flush()
	{
#if USE_BATCHED_GROUP
		flushGroup();
#endif
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

	uint32 const AttributeMask = BIT(EVertex::ATTRIBUTE_COLOR) | BIT(EVertex::ATTRIBUTE_TEXCOORD);

	void SetRDBState(RHICommandList& commandList, BatchedRender::RenderState const& state)
	{
		RHISetDepthStencilState(commandList, BatchedRender::GraphicsDepthState::GetRHI());
		RHISetBlendState(commandList, BatchedRender::GetBlendState(state.blendMode));
		RHISetRasterizerState(commandList, BatchedRender::GetRasterizerState(state.bEnableScissor));
		if (state.bEnableScissor)
		{
			auto const& rect = state.scissorRect;
			RHISetScissorRect(commandList, rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);
		}
	}

	void BatchedRender::SetupShaderState(RHICommandList& commandList, Math::Matrix4 const& baseXForm, RenderState const& state)
	{
		SimplePipelineProgram* program = SimplePipelineProgram::Get(AttributeMask, state.texture != nullptr);
		CHECK(program);

		RHISetShaderProgram(commandList, program->getRHI());
		SET_SHADER_PARAM(commandList, *program, XForm, baseXForm);
		SET_SHADER_PARAM(commandList, *program, Color, LinearColor(1, 1, 1, 1));
		if (state.texture)
		{
			program->setTextureParam(commandList, state.texture, state.sampler);
		}
	}

	void BatchedRender::commitRenderState(RHICommandList& commandList, RenderState const& state)
	{
		RHISetViewport(commandList, 0, 0, mWidth, mHeight);
		SetRDBState(commandList, state);
		SimplePipelineProgram* program = SimplePipelineProgram::Get(AttributeMask, state.texture != nullptr);
		RHISetShaderProgram(commandList, program->getRHI());

		mProgramCur = program;
		program->setParameters(commandList, mBaseTransform, LinearColor(1, 1, 1, 1), state.texture, state.sampler);
		setupInputState(commandList);
	}

	void BatchedRender::updateRenderState(RHICommandList& commandList, RenderState const& state)
	{
		if ( mbCustomState )
			return;

		SetRDBState(commandList, state);
		SimplePipelineProgram* program = SimplePipelineProgram::Get(AttributeMask, state.texture != nullptr);
		if (program != mProgramCur)
		{
			mProgramCur = program;
			RHISetShaderProgram(commandList, program->getRHI());
			SET_SHADER_PARAM(commandList, *program, XForm, mBaseTransform);
			SET_SHADER_PARAM(commandList, *program, Color, LinearColor(1, 1, 1, 1));
		}
		if (state.texture)
		{
			program->setTextureParam(commandList, state.texture, state.sampler);
		}
		setupInputState(commandList);
	}

	RHIBlendState& GraphicsDefinition::GetBlendState(ESimpleBlendMode blendMode)
	{
		switch (blendMode)
		{
		case ESimpleBlendMode::Translucent:
			return StaticTranslucentBlendState::GetRHI();
		case ESimpleBlendMode::Add:
			return TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI();
		case ESimpleBlendMode::Multiply:
			return TStaticBlendState< CWM_RGBA, EBlend::DestColor, EBlend::Zero >::GetRHI();
		case ESimpleBlendMode::InvDestColor:
			return TStaticBlendState< CWM_RGBA, EBlend::OneMinusDestColor, EBlend::Zero >::GetRHI();
		case ESimpleBlendMode::None:
			return TStaticBlendState<>::GetRHI();
		}
		NEVER_REACH("GetBlendState");
		return TStaticBlendState<>::GetRHI();
	}

	RHIRasterizerState& GraphicsDefinition::GetRasterizerState(bool bEnableScissor)
	{
		if (bEnableScissor)
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