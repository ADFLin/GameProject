#pragma once
#ifndef SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
#define SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A

#include "RHICommon.h"
#include "ShaderCore.h"
#include "Material.h"
#include "RenderContext.h"

namespace RenderGL
{

	class FrameRenderNode
	{
	public:





	};
	class RenderContext;
	struct LightInfo;

	class SceneRenderer
	{
	public:




		SceneInterface* mRenderScene;
	};

	struct GBufferParamData
	{
		enum BufferId
		{
			BufferA, //xyz : WorldPos
			BufferB, //xyz : Noraml
			BufferC, //xyz : BaseColor
			BufferD,

			NumBuffer,
		};

		RHITexture2DRef textures[NumBuffer];
		RHITextureDepthRef depthTexture;

		bool init(Vec2i const& size);
		void setupShader(ShaderProgram& program);

		void drawTextures( Vec2i const& size, Vec2i const& gapSize);
		void drawTexture(int x, int y, int width, int height, int idxBuffer);
		void drawTexture(int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask);
	};


	class SceneRenderTargets
	{
	public:
		bool init(Vec2i const& size);

		RHITexture2D&  getRenderFrameTexture() { return *mFrameTextures[mIdxRenderFrameTexture]; }
		RHITexture2D&  getPrevRenderFrameTexture() { return *mFrameTextures[1 - mIdxRenderFrameTexture]; }
		void swapFrameBufferTexture()
		{
			mIdxRenderFrameTexture = 1 - mIdxRenderFrameTexture;
			mFrameBuffer.setTexture(0, getRenderFrameTexture());
		}

		GBufferParamData&   getGBuffer() { return mGBuffer; }
		RHITextureDepth&    getDepthTexture() { return *mDepthTexture; }

		FrameBuffer& getFrameBuffer() { return mFrameBuffer; }

		void drawDepthTexture(int x, int y, int width, int height);

		GBufferParamData mGBuffer;
		RHITexture2DRef  mFrameTextures[2];
		int              mIdxRenderFrameTexture;
		FrameBuffer      mFrameBuffer;
		RHITextureDepthRef mDepthTexture;
	};


	class GBufferShaderParameters
	{
	public:
		void bindParameters(ShaderProgram& program, bool bUseDepth = false);

		void setParameters(ShaderProgram& program, GBufferParamData& GBufferData);
		void setParameters(ShaderProgram& program, SceneRenderTargets& sceneRenderTargets);

		ShaderParameter mParamGBufferTextureA;
		ShaderParameter mParamGBufferTextureB;
		ShaderParameter mParamGBufferTextureC;
		ShaderParameter mParamGBufferTextureD;

		ShaderParameter mParamFrameDepthTexture;
	};

	struct ShadowProjectParam
	{
		static int const MaxCascadeNum = 8;
		LightInfo const*   light;
		RHITextureBase* shadowTexture;
		Matrix4      shadowMatrix[8];
		Vector3      shadowParam;

		int          numCascade;
		float        cacadeDepth[MaxCascadeNum];

		void setupLight(LightInfo const& inLight);

		void setupShader(ShaderProgram& program) const;
	};



	class ShadowProjectShaderParameters
	{



	};

	class ForwardShadingTech : public RenderTechnique
	{



	};

#define USE_MATERIAL_SHADOW 1
	class ShadowDepthTech : public RenderTechnique
	{
	public:
		static int const ShadowTextureSize = 2048;
		static int const CascadeTextureSize = 2028;
		static int const CascadedShadowNum = 4;

		ShadowDepthTech()
		{
			mCascadeMaxDist = 40;
		}
		bool init();
		void renderLighting(ViewInfo& view, SceneInterface& scene, LightInfo const& light, bool bMultiple);
		void renderShadowDepth(ViewInfo& view, SceneInterface& scene, ShadowProjectParam& shadowProjectParam);
		void calcCascadeShadowProjectInfo(ViewInfo &view, LightInfo const &light, float depthParam[2], Matrix4 &worldToLight, Matrix4 &shadowProject);

		static void determineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[]);

		void drawShadowTexture(LightType type, Vec2i const& pos, int length);
		void reload();

		//RenderTechnique
		virtual void setupWorld(RenderContext& context , Matrix4 const& mat) override
		{
			RenderTechnique::setupWorld( context , mat);
			if( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam(SHADER_PARAM(Primitive.localToWorld), mat);
			}
		}

		virtual MaterialShaderProgram* getMaterialShader(RenderContext& context,  MaterialMaster& material , VertexFactory* vertexFactory ) override
		{
#if USE_MATERIAL_SHADOW
			if( mEffectCur == &mProgLighting )
				return nullptr;
			return material.getShader(RenderTechiqueUsage::Shadow, vertexFactory);

#else
			return nullptr;
#endif
		}

		virtual void setupMaterialShader(RenderContext& context, ShaderProgram& shader) override
		{
			shader.setParam(SHADER_PARAM(DepthShadowMatrix), mShadowMatrix);
			shader.setParam(SHADER_PARAM(ShadowParam), shadowParam.x, shadowParam.y);
		}

		float         depthParam[2];
		Vector3       shadowParam;

		float         mCascadeMaxDist;

		ShaderProgram* mEffectCur;

		Mesh          mCubeMesh;
		ShaderProgram  mEffectCube;

		RHITexture2DRef    mShadowMap2;
		RHITextureCubeRef  mShadowMap;
		RHITexture2DRef    mCascadeTexture;
		FrameBuffer     mShadowBuffer;

		ShaderProgram  mProgShadowDepth[3];
		ShaderProgram  mProgLighting;

		Matrix4       mShadowMatrix;

		RHIDepthRenderBufferRef depthBuffer1;
		RHIDepthRenderBufferRef depthBuffer2;
	};


	class PostProcess
	{




	};


	class PostProcessSSAO : public PostProcess
	{
	public:
		static int const NumDefaultKernel = 16;

		bool init(Vec2i const& size);

		void render(ViewInfo& view, SceneRenderTargets& sceneRenderTargets);
		void drawSSAOTexture( Vec2i const& pos , Vec2i const& size );

		void reload();
	private:

		float Random(float v0, float v1)
		{
			return v0 + (v1 - v0) * (float(::rand()) / RAND_MAX);
		}

		void generateKernelVectors(int numKernal)
		{
			mKernelVectors.resize(numKernal);
			for( int i = 0; i < numKernal; ++i )
			{
				mKernelVectors[i] = Vector3(Random(-1, 1), Random(-1, 1), Random(0, 1));
				//mKernelVectors[i] = Vector3(0,0,1);
				mKernelVectors[i].normalize();
				float scale = float(i) / numKernal;
				scale = Math::Lerp(0.1, 1, scale * scale);
				mKernelVectors[i] *= scale;
			}
		}

	public:
		std::vector< Vector3 > mKernelVectors;
		FrameBuffer  mFrameBuffer;
		RHITexture2DRef mSSAOTextureBlur;
		RHITexture2DRef mSSAOTexture;
		class SSAOGenerateProgram* mProgSSAOGenerate;
		class SSAOBlurProgram*     mProgSSAOBlur;
		class SSAOAmbientProgram*  mProgAmbient;

	};

	class PostProcessDOF : public PostProcess
	{
	public:
		bool init(Vec2i const& size);
		void render(ViewInfo& view, SceneRenderTargets& sceneRenderTargets);

		FrameBuffer mFrameBufferGen;
		RHITexture2DRef mTextureNear;
		RHITexture2DRef mTextureFar;

		FrameBuffer mFrameBufferBlur;
		RHITexture2DRef mTextureBlurR;
		RHITexture2DRef mTextureBlurG;
		RHITexture2DRef mTextureBlurB;


		class DOFGenerateCoC*  mProgGenCoc;
		class DOFBlurVProgram* mProgBlurV;
		class DOFBlurHAndCombineProgram* mProgBlurHAndCombine;
	};

	inline float NormalizePlane(Vector4& plane)
	{
		float len2 = plane.xyz().dot(plane.xyz());
		float invSqrt = Math::InvSqrt(len2);
		plane *= invSqrt;
		return invSqrt;
	}

	enum LightBoundMethod
	{
		LBM_SCREEN_RECT,
		LBM_GEMO_BOUND_SHAPE,
		LBM_GEMO_BOUND_SHAPE_WITH_STENCIL,

		NumLightBoundMethod,
	};

	class DefferredLightingProgram : public ShaderProgram
	{
	public:
		void bindParameters()
		{
			mParamGBuffer.bindParameters(*this , true);
		}
		void setParamters(SceneRenderTargets& sceneRenderTargets)
		{
			mParamGBuffer.setParameters(*this, sceneRenderTargets);
		}
		GBufferShaderParameters mParamGBuffer;
	};

	struct TitledLightInfo
	{
		DECLARE_UNIFORM_STRUCT(TitledLightBlock);

		Vector3 pos;
		int32   type; 
		Vector3 color;
		float   intensity;
		Vector3 dir;
		float   radius;
		Vector4 param; // x y: spotAngle  , z : shadowFactor
		Matrix4 worldToShadow;
	};


	struct SceneLightPrimtive
	{
		LightInfo const* light;
		RHITextureRef shadowMap;
	};

	class VolumetricLightingTech : public RenderTechnique
	{	

	public:

		bool init(Vec2i const& screenSize);

		void releaseRHI()
		{
			mVolumeBufferA.release();
			mVolumeBufferB.release();
			mScatteringBuffer.release();
			mTiledLightBuffer.release();
		}

		static int constexpr MaxTiledLightNum = 1024;
		bool setupBuffer(Vec2i const& screenSize, int sizeFactor, int depthSlices);


		void render(ViewInfo& view, std::vector< LightInfo > const& lights);
		RHITexture3DRef mVolumeBufferA;
		RHITexture3DRef mVolumeBufferB;
		RHITexture3DRef mScatteringBuffer;
		RHITexture2DRef mShadowMapAtlas;
		RHIUniformBufferRef mTiledLightBuffer;

		class ClearBufferProgram* mProgClearBuffer;
		class LightScatteringProgram* mProgLightScattering;
	};

	class DefferredShadingTech : public RenderTechnique
	{
	public:

		enum DebugMode
		{
			eNone,
			eShowVolume , 
			eShowShadingArea ,

			NumDebugMode ,
		};

		DebugMode debugMode = DebugMode::eNone;
		bool bShowBound = false;
		LightBoundMethod boundMethod = LBM_GEMO_BOUND_SHAPE_WITH_STENCIL;

		bool init(SceneRenderTargets& sceneRenderTargets);

		void renderBassPass(ViewInfo& view, SceneInterface& scene);

		void prevRenderLights(ViewInfo& view);
		void renderLight(ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam );

		FrameBuffer   mBassPassBuffer;
		FrameBuffer   mLightBuffer;
		DefferredLightingProgram mProgLightingScreenRect[3];
		DefferredLightingProgram mProgLighting[3];
		DefferredLightingProgram mProgLightingShowBound;

		SceneRenderTargets* mSceneRenderTargets;

		Mesh mSphereMesh;
		Mesh mConeMesh;

		virtual MaterialShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory)
		{
			//return &GSimpleBasePass;

			return material.getShader(RenderTechiqueUsage::BasePass, vertexFactory);
		}

		void reload();
	};

	class BMAResolveProgram : public ShaderProgram
	{
	public:
		void bindParameters();

		void setParameters( RHITexture2D& ColorStorageTexture , RHITexture2D& NodeAndDepthStorageTexture , RHITexture2D& NodeHeadTexture);

		ShaderParameter mParamColorStorageTexture;
		ShaderParameter mParamNodeAndDepthStorageTexture;
		ShaderParameter mParamNodeHeadTexture;
	};


	class OITTechnique :  public RenderTechnique
	{

	public:
		//backwards memory allocation
		bool bUseBMA = true;

		bool init(Vec2i const& screenSize);

		void render(ViewInfo& view, SceneInterface& scnenRender , SceneRenderTargets* sceneRenderTargets );
		void renderTest(ViewInfo& view, SceneRenderTargets& sceneRenderTargets, Mesh& mesh , Material* material );
		void reload();

		void renderInternal(ViewInfo& view , std::function< void() > drawFuncion , SceneRenderTargets* sceneRenderTargets = nullptr );

		
		static constexpr int BMA_MaxPixelCounts[] =
		{
			//256 ,
			//128 , 
			//64 , 
			//48 ,
			32 , 
			16 ,  
			//8 ,
		};
		static int const NumBMALevel = sizeof( BMA_MaxPixelCounts ) / sizeof( BMA_MaxPixelCounts[0] );
		int BMA_InternalValMin[NumBMALevel];

		ShaderProgram mShaderBassPassTest;
		BMAResolveProgram mShaderResolve;
		BMAResolveProgram mShaderBMAResolves[NumBMALevel];

		RHITexture2DRef mColorStorageTexture;
		RHITexture2DRef mNodeAndDepthStorageTexture;
		RHITexture2DRef mNodeHeadTexture;
		AtomicCounterBuffer mStorageUsageCounter;
		Mesh mMeshScreenTri;
		Mesh mScreenMesh;

		

		FrameBuffer mFrameBuffer;

		void setupShader(ShaderProgram& program);
		virtual MaterialShaderProgram* getMaterialShader( RenderContext& context , MaterialMaster& material , VertexFactory* vertexFactory) override;
		virtual void setupMaterialShader(RenderContext& context, ShaderProgram& program) override;

	};
}//namespace RenderGL


#endif // SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
