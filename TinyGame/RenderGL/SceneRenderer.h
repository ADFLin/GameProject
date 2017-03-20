#pragma once
#ifndef SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
#define SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A

#include "RHICommon.h"
#include "Material.h"

namespace RenderGL
{

	class RenderParam;

	class  Plane
	{
	public:
		Plane() {}

		Plane(Vector3 const& v0, Vector3 const& v1, Vector3 const&  v2)
		{
			Vector3 d1 = v1 - v0;
			Vector3 d2 = v2 - v0;
			mNormal = d1.cross(d2);
			mNormal.normalize();

			mDistance = mNormal.dot(v0);
		}

		Plane(Vector3 const& normal, Vector3 const& pos)
			:mNormal(normal)
		{
			mNormal.normalize();
			mDistance = mNormal.dot(pos);
		}

		Plane(Vector3 const& n, float d)
			:mNormal(n)
			, mDistance(d)
		{
			mNormal.normalize();
		}

#if 0
		void swap(Plane& p)
		{
			using std::swap;

			swap(mNormal, p.mNormal);
			swap(mDistance, p.mDistance);
		}
#endif

		float calcDistance(Vector3 const& p) const
		{
			return p.dot(mNormal) - mDistance;
		}
	private:
		Vector3 mNormal;
		float   mDistance;
	};

	struct ViewInfo
	{
		Vector3 worldPos;
		Vector3 direction;
		Matrix4 worldToClip;
		Matrix4 worldToView;
		Matrix4 viewToWorld;
		Matrix4 viewToClip;
		Matrix4 clipToWorld;
		Matrix4 clipToView;

		float   gameTime;
		float   realTime;


		Plane frustumPlanes[6];

		bool frustumTest(Vector3 const& pos, float radius) const
		{
			for(int i = 0 ; i < 6 ; ++i )
			{
				if( frustumPlanes[i].calcDistance(pos) > radius )
					return false;
			}
			return true;
		}

		Vector3 getViewForwardDir() const
		{
			return TransformVector(Vector3(0, 0, -1), viewToWorld);
		}
		Vector3 getViewRightDir() const
		{
			return TransformVector(Vector3(1, 0, 0), viewToWorld);
		}
		Vector3 getViewUpDir() const
		{
			return TransformVector(Vector3(0, 1, 0), viewToWorld);
		}

		void setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix);

		void setupShader(ShaderProgram& program);


		void updateFrustumPlanes();
	};


	class SceneRender
	{
	public:
		virtual void render(ViewInfo& view, RenderParam& param) = 0;
	};


	enum class LightType
	{
		Spot,
		Point,
		Directional,
	};

	struct LightInfo
	{
		LightType type;
		Vector3   pos;
		Vector3   color;
		Vector3   dir;
		Vector3   spotAngle;
		float     intensity;
		float     radius;

		Vector3   upDir;
		bool      bCastShadow;

		void setupShaderGlobalParam(ShaderProgram& shader) const;
		
		bool testVisible(ViewInfo const& view) const
		{
			switch( type )
			{
			case LightType::Spot:
				{
					float d = 0.5 * radius / Math::Cos(Math::Deg2Rad(spotAngle.y));
					return view.frustumTest(pos + d * dir, d);
				}
				break;
			case LightType::Point:
				return view.frustumTest(pos, radius);
			}

			return true;
		}
	};



	class RenderParam
	{
	public:
		ViewInfo*      mCurView;
		ShaderProgram* mCurMaterialShader;

		virtual ShaderProgram* getMaterialShader(MaterialMaster& material)
		{
			return nullptr;
		}

		virtual void setWorld(Matrix4 const& mat)
		{
			glMultMatrixf(mat);
			if( mCurMaterialShader )
			{
				Matrix4 matInv;
				float det;
				mat.inverseAffine(matInv, det);
				mCurMaterialShader->setParam(SHADER_PARAM(VertexFactoryParams.localToWorld), mat);
				mCurMaterialShader->setParam(SHADER_PARAM(VertexFactoryParams.worldToLocal), matInv);
			}
		}
		virtual void setupMaterialShader(ShaderProgram& shader) {}

		void beginRender(ViewInfo& view)
		{
			mCurView = &view;
			mCurMaterialShader = nullptr;
		}
		void endRender()
		{
			if( mCurMaterialShader )
			{
				mCurMaterialShader->unbind();
				mCurMaterialShader = nullptr;
			}
		}

		void setMaterial(Material* material)
		{
			ShaderProgram* shader;
			if( material )
			{
				shader = getMaterialShader(*material->getMaster());
				if( shader == nullptr || !shader->isVaildate() )
				{
					material = nullptr;
				}
			}

			if( material == nullptr )
			{
				material = GDefalutMaterial;
				shader = getMaterialShader(*GDefalutMaterial);
			}

			if( shader == nullptr )
				return;

			if( mCurMaterialShader != shader )
			{
				if( mCurMaterialShader )
				{
					mCurMaterialShader->unbind();
				}
				mCurMaterialShader = shader;
				mCurMaterialShader->bind();
				mCurView->setupShader(*mCurMaterialShader);
				setupMaterialShader(*mCurMaterialShader);
			}
			material->bindShaderParam(*mCurMaterialShader);
		}
		void setMaterialParameter(char const* name, Texture2D& texture)
		{
			if( mCurMaterialShader )
			{
				RHITexture2D* textureRHI = texture.getRHI() ? texture.getRHI() : GDefaultMaterialTexture2D;
				mCurMaterialShader->setTexture(name, *textureRHI);
			}
		}
		void setMaterialParameter(char const* name, RHITexture2D& texture)
		{
			if( mCurMaterialShader )
			{
				mCurMaterialShader->setTexture(name, texture);
			}
		}
		void setMaterialParameter(char const* name, Vector3 value)
		{
			if( mCurMaterialShader )
			{
				mCurMaterialShader->setParam(name, value);
			}
		}

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

		void setupLight(LightInfo const& inLight)
		{
			light = &inLight;
			switch( light->type )
			{
			case LightType::Spot:
			case LightType::Point:
				shadowParam.y = 1.0 / inLight.radius;
				break;
			case LightType::Directional:
				//#TODO
				shadowParam.y = 1.0;
				break;
			}
		}

		void setupShader(ShaderProgram& program) const;
	};

#define USE_MATERIAL_SHADOW 1
	class ShadowDepthTech : public RenderParam
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
		void renderLighting(ViewInfo& view, SceneRender& scene, LightInfo const& light, bool bMultiple);
		void renderShadowDepth(ViewInfo& view, SceneRender& scene, ShadowProjectParam& shadowProjectParam);
		void calcCascadeShadowProjectInfo(ViewInfo &view, LightInfo const &light, float depthParam[2], Matrix4 &worldToLight, Matrix4 &shadowProject);

		static void determineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[]);

		void drawShadowTexture(LightType type);
		void reload();

		virtual void setWorld(Matrix4 const& mat)
		{
			RenderParam::setWorld(mat);
			if( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam(SHADER_PARAM(VertexFactoryParams.localToWorld), mat);
			}
		}

		virtual ShaderProgram* getMaterialShader(MaterialMaster& material)
		{
#if USE_MATERIAL_SHADOW
			if( mEffectCur == &mProgLighting )
				return nullptr;
			return &material.mShadowShader;

#else
			return nullptr;
#endif
		}

		virtual void setupMaterialShader(ShaderProgram& shader)
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

		DepthRenderBuffer depthBuffer1;
		DepthRenderBuffer depthBuffer2;
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

	class TechBase
	{
	public:


	};


	enum LightBoundMethod
	{
		LBM_SCREEN_RECT,
		LBM_GEMO_BOUND_SHAPE,
		LBM_GEMO_BOUND_SHAPE_WITH_STENCIL,

		NumLightBoundMethod,
	};

	class DefferredLightingTech : public TechBase
		                       , public RenderParam
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

		void renderBassPass(ViewInfo& view, SceneRender& scene);

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

		virtual ShaderProgram* getMaterialShader(MaterialMaster& material)
		{
			//return &GSimpleBasePass;

			return &material.mShader;
		}

		void reload();
	};
}//namespace RenderGL


#endif // SceneRenderer_H_D5144663_1B3D_486C_A523_45B46EC4482A
