#pragma once
#ifndef SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
#define SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A

#include "RHICommon.h"
#include "ShaderCore.h"
#include "Material.h"
#include "RenderContext.h"

#include "Renderer/Mesh.h"
#include "Renderer/BasePassRendering.h"
#include "Renderer/ShadowDepthRendering.h"
#include "Renderer/SceneLighting.h"

//#REMOVE ME
#include "OpenGLCommon.h"

namespace Render
{

	class FrameRenderNode
	{
	public:





	};
	class RenderContext;
	struct LightInfo;




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

		static void DetermineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[]);

		void drawShadowTexture(RHICommandList& commandList, ELightType type, Matrix4 const& porjectMatrix, IntVector2 const& pos, int length);
		void reload();

		//RenderTechnique
		void setupWorld(RenderContext& context , Matrix4 const& mat) override
		{
			RHICommandList& commandList = context.getCommnadList();
			RenderTechnique::setupWorld( context , mat);
			if( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), mat);
			}
		}

		MaterialShaderProgram* getMaterialShader(RenderContext& context,  MaterialMaster& material , VertexFactory* vertexFactory ) override;

		void setupMaterialShader(RenderContext& context, MaterialShaderProgram& shader) override
		{
			RHICommandList& commandList = context.getCommnadList();
			shader.setParam(commandList, SHADER_PARAM(DepthShadowMatrix), mShadowMatrix);
			shader.setParam(commandList, SHADER_PARAM(ShadowParam), shadowParam.x, shadowParam.y);
		}

		float         depthParam[2];
		Vector3       shadowParam;

		float         mCascadeMaxDist;

		ShaderProgram* mEffectCur;

		Mesh           mCubeMesh;
		ShaderProgram  mEffectCube;

		RHITexture2DRef    mShadowMap2;
		RHITextureCubeRef  mShadowMap;
		RHITexture2DRef    mCascadeTexture;
		RHIFrameBufferRef  mShadowBuffer;

		ShaderProgram  mProgShadowDepthList[3];
		ShaderProgram  mProgLighting;

		Matrix4       mShadowMatrix;

		RHITexture2DRef depthBuffer1;
		RHITexture2DRef depthBuffer2;
	};


	class PostProcess
	{




	};


	class PostProcessSSAO : public PostProcess
	{
	public:
		static int const NumDefaultKernel = 16;

		bool init(IntVector2 const& size);

		void render(RHICommandList& commandList, ViewInfo& view, FrameRenderTargets& sceneRenderTargets);
		void drawSSAOTexture(RHICommandList& commandList, Matrix4 const& XForm, IntVector2 const& pos, IntVector2 const& size);

		void reload();
	private:

		float Random(float v0, float v1)
		{
			return v0 + (v1 - v0) * (float(::rand()) / RAND_MAX);
		}

		void generateKernelVectors(int numKernal);

	public:
		TArray< Vector3 > mKernelVectors;
		RHIFrameBufferRef  mFrameBuffer;
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
		void render(RHICommandList& commandList, ViewInfo& view, FrameRenderTargets& sceneRenderTargets);

		RHIFrameBufferRef mFrameBufferGen;
		RHITexture2DRef mTextureNear;
		RHITexture2DRef mTextureFar;

		RHIFrameBufferRef mFrameBufferBlur;
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


	struct GPU_ALIGN TiledLightInfo
	{
		DECLARE_BUFFER_STRUCT(TiledLightList);

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

	struct DecalInfo
	{
		Vector3    centerPos;
		Quaternion rotation;
		Vector3    size;

		Matrix4 getTransform()
		{
			return Matrix4::Translate(-centerPos) * Matrix4::Rotate(rotation.inverse()) * Matrix4::Scale(Vector3(1.0f).div(size)) * Matrix4::Translate(Vector3(0.5));
		}

		Material*  material;
		int        order;
	};

	class DecalRenderTech : public RenderTechnique
	{




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


		void render(RHICommandList& commandList, ViewInfo& view, TArray< LightInfo > const& lights);
		RHITexture3DRef mVolumeBufferA;
		RHITexture3DRef mVolumeBufferB;
		RHITexture3DRef mScatteringBuffer;
		RHITexture2DRef mShadowMapAtlas;
		RHIBufferRef mTiledLightBuffer;

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

		bool init(FrameRenderTargets& sceneRenderTargets);

		void renderBassPass(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene);

		void prevRenderLights(RHICommandList& commandList, ViewInfo& view);
		void renderLight(RHICommandList& commandList, ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam );

		RHIFrameBufferRef   mBassPassBuffer;
		RHIFrameBufferRef   mLightingBuffer;
		RHIFrameBufferRef   mLightingDepthBuffer;
		class DeferredLightingProgram* mProgLightingScreenRect[3];
		class DeferredLightingProgram* mProgLighting[3];
		class LightingShowBoundProgram* mProgLightingShowBound;

		FrameRenderTargets* mSceneRenderTargets;

		Mesh mSphereMesh;
		Mesh mConeMesh;

		MaterialShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory) override;

		void reload();
	};

	struct OITShaderData
	{

		bool init(int storageSize, IntVector2 const& screenSize);
		void releaseRHI();

		RHITexture2DRef colorStorageTexture;
		RHITexture2DRef nodeAndDepthStorageTexture;
		RHITexture2DRef nodeHeadTexture;
		RHIBufferRef storageUsageCounter;
	};

	struct OITCommonParameter
	{

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			mParamColorStorageTexture.bind(parameterMap, SHADER_PARAM(ColorStorageRWTexture));
			mParamNodeAndDepthStorageTexture.bind(parameterMap, SHADER_PARAM(NodeAndDepthStorageRWTexture));
			mParamNodeHeadTexture.bind(parameterMap, SHADER_PARAM(NodeHeadRWTexture));
			mParamNextIndex.bind(parameterMap, SHADER_PARAM(NextIndex));
		}

		void setParameters(RHICommandList& commandList, ShaderProgram& shader , OITShaderData& data)
		{
			shader.setRWTexture(commandList, mParamColorStorageTexture, *data.colorStorageTexture, 0, EAccessOp::WriteOnly);
			shader.setRWTexture(commandList, mParamNodeAndDepthStorageTexture, *data.nodeAndDepthStorageTexture, 0, EAccessOp::ReadAndWrite);
			shader.setRWTexture(commandList, mParamNodeHeadTexture, *data.nodeHeadTexture, 0, EAccessOp::ReadAndWrite);
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

		void render(RHICommandList& commandList, ViewInfo& view, SceneInterface& scnenRender , FrameRenderTargets* sceneRenderTargets );
		void renderTest(RHICommandList& commandList, ViewInfo& view, FrameRenderTargets& sceneRenderTargets, Mesh& mesh , Material* material );
		void reload();

		void renderInternal(RHICommandList& commandList, ViewInfo& view , std::function< void(RHICommandList&) > drawFuncion , FrameRenderTargets* sceneRenderTargets = nullptr );

		
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
 
		RHIFrameBufferRef mFrameBuffer;

		void setupShader(RHICommandList& commandList, ShaderProgram& program);
		MaterialShaderProgram* getMaterialShader( RenderContext& context , MaterialMaster& material , VertexFactory* vertexFactory) override;
		void setupMaterialShader(RenderContext& context, MaterialShaderProgram& program) override;

	};

	class SceneRenderer
	{
	public:

		bool Initialize(SceneInterface& scene)
		{
			mScene = &scene;
		}

		bool InitializeRHI()
		{
			//VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI(screenSize));
			VERIFY_RETURN_FALSE(mTechDeferredShading.init(mSceneRenderTargets));
		}

		void render(ViewInfo& view)
		{




		}




		FrameRenderTargets   mSceneRenderTargets;
		DeferredShadingTech  mTechDeferredShading;
		SceneInterface*      mScene;
	};

}//namespace Render


#endif // SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
