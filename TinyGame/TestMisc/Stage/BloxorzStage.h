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
#include "RHI/MeshUtility.h"
#include "RHI/SimpleRenderState.h"
#include "RHI/RenderContext.h"
#include "RHI/DrawUtility.h"
#include "RHI/SceneRenderer.h"
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


	class DownsampleProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(DownsampleProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(DownsamplePS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, Texture);
			BIND_SHADER_PARAM(parameterMap, ExtentInverse);
		}

		DEFINE_SHADER_PARAM(ExtentInverse);
		DEFINE_TEXTURE_PARAM(Texture);
	};


	class BloomSetupProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(BloomSetupProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{

		}
		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BloomSetupPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, Texture);
			BIND_SHADER_PARAM(parameterMap, BloomThreshold);
			//BIND_SHADER_PARAM(parameterMap, BloomIntensity);
		}

		DEFINE_TEXTURE_PARAM(Texture);
		DEFINE_SHADER_PARAM(BloomThreshold);
		//DEFINE_SHADER_PARAM(BloomIntensity);
	};

	class FliterProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(FliterProgram , Global)
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{
		
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Bloom";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(FliterPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, FliterTexture);
		}

		DEFINE_TEXTURE_PARAM(FliterTexture);
	};

	class FliterAddProgram : public FliterProgram
	{
		using BaseClass = FliterProgram;
	public:

		DECLARE_SHADER_PROGRAM(FliterAddProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(APPLY_ADD_TEXTURE), true);
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BaseClass::bindParameters(parameterMap);
			BIND_TEXTURE_PARAM(parameterMap, AddTexture);
		}

		DEFINE_TEXTURE_PARAM(AddTexture);
	};

	class TonemapProgram : public GlobalShaderProgram
	{
	public:
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(TonemapProgram, Global);
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_BLOOM), true);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/Tonemap";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BIND_TEXTURE_PARAM(parameterMap, TextureInput0);
			BIND_TEXTURE_PARAM(parameterMap, BloomTexture);
		}

		DEFINE_TEXTURE_PARAM(TextureInput0);
		DEFINE_TEXTURE_PARAM(BloomTexture);
	};


	struct RenderTargetDesc
	{
		HashString debugName;
		IntVector2 size;
		Texture::Format format;
		uint32 creationFlags;

		bool isMatch(RenderTargetDesc const& rhs) const
		{
			return size == rhs.size &&
				   format  == rhs.format &&
				   ( creationFlags | TCF_RenderTarget) == ( rhs.creationFlags | TCF_RenderTarget);
		}
	};

	struct PooledRenderTarget : public RefCountedObjectT< PooledRenderTarget >
	{
		RenderTargetDesc desc;
		RHITexture2DRef  texture;
	};

	typedef TRefCountPtr< PooledRenderTarget > PooledRenderTargetRef;


	class RenderTargetPool
	{
	public:

		PooledRenderTargetRef fetchElement(RenderTargetDesc const& desc)
		{
			for (auto iter = mFreeRTs.begin(); iter != mFreeRTs.end() ; ++iter )
			{
				auto& rt = *iter;
				if (rt->desc.isMatch(desc))
				{
					rt->desc.debugName = desc.debugName;
					mUsedRTs.push_back(rt);
#if 0	
					if (&rt != &mFreeRTs.back())
					{
						rt = mFreeRTs.back();
						mFreeRTs.pop_back();
					}
					
#else
					mFreeRTs.erase(iter);
#endif
					return mUsedRTs.back();
				}
			}
			PooledRenderTarget* result = new PooledRenderTarget;
			result->desc = desc;
			result->desc.creationFlags |= TCF_RenderTarget;
			result->texture = RHICreateTexture2D(desc.format, desc.size.x, desc.size.y, 0, 1, desc.creationFlags | TCF_RenderTarget );
			mUsedRTs.push_back(result);
			return result;
		}


		void freeUsedElement(PooledRenderTargetRef const& freeRT)
		{
			for (auto iter = mUsedRTs.begin(); iter != mUsedRTs.end(); ++iter)
			{
				auto& rt = *iter;
				if (rt == freeRT )
				{
					if (&rt != &mUsedRTs.back())
					{
						rt = mUsedRTs.back();
						mUsedRTs.pop_back();
					}
				}
			}
		}

		void freeAllUsedElements()
		{
			mFreeRTs.insert(mFreeRTs.end(), mUsedRTs.begin(), mUsedRTs.end());
			mUsedRTs.clear();
			for (auto iter = mFreeRTs.begin(); iter != mFreeRTs.end(); ++iter)
			{
				iter->get()->desc.debugName == EName::None;
			}
		}


		void releaseRHI()
		{
			mFreeRTs.clear();
			mUsedRTs.clear();
		}

		std::vector< PooledRenderTargetRef > mFreeRTs;
		std::vector< PooledRenderTargetRef > mUsedRTs;
	};



	int generateGaussianlDisburtionWeightAndOffset( float kernelRadius,  Vector2 outWeightAndOffset[128] )
	{
		float fliterScale = 1.0f;
		float filterScale = 1.0f;
		float sigmaScale = 0.2;

		int   sampleRadius = Math::CeilToInt(fliterScale * kernelRadius);
		sampleRadius = Math::Clamp(sampleRadius, 1 , 128 - 1);

		float sigma = kernelRadius * sigmaScale;
		auto GetWeight = [=](float x)
		{
			return Math::Exp(-Math::Squre(x / sigma));
		};

		int numSamples = 0;
		float totalSampleWeight = 0;
		for (int sampleIndex = -sampleRadius; sampleIndex <= sampleRadius; sampleIndex += 2)
		{
			float weightA = GetWeight(sampleIndex);


			float weightB = 0;
			
			if (sampleIndex != sampleRadius)
			{
				weightB = GetWeight(sampleIndex + 1);
			}
			float weightTotal = weightA + weightB;
			float offset = Math::Lerp(sampleIndex, sampleIndex + 1, weightB / weightTotal);
			
			outWeightAndOffset[numSamples].x = weightTotal;
			outWeightAndOffset[numSamples].y = offset;
			totalSampleWeight += weightTotal;
			++numSamples;
		}


		float invTotalSampleWeight = 1.0 / totalSampleWeight;
		for (int i = 0; i < numSamples; ++i)
		{
			outWeightAndOffset[i].x *= invTotalSampleWeight;
		}
		return numSamples;
	}

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

		class RayTraceProgram* mProgRayTrace;
		class RayTraceProgram* mProgRayTraceBuiltin;
		class RayTraceProgram* mProgRayTraceDeferred;
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
			Vector2 weightAndOffset[128];
			int numSamples = generateGaussianlDisburtionWeightAndOffset(bloomRadius, weightAndOffset);

			mWeightData.resize(128);
			mUVOffsetData.resize(128);

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
