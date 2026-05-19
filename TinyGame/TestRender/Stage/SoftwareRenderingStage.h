#pragma once
#ifndef SoftwareRenderingStage_H_6E10EA7D_C3B2_4E62_B6A5_67718B8C304F
#define SoftwareRenderingStage_H_6E10EA7D_C3B2_4E62_B6A5_67718B8C304F

#include "Stage/TestStageHeader.h"

#include "WindowsHeader.h"
#include "Widget/WidgetUtility.h"
#include "Math/Base.h"
#include "Math/SIMD.h"

#include "Math/Transform.h"
#include "RHI/RHIType.h"
#include "DataStructure/Array.h"

#include "RefCount.h"

#include "Renderer/SimpleCamera.h"
#include "Math/PrimitiveTest.h"
#include "DrawEngine.h"

#include <type_traits>
#include <atomic>
#include "Core/Color.h"

namespace SR
{
	using namespace Render;

	using XForm = Math::Transform;



	using LaneScalar = SIMD::TFloatVector<8>;
	using LaneVector2 = TVector2<LaneScalar>;
	using LaneVector3 = TVector3<LaneScalar>;
	using LaneVector4 = TVector4<LaneScalar>;
	using LaneMask = SIMD::TIntVector<LaneScalar::Size>;


	Vector3 Reflection(Vector3 InV, Vector3 n)
	{
		return InV - (2 * n.dot(InV)) * n;
	}



	struct LinearColor
	{
		float r, g, b, a;

		FORCEINLINE ColorBGRA8 toBGRA() const
		{
#if USE_MATH_SIMD
			return PackBGRA(SIMD::TFloatVector<4>(r, g, b, a));
#else
			return ColorBGRA8(ToByte(r), ToByte(g), ToByte(b), ToByte(a));
#endif
		}
		FORCEINLINE ColorBGRA8 toOpaqueBGRA() const
		{
#if USE_MATH_SIMD
			return PackBGRA(SIMD::TFloatVector<4>(r, g, b, 1.0f));
#else
			return ColorBGRA8(ToByte(r), ToByte(g), ToByte(b), 0xff);
#endif
		}

#if USE_MATH_SIMD
		FORCEINLINE static ColorBGRA8 PackBGRA(SIMD::TFloatVector<4> const& color)
		{
			SIMD::TFloatVector<4> clamped = min(max(color, SIMD::TFloatVector<4>::Zero()), SIMD::TFloatVector<4>(1.0f));
			__m128i rgba32 = _mm_cvttps_epi32((clamped * 255.0f).reg);
			__m128i rgba16 = _mm_packs_epi32(rgba32, _mm_setzero_si128());
			__m128i rgba8 = _mm_packus_epi16(rgba16, _mm_setzero_si128());
			uint32 rgba = uint32(_mm_cvtsi128_si32(rgba8));

			ColorBGRA8 result;
			result.word = (rgba & 0xff00ff00) | ((rgba & 0x000000ff) << 16) | ((rgba & 0x00ff0000) >> 16);
			return result;
		}
#else
		FORCEINLINE static uint8 ToByte(float value)
		{
			return uint8(255.0f * Math::Clamp<float>(value, 0.0f, 1.0f));
		}
#endif

		LinearColor operator + (LinearColor const& rhs) const
		{
			return LinearColor(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
		}
		LinearColor operator * (LinearColor const& rhs) const
		{
			return LinearColor(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a);
		}

		LinearColor& operator += (LinearColor const& rhs)
		{
			r += rhs.r;
			g += rhs.g;
			b += rhs.b;
			a += rhs.a;
			return *this;
		}

		LinearColor& operator *= (float s)
		{
			r *= s;
			g *= s;
			b *= s;
			a *= s;
			return *this;
		}
		LinearColor() {}
		LinearColor(float r, float g, float b, float a = 1.0f)
			:r(r), g(g), b(b), a(a)
		{
		}
		LinearColor(ColorBGRA8 const& rhs)
			:r(float(rhs.r) / 255.0f), g(float(rhs.g) / 255.0f), b(float(rhs.b) / 255.0f), a(float(rhs.a) / 255.0f)
		{
		}
		LinearColor(Color4ub const& rhs)
			:r(float(rhs.r) / 255.0f), g(float(rhs.g) / 255.0f), b(float(rhs.b) / 255.0f), a(float(rhs.a) / 255.0f)
		{
		}

		LinearColor(Vector3 const& v)
			:r(v.x), g(v.y), b(v.z), a(1.0)
		{
		}

		
	};

	struct LaneLinearColor
	{
		LaneScalar r, g, b, a;
	};

	FORCEINLINE LaneLinearColor operator * (LaneLinearColor const& lhs, LaneLinearColor const& rhs)
	{
		return LaneLinearColor{ lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a };
	}

	FORCEINLINE LaneLinearColor operator * (LaneScalar const& s, LaneLinearColor const& rhs)
	{
		return LaneLinearColor{ s * rhs.r, s * rhs.g, s * rhs.b, s * rhs.a };
	}

	FORCEINLINE LaneLinearColor operator + (LaneLinearColor const& lhs, LaneLinearColor const& rhs)
	{
		return LaneLinearColor{ lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a };
	}

	LinearColor operator * (float s, LinearColor const& rhs) { return LinearColor(s * rhs.r, s * rhs.g, s * rhs.b, s * rhs.a); }
	LinearColor operator - (LinearColor const& lhs, LinearColor& rhs) { return LinearColor(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a); }


	class ColorBuffer
	{
	public:
		bool create(Vec2i const& size);

		void clear(ColorBGRA8 const& color)
		{
			std::fill_n(mData, mSize.y * mRowStride, uint32(color));
		}

		ColorBGRA8 getPixel(uint32 x, uint32 y)
		{
			ColorBGRA8 result;
			result.word = mData[(x + mRowStride * y)];
			return result;
		}

		void setPixel(uint32 x, uint32 y, ColorBGRA8 color)
		{
			uint32* pData = mData + (x + mRowStride * y);
			*pData = color;
		}
		void setPixelCheck(uint32 x, uint32 y, ColorBGRA8 color)
		{
			if( 0 <= x && x < mSize.x &&
			   0 <= y && y < mSize.y )
				setPixel(x, y, color);
		}

		void draw(Graphics2D& g)
		{
			mBufferDC.bitBltTo(g.getRenderDC(), 0, 0);
		}

		Vec2i const& getSize() const { return mSize; }

		Vec2i    mSize;
		uint32   mRowStride;
		uint32*  mData;
		BitmapDC mBufferDC;
	};

	class DepthBuffer
	{
	public:
		bool create(Vec2i const& size)
		{
			mSize = size;
			mData.resize(size_t(size.x) * size.y);
			return mData.size() != 0;
		}

		void clear(float depth)
		{
			std::fill_n(mData.data(), mData.size(), depth);
		}

		FORCEINLINE float getDepth(uint32 x, uint32 y) const
		{
			return mData[x + mSize.x * y];
		}

		FORCEINLINE void setDepth(uint32 x, uint32 y, float depth)
		{
			mData[x + mSize.x * y] = depth;
		}

		bool testAndSet(uint32 x, uint32 y, float depth)
		{
			float& value = mData[x + mSize.x * y];
			if (depth >= value)
				return false;

			value = depth;
			return true;
		}

		Vec2i const& getSize() const { return mSize; }

		Vec2i mSize;
		TArray<float> mData;
	};

	struct PixelShaderType
	{


	};

	class Texture
	{
	public:
		bool load(char const* path);

		Color4ub getColor(Vec2i const& pos) const;

		LinearColor sample(Vector2 const& UV) const;

		LaneLinearColor sample(LaneVector2 const& UV) const;

		Vec2i mSize;
		TArray< Color4ub > mData;

	};


	void GenerateDistanceFieldTexture(Texture const& texture)
	{
		Vec2i size = texture.mSize;
		TArray< Vec2i > countMap(size.x * size.y, Vec2i(0, 0));

		{
			{
				uint32 c = texture.getColor(Vec2i(0, 0)).toRGBA();
				Vec2i& count = countMap[0 + size.x * 0];
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
			for( int i = 1; i < size.x; ++i )
			{
				uint32 c = texture.getColor(Vec2i(i, 0)).toRGBA();
				Vec2i& count = countMap[i + size.x * 0];
				Vec2i& countLeft = countMap[i - 1 + size.x * 0];

				count = countLeft;
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
		}

		for( int j = 1; j < size.y; ++j )
		{
			{
				uint32 c = texture.getColor(Vec2i(0, j)).toRGBA();
				Vec2i& countDown = countMap[0 + size.x * (j - 1)];
				Vec2i& count = countMap[0 + size.x * j];

				count = countDown;
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
			for( int i = 1; i < size.x; ++i )
			{
				uint32 c = texture.getColor(Vec2i(i, j)).toRGBA();
				Vec2i& count = countMap[i + size.x * 0];
				Vec2i& countLeft = countMap[i - 1 + size.x * j];
				Vec2i& countDown = countMap[i + size.x * (j - 1)];
				count = countLeft + countDown;
				if( c & 0x00ffffff )
				{
					count.x += 1;
				}
				else
				{
					count.y += 1;
				}
			}
		}

		for( int j = 0; j < size.y; ++j )
		{
			for( int i = 0; i < size.x; ++i )
			{
				uint32 c = texture.getColor(Vec2i(i, j)).toRGBA();


			}
		}
	}

	struct RenderTarget
	{
		ColorBuffer* colorBuffer = nullptr;
		DepthBuffer* depthBuffer = nullptr;

		Vec2i const& getSize() const { return colorBuffer->getSize(); }
	};

	enum class ECullMode
	{
		None,
		Front,
		Back,
	};

	template< ECullMode CullMode = ECullMode::Back >
	struct TRasterizerState
	{
		FORCEINLINE static bool ShouldCull(float signedArea)
		{
			if constexpr (CullMode == ECullMode::None)
				return false;
			if constexpr (CullMode == ECullMode::Front)
				return signedArea <= 0.0f;
			else
				return signedArea >= 0.0f;
		}
	};


	enum class EDepthFunc
	{
		Always,
		Less,
	};

	template< EDepthFunc DepthFunc = EDepthFunc::Less, bool bWriteDepth = true >
	struct TDepthState
	{
		static constexpr bool EnableTest = (DepthFunc != EDepthFunc::Always);
		static constexpr bool EnableWrite = bWriteDepth;
		static constexpr float ClearValue = Math::MaxFloat;

		FORCEINLINE static bool Pass(float srcDepth, float destDepth)
		{
			if constexpr (DepthFunc == EDepthFunc::Always)
				return true;
			
			return srcDepth < destDepth;
		}

		FORCEINLINE static LaneMask Pass(LaneScalar srcDepth, LaneScalar destDepth)
		{
			if constexpr (DepthFunc == EDepthFunc::Always)
				return LaneMask(-1);

			return LaneMask(_mm256_castps_si256((srcDepth < destDepth).reg));
		}
	};

	using DepthDisableState = TDepthState< EDepthFunc::Always, false >;
	using DepthLessWriteState = TDepthState< EDepthFunc::Less, true >;

	enum class EBlendMode
	{
		Opaque,
		Translucent,
		Add,
	};

	template< EBlendMode BlendMode = EBlendMode::Opaque >
	struct TBlendState
	{
		static constexpr bool Enable = (BlendMode != EBlendMode::Opaque);

		template<typename ColorType >
		FORCEINLINE static ColorType Blend(ColorType const& src, ColorType const& dest)
		{
			if constexpr (BlendMode == EBlendMode::Opaque)
			{
				return src;
			}
			else if constexpr (BlendMode == EBlendMode::Translucent)
			{
				return Math::LinearLerp(dest, src, src.a);
			}
			else
			{
				return dest + src;
			}
		}
	};

	class RendererBase
	{
	public:
		void setRenderTarget(RenderTarget& renderTarget)
		{
			mRenderTarget = renderTarget;
		}

		void clearBuffer(LinearColor const& color)
		{
			if( mRenderTarget.colorBuffer )
				mRenderTarget.colorBuffer->clear(color.toBGRA());
			if (mRenderTarget.depthBuffer)
				mRenderTarget.depthBuffer->clear(Math::MaxFloat);
		}
	protected:
		RenderTarget mRenderTarget;
	};

	template< class TRasterState, class TDepthState, class TBlendState >
	struct TRenderStatePipeline
	{
		using RasterState = TRasterState;
		using DepthState = TDepthState;
		using BlendState = TBlendState;
	};

	using DefaultRasterPipeline = TRenderStatePipeline< TRasterizerState<>, TDepthState<>, TBlendState<> >;

	class RasterizedRenderer : public RendererBase
	{
	public:
		bool init()
		{
			return true;
		}

		Vector2 viewportOrg;
		Vector2 viewportSize;
		Matrix4 worldToClip;

		Vector2 toScreenPos(Vector4 clipPos)
		{
			return viewportOrg + viewportSize.mul(0.5 *Vector2(clipPos.x / clipPos.w, clipPos.y / clipPos.w) + Vector2(0.5, 0.5));
		}

		template< class TPipeline >
		void clearBuffer(LinearColor const& color)
		{
			if (this->mRenderTarget.colorBuffer)
				this->mRenderTarget.colorBuffer->clear(color.toBGRA());
			using TDepthState = typename TPipeline::DepthState;
			if constexpr (TDepthState::EnableTest || TDepthState::EnableWrite)
			{
				if (this->mRenderTarget.depthBuffer)
					this->mRenderTarget.depthBuffer->clear(TDepthState::ClearValue);
			}
		}

		template< class TPipeline >
		void drawTriangle(Vector3 v0, LinearColor color0, Vector2 uv0,
						  Vector3 v1, LinearColor color1, Vector2 uv1,
						  Vector3 v2, LinearColor color2, Vector2 uv2);

		template< class TPipeline, class TVertex, class TVertexShader >
		void drawIndexedTriangleList(TVertex const* vertices, int numVertices, uint32 const* indices, int numIndices, TVertexShader const& vertexShader);

		template< class TPipeline, class TPixelShader >
		void drawTrianglePS(Vector3 v0, LinearColor color0, Vector2 uv0,
						    Vector3 v1, LinearColor color1, Vector2 uv1,
						    Vector3 v2, LinearColor color2, Vector2 uv2,
						    TPixelShader const& pixelShader);

		template< class TPipeline, class TVSOutput, class TPixelShader >
		void drawTrianglePS(Vector4 const& sv0, TVSOutput const& output0,
						    Vector4 const& sv1, TVSOutput const& output1,
						    Vector4 const& sv2, TVSOutput const& output2,
						    TPixelShader const& pixelShader);

		template< class TPipeline, class TVSOutput, class TPixelShader >
		void drawLinePS(Vector4 const& sv0, TVSOutput const& output0,
						Vector4 const& sv1, TVSOutput const& output1,
						TPixelShader const& pixelShader);

		template< class TPipeline, class TVertex, class TVertexShader, class TPixelShader >
		void drawIndexedTriangleListPS(TVertex const* vertices, int numVertices, uint32 const* indices, int numIndices, TVertexShader const& vertexShader, TPixelShader const& pixelShader);

	};

	class Material : public RefCountedObjectT< Material >
	{
	public:
		Material()
			:emissiveColor(0, 0, 0, 1)
		{

		}
		LinearColor emissiveColor;

	};

	struct RayTrace
	{
		Vector3 pos;
		Vector3 dir;
	};

	struct RayHitResult
	{
		float     distance;
		Vector3   normal;
		Material* material;

		RayHitResult()
		{
			distance = 0;
			material = nullptr;
		}
	};

	class PrimitiveShape : public RefCountedObjectT< PrimitiveShape >
	{
	public:
		virtual bool raycast( RayTrace const& trace , RayHitResult& result ) = 0;
	};

	class SphereShape : public PrimitiveShape
	{

	public:
		bool raycast(RayTrace const& trace, RayHitResult& result) override
		{
			float distances[2];
			if( !LineSphereTest(trace.pos, trace.dir, Vector3::Zero(), radius, distances) )
				return false;

			if( distances[0] > 0 )
				result.distance = distances[0];
			else if( distances[1] > 0 )
				result.distance = distances[1];
			else
			{
				return false;
			}

			result.normal = ( trace.pos + result.distance * trace.dir ).getNormal();
			return true;
		}

		float radius;
	};

	class PlaneShape : public PrimitiveShape
	{
	public:
		PlaneShape()
			:normal(Vector3(0, 0, 1))
			,d(0)
		{

		}
		bool raycast(RayTrace const& trace, RayHitResult& result) override
		{
			float DoN = normal.dot(trace.dir);
			if( Math::Abs( DoN ) < 1e-5 )
				return false;
			float distance = ( d - normal.dot( trace.pos ) ) / DoN;
			if( distance < 0 )
				return false;

			result.distance = distance;
			result.normal = normal;
			return true;
		}

		Vector3 normal;
		float   d;
	};


	class ObjectBase : public RefCountedObjectT< ObjectBase >
	{
	public:
		ObjectBase()
			:transform( ForceInit )
		{

		}
		XForm transform;
		
	};

	using MaterialPtr = TRefCountPtr< Material >;
	using PrimitiveShapePtr = TRefCountPtr< PrimitiveShape >;

	using ObjectPtr = TRefCountPtr< ObjectBase >;

	class KDTree
	{
		struct Node
		{


			Node* child[2];
		};




	};
	
	class PrimitiveCacheData
	{
	public:
		template< class T >
		void initType()
		{

		}

		template< class T >
		T& get(){}

		union 
		{
			uint8 data[32];

		};
	};

	class PrimitiveObject : public ObjectBase
	{
	public:
		bool raycast(RayTrace const& trace, RayHitResult& result)
		{
			RayTrace localTrace;
			localTrace.pos = transform.transformPositionInverse(trace.pos);
			localTrace.dir = transform.transformVectorInverseNoScale(trace.dir);

			if( !shape->raycast(localTrace , result) )
				return false;

			result.normal = transform.transformVectorNoScale(result.normal);
			result.material = material;
			if( !transform.scale.isUniform() )
			{
				Vector3 hitPosLocal = localTrace.pos + localTrace.dir * result.distance;
				Vector3 hitPos = transform.transformPosition(hitPosLocal);
				result.distance = Math::Distance(hitPos, trace.pos);
			}
			return true;
		}

		MaterialPtr       material;
		PrimitiveShapePtr shape;
	};

	using PrimitiveObjectPtr = TRefCountPtr< PrimitiveObject >;

	class Camera : public ObjectBase
	{
	public:
		void lookAt(Vector3 pos, Vector3 lookPos, Vector3 upDir)
		{
			transform.location = pos;
			Vector3 zAxis = Math::GetNormal(lookPos - pos);
			Vector3 xAxis = Math::GetNormal(upDir.cross(zAxis));
			Vector3 yAxis = zAxis.cross(xAxis);
			transform.rotation = Matrix3(xAxis, yAxis, zAxis).toQuatNoScale();
		}
		float fov;
		float aspect;
	};

	class Scene
	{
	public:

		Scene()
		{
		}

		void addPrimitive(PrimitiveObjectPtr const& obj)
		{
			mPrimitiveObjects.push_back(obj);
		}

		bool raycast(RayTrace const& trace, RayHitResult& outResult)
		{
			PrimitiveObject* hitObject = nullptr;
			outResult.distance = Math::MaxFloat;
			for( PrimitiveObject* object : mPrimitiveObjects )
			{
				RayHitResult hitResult;
				if( object->raycast(trace, hitResult) )
				{
					if ( hitResult.distance < 1e-4 )
						continue;

					if( hitResult.distance < outResult.distance )
					{
						hitObject = object;
						outResult = hitResult;
					}
				}
			}
		
			return hitObject != nullptr;
		}

		struct ScenePrimitiveInfo
		{
			PrimitiveObjectPtr object;
		};

		std::vector< PrimitiveObjectPtr > mPrimitiveObjects;
	};

	class RayTraceRenderer : public RendererBase
	{

	public:
		bool init();


		void render( Scene& scene , Camera& camera );


	};



	class TestStage : public StageBase
	{
		using BaseClass = StageBase;
	public:

		bool bRayTracerUsed = false;
		bool bRender = true;

		RayTraceRenderer   mRTRenderer;
		RasterizedRenderer mRenderer;

		ColorBuffer mColorBuffer;
		DepthBuffer mDepthBuffer;

		Scene  mScene;
		Camera mCamera;

		Texture simpleTexture;
		float mFPS = 0.0f;

		SimpleCamera mCameraControl;

		std::atomic<int> mPixelCountRendered = 0;


		void setupScene();

		bool onInit() override;


		void onEnd() override
		{

			BaseClass::onEnd();
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			if( !bPause )
			{
				mElapsedTime += deltaTime;
				mCameraControl.updatePosition(deltaTime);
			}

		}


		void onRender(float dFrame) override;

		void renderTest1(RenderTarget& renderTarget);
		void renderTest2(RenderTarget& renderTarget);

		void restart()
		{

		}

		bool bPause = false;
		float mElapsedTime = 0;
		float mCubeRotateYaw = 0;
		float mCubeRotatePitch = 0;
		Vec2i mLastMousePos = Vec2i(0, 0);
		GButton* mPauseButton = nullptr;
		GText* mTimeText = nullptr;

		void togglePause();

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				mLastMousePos = msg.getPos();
			}
			if (msg.onMoving() && msg.isLeftDown())
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - mLastMousePos);
				mCubeRotateYaw += off.x;
				mCubeRotatePitch += off.y;
				mLastMousePos = msg.getPos();
			}

			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override;


	};

}//namespace SWR



#endif // SoftwareRenderingStage_H_6E10EA7D_C3B2_4E62_B6A5_67718B8C304F



