#pragma once
#ifndef RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
#define RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080

#include "Stage/TestRenderStageBase.h"

#include "HashString.h"
#include "SystemPlatform.h"
#include "Asset.h"

#include "MarcoCommon.h"
#include "Math/TVector3.h"
#include "FastDelegate/FastDelegate.h"
#include "RandomUtility.h"

#include "RHI/ShaderCore.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/Material.h"
#include "RHI/Scene.h"
#include "RHI/SceneObject.h"
#include "RHI/SceneRenderer.h"
#include "RHI/LazyObject.h"


#include "RHI/OpenGLCommon.h"

#include "Renderer/SimpleCamera.h"

#include "SceneScript.h"

#include <limits>
#include <memory>
#include <typeindex>
#include "PlatformThread.h"
#include "Math/GeometryPrimitive.h"
#include "Math/Curve.h"


class Thread;

namespace Render
{
	float const FLT_DIV_ZERO_EPSILON = 1e-6f;

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
		virtual void getDependentFilePaths(TArray<std::wstring>& paths) override
		{
			if ( material )
			{
				std::string cPath = MaterialShaderMap::GetFilePath(material->mName.c_str());
				paths.push_back(FCString::CharToWChar(cPath.c_str()));
			}
		}
		virtual void postFileModify(EFileAction action) override
		{
			if ( action == EFileAction::Modify )
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
		virtual void getDependentFilePaths(TArray<std::wstring>& paths) override
		{
			std::string cPath = ISceneScript::GetFilePath( mName.c_str() );
			paths.push_back(FCString::CharToWChar(cPath.c_str()));
		}

		virtual void postFileModify(EFileAction action) override
		{
			if( action == EFileAction::Modify )
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

	using AABBox = Math::TAABBox< Vector3 >;

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
			float const esp = 1e-5f;
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
				factor[i] = std::numeric_limits<float>::lowest();
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

	struct AABB
	{
		Vector3 min, max;
	};

	class EnvTech
	{
	public:

		bool init()
		{
			VERIFY_RETURN_FALSE(mFrameBuffer = RHICreateFrameBuffer());

			mTexEnv = RHICreateTextureCube(ETexture::FloatRGBA, MapSize);
#if USE_DepthRenderBuffer
			RHIDepthRenderBufferRef depthBuffer = new RHIDepthRenderBuffer;
			if( !depthBuffer->create(MapSize, MapSize, ETexture::Depth24) )
				return false;
#endif

			mFrameBuffer->addTexture(*mTexEnv, ETexture::FaceX);
#if USE_DepthRenderBuffer
			mFrameBuffer->setDepth(*depthBuffer);
#endif
			return true;

		}
		static int const MapSize = 512;

		RHIFrameBufferRef  mFrameBuffer;

		RHITextureCubeRef mTexSky;
		RHITextureCubeRef mTexEnv;
	};


	class WaterTech
	{
	public:
		static int const MapSize = 512;
		bool init();

		void render(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene)
		{
			NormalizePlane(waterPlane);

			ViewportSaveScope vpScope(commandList);
			RHISetViewport(commandList, 0, 0, MapSize, MapSize);
			glPushMatrix();
			ReflectMatrix matReflect(waterPlane.xyz(), waterPlane.w);
			glMultMatrixf(matReflect);
			double equ[] = { waterPlane.x , waterPlane.y , waterPlane.z , waterPlane.w };
			glEnable(GL_CLIP_PLANE0);
			glClipPlane(GL_CLIP_PLANE0, equ);

			RHISetFrameBuffer(commandList, mFrameBuffer);
			//scene.render(view, *this);

			glDisable(GL_CLIP_PLANE0);
			glPopMatrix();
		}

		Vector4      waterPlane;

		Mesh         mWaterMesh;
		RHITexture2DRef mReflectMap;
		RHITexture2DRef mRefractMap;
		RHIFrameBufferRef  mFrameBuffer;
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
		              , public SharedAssetData
		              , public IGameRenderSetup
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

		virtual void onUpdate(GameTimeSpan deltaTime);

		void onRender( float dFrame );

		void render_SpherePlane(RHICommandList& commandList, ViewInfo& view );
		void render_ParallaxMapping(RHICommandList& commandList, ViewInfo& view );
		void renderTest2(RHICommandList& commandList, ViewInfo& view );
		void render_RaycastTest(RHICommandList& commandList, ViewInfo& view );
		void render_DeferredLighting(RHICommandList& commandList, ViewInfo& view );
		void render_Sprite(RHICommandList& commandList, ViewInfo& view );
		void render_OIT(RHICommandList& commandList, ViewInfo& view );
		void render_Terrain(RHICommandList& commandList, ViewInfo& view );
		void render_Portal(RHICommandList& commandList, ViewInfo& view);
		void render_ShadowVolume(RHICommandList& commandList, ViewInfo& view);

		void renderScene(RenderContext& param);

		void showLights(RHICommandList& commandList, ViewInfo& view);
		
		void buildScene1(Scene& scene);

		void restart()
		{

		}


		MsgReply onMouse(MouseMsg const& msg);
		MsgReply onKey(KeyMsg const& msg);
		
		enum
		{
			UI_SAMPLE_TEST = BaseClass::NEXT_UI_ID,
		};
		bool onWidgetEvent(int event, int id, GWidget* ui);

		void reloadMaterials();

		void drawAxis(RHICommandList& commandList, float len);
		void drawSky(RHICommandList& commandList);

		bool createFrustum( Mesh& mesh , Matrix4 const& matProj );


		void calcViewRay( float x , float y );

		//SceneInterface
		virtual void render(RenderContext& context) override;
		virtual void renderTranslucent(RenderContext& context) override;
		
		template< class TFunc >
		void visitLights(TFunc& func )
		{
			for( int i = 0; i < mNumLightDraw; ++i )
			{
				LightInfo const& light = mLights[i];
				func( i , light);
			}
			for( int i = 0; i < getScene().lights.size(); ++i )
			{
				LightInfo const& light = getScene().lights[i]->info;
				func(i, light);
			}
		}


		virtual void onRemoveLight(SceneLight* light) override;


		virtual void onRemoveObject(SceneObject* object) override;

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs)
		{
			systemConfigs.screenWidth = 1280;
			systemConfigs.screenHeight = 720;
		}

		ERenderSystem getDefaultRenderSystem() override;


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
		ShaderProgram mProgBump;
		ShaderProgram mProgParallax;
		ShaderProgram mEffectSimple;

		ShaderProgram mProgSimpleSprite;

		ShaderProgram mProgTerrain;

		class ScreenVS* mScreenVS;

		class ShadowVolumeProgram* mProgShadowVolume;
		class DecalRenderProgram*  mProgDecal;
		class DecalRenderPS*       mDecalRenderPS;
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
		SimpleCamera  mCamStorage[2];
		CameraMove  mCameraMove;
		ViewFrustum mViewFrustum;
		ViewInfo mCameraView;
		
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

		RHIFrameBufferRef mLayerFrameBuffer;

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

		FrameRenderTargets    mSceneRenderTargets;
		
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
			std::function< void (RHICommandList&, ViewInfo&) > renderFun;
		};
		std::vector< SampleTest > mSampleTests;
		int    mIdxTestChioce;
		bool bInitialized = false;



		Mutex mMutexGameThreadCommonds;
		std::vector< std::function< void() > > mGameThreadCommonds;

		template< class TFunc >
		void addGameCommand(TFunc func)
		{
			Mutex::Locker locker(mMutexGameThreadCommonds);
			mGameThreadCommonds.push_back(func);
		}

		void executeGameCommand()
		{
			assert(IsInGameThead());
			Mutex::Locker locker(mMutexGameThreadCommonds);
			for (auto& com : mGameThreadCommonds)
			{
				com();
			}
			mGameThreadCommonds.clear();
		}


	};

}//namespace Render


#endif // RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
