#pragma once
#ifndef SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
#define SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A

#include "RHICommon.h"
#include "ShaderCore.h"
#include "Material.h"
#include "MeshUtility.h"
#include "RenderContext.h"


namespace Render
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

		bool initializeRHI(IntVector2 const& size , int numSamples );
		void setupShader(RHICommandList& commandList, ShaderProgram& program);

		void drawTextures(RHICommandList& commandList, IntVector2 const& size, IntVector2 const& gapSize);
		void drawTexture(RHICommandList& commandList, int x, int y, int width, int height, int idxBuffer);
		void drawTexture(RHICommandList& commandList, int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask);
	};

	struct RenderTargetResource
	{
		RHITexture2DRef renderTargetTexture;
		RHITexture2DRef resolvedTexture;
	};

	class SceneRenderTargets
	{
	public:
		bool initializeRHI(IntVector2 const& size, int numSamples = 1);

		RHITexture2D&  getFrameTexture() { return *mFrameTextures[mIdxRenderFrameTexture]; }
		RHITexture2D&  getPrevFrameTexture() { return *mFrameTextures[1 - mIdxRenderFrameTexture]; }
		void swapFrameTexture()
		{
			mIdxRenderFrameTexture = 1 - mIdxRenderFrameTexture;
			mFrameBuffer.setTexture(0, getFrameTexture());
		}

		GBufferParamData&   getGBuffer() { return mGBuffer; }
		RHITextureDepth&    getDepthTexture() { return *mDepthTexture; }



		OpenGLFrameBuffer& getFrameBuffer() { return mFrameBuffer; }

		void attachDepthTexture() { mFrameBuffer.setDepth(*mDepthTexture); }
		void detachDepthTexture() { mFrameBuffer.removeDepthBuffer(); }


		void drawDepthTexture(RHICommandList& commandList, int x, int y, int width, int height);

		GBufferParamData   mGBuffer;
		RHITexture2DRef    mFrameTextures[2];
		int                mIdxRenderFrameTexture;
		OpenGLFrameBuffer  mFrameBuffer;
		RHITextureDepthRef mDepthTexture;
		RHITextureDepthRef mResolvedDepthTexture;
	};


	class GBufferShaderParameters
	{
	public:
		void bindParameters(ShaderParameterMap& parameterMap, bool bUseDepth = false);

		void setParameters(RHICommandList& commandList, ShaderProgram& program, GBufferParamData& GBufferData);
		void setParameters(RHICommandList& commandList, ShaderProgram& program, SceneRenderTargets& sceneRenderTargets);

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

		void setupShader(RHICommandList& commandList, ShaderProgram& program) const;
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
		void renderLighting(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene, LightInfo const& light, bool bMultiple);
		void renderShadowDepth(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene, ShadowProjectParam& shadowProjectParam);
		void calcCascadeShadowProjectInfo(ViewInfo &view, LightInfo const &light, float depthParam[2], Matrix4 &worldToLight, Matrix4 &shadowProject);

		static void determineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[]);

		void drawShadowTexture(RHICommandList& commandList, LightType type, IntVector2 const& pos, int length);
		void reload();

		//RenderTechnique
		virtual void setupWorld(RenderContext& context , Matrix4 const& mat) override
		{
			RHICommandList& commandList = context.getCommnadList();
			RenderTechnique::setupWorld( context , mat);
			if( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), mat);
			}
		}

		virtual MaterialShaderProgram* getMaterialShader(RenderContext& context,  MaterialMaster& material , VertexFactory* vertexFactory ) override;

		virtual void setupMaterialShader(RenderContext& context, MaterialShaderProgram& shader) override
		{
			RHICommandList& commandList = context.getCommnadList();
			shader.setParam(commandList, SHADER_PARAM(DepthShadowMatrix), mShadowMatrix);
			shader.setParam(commandList, SHADER_PARAM(ShadowParam), shadowParam.x, shadowParam.y);
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
		OpenGLFrameBuffer     mShadowBuffer;

		ShaderProgram  mProgShadowDepth[3];
		ShaderProgram  mProgLighting;

		Matrix4       mShadowMatrix;

#if USE_DepthRenderBuffer
		RHIDepthRenderBufferRef depthBuffer1;
		RHIDepthRenderBufferRef depthBuffer2;
#endif
	};


	class PostProcess
	{




	};


	class PostProcessSSAO : public PostProcess
	{
	public:
		static int const NumDefaultKernel = 16;

		bool init(IntVector2 const& size);

		void render(RHICommandList& commandList, ViewInfo& view, SceneRenderTargets& sceneRenderTargets);
		void drawSSAOTexture(RHICommandList& commandList, IntVector2 const& pos , IntVector2 const& size );

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
		OpenGLFrameBuffer  mFrameBuffer;
		RHITexture2DRef mSSAOTextureBlur;
		RHITexture2DRef mSSAOTexture;
		class SSAOGenerateProgram* mProgSSAOGenerate;
		class SSAOBlurProgram*     mProgSSAOBlur;
		class SSAOAmbientProgram*  mProgAmbient;

	};

	class PostProcessDOF : public PostProcess
	{
	public:
		bool init(IntVector2 const& size);
		void render(RHICommandList& commandList, ViewInfo& view, SceneRenderTargets& sceneRenderTargets);

		OpenGLFrameBuffer mFrameBufferGen;
		RHITexture2DRef mTextureNear;
		RHITexture2DRef mTextureFar;

		OpenGLFrameBuffer mFrameBufferBlur;
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


	struct GPU_BUFFER_ALIGN TiledLightInfo
	{
		DECLARE_BUFFER_STRUCT(TiledLightBlock);

		Vector3 pos;
		int32   type; 
		Vector3 color;
		float   intensity;
		Vector3 dir;
		float   radius;
		Vector4 param; // x y: spotAngle  , z : shadowFactor
		Matrix4 worldToShadow;

		void setValue(LightInfo const& light);
	};


	struct SceneLightPrimtive
	{
		LightInfo const* light;
		RHITextureRef shadowMap;
	};

	class VolumetricLightingTech : public RenderTechnique
	{	

	public:

		bool init(IntVector2 const& screenSize);

		void releaseRHI()
		{
			mVolumeBufferA.release();
			mVolumeBufferB.release();
			mScatteringBuffer.release();
			mTiledLightBuffer.release();
		}

		static int constexpr MaxTiledLightNum = 1024;
		bool setupBuffer(IntVector2 const& screenSize, int sizeFactor, int depthSlices);


		void render(RHICommandList& commandList, ViewInfo& view, std::vector< LightInfo > const& lights);
		RHITexture3DRef mVolumeBufferA;
		RHITexture3DRef mVolumeBufferB;
		RHITexture3DRef mScatteringBuffer;
		RHITexture2DRef mShadowMapAtlas;
		RHIVertexBufferRef mTiledLightBuffer;

		class ClearBufferProgram* mProgClearBuffer;
		class LightScatteringProgram* mProgLightScattering;
	};

	class DeferredShadingTech : public RenderTechnique
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

		void renderBassPass(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene);

		void prevRenderLights(RHICommandList& commandList, ViewInfo& view);
		void renderLight(RHICommandList& commandList, ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam );

		OpenGLFrameBuffer   mBassPassBuffer;
		OpenGLFrameBuffer   mLightBuffer;
		class DeferredLightingProgram* mProgLightingScreenRect[3];
		class DeferredLightingProgram* mProgLighting[3];
		class DeferredLightingProgram* mProgLightingShowBound;

		SceneRenderTargets* mSceneRenderTargets;

		Mesh mSphereMesh;
		Mesh mConeMesh;

		virtual MaterialShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory);

		void reload();
	};

	struct OITShaderData
	{

		bool init(int storageSize, IntVector2 const& screenSize);
		void releaseRHI();

		RHITexture2DRef colorStorageTexture;
		RHITexture2DRef nodeAndDepthStorageTexture;
		RHITexture2DRef nodeHeadTexture;
		RHIVertexBufferRef storageUsageCounter;
	};

	struct OITCommonParameter
	{

		void bindParameters(ShaderParameterMap& parameterMap)
		{
			parameterMap.bind(mParamColorStorageTexture, SHADER_PARAM(ColorStorageRWTexture));
			parameterMap.bind(mParamNodeAndDepthStorageTexture, SHADER_PARAM(NodeAndDepthStorageRWTexture));
			parameterMap.bind(mParamNodeHeadTexture, SHADER_PARAM(NodeHeadRWTexture));
			parameterMap.bind(mParamNextIndex, SHADER_PARAM(NextIndex));
		}

		void setParameters(RHICommandList& commandList, ShaderProgram& shader , OITShaderData& data)
		{
			shader.setRWTexture(commandList, mParamColorStorageTexture, *data.colorStorageTexture, AO_WRITE_ONLY);
			shader.setRWTexture(commandList, mParamNodeAndDepthStorageTexture, *data.nodeAndDepthStorageTexture, AO_READ_AND_WRITE);
			shader.setRWTexture(commandList, mParamNodeHeadTexture, *data.nodeHeadTexture, AO_READ_AND_WRITE);
			shader.setAtomicCounterBuffer(commandList, mParamNextIndex, *data.storageUsageCounter);
		}

		ShaderParameter mParamColorStorageTexture;
		ShaderParameter mParamNodeAndDepthStorageTexture;
		ShaderParameter mParamNodeHeadTexture;
		ShaderParameter mParamNextIndex;

	};


	class Material;
	class Mesh;


	class OITTechnique :  public RenderTechnique
	{

	public:
		//backwards memory allocation
		bool bUseBMA = true;

		bool init(IntVector2 const& screenSize);

		void render(RHICommandList& commandList, ViewInfo& view, SceneInterface& scnenRender , SceneRenderTargets* sceneRenderTargets );
		void renderTest(RHICommandList& commandList, ViewInfo& view, SceneRenderTargets& sceneRenderTargets, Mesh& mesh , Material* material );
		void reload();

		void renderInternal(RHICommandList& commandList, ViewInfo& view , std::function< void(RHICommandList&) > drawFuncion , SceneRenderTargets* sceneRenderTargets = nullptr );

		
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
		class BMAResolveProgram* mShaderResolve;
		class BMAResolveProgram* mShaderBMAResolves[NumBMALevel];

		OITShaderData mShaderData;

		Mesh mScreenMesh;
 
		OpenGLFrameBuffer mFrameBuffer;

		void setupShader(RHICommandList& commandList, ShaderProgram& program);
		virtual MaterialShaderProgram* getMaterialShader( RenderContext& context , MaterialMaster& material , VertexFactory* vertexFactory) override;
		virtual void setupMaterialShader(RenderContext& context, MaterialShaderProgram& program) override;

	};
}//namespace Render


#endif // SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
