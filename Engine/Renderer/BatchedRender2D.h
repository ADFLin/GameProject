#pragma once

#ifndef BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B
#define BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B

#include "Math/Base.h"
#include "RHI/RHICommon.h"
#include "Renderer/RenderTransform2D.h"

#include "FrameAllocator.h"
#include "TypeMemoryOp.h"
#include "RHI/Font.h"

#include "DataStructure/Array.h"

#define BATCHED_ELEMENT_NO_DESTRUCT 1


namespace Render
{

	static inline int calcCircleSemgmentNum(int r)
	{
		return std::max(4 * (r / 2), 32) * 10;
	}

	class ShapeVertexBuilder
	{
	public:

		ShapeVertexBuilder(TArray< Vector2 >& buffer)
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
		TArray< Vector2 >& mBuffer;
	};



	struct RenderBatchedElement
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
			ArcLine,
			Text ,
			GradientRect ,
		};

		EType type;
		RenderTransform2D transform;
		int32 layer;
	};

	template< class TPayload >
	struct TRenderBatchedElement : RenderBatchedElement
	{
		TPayload payload;
	};

	struct ShapePaintArgs
	{
#if 1
		using Color4Type = Color4ub;
		using Color3Type = Color3ub;
#else
		using Color4Type = Color4f;
		using Color3Type = Color3f;
#endif

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
#if BATCHED_ELEMENT_NO_DESTRUCT
			Vector2* posList;
			int posCount;
#else
			TArray< Vector2 > posList;
#endif
		};

		struct TextureRectPayload
		{
			Color4Type color;
			Vector2 min;
			Vector2 max;
			Vector2 uvMin;
			Vector2 uvMax;
		};

		struct LineShapePlayload
		{
			Color4Type color;
			int        width;
		};

		struct LinePayload
		{
			Color4Type color;
			Vector2 p1;
			Vector2 p2;
			int     width;
		};

		struct LineStripPayload
		{
			Color4Type color;
#if BATCHED_ELEMENT_NO_DESTRUCT
			Vector2* posList;
			int posCount;
#else
			TArray< Vector2 > posList;
#endif
			int     width;
		};

		struct ArcLinePlayload
		{
			Color4Type color;
			Vector2 center;
			float radius;
			float start;
			float end;
		};

		struct TextPayload
		{
			Color4Type color;
#if BATCHED_ELEMENT_NO_DESTRUCT
			FontVertex* vertices;
			int verticesCount;
#else
			TArray< FontVertex > vertices;
#endif
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
		static TPayload& GetPayload(RenderBatchedElement* ptr)
		{
			return static_cast<TRenderBatchedElement<TPayload>*> (ptr)->payload;
		}

		RenderBatchedElement& addRect(ShapePaintArgs const& paintArgs, Vector2 const& min, Vector2 const& max);
		RenderBatchedElement& addRoundRect(ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& rectSize, Vector2 const& circleRadius);

		template< class TVector2 >
		RenderBatchedElement& addPolygon(RenderTransform2D const& transform, ShapePaintArgs const& paintArgs, TVector2 const v[], int numV)
		{
			CHECK(numV > 2);
			TRenderBatchedElement<PolygonPayload>* element = addElement< PolygonPayload >();
			element->type = RenderBatchedElement::Polygon;
			element->payload.paintArgs = paintArgs;
#if BATCHED_ELEMENT_NO_DESTRUCT
			element->payload.posList = (Vector2*)mAllocator.alloc(sizeof(Vector2) * numV);
			element->payload.posCount = numV;
#else
			element->payload.posList.resize(numV);
#endif
			for (int i = 0; i < numV; ++i)
			{
				element->payload.posList[i] = transform.transformPosition(v[i]);
			}
			return *element;
		}

		template< class TVector2 >
		RenderBatchedElement& addLineStrip(RenderTransform2D const& transform, Color4Type const& color, TVector2 const v[], int numV, int width)
		{
			CHECK(numV >= 2);
			TRenderBatchedElement<LineStripPayload>* element = addElement< LineStripPayload >();
			element->type = RenderBatchedElement::LineStrip;
			element->payload.color = color;
			element->payload.width = width;
#if BATCHED_ELEMENT_NO_DESTRUCT
			element->payload.posList = (Vector2*)mAllocator.alloc(sizeof(Vector2) * numV);
			element->payload.posCount = numV;
#else
			element->payload.posList.resize(numV);
#endif
			for (int i = 0; i < numV; ++i)
			{
				element->payload.posList[i] = transform.transformPosition(v[i]);
			}
			return *element;
		}


		template< class TVector2 >
		RenderBatchedElement& addArcLine(RenderTransform2D const& transform, Color4Type const& color, TVector2 const& center, float radius, float start, float end)
		{
			CHECK(numV >= 2);
			TRenderBatchedElement<LineStripPayload>* element = addElement< ArcLinePlayload >();
			element->type = RenderBatchedElement::ArcLine;
			element->payload.color = color;
			element->payload.center = center;
			element->payload.radius = radius;
			element->payload.start = start;
			element->payload.end = end;
			return *element;
		}


		RenderBatchedElement& addCircle( ShapePaintArgs const& paintArgs, Vector2 const& pos, float radius);
		RenderBatchedElement& addEllipse( ShapePaintArgs const& paintArgs, Vector2 const& pos, Vector2 const& size);
		RenderBatchedElement& addTextureRect(Color4Type const& color, Vector2 const& min, Vector2 const& max, Vector2 const& uvMin, Vector2 const& uvMax);
		RenderBatchedElement& addLine(Color4Type const& color, Vector2 const& p1, Vector2 const& p2, int width);
		RenderBatchedElement& addText(Color4Type const& color, TArray< FontVertex >&& vertices);
		RenderBatchedElement& addGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad);

		template< class TPayload >
		TRenderBatchedElement<TPayload>* addElement()
		{
			TRenderBatchedElement<TPayload>* ptr = (TRenderBatchedElement<TPayload>*)mAllocator.alloc(sizeof(TRenderBatchedElement<TPayload>));
			FTypeMemoryOp::Construct(ptr);
			mElements.push_back(ptr);

			static_assert(std::is_trivially_destructible_v<RenderBatchedElement>);
#if BATCHED_ELEMENT_NO_DESTRUCT
			static_assert(std::is_trivially_destructible_v< TPayload >);
#else
			if constexpr (std::is_trivially_destructible_v< TPayload > == false)
			{
				mDestructElements.push_back(ptr);
			}
#endif
			return ptr;
		}

		void releaseElements();
		bool isEmpty() const { return mElements.empty(); }

		FrameAllocator mAllocator;
		TArray< RenderBatchedElement* > mElements;
#if BATCHED_ELEMENT_NO_DESTRUCT
#else
		TArray< RenderBatchedElement* > mDestructElements;
#endif
	};


	struct FPrimitiveHelper
	{
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

		FORCEINLINE static uint32* FillQuad(uint32* pIndices, int base, int i0, int i1, int i2, int i3)
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
	};

	class BatchedRender : public FPrimitiveHelper
	{
	public:

		using Color4Type = ShapePaintArgs::Color4Type;
		using Color3Type = ShapePaintArgs::Color3Type;


		BatchedRender();

		void initializeRHI();
		void releaseRHI();
		void render(RHICommandList& commandList, RenderBachedElementList& elementList);


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

		BaseVertex* fetchBaseVertex(int size, int& baseIndex);
		TexVertex* fetchTexVertex(int size, int& baseIndex);
		uint32* fetchIndex(int size);

		void emitPolygon(Vector2 v[], int numV, Color4Type const& color);
		void emitPolygonLine(Vector2 v[], int numV, Color4Type const& color, int lineWidth);

		TArray< Vector2 >    mCachedPositionList;
		TArray< uint32 >     mBaseIndices;
		TArray< BaseVertex > mBaseVertices;
		TArray< TexVertex >  mTexVerteices;


		void flushDrawCommandForBuffer(bool bEndRender);
		void flushDrawCommand();

		template< class T>
		struct TBufferData
		{
			T*  dataPtr = nullptr;
			int usedCount = 0;

			bool initialize(int num, uint32 flags)
			{
				VERIFY_RETURN_FALSE(buffer = RHICreateBuffer(sizeof(T), num, flags));
				return true;
			}

			void release()
			{
				CHECK(dataPtr == nullptr);
				buffer.release();
			}

			bool canFetch(int num)
			{
				CHECK(dataPtr);
				return usedCount + num <= buffer->getNumElements();
			}
			T* fetch(int num)
			{
				CHECK(canFetch(num));
				T* result = dataPtr + usedCount;
				usedCount += num;
				return result;
			}
			void fill(T value[], int num)
			{
				CHECK(usedCount + num <= totalCount);
				FMemory::Copy(dataPtr + usedCount, value, sizeof(T) * num);
				usedCount += num;
			}

			bool lock()
			{
				CHECK(dataPtr == nullptr);
				dataPtr = (T*)RHILockBuffer(buffer, ELockAccess::WriteDiscard);
				if (dataPtr == nullptr)
					return false;

				usedCount = 0;
				return true;
			}

			void unlock()
			{
				CHECK(dataPtr);
				RHIUnlockBuffer(buffer);
				dataPtr = nullptr;
			}

			RHIBufferRef buffer;
			operator RHIBuffer*() { return buffer; }
		};


		TBufferData< BaseVertex > mBaseVertexBuffer;
		TBufferData< TexVertex >  mTexVertexBuffer;
		TBufferData< uint32 >     mIndexBuffer;

		bool bUseBuffer = false;
		RHICommandList* mCommandList = nullptr;
	};




}//namespace Render

#endif // BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B
