#pragma once

#ifndef BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B
#define BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B

#include <vector>
#include "Math/Base.h"
#include "RHI/RHICommon.h"
#include "Renderer/RenderTransform2D.h"

#include "FrameAllocator.h"
#include "TypeConstruct.h"
#include "RHI/Font.h"


namespace Render
{

	static inline int calcCircleSemgmentNum(int r)
	{
		return std::max(4 * (r / 2), 32) * 10;
	}

	class ShapeVertexBuilder
	{
	public:

		ShapeVertexBuilder(std::vector< Vector2 >& buffer)
			:mBuffer(buffer)
		{

		}

		void buildRect(Vector2 const& p1, Vector2 const& p2);

		void buildCircle(Vector2 const& pos, float r, int numSegment);
		void buildEllipse(Vector2 const& pos, float r, float yFactor, int numSegment);

		template< class TVector2 >
		void buildPolygon(TVector2 pos[], int num)
		{
			mBuffer.resize(num);
			Vector2* pVertices = mBuffer.data();
			auto EmitVertex = [&](TVector2 const& p)
			{
				*pVertices = mTransform.transformPosition(p);
				++pVertices;
			};
			for (int i = 0; i < num; ++i)
			{
				EmitVertex(pos[i]);
			}
		}

		void buildRoundRect(Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius);


		RenderTransform2D       mTransform;
		std::vector< Vector2 >& mBuffer;
	};



	struct RenderBachedElement
	{
		enum EType
		{
			Rect,
			Circle,
			Polygon,
			Ellipse,
			RoundRect,
			TextureRect,
			Line,
			LineStrip,
			Text ,
			GradientRect ,
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

	struct ShapePaintArgs
	{
		using Color4Type = Color4f;
		using Color3Type = Color3f;


		Color4Type penColor;
		Color4Type brushColor;
		bool bUsePen;
		bool bUseBrush;
		int  penWidth;
	};


	class RenderBachedElementList
	{
	public:
		using Color4Type = ShapePaintArgs::Color4Type;
		using Color3Type = ShapePaintArgs::Color3Type;

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

		struct LineStripPayload : ShapePayload
		{
			Color4Type color;
			std::vector< Vector2 > posList;
			int     width;
		};

		struct TextureRectPayload
		{
			Color4Type color;
			Vector2 min;
			Vector2 max;
			Vector2 uvMin;
			Vector2 uvMax;
		};

		struct LinePayload
		{
			Color4Type color;
			Vector2 p1;
			Vector2 p2;
			int     width;
		};

		struct TextPayload
		{
			Color4Type color;
			std::vector< FontVertex > vertices;
		};

		struct GradientRectPayload
		{
			Vector2 posLT;
			Color4Type colorLT;
			Vector2 posRB;
			Color4Type colorRB;
			bool bHGrad;
		};

		template < class TPayload >
		static TPayload& GetPayload(RenderBachedElement* ptr)
		{
			return static_cast<TRenderBachedElement<TPayload>*> (ptr)->payload;
		}

		RenderBachedElement& addRect(ShapePaintArgs const& paintArgs, Vector2 const& min, Vector2 const& max);
		RenderBachedElement& addRoundRect(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius);

		template< class TVector2 >
		RenderBachedElement& addPolygon(RenderTransform2D const& transform, ShapePaintArgs const& paintArgs, TVector2 const v[], int numV)
		{
			CHECK(numV > 2);
			TRenderBachedElement<PolygonPayload>* element = addElement< PolygonPayload >();
			element->type = RenderBachedElement::Polygon;
			element->payload.paintArgs = paintArgs;
			element->payload.posList.resize(numV);
			for (int i = 0; i < numV; ++i)
			{
				element->payload.posList[i] = transform.transformPosition(v[i]);
			}
			return *element;
		}

		template< class TVector2 >
		RenderBachedElement& addLineStrip(RenderTransform2D const& transform, Color4Type const& color, TVector2 const v[], int numV, int width)
		{
			CHECK(numV >= 2);
			TRenderBachedElement<LineStripPayload>* element = addElement< LineStripPayload >();
			element->type = RenderBachedElement::LineStrip;
			element->payload.color = color;
			element->payload.width = width;
			element->payload.posList.resize(numV);
			for (int i = 0; i < numV; ++i)
			{
				element->payload.posList[i] = transform.transformPosition(v[i]);
			}
			return *element;
		}

		RenderBachedElement& addCircle( ShapePaintArgs const& paintArgs, Vector2 const& pos, float radius);
		RenderBachedElement& addEllipse( ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& size);
		RenderBachedElement& addTextureRect(Color4Type const& color, Vector2 const& min, Vector2 const& max, Vector2 const& uvMin, Vector2 const& uvMax);
		RenderBachedElement& addLine(Color4Type const& color, Vector2 const& p1, Vector2 const& p2, int width);
		RenderBachedElement& addText(Color4Type const& color, std::vector< FontVertex >&& vertices);
		RenderBachedElement& addGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad);

		template< class TPayload >
		TRenderBachedElement<TPayload>* addElement()
		{
			TRenderBachedElement<TPayload>* ptr = (TRenderBachedElement<TPayload>*)mAllocator.alloc(sizeof(TRenderBachedElement<TPayload>));
			TypeDataHelper::Construct(ptr);
			mElements.push_back(ptr);
			return ptr;
		}

		void releaseElements();
		bool isEmpty() const { return mElements.empty(); }

		FrameAllocator mAllocator;
		std::vector< RenderBachedElement* > mElements;
	};

	class BatchedRender
	{
	public:

		using Color4Type = ShapePaintArgs::Color4Type;
		using Color3Type = ShapePaintArgs::Color3Type;


		BatchedRender();

		void initializeRHI();
		void releaseRHI();
		void render(RHICommandList& commandList, RenderBachedElementList& elementList);
		void flushDrawCommond(RHICommandList& commandList);


		void emitPolygon(Vector2 v[], int numV, ShapePaintArgs const& paintArgs);
		void emitRect(Vector2 v[], ShapePaintArgs const& paintArgs);

		struct BaseVertex
		{
			Vector2  pos;
			Color4Type  color;
		};

		struct TexVertex
		{
			Vector2  pos;
			Color4Type  color;
			Vector2 uv;
		};

		BaseVertex* fetchBaseBuffer(int size, int& baseIndex)
		{
			baseIndex = mBaseVertices.size();
			mBaseVertices.resize(baseIndex + size);
			return &mBaseVertices[baseIndex];
		}
		TexVertex* fetchTexBuffer(int size, int& baseIndex)
		{
			baseIndex = mTexVerteices.size();
			mTexVerteices.resize(baseIndex + size);
			return &mTexVerteices[baseIndex];
		}
		uint32* fetchIndexBuffer(int size)
		{
			int index = mBaseIndices.size();
			mBaseIndices.resize(index + size);
			return &mBaseIndices[index];
		}

		void emitPolygon(Vector2 v[], int numV, Color4Type const& color);
		void emitPolygonLine(Vector2 v[], int numV, Color4Type const& color, int lineWidth);

		FORCEINLINE static uint32* FillTriangle(uint32* pIndices, int i0, int i1, int i2)
		{
			pIndices[0] = i0;
			pIndices[1] = i1;
			pIndices[2] = i2;
			return pIndices + 3;
		}
		FORCEINLINE static uint32* FillTriangle(uint32* pIndices, int base, int i0, int i1, int i2)
		{
			pIndices[0] = base + i0;
			pIndices[1] = base + i1;
			pIndices[2] = base + i2;
			return pIndices + 3;
		}

		FORCEINLINE static uint32* FillQuad(uint32* pIndices,int base, int i0, int i1, int i2, int i3)
		{
			pIndices = FillTriangle(pIndices, base, i0, i1, i2);
			pIndices = FillTriangle(pIndices, base, i0, i2, i3);
			return pIndices;
		}

		FORCEINLINE static uint32* FillQuad(uint32* pIndices, int i0, int i1, int i2, int i3)
		{
			pIndices = FillTriangle(pIndices, i0, i1, i2);
			pIndices = FillTriangle(pIndices, i0, i2, i3);
			return pIndices;
		}

		FORCEINLINE static uint32* FillLineShapeIndices(uint32* pIndices, int baseIndexA, int baseIndexB)
		{
			pIndices = FillQuad(pIndices, baseIndexA, baseIndexA + 1, baseIndexB + 1, baseIndexB);
			pIndices = FillQuad(pIndices, baseIndexA + 1, baseIndexA + 2, baseIndexB + 2, baseIndexB + 1);
			pIndices = FillQuad(pIndices, baseIndexA + 2, baseIndexA + 3, baseIndexB + 3, baseIndexB + 2);
			pIndices = FillQuad(pIndices, baseIndexA + 3, baseIndexA, baseIndexB, baseIndexB + 3);
			return pIndices;
		}

		std::vector< Vector2 >    mCachedPositionList;
		std::vector< BaseVertex > mBaseVertices;
		std::vector< uint32 >     mBaseIndices;

		std::vector< TexVertex > mTexVerteices;

		RHIVertexBufferRef mBaseVertexBuffer;
		int numBaseVertices;
		RHIVertexBufferRef mTexVertexBuffer;
		int numTexVertices;

		RHIIndexBufferRef mIndexBuffer;

	};




}//namespace Render

#endif // BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B
