#pragma once
#ifndef SoftwareRenderStage_H_6E10EA7D_C3B2_4E62_B6A5_67718B8C304F
#define SoftwareRenderStage_H_6E10EA7D_C3B2_4E62_B6A5_67718B8C304F

#include "Stage/TestStageHeader.h"

#include "WindowsHeader.h"
#include "Widget/WidgetUtility.h"
#include "Math/Base.h"

#include "Math/Transform.h"
#include "RHI/RHIType.h"

#include "RefCount.h"

namespace SR
{
	using namespace Render;

	using XForm = Math::Transform;
	struct Color
	{
		union
		{
			struct
			{
				uint8 b, g, r, a;
			};
			uint32 word;
		};

		operator uint32() const { return word; }

		Color() {}
		Color(uint8 r, uint8 g, uint8 b, uint8 a = 255)
			:r(r), g(g), b(b), a(a)
		{
		}
		Color(Color const& other) :word(other.word) {}
	};


	Vector3 Reflection(Vector3 InV, Vector3 n)
	{
		return InV - (2 * n.dot(InV)) * n;
	}


	struct LinearColor
	{
		float r, g, b, a;

		operator Color() const
		{
			uint8 byteR = uint8(255 * Math::Clamp<float>(r, 0, 1));
			uint8 byteG = uint8(255 * Math::Clamp<float>(g, 0, 1));
			uint8 byteB = uint8(255 * Math::Clamp<float>(b, 0, 1));
			uint8 byteA = uint8(255 * Math::Clamp<float>(a, 0, 1));
			return Color(byteR, byteG, byteB, byteA);
		}
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
		LinearColor(Color const& rhs)
			:r(float(rhs.r) / 255.0f), g(float(rhs.g) / 255.0f), b(float(rhs.b) / 255.0f), a(float(rhs.a) / 255.0f)
		{
		}

		LinearColor(Vector3 const& v)
			:r(v.x), g(v.y), b(v.z), a(1.0)
		{
		}
	};

	LinearColor operator * (float s, LinearColor const& rhs) { return LinearColor(s * rhs.r, s * rhs.g, s * rhs.b, s * rhs.a); }



	class ColorBuffer
	{
	public:
		bool create(Vec2i const& size);

		void clear(Color const& color)
		{
			std::fill_n(mData, mSize.y * mRowStride, uint32(color));
		}

		Color getPixel(uint32 x, uint32 y)
		{
			Color result;
			result.word = mData[(x + mRowStride * y)];
			return result;
		}

		void setPixel(uint32 x, uint32 y, Color color)
		{
			uint32* pData = mData + (x + mRowStride * y);
			*pData = color;
		}
		void setPixelCheck(uint32 x, uint32 y, Color color)
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

	struct PixelShaderType
	{


	};

	class Texture
	{
	public:
		bool load(char const* path);

		Color getColor(Vec2i const& pos) const;

		LinearColor sample(Vector2 const& UV) const;

		Vec2i mSize;
		std::vector< Color > mData;

	};


	void GenerateDistanceFieldTexture(Texture const& texture)
	{
		Vec2i size = texture.mSize;
		std::vector< Vec2i > countMap(size.x * size.y, Vec2i(0, 0));

		{
			{
				uint32 c = texture.getColor(Vec2i(0, 0));
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
				uint32 c = texture.getColor(Vec2i(i, 0));
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
				uint32 c = texture.getColor(Vec2i(0, j));
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
				uint32 c = texture.getColor(Vec2i(i, j));
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
				uint32 c = texture.getColor(Vec2i(i, j));


			}
		}
	}

	struct RenderTarget
	{
		ColorBuffer* colorBuffer;
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
				mRenderTarget.colorBuffer->clear(color);
		}
	protected:
		RenderTarget mRenderTarget;
	};
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

		void drawTriangle(Vector3 v0, LinearColor color0, Vector2 uv0,
						  Vector3 v1, LinearColor color1, Vector2 uv1,
						  Vector3 v2, LinearColor color2, Vector2 uv2);


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

	struct RayResult
	{
		float     distance;
		Vector3   normal;
		Material* material;

		RayResult()
		{
			distance = 0;
			material = nullptr;
		}
	};

	bool LineSphereTest(Vector3 const& linePos, Vector3 const& dir, Vector3 const& spherePos, float radius,  float outDistance[2])
	{
		Vector3 offset = spherePos - linePos;

		float b = offset.dot( dir );
		float c = offset.length2() - radius * radius;

		float d = b * b - c;
		if( d < 0 )
			return false;

		d = Math::Sqrt(d);
		outDistance[0] = b - d;
		outDistance[1] = b + d;
		return true;
	}


	class PrimitiveShape : public RefCountedObjectT< PrimitiveShape >
	{
	public:
		virtual bool raycast( RayTrace const& trace , RayResult& result ) = 0;
	};

	class SphereShape : public PrimitiveShape
	{

	public:
		bool raycast(RayTrace const& trace, RayResult& result) override
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
		bool raycast(RayTrace const& trace, RayResult& result) override
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
		bool raycast(RayTrace const& trace, RayResult& result)
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
			Vector3 axisZ = Math::GetNormal(pos - lookPos);
			Vector3 axisY = Math::GetNormal(upDir - upDir.projectNormal(axisZ));
			Vector3 axisX = axisY.cross(axisZ);
			transform.rotation = Matrix3(axisX, axisY, axisZ).toQuatNoScale();
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

		bool raycast(RayTrace const& trace, RayResult& result)
		{
			PrimitiveObject* hitObject = nullptr;
			result.distance = Math::MaxFloat;
			for( PrimitiveObject* object : mPrimitiveObjects )
			{
				RayResult objResult;
				if( object->raycast(trace, objResult) )
				{
					if ( objResult.distance < 1e-4 )
						continue;

					if( objResult.distance < result.distance )
					{
						hitObject = object;
						result = objResult;
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

		bool bRayTracerUsed = true;
		RayTraceRenderer mRTRenderer;
		RasterizedRenderer mRenderer;

		ColorBuffer mColorBuffer;

		Scene  mScene;
		Camera mCamera;


		void setupScene();

		bool onInit() override;


		void onEnd() override
		{

			BaseClass::onEnd();
		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			if( !bPause )
			{
				for( int i = 0; i < frame; ++i )
					tick();
			}

			updateFrame(frame);
		}

		void renderTest2()
		{
			Graphics2D& g = Global::GetGraphics2D();

			RenderTarget renderTarget;
			renderTarget.colorBuffer = &mColorBuffer;
			mRTRenderer.setRenderTarget(renderTarget);
			mRTRenderer.clearBuffer(LinearColor(0.2, 0.2, 0.2, 0));

			mRTRenderer.render(mScene, mCamera);

			mColorBuffer.draw(g);
		}

		void renderTest1();

		void onRender(float dFrame) override
		{
			if( bRayTracerUsed )
			{
				renderTest2();
			}
			else
			{
				renderTest1();
			}
			
		}

		void restart()
		{

		}

		bool bPause = false;
		float angle = 0;

		void tick()
		{

			angle += float(gDefaultTickTime) / 1000.0;
		}

		void updateFrame(int frame)
		{

		}

		bool onMouse(MouseMsg const& msg) override
		{
			if( !BaseClass::onMouse(msg) )
				return false;
			return true;
		}

		bool onKey(KeyMsg const& msg) override
		{
			if( !msg.isDown() )
				return false;

			switch(msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::P: bPause = !bPause; break;
			case EKeyCode::F2: bRayTracerUsed = !bRayTracerUsed; break;
			}
			return false;
		}


	};

}//namespace SWR



#endif // SoftwareRenderStage_H_6E10EA7D_C3B2_4E62_B6A5_67718B8C304F



