#include "RHIGraphics2D.h"

#include "Math/Base.h"

#include "RHI/RHICommon.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/DrawUtility.h"

#include <cassert>
#include <algorithm>
#include "FrameAllocator.h"
#include "TypeConstruct.h"
#include "ConsoleSystem.h"


CORE_API extern TConsoleVariable<bool> CVarUseBachedRender2D;

#if CORE_SHARE_CODE
TConsoleVariable<bool> CVarUseBachedRender2D(true, "r.UseBachedRender2D");
#endif
using namespace Render;

static inline int calcCircleSemgmentNum(int r)
{
	return std::max(4 * (r / 2), 32) * 10;
}

class ShpaeVertexBuilder
{
public:
	ShpaeVertexBuilder(std::vector< Vector2 >& buffer)
		:mBuffer(buffer)
	{

	}


	void buildRect(Vector2 const& p1, Vector2 const& p2)
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

	void buildCircle(Vector2 const& pos, float r, int numSegment)
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
	void buildEllipse(Vector2 const& pos, float r, float yFactor, int numSegment)
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

	template< class TVector2 >
	void buildPolygon(TVector2 pos[], int num)
	{
		mBuffer.resize(num);
		Vector2* pVertices = mBuffer.data();
		auto EmitVertex = [&](Vector2 const& p)
		{
			*pVertices = mTransform.transformPosition(p);
			++pVertices;
		};
		for (int i = 0; i < num; ++i)
		{
			EmitVertex(pos[i]);
		}
	}


	void buildRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius)
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


	RenderTransform2D       mTransform;
	std::vector< Vector2 >& mBuffer;
};



struct RenderBachedElement
{
	enum EType
	{
		Rect,

		Circle ,
		Polygon ,
		Ellipse , 
		RoundRect ,
		TextureRect,
		Line ,
	};

	EType type;
	RenderTransform2D transform;
	//int32 layer;
};

template< class TPayload >
struct TRenderBachedElement : RenderBachedElement
{
	TPayload payload;
};

class RenderBachedElementList
{
public:
	RenderBachedElementList()
		:mAllocator(2048)
	{

	}

	struct ShapePayload
	{
		ShapePaintArgs paintArgs;
	};

	struct CirclePayload : ShapePayload
	{
		Vector2 pos;
		float radius;
	};

	struct EllipsePayload : ShapePayload
	{
		Vector2 pos;
		Vector2 size;
	};

	struct RectPayload : ShapePayload
	{
		Vector2 min;
		Vector2 max;
	};

	struct RoundRectPayload : ShapePayload
	{
		Vector2 pos;
		Vector2 rectSize;
		Vector2 circleRadius;
	};

	struct PolygonPayload : ShapePayload
	{
		std::vector< Vector2 > posList;
	};

	struct TextureRectPayload
	{
		Color4f color;
		Vector2 min;
		Vector2 max;
		Vector2 uvMin;
		Vector2 uvMax;
	};

	struct LinePayload
	{
		Color4f color;
		Vector2 p1;
		Vector2 p2;
		int     width;
	};

	template < class TPayload >
	static TPayload& GetPayload(RenderBachedElement* ptr)
	{
		return static_cast< TRenderBachedElement<TPayload>* > (ptr)->payload;
	}

	void addRect(RenderTransform2D const& transform , ShapePaintArgs const& paintArgs , Vector2 const& min, Vector2 const& max)
	{
		TRenderBachedElement<RectPayload>* element = addElement< RectPayload >();
		element->type = RenderBachedElement::Rect;
		element->transform = transform;
		element->payload.paintArgs = paintArgs;
		element->payload.min = min;
		element->payload.max = max;
	}

	void addRoundRect(RenderTransform2D const& transform, ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& rectSize , Vector2 const& circleRadius )
	{
		TRenderBachedElement<RoundRectPayload>* element = addElement< RoundRectPayload >();
		element->type = RenderBachedElement::RoundRect;
		element->transform = transform;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.rectSize = rectSize;
		element->payload.circleRadius = circleRadius;
	}

	template< class TVector2 >
	void addPolygon(RenderTransform2D const& transform, ShapePaintArgs const& paintArgs, TVector2 const v[] , int numV )
	{
		if (numV <= 2)
			return;

		TRenderBachedElement<PolygonPayload>* element = addElement< PolygonPayload >();
		element->type = RenderBachedElement::Polygon;
		element->transform = transform;
		element->payload.paintArgs = paintArgs;
		element->payload.posList.resize( numV );
		for (int i = 0; i < numV; ++i)
		{
			element->payload.posList[i] = transform.transformPosition(v[i]);
		}
	}

	void addCircle(RenderTransform2D const& transform, ShapePaintArgs const& paintArgs, Vector2 const& pos , float radius)
	{
		TRenderBachedElement<CirclePayload>* element = addElement< CirclePayload >();
		element->type = RenderBachedElement::Circle;
		element->transform = transform;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.radius = radius;
	}

	void addEllipse(RenderTransform2D const& transform, ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& size)
	{
		TRenderBachedElement<EllipsePayload>* element = addElement< EllipsePayload >();
		element->type = RenderBachedElement::Ellipse;
		element->transform = transform;
		element->payload.paintArgs = paintArgs;
		element->payload.pos = pos;
		element->payload.size = size;
	}

	void addTextureRect(RenderTransform2D const& transform, Color4f const& color , Vector2 const& min , Vector2 const& max , Vector2 const& uvMin , Vector2 const& uvMax )
	{
		TRenderBachedElement<TextureRectPayload>* element = addElement< TextureRectPayload >();
		element->type = RenderBachedElement::TextureRect;
		element->transform = transform;
		element->payload.color = color;
		element->payload.min = min;
		element->payload.max = max;
		element->payload.uvMin = uvMin;
		element->payload.uvMax = uvMax;
	}

	void addLine(RenderTransform2D const& transform, Color4f const& color, Vector2 const& p1, Vector2 const& p2, int width)
	{
		TRenderBachedElement<LinePayload>* element = addElement< LinePayload >();
		element->type = RenderBachedElement::Line;
		element->transform = transform;
		element->payload.color = color;
		element->payload.p1 = p1;
		element->payload.p2 = p2;
		element->payload.width = width;
	}

	template< class TPayload >
	TRenderBachedElement<TPayload>* addElement()
	{
		TRenderBachedElement<TPayload>* ptr = (TRenderBachedElement<TPayload>*)mAllocator.alloc(sizeof(TRenderBachedElement<TPayload>));
		TypeDataHelper::Construct(ptr);
		mElements.push_back(ptr);
		return ptr;
	}

	void releaseElements()
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
			}
		}

		mElements.clear();
		mAllocator.clearFrame();
	}

	FrameAllocator mAllocator;
	std::vector< RenderBachedElement* > mElements;
};

class BatchedDrawer
{
public:

	void render(RHICommandList& commandList , RenderBachedElementList& elementList)
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
					emitPolygon(vertices, 4, payload.paintArgs);
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
					ShpaeVertexBuilder builder(mCachedPositionList);
					builder.buildCircle(payload.pos, payload.radius, calcCircleSemgmentNum(payload.radius));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBachedElement::Ellipse:
				{
					RenderBachedElementList::EllipsePayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::EllipsePayload >(element);
					ShpaeVertexBuilder builder(mCachedPositionList);
					builder.mTransform = element->transform;
					float yFactor = float(payload.size.y) / payload.size.x;
					builder.buildEllipse(payload.pos, payload.size.x, yFactor, calcCircleSemgmentNum((payload.size.x + payload.size.y) / 2));
					emitPolygon(mCachedPositionList.data(), mCachedPositionList.size(), payload.paintArgs);
				}
				break;
			case RenderBachedElement::RoundRect:
				{
					RenderBachedElementList::RoundRectPayload& payload = RenderBachedElementList::GetPayload< RenderBachedElementList::RoundRectPayload >(element);
					ShpaeVertexBuilder builder(mCachedPositionList);
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

					int* pIndices = fetchIndexBuffer(6);
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

					int* pIndices = fetchIndexBuffer(4 * 6);
					EmitLineShapeIndices(pIndices, baseIndex, baseIndex + 4);
				}
				break;
			}
		}

		flushDrawCommond(commandList);
	}


	void flushDrawCommond(RHICommandList& commandList)
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

	std::vector< Vector2 > mCachedPositionList;

	void emitPolygon(Vector2 v[], int numV, ShapePaintArgs const& paintArgs)
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

	struct BaseVertex
	{
		Vector2  pos;
		Color4f  color;
	};

	struct TexVertex : BaseVertex
	{
		Vector2 uv;
	};

	BaseVertex* fetchBaseBuffer(int size , int& baseIndex)
	{
		baseIndex = mBaseVertices.size();
		mBaseVertices.resize(baseIndex + size);
		return &mBaseVertices[baseIndex];
	}

	int* fetchIndexBuffer(int size)
	{
		int index = mBaseIndices.size();
		mBaseIndices.resize(index + size);
		return &mBaseIndices[index];
	}

	void emitPolygon(Vector2 v[], int numV, Color4f const& color)
	{
		assert(numV > 2);
		int baseIndex;
		BaseVertex* pVertices = fetchBaseBuffer(numV, baseIndex);
		for (int i = 0; i < numV; ++i)
		{
			BaseVertex& vtx = mBaseVertices[baseIndex + i];
			pVertices->pos   = v[i];
			pVertices->color = color;
			++pVertices;
		}

		int numTriangle = (numV - 2);
		int* pIndices = fetchIndexBuffer(3 * numTriangle);
		for (int i = 0; i < numTriangle; ++i)
		{
			pIndices[0] = baseIndex;
			pIndices[1] = baseIndex + i + 1;
			pIndices[2] = baseIndex + i + 2;
			pIndices += 3;
		}
	}



	void emitPolygonLine(Vector2 v[], int numV, Color4ub const& color , int lineWidth )
	{
		float halfWidth = 0.5 * float(lineWidth);
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
	}

	static void FillQuad(int*& pIndices, int i0, int i1, int i2, int i3)
	{
		pIndices[0] = i0; pIndices[1] = i1; pIndices[2] = i2;
		pIndices[3] = i0; pIndices[4] = i2; pIndices[5] = i3;	
	}

	static void EmitLineShapeIndices( int*& pIndices, int baseIndexA , int baseIndexB )
	{
		FillQuad(pIndices, baseIndexA, baseIndexA + 1, baseIndexB, baseIndexB + 1); pIndices += 6;
		FillQuad(pIndices, baseIndexA + 1, baseIndexA + 2, baseIndexB + 1, baseIndexB + 2); pIndices += 6;
		FillQuad(pIndices, baseIndexA + 2, baseIndexA + 3, baseIndexB + 2, baseIndexB + 3); pIndices += 6;
		FillQuad(pIndices, baseIndexA + 3, baseIndexA, baseIndexB + 3, baseIndexB); pIndices += 6;
	}

	std::vector< BaseVertex > mBaseVertices;
	std::vector< int32 >      mBaseIndices;

	std::vector< TexVertex > mTexVerteices;
};



CORE_API extern RenderBachedElementList GBachedElementList;
CORE_API extern BatchedDrawer GBatchedDrawer;

#if CORE_SHARE_CODE
RenderBachedElementList GBachedElementList;
BatchedDrawer GBatchedDrawer;
#endif

RHIGraphics2D::RHIGraphics2D()
	: mWidth(1)
	, mHeight(1)
{
	mFont = nullptr;
	mPenWidth = 1;
	mDrawBrush = true;
	mDrawPen = true;
	mAlpha = 1.0;
	mColorBrush = Color3f(0, 0, 0);
	mColorPen = Color3f(0, 0, 0);
	mColorFont = Color3f(0, 0, 0);
}

void RHIGraphics2D::init(int w, int h)
{
	mWidth = w;
	mHeight = h;
}

void RHIGraphics2D::beginXForm()
{

}

void RHIGraphics2D::finishXForm()
{

}

void RHIGraphics2D::pushXForm()
{
	mXFormStack.push();
}

void RHIGraphics2D::popXForm()
{
	mXFormStack.pop();
}

void RHIGraphics2D::identityXForm()
{
	mXFormStack.set(RenderTransform2D::Identity());
}

void RHIGraphics2D::translateXForm(float ox, float oy)
{
	mXFormStack.translate(Vector2(ox, oy));
}

void RHIGraphics2D::rotateXForm(float angle)
{
	mXFormStack.rotate(Math::Deg2Rad(angle));
}

void RHIGraphics2D::scaleXForm(float sx, float sy)
{
	mXFormStack.scale(Vector2(sx, sy));
}

void RHIGraphics2D::beginRender()
{
	//glPushAttrib(GL_ENABLE_BIT | GL_VIEWPORT_BIT);

	using namespace Render;
	RHICommandList& commandList = GetCommandList();

	RHISetViewport(commandList, 0, 0, mWidth, mHeight);

	RHISetShaderProgram(commandList, nullptr);
	RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
	RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());

	mBaseTransform = AdjProjectionMatrixForRHI(OrthoMatrix(0, mWidth, mHeight, 0, -1, 1));

	RHISetFixedShaderPipelineState(commandList, mBaseTransform);
	mbResetPipelineState = false;
	mCurTexture = nullptr;
	RHISetInputStream(commandList, &TStaticRenderRTInputLayout<RTVF_XY>::GetRHI(), nullptr, 0);

	mXFormStack.clear();
	//glDisable(GL_TEXTURE_2D);
}

void RHIGraphics2D::endRender()
{
	RHICommandList& commandList = GetCommandList();

	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}

	RHIFlushCommand(commandList);
	//glPopAttrib();
}

void RHIGraphics2D::emitLineVertex(Vector2 const &p1, Vector2 const &p2)
{
	emitVertex(p1);
	emitVertex(p2);
}

void RHIGraphics2D::emitVertex(Vector2 const& v)
{
	Vector2 temp = mXFormStack.get().transformPosition(v);
	mBuffer.push_back(temp);
}

void RHIGraphics2D::drawPixel(Vector2 const& p, Color3ub const& color)
{
	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}
	struct Vertex_XY_CA
	{
		Math::Vector2 pos;
		Color4f c;
	} v = { p , Color4f(color , mAlpha) };
	TRenderRT<RTVF_XY_CA>::Draw(GetCommandList(), EPrimitive::Points, &v, 1);
}

void RHIGraphics2D::drawRect(int left, int top, int right, int bottom)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify();
		Vector2 p1(left, top);
		Vector2 p2(right, bottom);
		GBachedElementList.addRect(mXFormStack.get(), getPaintArgs(), p1, p2);
	}
	else
	{
		Vector2 p1(left, top);
		Vector2 p2(right, bottom);
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildRect(p1, p2);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawRect(Vector2 const& pos, Vector2 const& size)
{
	if (CVarUseBachedRender2D)
	{	
		checkRenderStateNeedModify();
		GBachedElementList.addRect(mXFormStack.get(), getPaintArgs(), pos, pos + size);
	}
	else
	{
		Vector2 p2 = pos + size;
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildRect(pos, p2);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawCircle(Vector2 const& center, float r)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify();
		GBachedElementList.addCircle(mXFormStack.get(), getPaintArgs(), center, r);
	}
	else
	{
		int numSeg = std::max(2 * r * r, 4.0f);
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildCircle(center, r, calcCircleSemgmentNum(r));
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawEllipse(Vector2 const& center, Vector2 const& size)
{

	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify();
		GBachedElementList.addEllipse(mXFormStack.get(), getPaintArgs(), center, size );
	}
	else
	{
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		float yFactor = float(size.y) / size.x;
		builder.buildEllipse(center, size.x, yFactor, calcCircleSemgmentNum((size.x + size.y) / 2));
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawPolygon(Vector2 pos[], int num)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify();
		GBachedElementList.addPolygon(mXFormStack.get(), getPaintArgs(), pos , num);
	}
	else
	{
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildPolygon(pos, num);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawPolygon(Vec2i pos[], int num)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify();
		GBachedElementList.addPolygon(mXFormStack.get(), getPaintArgs(), pos, num);
	}
	else
	{
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildPolygon(pos, num);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::drawLine(Vector2 const& p1, Vector2 const& p2)
{
	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
		checkRenderStateNeedModify();
		GBachedElementList.addLine(mXFormStack.get(), LinearColor(mColorPen, mAlpha), p1, p2, mPenWidth);
	}
	else
	{
		mBuffer.clear();
		emitLineVertex(p1, p2);
		drawLineBuffer();
	}
}

void RHIGraphics2D::drawRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleSize)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify();
		GBachedElementList.addRoundRect(mXFormStack.get(), getPaintArgs(),  pos, rectSize, circleSize / 2);
	}
	else
	{
		ShpaeVertexBuilder builder(mBuffer);
		builder.mTransform = mXFormStack.get();
		builder.buildRoundRect(pos, rectSize, circleSize / 2);
		drawPolygonBuffer();
	}
}

void RHIGraphics2D::fillGradientRect(Vector2 const& posLT, Color3ub const& colorLT, Vector2 const& posRB, Color3ub const& colorRB, bool bHGrad)
{

}

void RHIGraphics2D::setTextColor(Color3ub const& color)
{
	mColorFont = color;
}

void RHIGraphics2D::drawText(Vector2 const& pos, char const* str)
{
	if (!mFont || !mFont->isValid())
		return;

	int fontSize = mFont->getSize();
	float ox = pos.x;
	float oy = pos.y;
	drawTextImpl(ox, oy, str);
}

void RHIGraphics2D::drawText(Vector2 const& pos, Vector2 const& size, char const* str, bool beClip /*= false */)
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif

	if (!mFont || !mFont->isValid())
		return;
	if (beClip)
		beginClip(pos, size);

	Vector2 extent = mFont->calcTextExtent(str);
	int fontSize = mFont->getSize();
	int strLen = strlen(str);
	Vector2 renderPos = pos + (size - extent) / 2;
	drawTextImpl(renderPos.x, renderPos.y, str);

	if (beClip)
		endClip();
}

void RHIGraphics2D::drawTextImpl(float  ox, float  oy, char const* str)
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif
	preModifyRenderState();
	assert(mFont);
	Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
	mFont->draw(GetCommandList(), Vector2(int(ox), int(oy)), transform, LinearColor(mColorFont, mAlpha), str);
	mbResetPipelineState = true;
}


void RHIGraphics2D::drawPolygonBuffer()
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif

	assert(!mBuffer.empty());
	comitPipelineState();

	if (mDrawBrush)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::Polygon, mBuffer.data(), mBuffer.size() , LinearColor(mColorBrush, mAlpha));
	}
	if (mDrawPen)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() , LinearColor(mColorPen, mAlpha));
	}

}

void RHIGraphics2D::drawLineBuffer()
{
#if	IGNORE_NSIGHT_UNSUPPORT_CODE
	return;
#endif
	assert(!mBuffer.empty());
	comitPipelineState();
	if (mDrawPen)
	{
		TRenderRT<RTVF_XY>::Draw(GetCommandList(), EPrimitive::LineLoop, mBuffer.data(), mBuffer.size() , LinearColor(mColorPen, mAlpha));
	}
}


RHICommandList& RHIGraphics2D::GetCommandList()
{
	return RHICommandList::GetImmediateList();
}

void RHIGraphics2D::flushBatchedElements()
{
	if ( !GBachedElementList.mElements.empty() )
	{
		comitPipelineState();
		GBatchedDrawer.render(GetCommandList(), GBachedElementList);
		GBachedElementList.releaseElements();
	}
}

void RHIGraphics2D::checkRenderStateNeedModify(RHITexture2D* texture /*= nullptr*/)
{
	if (texture != mCurTexture)
	{
		preModifyRenderState();
		mCurTexture = texture;
		mbResetPipelineState = true;
	}
}

void RHIGraphics2D::preModifyRenderState()
{
	if (CVarUseBachedRender2D)
	{
		flushBatchedElements();
	}
}

void RHIGraphics2D::comitPipelineState()
{
	if (mbResetPipelineState)
	{
		mbResetPipelineState = false;
		RHISetFixedShaderPipelineState(GetCommandList(), mBaseTransform, LinearColor(1,1,1,1), mCurTexture);
	}
}

void RHIGraphics2D::drawTexture(RHITexture2D& texture, Vector2 const& pos)
{
	drawTexture(texture, pos, Vector2(texture.getSizeX(), texture.getSizeY()));
}

void RHIGraphics2D::drawTexture(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify(&texture);
		GBachedElementList.addTextureRect(mXFormStack.get(), LinearColor(mColorBrush, mAlpha), pos, pos + size, Vector2(0, 0), Vector2(1, 1));
	}
	else
	{
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		RHISetFixedShaderPipelineState(GetCommandList(), transform, LinearColor(mColorBrush, mAlpha), &texture);
		DrawUtility::Sprite(GetCommandList(), pos, size, Vector2(0, 0), Vector2(0, 0), Vector2(1, 1));
		mbResetPipelineState = true;
	}
}

void RHIGraphics2D::drawTexture(RHITexture2D& texture, Vector2 const& pos, Vector2 const& texPos, Vector2 const& texSize)
{
	drawTexture(texture, pos, Vector2(texture.getSizeX(), texture.getSizeY()), texPos, texSize);
}

void RHIGraphics2D::drawTexture(RHITexture2D& texture, Vector2 const& pos, Vector2 const& size, Vector2 const& texPos, Vector2 const& texSize)
{
	if (CVarUseBachedRender2D)
	{
		checkRenderStateNeedModify(&texture);
		Vector2 textureSize = Vector2(texture.getSizeX(), texture.getSizeY());
		Vector2 uvPos = Vector2(texPos.div(textureSize));
		Vector2 uvSize = Vector2(texSize.div(textureSize));
		GBachedElementList.addTextureRect(mXFormStack.get(), LinearColor(mColorBrush, mAlpha), pos, pos + size, uvPos , uvPos + uvSize);
	}
	else
	{
		Matrix4 transform = mXFormStack.get().toMatrix4() * mBaseTransform;
		RHISetFixedShaderPipelineState(GetCommandList(), transform, LinearColor(mColorBrush, mAlpha), &texture);
		Vector2 textureSize = Vector2(texture.getSizeX(), texture.getSizeY());
		DrawUtility::Sprite(GetCommandList(), pos, size, Vector2(0, 0), Vector2(texPos.div(textureSize)), Vector2(texSize.div(textureSize)));
		mbResetPipelineState = true;
	}
}

void RHIGraphics2D::setPen(Color3ub const& color, int width)
{
	mColorPen = color;
	if (mPenWidth != width)
	{
		if (GRHISystem->getName() == RHISytemName::OpenGL)
		{
			glLineWidth(width);
		}
		mPenWidth = width;
	}
}

void RHIGraphics2D::beginClip(Vec2i const& pos, Vec2i const& size)
{
	preModifyRenderState();
	RHISetRasterizerState(GetCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, true >::GetRHI());
	RHISetScissorRect(GetCommandList(), pos.x, mHeight - pos.y - size.y, size.x, size.y);
}

void RHIGraphics2D::endClip()
{
	preModifyRenderState();
	RHISetRasterizerState(GetCommandList(), TStaticRasterizerState< ECullMode::None, EFillMode::Solid, EFrontFace::Default, false >::GetRHI());
}

void RHIGraphics2D::beginBlend(Vector2 const& pos, Vector2 const& size, float alpha)
{
	beginBlend(alpha);
}

void RHIGraphics2D::beginBlend(float alpha, ESimpleBlendMode mode)
{
	preModifyRenderState();
	switch (mode)
	{
	case ESimpleBlendMode::Translucent:
		RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());
		break;
	case ESimpleBlendMode::Add:
		RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
		break;
	case ESimpleBlendMode::Multiply:
		RHISetBlendState(GetCommandList(), TStaticBlendState< CWM_RGBA, Blend::eDestColor, Blend::eZero >::GetRHI());
		break;
	default:
		NEVER_REACH("RHIGraphics2D::beginBlend");
	}

	mAlpha = alpha;
}

void RHIGraphics2D::endBlend()
{
	preModifyRenderState();
	RHISetBlendState(GetCommandList(), TStaticBlendState<>::GetRHI());
	mAlpha = 1.0f;
}
