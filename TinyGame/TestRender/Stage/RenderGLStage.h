#pragma once
#ifndef RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
#define RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080

#include "StageBase.h"

#include "HashString.h"
#include "GL/glew.h"
#include "WinGLPlatform.h"
#include "SystemPlatform.h"
#include "Asset.h"

#include "MarcoCommon.h"
#include "TVector3.h"
#include "FastDelegate/FastDelegate.h"
#include "RandomUtility.h"

#include "RHI/OpenGLCommon.h"
#include "RHI/ShaderCore.h"
#include "RHI/MeshUtility.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/Material.h"
#include "RHI/Scene.h"
#include "RHI/SceneObject.h"
#include "RHI/SceneRenderer.h"
#include "RHI/LazyObject.h"

#include "SceneScript.h"

#include <limits>
#include <memory>
#include <typeindex>

class Thread;

namespace Render
{
	float const FLT_DIV_ZERO_EPSILON = 1e-6;

	//#MOVE


	class MaterialMaster;

	typedef TVector2<int> Vec2i;

	class CameraMove
	{
	public:
		CameraMove()
		{
			vel = Vector3::Zero();
			frameMoveDir = Vector3::Zero();
			maxSpeed = 50.0f;
			accMe = 5000.f * maxSpeed;
			
		}

		void moveFront() { frameMoveDir += target->getViewDir(); }
		void moveBack() { frameMoveDir -= target->getViewDir(); }
		void moveRight() { frameMoveDir += target->getRightDir(); }
		void moveLeft() { frameMoveDir -= target->getRightDir(); }

		void update(float dt)
		{
			float len = frameMoveDir.normalize();

			Vector3 dv = Vector3::Zero();
			if( len == 0 )
			{
				frameMoveDir = -vel;
				len = frameMoveDir.normalize();
				if( len != 0 )
				{
					dv = Math::Min( len , accMe * dt ) * frameMoveDir;
				}
			}
			else
			{
				dv = ( accMe * dt ) * frameMoveDir;
			}

			Vector3 velPrev = vel;
			vel += dv;

			float speedSqr = vel.length();

			if( speedSqr > maxSpeed * maxSpeed )
			{
				vel = ( sqrt( speedSqr ) / maxSpeed ) * vel;
				dv = vel - velPrev;
			}

			Vector3 offset = vel * dt + dv * (0.5 * dt);
			target->setPos(target->getPos() + offset);
			frameMoveDir = Vector3::Zero();
		}

		float maxSpeed;
		float accMe;

		Vector3 frameMoveDir;
		Vector3 vel;
		//Vector3 acc;
		SimpleCamera* target;
	};

	//



	class MaterialAsset : public IAssetViewer
	{
	public:
		TLazyObjectGuid< Material > materialId;
		std::shared_ptr< MaterialMaster > material;
		bool load(char const* file)
		{
			material.reset( new MaterialMaster );
			if( !material->loadFile(file) )
				return false;
			materialId = material.get();
			return true;
		}
	protected:
		virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override
		{
			if ( material )
			{
				std::string cPath = MaterialShaderMap::GetFilePath(material->mName.c_str());
				paths.push_back(FCString::CharToWChar(cPath.c_str()));
			}
		}
		virtual void postFileModify(FileAction action) override
		{
			if ( action == FileAction::Modify )
				material->reload();
		}
	};

	class SceneAsset : public IAssetViewer
	{
	public:
		bool load(char const* name)
		{
			mName = name;
			return loadInternal();
		}

		static IAssetProvider* sAssetProvider;
		Scene  scene;

		std::function< void(Scene&) > setupDelegate;
	protected:
		virtual void getDependentFilePaths(std::vector<std::wstring>& paths) override
		{
			std::string cPath = ISceneScript::GetFilePath( mName.c_str() );
			paths.push_back(FCString::CharToWChar(cPath.c_str()));
		}

		virtual void postFileModify(FileAction action) override
		{
			if( action == FileAction::Modify )
			{
				scene.removeAll();
				loadInternal();
			}
		}
		
		bool loadInternal()
		{
			if( setupDelegate )
			{
				setupDelegate(scene);
			}
			auto script = ISceneScript::Create();
			bool result = script->setup(scene, *sAssetProvider, mName.c_str());
			script->release();
			return result;
		}
		

		std::string mName;
		
	};

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


	inline bool IsInside( Vector3 const& min , Vector3 const& max , Vector3 const& p )
	{
		return min.x < p.x && p.x < max.x &&
			   min.y < p.y && p.y < max.y && 
			   min.z < p.z && p.z < max.z ;
	}
	inline bool TestRayTriangle( Vector3 const& org , Vector3 const& dir , Vector3 const& p0 , Vector3 const& p1 , Vector3 const& p2 , float& t )
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

	inline bool TestRayAABB( Vector3 const& org , Vector3 const& dir , Vector3 const& min , Vector3 const& max , float&  t )
	{
		if ( IsInside( min , max , org ) )
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


	class IValueModifier
	{
	public:
		virtual bool isHook(void* ptr) { return false; }
		virtual void update(float time) = 0;
	};

	template< class TrackType >
	class TVectorTrackModifier : public IValueModifier
	{
	public:
		TVectorTrackModifier(Vector3& value)
			:mValue(value)
		{

		}
		virtual void update(float time)
		{
			mValue = track.getValue(time);
		}
		virtual bool isHook(void* ptr) { return &mValue == ptr; }
		TrackType track;
	private:
		Vector3&  mValue;
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


	class DualQuat
	{
	public:
		DualQuat( Quaternion const& InQr , Quaternion const& InQd )
			:Qr(InQr), Qd(InQd){ }

		DualQuat(Quaternion const& InQr, Vector3 const& InOrg )
			:Qr(InQr), Qd(0.5 * InOrg.x , 0.5 *InOrg.y , 0.5 *InOrg.z , 0 )
		{
		}
		DualQuat(Vector3 const& InOrg)
			:Qr(Quaternion::Identity()), Qd(0.5 * InOrg.x, 0.5 *InOrg.y, 0.5 *InOrg.z, 0)
		{
		}

		Quaternion Qr;
		Quaternion Qd;

		DualQuat operator + (DualQuat const& rhs)
		{
			return DualQuat(Qr + rhs.Qr, Qd + rhs.Qd);
		}

		DualQuat operator - (DualQuat const& rhs)
		{
			return DualQuat(Qr - rhs.Qr, Qd - rhs.Qd);
		}

		friend DualQuat operator * (float s, DualQuat const& DQ)
		{
			return DualQuat(s * DQ.Qr, s * DQ.Qd);
		}

		DualQuat operator * (DualQuat const& rhs)
		{
			return DualQuat(Qr * rhs.Qr, Qr * rhs.Qd + Qd * rhs.Qr);
		}
	};

	class InstancedMesh
	{
	public:
		Mesh* mesh;

		int addInstance(Vector3 const& pos, Vector3 const& scale, Quaternion const& rotation , Vector4 const& param )
		{
			Matrix4 xform= Matrix4::Scale(scale) * Matrix4::Rotate(rotation) * Matrix4::Translate( pos );
			mInstanceTransforms.push_back(xform);
			mInstanceParams.push_back(param);
			bBufferValid = false;
		}

		bool UpdateInstanceBuffer()
		{
			int newBufferSize = sizeof(Vector4) * 4 * mInstanceTransforms.size();
			if( !mInstancedBuffer.isValid() )
			{
				mInstancedBuffer = RHICreateVertexBuffer(sizeof(Vector4) * 4, mInstanceTransforms.size(), BCF_UsageDynamic, nullptr);
				if( !mInstancedBuffer.isValid() )
				{
					LogMsg("Can't create vertex buffer!!");
					return false;
				}
			}
			else
			{
				if( mInstancedBuffer->getSize() < newBufferSize )
				{
					mInstancedBuffer->resetData(sizeof(Vector4) * 4, mInstanceTransforms.size(), BCF_UsageDynamic, nullptr);
				}
			}

			Vector4* ptr = (Vector4*) RHILockBuffer( mInstancedBuffer , ELockAccess::WriteOnly);
			if( ptr == nullptr )
			{
				return false;
			}

			for( int i = 0; i < mInstanceTransforms.size(); ++i )
			{
				ptr[0] = mInstanceTransforms[i].row(0);
				ptr[0].w = mInstanceParams[i].x;
				ptr[1] = mInstanceTransforms[i].row(1);
				ptr[1].w = mInstanceParams[i].y;
				ptr[2] = mInstanceTransforms[i].row(2);
				ptr[2].w = mInstanceParams[i].z;
				ptr[3] = mInstanceTransforms[i].row(3);
				ptr[3].w = mInstanceParams[i].w;
			}
			RHIUnlockBuffer( mInstancedBuffer );

			return true;
		}

		void draw()
		{
			if( !bBufferValid )
			{
				if( UpdateInstanceBuffer() )
				{
					bBufferValid = true;
				}
				else
				{
					return;
				}
			}
			mesh->bindVAO();


			mesh->unbindVAO();
		}

		void setupShader(ShaderProgram& program);
		std::vector< Matrix4 > mInstanceTransforms;
		std::vector< Vector4 > mInstanceParams;
		RHIVertexBufferRef mInstancedBuffer;
		bool bBufferValid = false;
	};


	class EnvTech
	{
	public:

		bool init()
		{
			if( !mBuffer.create() )
				return false;

			mTexEnv = RHICreateTextureCube();
			if( !mTexEnv->create(Texture::eFloatRGBA, MapSize, MapSize) )
				return false;
#if USE_DepthRenderBuffer
			RHIDepthRenderBufferRef depthBuffer = new RHIDepthRenderBuffer;
			if( !depthBuffer->create(MapSize, MapSize, Texture::eDepth24) )
				return false;
#endif

			mBuffer.addTexture(*mTexEnv, Texture::eFaceX);
#if USE_DepthRenderBuffer
			mBuffer.setDepth(*depthBuffer);
#endif
			return true;

		}
		static int const MapSize = 512;

		OpenGLFrameBuffer mBuffer;

		RHITextureCubeRef mTexSky;
		RHITextureCubeRef mTexEnv;
	};


	class WaterTech
	{
	public:
		static int const MapSize = 512;
		bool init();

		void render(ViewInfo& view, SceneInterface& scene)
		{
			NormalizePlane(waterPlane);

			ViewportSaveScope vpScope;
			RHISetViewport(0, 0, MapSize, MapSize);
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
		}

		Vector4      waterPlane;

		Mesh         mWaterMesh;
		RHITexture2DRef mReflectMap;
		RHITexture2DRef mRefractMap;
		OpenGLFrameBuffer  mBuffer;
	};

	struct Portal
	{
		Matrix4 transform;
		Vector2 size;
	};


	class SampleStage : public StageBase
		              , public SceneInterface
		              , public IAssetProvider
		              , public SceneListener
	{
		typedef StageBase BaseClass;
		
	public:
		SampleStage();
		~SampleStage();

		virtual bool onInit();
		virtual void onInitFail();
		virtual void onEnd();

		void testCode();
		bool loadAssetResouse();
		void setupScene();
		void unregisterAllAsset();

		virtual void onUpdate( long time )
		{
			BaseClass::onUpdate( time );

			int frame = time / gDefaultTickTime;
			for( int i = 0 ; i < frame ; ++i )
				tick();
			updateFrame( frame );
		}

		void onRender( float dFrame );

		void render_SpherePlane( ViewInfo& view );
		void render_ParallaxMapping( ViewInfo& view );
		void renderTest2( ViewInfo& view );
		void render_RaycastTest( ViewInfo& view );
		void render_DeferredLighting( ViewInfo& view );
		void render_Sprite( ViewInfo& view );
		void render_OIT( ViewInfo& view );
		void render_Terrain( ViewInfo& view );
		void render_Portal(ViewInfo& view);
		void render_ShadowVolume(ViewInfo& view);

		void renderScene(RenderContext& param);

		void showLight(ViewInfo& view);
		
		void buildScene1(Scene& scene);

		void restart()
		{

		}


		void tick();

		void updateFrame( int frame )
		{

		}

		bool onMouse( MouseMsg const& msg );

		bool onKey( unsigned key , bool isDown );
		
		enum
		{
			UI_SAMPLE_TEST = BaseClass::NEXT_UI_ID,
		};
		bool onWidgetEvent(int event, int id, GWidget* ui);

		void reloadMaterials();

		void drawAxis(float len);
		void drawSky();

		bool createFrustum( Mesh& mesh , Matrix4 const& matProj );


		void calcViewRay( float x , float y );

		virtual void render( RenderContext& context) override;
		virtual void renderTranslucent( RenderContext& context) override;
		
		template< class Fun >
		void visitLights( Fun& fun )
		{
			for( int i = 0; i < mNumLightDraw; ++i )
			{
				LightInfo const& light = mLights[i];
				fun( i , light);
			}
			for( int i = 0; i < getScene().lights.size(); ++i )
			{
				LightInfo const& light = getScene().lights[i]->info;
				fun(i, light);
			}
		}


		virtual void onRemoveLight(SceneLight* light) override;


		virtual void onRemoveObject(SceneObject* object) override;

	protected:
		GLGpuSync   mGpuSync;
		void prevLoading()
		{
			while( !mGpuSync.pervLoading() )
			{
				SystemPlatform::Sleep(1);
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

		ShaderProgram mProgSimpleSprite;

		ShaderProgram mProgTerrain;

		class ShadowVolumeProgram* mProgShadowVolume;


		struct SimpleMeshId
		{
			enum
			{
				Tile,
				Sphere,
				Sphere2,
				SpherePlane,
				Box,
				Plane,
				Doughnut,
				SkyBox ,
				SimpleSkin ,
				Terrain ,
				NumSimpleMesh,
			};
		};


		Mesh   mSimpleMeshs[ SimpleMeshId::NumSimpleMesh ];

		Mesh   mFrustumMesh;
		Mesh   mSpritePosMesh;
		RHITextureCubeRef mTexSky;


		std::vector< std::unique_ptr< IValueModifier > > mValueModifiers;

		virtual CycleTrack* addCycleTrack(Vector3& value) final
		{
			auto modifier = std::make_unique< TVectorTrackModifier< CycleTrack > >(value);
			CycleTrack* result = &modifier->track;
			mValueModifiers.push_back(std::move(modifier));
			return result;
		}
		virtual VectorCurveTrack* addCurveTrack(Vector3& value) final 
		{ 
			auto modifier = std::make_unique< TVectorTrackModifier< VectorCurveTrack > >(value);
			VectorCurveTrack* result = &modifier->track;
			mValueModifiers.push_back(std::move(modifier));
			return result;
		}

		Thread* mLoadingThread = nullptr;

		int  mNumLightDraw = 4;
		std::vector< LightInfo > mLights;
		CycleTrack mTracks[4];

		Vector3 mPos;
		Render::SimpleCamera  mCamStorage[2];
		CameraMove  mCameraMove;
		ViewFrustum mViewFrustum;

		
		typedef std::unique_ptr< SceneAsset > SceneAssetPtr;
		std::vector< SceneAssetPtr > mSceneAssets;
		Scene& getScene(int idx = 0) { return mSceneAssets[idx]->scene; }
		Scene& addScene(char const* name = nullptr )
		{
			auto sceneAsset = std::make_unique< SceneAsset >();
			sceneAsset->scene.mListener = this;
			sceneAsset->setupDelegate = [this](Scene& scene)
			{
				auto debugObject = new DebugGeometryObject;
				scene.addObject(debugObject);
				mDebugGeometryObject = debugObject;
			};
			if( name )
			{
				sceneAsset->load(name);
				::Global::GetAssetManager().registerViewer(mSceneAssets.back().get());
			}	
			mSceneAssets.push_back(std::move(sceneAsset));
			return mSceneAssets.back()->scene;
		}
		Scene mScene;

		bool bUseFrustumTest = true;

		static int const NumAabb = 3;
		AABB    mAabb[3];
		bool    mIsIntersected;
		Vector3 mIntersectPos;
		Vector3 rayStart , rayEnd;

		OpenGLFrameBuffer mLayerFrameBuffer;

		std::vector< MaterialAsset > mMaterialAssets;
		typedef std::shared_ptr< RHITexture2D > Texture2DPtr;
		std::vector< Texture2D > mTextures; 
		std::vector< TLazyObjectGuid< StaticMesh > > mMeshs;
		Texture2D mEmptyTeture;
		virtual TLazyObjectGuid< Material >  getMaterial(int idx) final
		{ 
			if( mMaterialAssets.empty() )
				return TLazyObjectGuid<Material>();
			return mMaterialAssets[idx].materialId;
		}
		virtual Texture2D& getTexture(int idx) final
		{ 
			if ( mTextures.empty() )
				return mEmptyTeture;
			return mTextures[idx];
		}
		virtual TLazyObjectGuid< StaticMesh > getMesh(int idx)  final
		{ 
			if( mMeshs.empty() )
				return TLazyObjectGuid<StaticMesh>();
			return mMeshs[idx];
		}
		virtual Mesh& getSimpleMesh(int idx) final
		{
			return mSimpleMeshs[idx];
		}
		virtual DebugGeometryObject* getDebugGeometryObject() final
		{ 
			return mDebugGeometryObject;
		}

		DebugGeometryObject* mDebugGeometryObject = nullptr;

		bool  mLineMode;

		SceneRenderTargets    mSceneRenderTargets;
		
		DeferredShadingTech    mTechDeferredShading;
		ShadowDepthTech        mTechShadow;
		OITTechnique           mTechOIT;
		VolumetricLightingTech mTechVolumetricLighing;

		PostProcessSSAO        mSSAO;
		PostProcessDOF         mDOF;

		bool   mbShowBuffer;

		int    renderLightCount = 0;
		

		StaticMesh  mEmptyMesh;
		
		RHITexture2DRef mTexBase;
		RHITexture2DRef mTexNormal;


		RHITexture2DRef mTexTerrainHeight;

		bool   mPause;
		float  mTime;
		float  mRealTime;

		struct SampleTest
		{
			char const* name;
			std::function< void (ViewInfo&) > renderFun;
		};
		std::vector< SampleTest > mSampleTests;
		int    mIdxTestChioce;
		bool bInitialized = false;

	};

}//namespace Render


#endif // RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
