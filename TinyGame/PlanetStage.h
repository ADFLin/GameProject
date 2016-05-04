#ifndef PlanetStage_h__
#define PlanetStage_h__

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

#include <limits>

namespace GS
{
	float const FLT_DIV_ZERO_EPSILON = 1e-6;

	class PostProcessEffect
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
		void render( RenderParam& param )
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



	typedef ::fastdelegate::FastDelegate< void ( RenderParam& ) > RenderFun;

	class EnvTech
	{
	public:

		bool init()
		{
			if ( !mBuffer.create() )
				return false;

			if ( !mTexEnv.create( Texture::eRGBA32F , MapSize , MapSize ) )
				return false;


			DepthBuffer depthBuffer;
			if ( !depthBuffer.create( MapSize , MapSize , Texture::eDepth24 ) )
				return false;

			mBuffer.addTexture( mTexEnv , Texture::eFaceX );
			mBuffer.setBuffer( depthBuffer , true );
			
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

		void drawCubeTexture( TextureCube& texCube );
	};

	class WaterTech : TechBase
	{
	public:
		static int const MapSize = 512;
		bool init();

		void render( RenderFun fun )
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
			//fun( *this );
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

	class ShadowMapTech : public RenderParam
	{
	public:
		static int const MapSize = 512;
		bool init();
		void render( RenderFun fun , bool bMultiple );

		void renderShadowMapPass(RenderFun fun);

		void drawShadowMap();
		
		void reload()
		{
			mProgMap.reload();
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

		Vector3       lightPos;
		Vector3       lightColor;
		float         depthParam[2];
		Vector3       viewPos;
		Matrix4       matLightView;
		
		ShaderEffect* mEffectCur;

		Mesh         mCubeMesh;
		ShaderEffect mEffectCube;

		Texture2D    mShadowMap2;
		TextureCube  mShadowMap;
		FrameBuffer  mBuffer;
		ShaderEffect mProgMap;
		ShaderEffect mProgLighting;
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

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
		
	public:
		TestStage();
		~TestStage();

		virtual bool onInit();

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

		void renderTest0();
		void renderTest1();
		void renderTest2();
		void renderTest3();

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
		ShadowMapTech mTech;
		
		Mesh   mSkyMesh;
		TextureCube mTexSky;

		Texture2D mTexBase;
		Texture2D mTexNormal;

		int   mIdxChioce;

	};



}//namespace Planet

#endif // PlanetStage_h__
