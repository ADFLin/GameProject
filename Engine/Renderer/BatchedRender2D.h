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

#define USE_POLYGON_LINE_NEW 1

namespace Render
{

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
		void buildPolygonLine(Vector2 v[], int numV, float lineWidth);


		RenderTransform2D  mTransform;
		TArray< Vector2 >& mBuffer;
	};

	template< class TVector2 >
	FORCEINLINE void Transform(TVector2 const* inList, Vector2* outList, int num, RenderTransform2D const& xForm)
	{
		for (int i = 0; i < num; ++i)
		{
			outList[i] = xForm.transformPosition(inList[i]);
		}
	}

	struct ShapeCachedData
	{
		int id;
		int frame;
		TArray< Vector2 > posList;

		TArrayView< Vector2 > build(FrameAllocator& allocator, RenderTransform2D const& xForm)
		{
			Vector2* result = (Vector2*)allocator.alloc(posList.size() * sizeof(Vector2));
			Transform(posList.data(), result, posList.size(), xForm);
			return TArrayView< Vector2 >(result, posList.size());
		}
	};

	class ShapeVertexCache
	{
	public:
		struct CacheKey
		{
			enum EType
			{
				eCircle,
				eEllipse,
				eRoundRect,
				eShapLine,
			};

			EType   type;
			union 
			{
				Vector4 v4;
				struct  
				{
					uint64  ptr;
					Vector2 v2;
				};
			};
			

			bool operator == (CacheKey const& rhs) const
			{
				return type == rhs.type && v4 == rhs.v4;
			}

			uint32 getTypeHash() const
			{
				uint32 result = HashValue(type);
				result = HashCombine(result, v4.x);
				result = HashCombine(result, v4.y);
				result = HashCombine(result, v4.z);
				result = HashCombine(result, v4.w);
				return result;
			}

		};


		ShapeCachedData* getCircle(int numSegment);

		ShapeCachedData* getEllipse(float yFactor, int numSegment);

		ShapeCachedData* getRoundRect(Vector2 const& rectSize, Vector2 const& circleRadius);


		ShapeCachedData* getShapeLine(ShapeCachedData& cacheData, float width);

		template< typename TFunc >
		ShapeCachedData* findOrCache(CacheKey const& key, TFunc& func);

		template< typename TFunc >
		TArrayView< Vector2 > getOrBuild(FrameAllocator& allocator, RenderTransform2D const& xForm, CacheKey const& key, TFunc& func);

		std::unordered_map< CacheKey, ShapeCachedData, MemberFuncHasher > mCachedMap;

		int mFrame  = 0;
		int mNextId = 0;
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
			GradientRect,
			CustomState,
			CustomRender,
		};

		EType type;
		RenderTransform2D transform;
		int32 layer;
	};

	enum class EObjectManageMode
	{
		Detete,
		DestructOnly,
		None,
	};

	class ICustomElementRenderer
	{
	public:
		virtual ~ICustomElementRenderer() = default;
		virtual void render(RHICommandList& commandList, RenderBatchedElement& element) = 0;
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

	class RenderBatchedElementList
	{
	public:
		RenderBatchedElementList(FrameAllocator& allocator)
			:mAllocator(allocator)
		{

		}

		using Color4Type = ShapePaintArgs::Color4Type;
		using Color3Type = ShapePaintArgs::Color3Type;

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
			Vector2* posList;
			int posCount;
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
			Vector2* posList;
			int posCount;

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
			FontVertex* vertices;
			int verticesCount;
		};

		struct GradientRectPayload
		{
			Vector2 posLT;
			Color4Type colorLT;
			Vector2 posRB;
			Color4Type colorRB;
			bool bHGrad;
		};

		struct CustomRenderPayload
		{
			ICustomElementRenderer* renderer;
			EObjectManageMode manageMode;
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
			element->payload.posList = (Vector2*)mAllocator.alloc(sizeof(Vector2) * numV);
			element->payload.posCount = numV;

			Transform(v, &element->payload.posList[0], numV, transform);
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
			element->payload.posList = (Vector2*)mAllocator.alloc(sizeof(Vector2) * numV);
			element->payload.posCount = numV;

			Transform(v, &element->payload.posList[0], numV, transform);
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
		RenderBatchedElement& addText(Color4Type const& color, TArray< FontVertex > const& vertices);
		template< typename CharT >
		RenderBatchedElement& addText(Color4Type const& color, Vector2 const& pos, FontDrawer& front, CharT const* str, int charCount = 0)
		{
			int verticesCount = 4 * ((charCount) ? charCount : front.getCharCount(str));
			TRenderBatchedElement<TextPayload>* element = addElement< TextPayload >();
			element->type = RenderBatchedElement::Text;
			element->payload.color = color;
			element->payload.vertices = (FontVertex*)mAllocator.alloc(verticesCount * sizeof(FontVertex));
			element->payload.verticesCount = verticesCount;
			front.generateVertices(pos, str, element->payload.vertices);

			return *element;
		}

		RenderBatchedElement& addGradientRect(Vector2 const& posLT, Color3Type const& colorLT, Vector2 const& posRB, Color3Type const& colorRB, bool bHGrad);

		RenderBatchedElement& addCustomState(ICustomElementRenderer* renderer, EObjectManageMode mode);
		RenderBatchedElement& addCustomRender(ICustomElementRenderer* renderer, EObjectManageMode mode);

		template< class TPayload >
		TRenderBatchedElement<TPayload>* addElement()
		{
			TRenderBatchedElement<TPayload>* ptr = (TRenderBatchedElement<TPayload>*)mAllocator.alloc(sizeof(TRenderBatchedElement<TPayload>));
			FTypeMemoryOp::Construct(ptr);
			mElements.push_back(ptr);

			static_assert(std::is_trivially_destructible_v<RenderBatchedElement>);
			static_assert(std::is_trivially_destructible_v< TPayload >);

			return ptr;
		}

		void releaseElements();
		bool isEmpty() const { return mElements.empty(); }

		FrameAllocator& mAllocator;
		TArray< RenderBatchedElement* > mElements;
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


		FORCEINLINE static uint32* FillPolygon(uint32* pIndices, int baseIndex, int numTriangle)
		{
			for (int i = 0; i < numTriangle; ++i)
			{
				pIndices = FillTriangle(pIndices, baseIndex, 0, i + 1, i + 2);
			}
			return pIndices;
		}

		FORCEINLINE static uint32* FillPolygonLine(uint32* pIndices, int baseIndex, int numV)
		{
#if USE_POLYGON_LINE_NEW
			int baseIndexStart = baseIndex;
			for (int i = 0; i < numV - 1; ++i)
			{
				pIndices = FillQuad(pIndices, baseIndex, 0, 1, 3, 2);
				baseIndex += 2;
			}
			pIndices = FillQuad(pIndices, baseIndex, baseIndex + 1, baseIndexStart + 1, baseIndexStart + 0);
#else
			int indexCur = baseIndex;
			for (int i = 0; i < numV - 1; ++i)
			{
				pIndices = FillLineShapeIndices(pIndices, indexCur, indexCur + 4);
				indexCur += 4;
			}
			pIndices = FillLineShapeIndices(pIndices, indexCur, baseIndex);
#endif
			return pIndices;
		}
	};

	enum class ESimpleBlendMode
	{
		None,
		Translucent,
		Add,
		Multiply,
	};

	class GraphicsDefinition
	{
	public:

		typedef TVector2<int> Vec2i;
		struct Rect
		{
			Vec2i pos;
			Vec2i size;

			bool isValid() const
			{
				return size.x > 0 && size.y > 0;
			}

			static Rect Intersect(Rect const& r1, Rect const& r2)
			{
				Vec2i min = r1.pos.max(r2.pos);
				Vec2i max = (r1.pos + r1.size).min(r2.pos + r2.size);
				return { min , max - min };
			}

			bool operator != (Rect const& rhs) const
			{
				return pos != rhs.pos || size != rhs.size;
			}
		};

		struct RenderState
		{
			RHITexture2D*    texture;
			RHISamplerState* sampler;
			ESimpleBlendMode blendMode;
			bool bEnableMultiSample;
			bool bEnableScissor;

			Rect  scissorRect;

			void setInit()
			{
				texture = nullptr;
				sampler = nullptr;
				blendMode = ESimpleBlendMode::None;
				bEnableMultiSample = false;
				bEnableScissor = false;
			}
		};

		struct StateFlags
		{
			union
			{
				struct
				{
					uint8 pipeline : 1;
					uint8 scissorRect : 1;
					uint8 blend : 1;
					uint8 rasterizer : 1;
				};
				uint8 value;
			};
		};



		static RHIBlendState& GetBlendState(ESimpleBlendMode blendMode);

		static RHIRasterizerState& GetRasterizerState(bool bEnableScissor, bool bEnableMultiSample);

		using GraphicsDepthState = StaticDepthDisableState;
	};

	class BatchedRender : public FPrimitiveHelper , public GraphicsDefinition
	{
	public:

		using Color4Type = ShapePaintArgs::Color4Type;
		using Color3Type = ShapePaintArgs::Color3Type;


		BatchedRender();

		CORE_API ShapeVertexCache& GetShapeCache();

		void initializeRHI();
		void releaseRHI();
		void beginRender(RHICommandList& commandList);
		void render(RenderState const& renderState, RenderBatchedElementList& elementList);

		void setViewportSize(int width, int height)
		{
			mWidth = width;
			mHeight = height;
			mBaseTransform = AdjProjectionMatrixForRHI(OrthoMatrix(0, mWidth, mHeight, 0, -1, 1));
		}

		void commitRenderState(RHICommandList& commandList, RenderState const& state);

		void setupInputState(RHICommandList& commandList);

		void updateRenderState(RHICommandList& commandList, RenderState const& state);

		void flush();
		void endRender()
		{

		}

		void emitPolygon(Vector2 v[], int numV, ShapePaintArgs const& paintArgs);
		void emitPolygon(ShapeCachedData& cachedData, RenderTransform2D const& xForm, ShapePaintArgs const& paintArgs);
		void emitRect(Vector2 v[], ShapePaintArgs const& paintArgs);
		void emitElements(TArray<RenderBatchedElement* > const& elements, RenderState const& renderState);

		struct RenderGroup
		{
			RenderState state;
			TArray<RenderBatchedElement* > elements;

			int indexStart;
			int indexCount;
		};

		TArray< RenderGroup > mGroups;
		int mIndexRender = 0;
		int mIndexEmit = 0;	
		void flushGroup();


		struct TexVertex
		{
			Vector2  pos;
			Color4Type  color;
			Vector2 uv;
		};

		TexVertex* fetchVertex(int size, int& baseIndex);
		uint32*    fetchIndex(int size);

		struct FetchedData
		{
			int        base;
			TexVertex* vertices;
			uint32*    indices;
		};
		FetchedData fetchBuffer(int vSize, int iSize);

		void emitPolygon(Vector2 v[], int numV, Color4Type const& color);
		void emitPolygonLine(Vector2 v[], int numV, Color4Type const& color, int lineWidth);

		void flushDrawCommand(bool bEndRender);

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

			void tryUnlock()
			{
				if (dataPtr)
				{
					RHIUnlockBuffer(buffer);
				}
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

		int       mWidth;
		int       mHeight;
		Math::Matrix4   mBaseTransform;
		TArray< Vector2 >         mCachedPositionList;
		TBufferData< TexVertex >  mTexVertexBuffer;
		TBufferData< uint32 >     mIndexBuffer;
		RHICommandList* mCommandList = nullptr;
		SimplePipelineProgram* mProgramCur = nullptr;
	};




}//namespace Render

#endif // BatchedRender2D_H_0BD59426_7313_44DD_9E82_8954B70E3C1B
