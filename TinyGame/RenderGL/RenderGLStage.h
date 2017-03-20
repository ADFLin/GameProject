#pragma once
#ifndef RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
#define RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080

#include "StageBase.h"

#include "HashString.h"
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
#include "CommonMarco.h"

#include <limits>
#include <memory>

#include "Material.h"
#include "Scene.h"
#include "SceneRenderer.h"
#include "GpuProfiler.h"


namespace RenderGL
{
	using namespace GL;
	float const FLT_DIV_ZERO_EPSILON = 1e-6;

	class MaterialMaster;

	typedef TVector2<int> Vec2i;

	class AABBox
	{
	public:

		AABBox() {}
		AABBox(Vector3 const& min, Vector3 const& max)
			:mMin(min), mMax(max)
		{
		}

		static AABBox Empty() { return AABBox(Vector3::Zero(), Vector3::Zero()); }

		Vector3 getMin() const { return mMin; }
		Vector3 getMax() const { return mMax; }
		Vector3 getSize() const { return mMax - mMin; }
		float getVolume()
		{
			Vector3 size = getSize();
			return size.x * size.y * size.z;
		}
		void setEmpty()
		{
			mMin = mMax = Vector3::Zero();
		}

		void addPoint(Vector3 const& v)
		{
			mMax.max(v);
			mMin.min(v);
		}
		void translate(Vector3 const& offset)
		{
			mMax += offset;
			mMin += offset;
		}
		void expand(Vector3 const& dv)
		{
			mMax += dv;
			mMin -= dv;
		}

		void makeUnion(AABBox const& other)
		{
			mMin.min(other.mMin);
			mMax.max(other.mMax);
		}
	private:
		Vector3 mMin;
		Vector3 mMax;
	};



	struct GLGpuSync
	{
		GLGpuSync()
		{
			bUseFence = false;
			renderSync = nullptr;
			loadingSync = nullptr;
		}
		bool pervRender();
		void postRender();
		bool pervLoading();
		void postLoading();

		bool   bUseFence;
		GLsync renderSync;
		GLsync loadingSync;
	};


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

	//??
	struct TileNode
	{
		int   pos[2];
		float scale;

		TileNode** parentLink;
		TileNode*  child[4];
	};

	struct CycleTrack
	{
		Vector3 planeNormal;
		Vector3 center;
		float   radius;
		float   period;

		Vector3 getValue(float time)
		{
			float theta = 2 * Math::PI * (time / period);
			Quaternion q;
			q.setRotation(planeNormal, theta);

			Vector3 p;
			if( planeNormal.z < planeNormal.y )
				p = Vector3(planeNormal.y, -planeNormal.x, 0);
			else
				p = Vector3( 0 , -planeNormal.z, planeNormal.y);
			p.normalize();
			return center + radius * q.rotate(p);
		}
	};

	class ViewFrustum
	{
	public:
		Matrix4 getPerspectiveMatrix()
		{
			return PerspectiveMatrix(mYFov, mAspect, mNear, mFar);
		}

		float mNear;
		float mFar;
		float mYFov;
		float mAspect;
	};

	struct AABB
	{
		Vector3 min, max;
	};


	typedef std::shared_ptr< Material > MaterialPtr;
	class StaticMesh : public Mesh
	{
	public:
		void postLoad()
		{



		}

		void render(Matrix4 const& worldTrans , RenderParam& param , Material* material)
		{
			param.setMaterial(material);
			param.setWorld(worldTrans);

			{
				//GPU_PROFILE_VA("MeshDraw %s", name.c_str());
				draw();
			}
		}
		void render(Matrix4 const& worldTrans , RenderParam& param)
		{
			glPushMatrix();
			for( int i = 0; i < mSections.size(); ++i )
			{
				//if ( i != 5  )
					//continue;
				Material* material = getMaterial(i);
				param.setMaterial(material);
				param.setWorld(worldTrans);

				{
					char const* matName = material ? material->getMaster()->mName.c_str() : "DefalutMaterial";
					//GPU_PROFILE_VA( "MeshDraw %s %s %d" , name.c_str() , matName , mSections[i].num);
					this->drawSection(i);
				}
			}
			glPopMatrix();
		}

		void setMaterial(int idx, MaterialPtr material)
		{
			if( idx >= mMaterials.size() )
				mMaterials.resize(idx + 1);
			mMaterials[idx] = material;
		}
		Material* getMaterial(int idx)
		{
			if( idx < mMaterials.size() )
				return mMaterials[idx].get();
			return nullptr;
		}
		std::vector< MaterialPtr > mMaterials;
		std::string name;
	};

	struct Transform
	{
		Vector3    scale;
		Vector3    translate;
		Quaternion rotation;
	};

	class StaticMeshObject
	{
		StaticMesh* mesh;
		Transform   localTransform;
	};

	struct MeshBatchElement
	{
		ShaderProgram* Shader;
		Mesh*    mesh;
		Matrix4  world;
		int      idxSection;
	};

	class Scene
	{
	public:
		void render( ViewInfo& view , RenderParam& param )
		{
			for( RenderUnitVec::iterator iter = mUnits.begin() , itEnd = mUnits.end();
				 iter != itEnd ; ++iter )
			{
				MeshBatchElement& unit = *iter;
				param.setWorld( unit.world );
				unit.mesh->draw();
			}
		}

		typedef std::vector< MeshBatchElement > RenderUnitVec;
		RenderUnitVec mUnits;
	};

	class EnvTech
	{
	public:

		bool init()
		{
			if( !mBuffer.create() )
				return false;

			if( !mTexEnv.create(Texture::eFloatRGBA, MapSize, MapSize) )
				return false;

			DepthRenderBuffer depthBuffer;
			if( !depthBuffer.create(MapSize, MapSize, Texture::eDepth24) )
				return false;

			mBuffer.addTexture(mTexEnv, Texture::eFaceX);
			mBuffer.setDepth(depthBuffer, true);
			return true;

		}
		static int const MapSize = 512;

		FrameBuffer mBuffer;

		RHITextureCube mTexSky;
		RHITextureCube mTexEnv;


	};
	class WaterTech : TechBase
	{
	public:
		static int const MapSize = 512;
		bool init();

		void render(ViewInfo& view, SceneRender& scene)
		{
			normalizePlane(waterPlane);
			int vp[4];
			glGetIntegerv(GL_VIEWPORT, vp);

			glViewport(0, 0, MapSize, MapSize);
			glPushMatrix();
			ReflectMatrix matReflect(waterPlane.xyz(), waterPlane.w);
			glMultMatrixf(matReflect);
			double equ[] = { waterPlane.x , waterPlane.y , waterPlane.z , waterPlane.w };
			glEnable(GL_CLIP_PLANE0);
			glClipPlane(GL_CLIP_PLANE0, equ);

			mBuffer.setTexture(0, *mReflectMap);
			mBuffer.bind();
			//scene.render(view, *this);
			mBuffer.unbind();

			glDisable(GL_CLIP_PLANE0);
			glPopMatrix();
			glViewport(vp[0], vp[1], vp[2], vp[3]);
		}

		Vector4      waterPlane;

		Mesh         mWaterMesh;
		RHITexture2DRef mReflectMap;
		RHITexture2DRef mRefractMap;
		FrameBuffer  mBuffer;
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

		bool loadAssetResouse();
		void setupScene();

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

		void showLight( ViewInfo& view );

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

		void reloadMaterials();

		void drawAxis(float len);
		void drawSky();

		bool createFrustum( Mesh& mesh , Matrix4 const& matProj );


		void calcViewRay( float x , float y );


		virtual void render(ViewInfo& view, RenderParam& param) override;


		template< class Fun >
		void visitLights( Fun& fun )
		{
			for( int i = 0; i < mNumLightDraw; ++i )
			{
				LightInfo const& light = mLights[i];
				fun( i , light);
			}
		}

	protected:
		GLGpuSync   mGpuSync;
		void prevLoading()
		{
			while( !mGpuSync.pervLoading() )
			{
				::Sleep(1);
			}
		}
		void postLoading()
		{
			mGpuSync.postLoading();
		}

		ShaderProgram mProgPlanet;
		ShaderProgram mProgSphere;
		ShaderProgram mProgBump;
		ShaderProgram mProgParallax;
		ShaderProgram mEffectSphereSM;
		ShaderProgram mEffectSimple;

		Mesh   mMesh;
		Mesh   mSphereMesh;
		Mesh   mSphereMesh2;
		Mesh   mSpherePlane;
		Mesh   mBoxMesh;
		Mesh   mPlane;
		Mesh   mDoughnutMesh;
		Mesh   mFrustumMesh;

		int  mNumLightDraw = 4;
		std::vector< LightInfo > mLights;
		CycleTrack mTracks[4];

		Vector3 mPos;
		GL::Camera  mCamStorage[2];
		GL::Camera* mCamera;
		ViewFrustum mViewFrustum;
		Scene mScene;

		bool bUseFrustumTest = true;

		static int const NumAabb = 3;
		AABB    mAabb[3];
		bool    mIsIntersected;
		Vector3 mIntersectPos;
		Vector3 rayStart , rayEnd;


		std::vector< MaterialMaster > mMaterials;
		typedef std::shared_ptr< RHITexture2D > Texture2DPtr;
		std::vector< Texture2D > mTextures;
		typedef std::shared_ptr< StaticMesh > StaticMeshPtr;
		std::vector< StaticMeshPtr > mMeshs;

		Material*  getMaterial(int idx)
		{ 
			return &mMaterials[idx]; 
		}
		Texture2D& getTexture(int idx) 
		{ 
			return mTextures[idx];
		}
		StaticMesh&  getMesh(int idx) 
		{ 
			if( mMeshs[idx] )
				return *mMeshs[idx];
			return mEmptyMesh;
		}

		RHITexture2D* RHICreateTexture2D()
		{
			return new RHITexture2D;
		}

		bool  mLineMode;

		SceneRenderTargets mSceneRenderTargets;

		ShadowDepthTech mShadowTech;
		DefferredLightingTech mDefferredLightingTech;

		PostProcessSSAO  mSSAO;

		bool   mbShowBuffer;

		int    renderLightCount = 0;
		
		Mesh   mSkyMesh;
		RHITextureCubeRef mTexSky;

		StaticMesh  mEmptyMesh;
		
		RHITexture2DRef mTexBase;
		RHITexture2DRef mTexNormal;

		bool   mPause;
		float  mTime;
		float  mRealTime;
		int    mIdxChioce;




		bool bInitialized = false;

	};





}//namespace RenderGL


#endif // RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
