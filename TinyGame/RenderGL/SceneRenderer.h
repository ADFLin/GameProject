#pragma once
#ifndef SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
#define SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A

#include "RHICommon.h"
#include "Material.h"
#include "RenderContext.h"

namespace RenderGL
{

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

		void setupShaderGBuffer(ShaderProgram& program, bool bUseDepth)
		{
			if( bUseDepth )
				program.setTexture(SHADER_PARAM(FrameDepthTexture), *mDepthTexture);

			mGBuffer.setupShader(program);
		}

		FrameBuffer& getFrameBuffer() { return mFrameBuffer; }

		void drawDepthTexture(int x, int y, int width, int height);

		GBufferParamData mGBuffer;
		RHITexture2DRef  mFrameTextures[2];
		int              mIdxRenderFrameTexture;
		FrameBuffer      mFrameBuffer;
		RHITextureDepthRef mDepthTexture;
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

	class ForwardShadingTech : public RenderTechique
	{



	};

#define USE_MATERIAL_SHADOW 1
	class ShadowDepthTech : public RenderTechique
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

		//RenderTechique
		virtual void setupWorld(RenderContext& context , Matrix4 const& mat) override
		{
			RenderTechique::setupWorld( context , mat);
			if( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam(SHADER_PARAM(Primitive.localToWorld), mat);
			}
		}

		virtual ShaderProgram* getMaterialShader(RenderContext& context,  MaterialMaster& material , VertexFactory* vertexFactory ) override
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
		void drawSSAOTexture( Vec2i const& pos , Vec2i const& size )
		{
			ShaderHelper::drawTexture(*mSSAOTextureBlur, pos , size );
		}

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
		ShaderProgram mSSAOShader;
		ShaderProgram mSSAOBlurShader;
		ShaderProgram mAmbientShader;
	};
	inline float normalizePlane(Vector4& plane)
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

	class DefferredShadingTech : public RenderTechique
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
		ShaderProgram mProgLightingScreenRect[3];
		ShaderProgram mProgLighting[3];
		ShaderProgram mProgLightingShowBound;

		SceneRenderTargets* mSceneRenderTargets;
		GBufferParamData*   mGBufferParamData;

		Mesh mSphereMesh;
		Mesh mConeMesh;

		virtual ShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory)
		{
			//return &GSimpleBasePass;

			return material.getShader(RenderTechiqueUsage::BasePass, vertexFactory);
		}

		void reload();
	};


	class OITTech :  public RenderTechique
	{

	public:
		//backwards memory allocation
		bool bUseBMA = true;

		bool init(Vec2i const& size);

		void render(ViewInfo& view, SceneInterface& scnenRender , SceneRenderTargets* sceneRenderTargets );
		void renderTest(ViewInfo& view, SceneRenderTargets& sceneRenderTargets, Mesh& mesh , Material* material );
		void reload();

		void renderInternal(ViewInfo& view , std::function< void() > drawFuncion , SceneRenderTargets* sceneRenderTargets = nullptr );

		static int const NumBMALevel = 2;
		int BMA_MaxPixelCounts[NumBMALevel] =
		{
			//256 ,
			//128 , 
			//64 , 
			//48 ,
			32 , 
			16 ,  
			//8 ,
		};
		int BMA_InternalValMin[NumBMALevel];

		ShaderProgram mShaderBassPassTest;
		ShaderProgram mShaderResolve;
		ShaderProgram mShaderBMAResolves[NumBMALevel];
		RHITexture2DRef mColorStorageTexture;
		RHITexture2DRef mNodeAndDepthStorageTexture;
		RHITexture2DRef mNodeHeadTexture;
		AtomicCounterBuffer mStorageUsageCounter;
		Mesh mMeshScreenTri;
		Mesh mScreenMesh;

		

		FrameBuffer mFrameBuffer;

		virtual ShaderProgram* getMaterialShader( RenderContext& context , MaterialMaster& material , VertexFactory* vertexFactory) override;
		virtual void setupMaterialShader(RenderContext& context, ShaderProgram& shader) override;

	};
}//namespace RenderGL


#endif // SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
