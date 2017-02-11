#pragma once
#ifndef RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
#define RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080

#include "StageBase.h"

#include "GL/glew.h"
#include "WinGLPlatform.h"

#include "TVector3.h"

#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"

#include "FastDelegate/FastDelegate.h"

#include "GLCommon.h"
#include "GLUtility.h"
#include "GLDrawUtility.h"

#include <limits>

namespace RenderGL
{
	typedef GL::Vec2i Vec2i;

	float const FLT_DIV_ZERO_EPSILON = 1e-6;

	class PostProcess
	{




	};

	using namespace GL;

	inline bool isInside( Vector3 const& min , Vector3 const& max , Vector3 const& p )
	{
		return min.x < p.x && p.x < max.x &&
			   min.y < p.y && p.y < max.y && 
			   min.z < p.z && p.z < max.z ;
	}
	inline bool testRayTriangle( Vector3 const& org , Vector3 const& dir , Vector3 const& p0 , Vector3 const& p1 , Vector3 const& p2 , float& t )
	{
		Vector3 v0 = p1 - p0;
		Vector3 v1 = p2 - p0;
		Vector3 normal = v0.cross( v1 );

		float ndir = dir.dot( normal );
		if ( Math::Abs( ndir ) < FLT_DIV_ZERO_EPSILON )
		{
			return false;
		}

		t = normal.dot( p0 - org ) / ndir;
		if ( t < 0 )
			return false;

		//test p in triangle
		Vector3 p = org + t * dir;
		Vector3 v2 = p - p0;
		float d00 = v0.dot(v0);
		float d01 = v0.dot(v1);
		float d11 = v1.dot(v1);
		float d20 = v2.dot(v0);
		float d21 = v2.dot(v1);
		float denom = d00 * d11 - d01 * d01;

		if ( Math::Abs( denom ) < FLT_DIV_ZERO_EPSILON )
			return false;

		float u = (d11 * d20 - d01 * d21) / denom;
		float v = (d00 * d21 - d01 * d20) / denom;

		return ( u >= 0 || v >= 0 || u + v <= 1 );
	}

	inline bool testRayAABB( Vector3 const& org , Vector3 const& dir , Vector3 const& min , Vector3 const& max , float&  t )
	{
		if ( isInside( min , max , org ) )
		{
			t = 0;
			return true;
		}
		
		float factor[3];
		for( int i = 0 ; i < 3 ; ++i )
		{
			float const esp = 1e-5;
			if ( dir[i] > FLT_DIV_ZERO_EPSILON )
			{
				factor[i] = ( min[i] - org[i] ) / dir[i];
			}
			else if ( dir[i] < -FLT_DIV_ZERO_EPSILON )
			{
				factor[i] = ( max[i] - org[i] ) / dir[i];
			}
			else
			{
				factor[i] = std::numeric_limits<float>::min();
			}
		}

		int idx = ( factor[0] > factor[1] ) ? ( ( factor[0] > factor[2] ) ? 0 : 2 ) : ( ( factor[1] > factor[2] ) ? 1 : 2 );

		if ( factor[idx] < 0 )
			return false;

		Vector3 p = org + dir * factor[idx];
		int idx1 = ( idx + 1 ) % 3;
		int idx2 = ( idx + 2 ) % 3;
		if ( min[idx1] > p[idx1] || p[idx1] > max[idx1] || 
			 min[idx2] > p[idx2] || p[idx2] > max[idx2] )
		{
			return false;
		}

		t = factor[idx];
		return true;
	}

	struct TileNode
	{
		int   pos[2];
		float scale;

		TileNode** parentLink;
		TileNode*  child[4];
	};

	enum class LightType
	{
		Spot ,
		Point ,
		Directional ,
	};

	struct LightInfo
	{
		LightType type;
		Vector3   pos;
		Vector3   color;
		Vector3   dir;
		float     radius;
	};

	struct ViewInfo
	{
		Camera* camera;

		Matrix4 matVP;
		Matrix4 matView;
		Matrix4 matViewInv;

		void setParam(ShaderProgram& program)
		{
			program.setParam("gParam.viewPos", camera->getPos());
			program.setParam("gParam.matView", matView);
			program.setParam("gParam.matView", matViewInv);
		}
	};

	class RenderParam;
	class SceneRender
	{
	public:
		virtual void render( ViewInfo& view , RenderParam& param ) = 0;
	};

	class RenderParam
	{
	public:
		virtual void setWorld( Matrix4 const& mat ) = 0;
	};

	struct RenderUnit
	{
		bool     beShow;
		Mesh*    mesh;
		Matrix4  world;
	};

	class Scene
	{
	public:
		void render( ViewInfo& view , RenderParam& param )
		{
			for( RenderUnitVec::iterator iter = mUnits.begin() , itEnd = mUnits.end();
				 iter != itEnd ; ++iter )
			{
				RenderUnit& unit = *iter;
				if ( !unit.beShow )
					continue;

				param.setWorld( unit.world );
				unit.mesh->draw();
			}
		}

		typedef std::vector< RenderUnit > RenderUnitVec;
		RenderUnitVec mUnits;
	};



	class EnvTech
	{
	public:

		bool init()
		{
			if ( !mBuffer.create() )
				return false;

			if ( !mTexEnv.create( Texture::eRGBA32F , MapSize , MapSize ) )
				return false;


			DepthRenderBuffer depthBuffer;
			if ( !depthBuffer.create( MapSize , MapSize , Texture::eDepth24 ) )
				return false;

			mBuffer.addTexture( mTexEnv , Texture::eFaceX );
			mBuffer.setDepth( depthBuffer , true );
			return true;
			
		}
		static int const MapSize = 512;

		FrameBuffer mBuffer;

		TextureCube mTexSky;
		TextureCube mTexEnv;


	};

	inline float normalizePlane( Vector4& plane )
	{
		float len2 = plane.xyz().dot( plane.xyz() );
		float invSqrt =  Math::InvSqrt( len2 );
		plane *= invSqrt;
		return invSqrt;
	}

	class TechBase
	{
	public:

		static void drawCubeTexture( TextureCube& texCube );
	};



	class DefferredLightingTech : public TechBase
		                        , public RenderParam
	{
	public:
		bool init();

		void renderBassPass(ViewInfo& view, SceneRender& scene);
		void renderLight(ViewInfo& view, LightInfo const& light, TextureCube& texShadow, float shadowParam[2]);

		enum BufferId
		{
			BufferA , //xyz : WorldPos
			BufferB , //xyz : Noraml
			BufferC , //xyz : BaseColor
			BufferD ,

			NumBuffer ,
		};

		FrameBuffer  mBuffer;
		Texture2D    mGTextures[ NumBuffer ];
		DepthTexture mDepthTexture;
		ShaderEffect mBassPass;
		ShaderEffect mLightingPass;
		

		virtual void setWorld(Matrix4 const& mat) override
		{
			glMultMatrixf(mat);
			Matrix4 matInv;
			float det;
			mat.inverseAffine(matInv, det);
			mBassPass.setMatrix44("gParam.matWorld", mat);
			mBassPass.setMatrix44("gParam.matWorldInv", matInv);
		}

		void renderBuffer();

		void renderDepthTexture(int x, int y, int width, int height);
		void renderTexture(int x, int y, int width, int height, int idxBuffer);

	};
	class WaterTech : TechBase
	{
	public:
		static int const MapSize = 512;
		bool init();

		void render( ViewInfo& view , SceneRender& scene)
		{
			normalizePlane( waterPlane );
			int vp[4];
			glGetIntegerv( GL_VIEWPORT , vp );

			glViewport( 0 , 0 , MapSize , MapSize );
			glPushMatrix();
			ReflectMatrix matReflect( waterPlane.xyz() , waterPlane.w );
			glMultMatrixf( matReflect );
			double equ[] = { waterPlane.x , waterPlane.y , waterPlane.z , waterPlane.w };
			glEnable( GL_CLIP_PLANE0 );
			glClipPlane( GL_CLIP_PLANE0 , equ );

			mBuffer.setTexture( 0 , mReflectMap );
			mBuffer.bind();
			//scene.render(view, *this);
			mBuffer.unbind();

			glDisable( GL_CLIP_PLANE0 );
			glPopMatrix();
			glViewport( vp[0] , vp[1] , vp[2] , vp[3] );
		}

		Vector4     waterPlane;

		Mesh        mWaterMesh;
		Texture2D   mReflectMap;
		Texture2D   mRefractMap;
		FrameBuffer mBuffer;
	};

	class ShadowDepthTech : public RenderParam
	{
	public:
		static int const MapSize = 512;
		bool init();

		void renderLighting( ViewInfo& view, SceneRender& scene , LightInfo const& light , bool bMultiple );
		void renderShadowDepth(ViewInfo& view, SceneRender& scene , LightInfo const& light );

		void drawShadowTexture();
		
		void reload()
		{
			mProgShadowDepth.reload();
			mProgLighting.reload();
		}

		virtual void setWorld( Matrix4 const& mat )
		{
			glMultMatrixf( mat );
			if ( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam( "gParam.matWorld" , mat );
				//Mat44 matInv; float det;
				//if ( mat.inverse( matInv , det ) )
				//	mEffectCur->setParam( "gParam.matWorldInv" , matInv );
			}
		}

		float         depthParam[2];
		Matrix4       matLightView;
		
		ShaderEffect* mEffectCur;

		Mesh          mCubeMesh;
		ShaderEffect  mEffectCube;

		Texture2D     mShadowMap2;
		TextureCube   mShadowMap;
		FrameBuffer   mShadowBuffer;
		ShaderEffect  mProgShadowDepth;
		ShaderEffect  mProgLighting;
	};


	class ViewFrustum
	{
	public:
		Matrix4 getPerspectiveMatrix()
		{
			return GL::PerspectiveMatrix( mYFov , mAspect , mNear , mFar );
		}
	
		float mNear;
		float mFar;
		float mYFov;
		float mAspect;
	};

	struct AABB
	{
		Vector3 min,max;
	};

	class SampleStage : public StageBase
		              , public SceneRender
	{
		typedef StageBase BaseClass;
		
	public:
		SampleStage();
		~SampleStage();

		virtual bool onInit();
		virtual void onInitFail();
		virtual void onEnd();

		bool createPlaneMesh(float len, float texF);


		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();
			updateFrame( frame );
		}

		void onRender( float dFrame );

		void renderTest0( ViewInfo& view );
		void renderTest1( ViewInfo& view );
		void renderTest2( ViewInfo& view );
		void renderTest3( ViewInfo& view );
		void renderTest4( ViewInfo& view );

		void showLight(int num)
		{
			mProgSphere.bind();
			mProgSphere.setParam("gParam.viewPos", mCamera->getPos());
			mProgSphere.setParam("gParam.matWorldInv", Matrix4::Identity());
			mProgSphere.setParam("sphere.radius", 0.2f);
			for( int i = 0; i < num; ++i )
			{
				mProgSphere.setParam("sphere.localPos", mLightPos[i]);
				mSpherePlane.draw();
			}
			mProgSphere.unbind();
		}

		void renderScene( RenderParam& param );

		void restart()
		{

		}


		void tick();

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg );

		bool onKey( unsigned key , bool isDown );
		void drawAxis( float len );
		void drawSky();

		bool createFrustum( Mesh& mesh , Matrix4 const& matProj )
		{
			float depth = 0.9999;
			Vector3 v[8] =
			{
				Vector3(1,1,depth),Vector3(1,-1,depth),Vector3(-1,-1,depth),Vector3(-1,1,depth),
				Vector3(1,1,-depth),Vector3(1,-1,-depth),Vector3(-1,-1,-depth),Vector3(-1,1,-depth),
			};

			Matrix4 matInv;
			float det;
			matProj.inverse( matInv , det );
			for( int i = 0 ; i < 8 ; ++i )
			{
				v[i] = transform( v[i] , matInv );
			}

			int idx[24] =
			{
				0,1, 1,2, 2,3, 3,0,
				4,5, 5,6, 6,7, 7,4,
				0,4, 1,5, 2,6, 3,7,
			};
			mesh.mDecl.addElement( Vertex::ePosition , Vertex::eFloat3 );
			if ( !mesh.createBuffer( v , 8 , idx , 24 , true ) )
				return false;
			mesh.mType = Mesh::eLineList;
			return true;
		}


		void calcViewRay( float x , float y )
		{
			Matrix4 matViewProj = mCamera->getViewMatrix() * mViewFrustum.getPerspectiveMatrix();
			Matrix4 matVPInv;
			float det;
			matViewProj.inverse( matVPInv, det );

			rayStart = transform( Vector3( x , y , -1 ) , matVPInv );
			rayEnd   = transform( Vector3( x , y ,  1 ) , matVPInv );

		}


		virtual void render(ViewInfo& view, RenderParam& param) override;

	protected:

		float mTime;

		GL::ShaderEffect mProgPlanet;
		GL::ShaderEffect mProgSphere;
		GL::ShaderEffect mProgBump;
		GL::ShaderEffect mProgParallax;
		GL::ShaderEffect mEffectSphereSM;
		GL::ShaderEffect mEffectSimple;

		Mesh   mMesh;
		Mesh   mSphereMesh;
		Mesh   mSphereMesh2;
		Mesh   mSpherePlane;
		Mesh   mBoxMesh;
		Mesh   mPlane;
		Mesh   mDoughnutMesh;
		Mesh   mFrustumMesh;
		static int const LightNum = 3;
		Vector3 mLightPos[ LightNum ];
		Vector3 mPos;
		GL::Camera  mCamStorage[2];
		GL::Camera* mCamera;
		ViewFrustum mViewFrustum;

		static int const NumAabb = 3;
		AABB    mAabb[3];
		bool    mIsIntersected;
		Vector3 mIntersectPos;
		Vector3 rayStart , rayEnd;

		bool   mPause;

		bool  mLineMode;
		ShadowDepthTech mShadowTech;
		DefferredLightingTech mDefferredLightingTech;
		
		Mesh   mSkyMesh;
		TextureCube mTexSky;

		Texture2D mTexBase;
		Texture2D mTexNormal;

		int   mIdxChioce;

		bool initFrameBuffer( Vec2i const& size );

		void copyTexture(Texture2D& destTexture, Texture2D& srcTexture);
		void copyTextureToBuffer(Texture2D& copyTexture);

		Texture2D&  getRenderFrameTexture() { return mFrameTextures[mIdxRenderFrameTexture]; }
		Texture2D&  getPrevRednerFrameTexture() { return mFrameTextures[1 - mIdxRenderFrameTexture]; }
		void swapFrameBufferTexture()
		{
			mIdxRenderFrameTexture = 1 - mIdxRenderFrameTexture;
			mFrameBuffer.setTexture(0, getRenderFrameTexture());
		}
		Texture2D   mFrameTextures[2];
		int         mIdxRenderFrameTexture;
		FrameBuffer mFrameBuffer;

		ShaderEffect mProgCopyTexture;
	};





}//namespace RenderGL


#endif // RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
