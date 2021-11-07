#ifndef BloxorzStage_h__
#define BloxorzStage_h__

#include "StageBase.h"

#include "Math/TVector3.h"
#include "Math/TVector2.h"
#include "Core/IntegerType.h"

#include "DataStructure/Grid2D.h"
#include "Tween.h"
#include "HashString.h"

#include "GameRenderSetup.h"
#include "RHI/SimpleRenderState.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/SceneRenderer.h"

#include "Renderer/RenderTargetPool.h"
#include "Renderer/Bloom.h"

#include "TestRender/Stage/TestRenderStageBase.h"



namespace Bloxorz
{
	using namespace Render;

	typedef Render::IntVector2 Vec2i;
	typedef Render::IntVector3 Vec3i;

	enum Dir
	{
		DIR_X  = 0 ,
		DIR_Y  = 1 ,
		DIR_NX = 2 ,
		DIR_NY = 3 ,
		DIR_NONE = 0xff,
	};
	inline bool isNegative( Dir dir ){ return dir >= DIR_NX; }
	inline Dir  inverseDir( Dir dir ){ return Dir( (dir + 2 ) % 4 );}

	class Object
	{
	public:

		Object();
		Object( Object const& rhs );

		void addBody( Vec3i const& pos );
		void move( Dir dir );
		int  getMaxLocalPosX() const;
		int  getMaxLocalPosY() const;
		int  getMaxLocalPosZ() const;
		int  getBodyNum() const { return mNumBodies; }
		Vec3i const& getBodyLocalPos(int idx) const { return mBodiesPos[idx]; }

		Vec3i const& getPos() const { return mPos; }
		void         setPos( Vec3i const& val ) { mPos = val; }
		
		static int const MaxBodyNum = 32;


		Vec3i mPos;

		int   mNumBodies;
		Vec3i mBodiesPos[ MaxBodyNum ];

	};

	struct GPU_ALIGN ObjectData
	{
		DECLARE_BUFFER_STRUCT(ObjectDataBlock, Objects);

		Matrix4 worldToLocal;
		Vector4 typeParams;
		int     Type;
		int     MatID;
		int     dummy[2];
	};
	struct GPU_ALIGN MaterialData
	{
		DECLARE_BUFFER_STRUCT(MaterialDataBlock, Materials);

		Color3f baseColor;
		float   shininess;
		Color3f emissiveColor;
		float   dummy;
	};

	struct GPU_ALIGN MapTileData
	{
		DECLARE_BUFFER_STRUCT(MapTileDataBlock, MapTiles);

		Vector4 posAndSize;
	};

	struct GPU_ALIGN SceneEnvData
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(SceneEnvDataBlock)
		Vector3 sunDirection;
		float   sunIntensity;
		Vector3 fogColor;
		float   fogDistance;

		SceneEnvData()
		{
			sunDirection = Math::GetNormal(Vector3(-0.5, -0.6, 0.2));
			sunIntensity = 1.0f;
			fogColor = Vector3(0.7, 0.7, 0.9);
			fogDistance = 100;
		}
	};


	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		typedef StageBase BaseClass;
	public:
		TestStage(){}

		virtual bool onInit();
		virtual void onEnd();
		virtual void onUpdate( long time );
		void onRender( float dFrame );

		void restart();
		void tick();
		
		void requestMove( Dir dir );
		void moveObject();

		void updateFrame( int frame );

		bool onMouse( MouseMsg const& msg );

		bool onKey(KeyMsg const& msg);

		void drawObjectBody( Vector3 const& color );

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}
		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;
		virtual bool setupRenderSystem(ERenderSystem systemName) override;

		void UpdateSceneEnvBuffer();

		virtual void preShutdownRenderSystem(bool bReInit = false) override;
		
	protected:
		enum State
		{
			eGoal,
			eFall,
			eMove,
			eBlock ,
		};
		State checkObjectState( Object const& object , uint8& holeDirMask );

		Dir    mMoveCur;
		Dir    mMoveStack;
		
		Object   mObject;
		Vector3  mLookPos;
		Vector3  mCamRefPos;
		TGrid2D< int > mMap;

		float  blurRadiusScale = 1.0;
		bool   mIsGoal;
		bool   mbEditMode;

		bool bUseRayTrace = true;
		bool bUseSceneBuitin = true;
		bool bUseDeferredRending = false;
		bool bFreeView = true;
		bool bMoveCamera = true;
		bool bUseBloom = true;

		bool canInput()
		{
			if (bFreeView)
			{
				return !bMoveCamera;
			}
			return true;
		}
		SimpleCamera mCamera;
		TransformStack mStack;
		
		int mNumMapTile;
		std::vector< ObjectData > mObjectList;
		SceneEnvData mSceneEnv;

		class RayTraceLightingProgram* mProgRayTraceLighting;
		TStructuredBuffer< ObjectData >   mObjectBuffer;
		TStructuredBuffer< MaterialData > mMaterialBuffer;
		TStructuredBuffer< MapTileData >  mMapTileBuffer;
		TStructuredBuffer< SceneEnvData > mSceneEnvBuffer;
		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef mFrameBuffer;

		RenderTargetPool mRenderTargetPool;

		

		RHIFrameBufferRef mBloomFrameBuffer;
		RHITexture2DRef mGirdTexture;


		std::vector< Vector4 > mWeightData;
		std::vector< Vector2 > mUVOffsetData;
		float mBloomThreshold = -1.0;
		float mBloomIntensity = 1.0;

		int generateFliterData(int imageSize , Vector2 const& offsetDir , LinearColor const& bloomTint, float bloomRadius)
		{
			Vector2 weightAndOffset[MaxWeightNum];
			int numSamples = generateGaussianlDisburtionWeightAndOffset(bloomRadius, weightAndOffset);

			mWeightData.resize(MaxWeightNum);
			mUVOffsetData.resize(MaxWeightNum);

			for (int i = 0; i < numSamples; ++i)
			{
				mWeightData[i] = weightAndOffset[i].x * Vector4( bloomTint );
				mUVOffsetData[i] = ( weightAndOffset[i].y / imageSize ) * offsetDir;
			}

			return numSamples;
		}

		FliterProgram* mProgFliter;
		FliterAddProgram* mProgFliterAdd;
		DownsampleProgram* mProgDownsample;
		BloomSetupProgram* mProgBloomSetup;
		TonemapProgram*    mProgTonemap;
		ViewInfo mView;
		Mesh     mCube;

		float    mRotateAngle;
		Vector3  mRotateAxis;
		Vector3  mRotatePos;
		float    mSpot;

		typedef std::vector< Object* > ObjectVec;
		ObjectVec mObjects;
		typedef Tween::GroupTweener< float > Tweener;
		Tweener   mTweener;

		void updateRenderTargetShow()
		{
			for (auto& RT : mRenderTargetPool.mUsedRTs)
			{
				if (RT->desc.debugName != EName::None)
				{
					mTextureShowManager.registerTexture(RT->desc.debugName, RT->texture);
				}
			}
		}

		TextureShowManager mTextureShowManager;

	};



}//namespace Bloxorz

#endif // BloxorzStage_h__
