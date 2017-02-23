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

#include <limits>
#include <memory>

namespace RenderGL
{
	typedef GL::Vec2i Vec2i;

	float const FLT_DIV_ZERO_EPSILON = 1e-6;

	struct GpuSync
	{
		GpuSync()
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


	using namespace GL;

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

	class ShaderHelper : public SingletonT< ShaderHelper >
	{
	public:
		bool init();

		void copyTextureToBuffer(Texture2DRHI& copyTexture);
		void copyTextureMaskToBuffer(Texture2DRHI& copyTexture, Vector4 const& colorMask);
		void mapTextureColorToBuffer(Texture2DRHI& copyTexture, Vector4 const& colorMask, float valueFactor[2]);
		void copyTexture(Texture2DRHI& destTexture, Texture2DRHI& srcTexture);


		static void drawCubeTexture(TextureCubeRHI& texCube, Vec2i const& pos, int length);
		static void drawTexture(Texture2DRHI& texture, Vec2i const& pos, Vec2i const& size);

		void reload();
		GlobalShader mProgCopyTexture;
		GlobalShader mProgCopyTextureMask;
		GlobalShader mProgMappingTextureColor;
	};



	enum class LightType
	{
		Spot ,
		Point ,
		Directional ,
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

		void bindGlobalParam(ShaderProgram& program) const
		{
			program.setParam(SHADER_PARAM(GLight.worldPosAndRadius), Vector4( pos , radius ) );
			program.setParam(SHADER_PARAM(GLight.color), intensity * color);
			program.setParam(SHADER_PARAM(GLight.type), int(type));
			program.setParam(SHADER_PARAM(GLight.dir), normalize(dir));

			Vector3 spotParam;
			float angleInner = Math::Min(spotAngle.x, spotAngle.y);
			spotParam.x = Math::Cos( Math::Deg2Rad( Math::Min(89.9, angleInner)) );
			spotParam.y = Math::Cos( Math::Deg2Rad( Math::Min(89.9, spotAngle.y)) );
			program.setParam(SHADER_PARAM(GLight.spotParam), spotParam);
		}
	};

	struct ViewInfo
	{
		Vector3 worldPos;
		Matrix4 worldToClip;
		Matrix4 worldToView;
		Matrix4 viewToWorld;
		Matrix4 viewToClip;
		Matrix4 clipToWorld;
		Matrix4 clipToView;
		
		float   gameTime;
		float   realTime;

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

		void setupTransform( Matrix4 const& inViewMatrix  , Matrix4 const& inProjectMatrix )
		{
			
			worldToView = inViewMatrix;
			float det;
			worldToView.inverse(viewToWorld, det);
			worldPos = TransformPosition(Vector3(0, 0, 0) , viewToWorld);
			viewToClip = inProjectMatrix;
			worldToClip = worldToView * viewToClip;

			viewToClip.inverse(clipToView, det);
			clipToWorld = clipToView * viewToWorld;
		}

		void bindParam(ShaderProgram& program)
		{
			program.setParam(SHADER_PARAM(View.worldPos), worldPos);
			program.setParam(SHADER_PARAM(View.worldToView), worldToView);
			program.setParam(SHADER_PARAM(View.worldToClip), worldToClip);
			program.setParam(SHADER_PARAM(View.viewToWorld), viewToWorld);
			program.setParam(SHADER_PARAM(View.viewToClip), viewToClip);
			program.setParam(SHADER_PARAM(View.clipToView), clipToView);
			program.setParam(SHADER_PARAM(View.clipToWorld), clipToWorld);
			program.setParam(SHADER_PARAM(View.gameTime), gameTime);
			program.setParam(SHADER_PARAM(View.realTime), realTime);
		}
	};


	class RenderParam;
	class SceneRender
	{
	public:
		virtual void render( ViewInfo& view , RenderParam& param ) = 0;
	};

	struct ShaderParameter
	{
		uint8 Index;
	};

	class Material
	{
	public:
		bool loadFile(char const* name)
		{
			mName = name;
			return loadInternal();
		}

		void reload()
		{
			loadInternal();
		}

		void setParameter(char const* name, Texture2DRHI& texture) { setTextureParameterInternal(name, ParamType::eTexture2DRHI , texture); }
		void setParameter(char const* name, Texture3DRHI& texture) { setTextureParameterInternal(name, ParamType::eTexture3DRHI, texture); }
		void setParameter(char const* name, TextureCubeRHI& texture) { setTextureParameterInternal(name, ParamType::eTextureCubeRHI, texture); }
		void setParameter(char const* name, TextureDepthRHI& texture) { setTextureParameterInternal(name, ParamType::eTextureDepthRHI, texture); }

		void setParameter(char const* name, Matrix4 const& value){  setParameterT(name, ParamType::eMatrix4 , value );  }
		void setParameter(char const* name, Matrix3 const& value) {  setParameterT(name, ParamType::eMatrix3, value);  }
		void setParameter(char const* name, Vector4 const& value) {  setParameterT(name, ParamType::eVector, value);  }
		void setParameter(char const* name, float value) {  setParameterT(name, ParamType::eScale , value);  }

		enum class ParamType
		{
			
			eMatrix4,
			eMatrix3,
			eVector,
			eScale,

			eTexture2DRHI,
			eTexture3DRHI,
			eTextureCubeRHI,
			eTextureDepthRHI,
		};
		struct ParamSlot
		{
			HashString    name;
			ParamType     type;
			int16         offset;
		};

		template< class T >
		void setParameterT(char const* name, ParamType type , T const& value)
		{
			ParamSlot& slot = fetchParam(name , type);
			if( slot.offset != -1 )
			{
				T& dateStorage = *reinterpret_cast<T*>(&mParamDataStorage[slot.offset]);
				dateStorage = value;
			}
		}

		void setTextureParameterInternal(char const* name, ParamType type, TextureBaseRHI& texture)
		{
			ParamSlot& slot = fetchParam(name, type);
			if( slot.offset != -1 )
			{
				TextureBaseRHI*& dateStorage = *reinterpret_cast<TextureBaseRHI**>(&mParamDataStorage[slot.offset]);
				dateStorage = &texture;
			}
		}

		void bindShaderParam(ShaderProgram& shader);
		bool loadInternal();

		



		ParamSlot& fetchParam(char const* name , ParamType type );


		std::vector< uint8 > mParamDataStorage;
		std::vector< ParamSlot > mParams;

		//
		GlobalShader mShader;
		GlobalShader mShadowShader;
		std::string  mName;

	};


	class RenderParam
	{
	public:
		ViewInfo*      mCurView;
		ShaderProgram* mCurMaterialShader;

		virtual ShaderProgram* getMaterialShader(Material& material)
		{
			return nullptr;
		}

		virtual void setWorld(Matrix4 const& mat)
		{
			glMultMatrixf(mat);
			if ( mCurMaterialShader )
			{
				Matrix4 matInv;
				float det;
				mat.inverseAffine(matInv, det);
				mCurMaterialShader->setParam(SHADER_PARAM(VertexFactoryParams.localToWorld), mat);
				mCurMaterialShader->setParam(SHADER_PARAM(VertexFactoryParams.worldToLocal), matInv);
			}
		}
		virtual void setupMaterialShader(ShaderProgram& shader){}

		void beginRender( ViewInfo& view )
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

		void setMaterial(Material& material)
		{
			ShaderProgram* shader = getMaterialShader(material);
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
				mCurView->bindParam(*mCurMaterialShader);
				material.bindShaderParam(*mCurMaterialShader);
				setupMaterialShader(*mCurMaterialShader);
			}
		}
		void setMaterialParameter(char const* name, Texture2DRHI& texture )
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

	struct RenderUnit
	{
		bool     beShow;
		Mesh*    mesh;
		Matrix4  world;
	};



	class Scene
	{
	public:
		void render( ViewInfo& view , RenderParam& param )
		{
			for( RenderUnitVec::iterator iter = mUnits.begin() , itEnd = mUnits.end();
				 iter != itEnd ; ++iter )
			{
				RenderUnit& unit = *iter;
				if ( !unit.beShow )
					continue;

				param.setWorld( unit.world );
				unit.mesh->draw();
			}
		}

		typedef std::vector< RenderUnit > RenderUnitVec;
		RenderUnitVec mUnits;
	};



	class PostProcess
	{




	};


	inline float normalizePlane( Vector4& plane )
	{
		float len2 = plane.xyz().dot( plane.xyz() );
		float invSqrt =  Math::InvSqrt( len2 );
		plane *= invSqrt;
		return invSqrt;
	}

	class TechBase
	{
	public:

		
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

		Texture2DRHI    textures[NumBuffer];
		TextureDepthRHI depthTexture;

		bool init( Vec2i const& size );
		void bindParam(ShaderProgram& program , bool bUseDepth );

		void drawBuffer();
		void drawDepthTexture(int x, int y, int width, int height);
		void drawTexture(int x, int y, int width, int height, int idxBuffer);
		void drawTexture(int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask);
	};

	struct ShadowProjectParam
	{
		static int const MaxCascadeNum = 8;
		LightInfo const*   light;
		TextureBaseRHI* shadowTexture;
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

		void bindParam(ShaderProgram& program) const
		{
			program.setParam(SHADER_PARAM(ShadowParam), shadowParam.x, shadowParam.y);
			switch( light->type )
			{
			case LightType::Spot:
				program.setParam(SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 1);
				program.setTexture(SHADER_PARAM(ShadowTexture2D), *(Texture2DRHI*)shadowTexture);
				break;
			case LightType::Point:
				program.setParam(SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 6);
				program.setTexture(SHADER_PARAM(ShadowTextureCube), *(TextureCubeRHI*)shadowTexture);
				break;
			case LightType::Directional:
				program.setParam(SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, numCascade);
				program.setTexture(SHADER_PARAM(ShadowTexture2D), *(Texture2DRHI*)shadowTexture);
				program.setParam(SHADER_PARAM(NumCascade), numCascade);
				program.setParam(SHADER_PARAM(CacadeDepth), cacadeDepth , numCascade );
				break;
			}
		}
	};

	class DefferredLightingTech : public TechBase
		                        , public RenderParam
	{
	public:
		bool init(GBufferParamData& inGBufferParamData);

		void renderBassPass(ViewInfo& view, SceneRender& scene , Texture2DRHI& frameTexture );
		void renderLight(ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam);

		FrameBuffer  mBuffer;
		GlobalShader mProgLighting[3];

		GBufferParamData* mGBufferParamData;
		
		virtual ShaderProgram* getMaterialShader(Material& material)
		{
			return &material.mShader;
		}

		void reload()
		{
			for( int i= 0;i < 3 ; ++i )
				mProgLighting[i].reload();
		}
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
		void renderShadowDepth(ViewInfo& view, SceneRender& scene , ShadowProjectParam& shadowProjectParam );
		void calcCascadeShadowProjectInfo(ViewInfo &view, LightInfo const &light, float depthParam[2], Matrix4 &worldToLight, Matrix4 &shadowProject);

		static void determineCascadeSplitDist(float nearDist , float farDist , int numCascade , float lambda  , float outDist[]);

		void drawShadowTexture(LightType type);
		void reload()
		{
			for( int i = 0; i < 3; ++i )
			{
				mProgShadowDepth[i].reload();
			}
			mProgLighting.reload();
		}

		virtual void setWorld(Matrix4 const& mat)
		{
			RenderParam::setWorld(mat);
			if( mEffectCur == &mProgLighting )
			{
				mEffectCur->setParam(SHADER_PARAM(VertexFactoryParams.localToWorld), mat);
			}
		}

		virtual ShaderProgram* getMaterialShader(Material& material)
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
		
		GlobalShader* mEffectCur;

		Mesh          mCubeMesh;
		GlobalShader  mEffectCube;

		Texture2DRHI     mShadowMap2;
		TextureCubeRHI   mShadowMap;
		Texture2DRHI     mCascadeTexture;
		FrameBuffer   mShadowBuffer;

		GlobalShader  mProgShadowDepth[3];
		GlobalShader  mProgLighting;

		Matrix4       mShadowMatrix;

		DepthRenderBuffer depthBuffer1;
		DepthRenderBuffer depthBuffer2;
	};

	class EnvTech
	{
	public:

		bool init()
		{
			if( !mBuffer.create() )
				return false;

			if( !mTexEnv.create(Texture::eRGBA32F, MapSize, MapSize) )
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

		TextureCubeRHI mTexSky;
		TextureCubeRHI mTexEnv;


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

			mBuffer.setTexture(0, mReflectMap);
			mBuffer.bind();
			//scene.render(view, *this);
			mBuffer.unbind();

			glDisable(GL_CLIP_PLANE0);
			glPopMatrix();
			glViewport(vp[0], vp[1], vp[2], vp[3]);
		}

		Vector4     waterPlane;

		Mesh        mWaterMesh;
		Texture2DRHI   mReflectMap;
		Texture2DRHI   mRefractMap;
		FrameBuffer mBuffer;
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
		void buildLights();

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
		GpuSync mGpuSync;

		GL::GlobalShader mProgPlanet;
		GL::GlobalShader mProgSphere;
		GL::GlobalShader mProgBump;
		GL::GlobalShader mProgParallax;
		GL::GlobalShader mEffectSphereSM;
		GL::GlobalShader mEffectSimple;

		Mesh   mMesh;
		Mesh   mSphereMesh;
		Mesh   mSphereMesh2;
		Mesh   mSpherePlane;
		Mesh   mBoxMesh;
		Mesh   mPlane;
		Mesh   mDoughnutMesh;
		Mesh   mFrustumMesh;

		int  mNumLightDraw = 4;
		static int const LightNum = 4;
		LightInfo  mLights[LightNum];
		CycleTrack mTracks[4];

		Vector3 mPos;
		GL::Camera  mCamStorage[2];
		GL::Camera* mCamera;
		ViewFrustum mViewFrustum;

		static int const NumAabb = 3;
		AABB    mAabb[3];
		bool    mIsIntersected;
		Vector3 mIntersectPos;
		Vector3 rayStart , rayEnd;


		typedef std::shared_ptr< Material > MaterialPtr;
		std::vector< MaterialPtr > mMaterials;
		typedef std::shared_ptr< Texture2DRHI > Texture2DPtr;
		std::vector< Texture2DPtr > mTextures;
		typedef std::shared_ptr< Mesh > MeshPtr;
		std::vector< MeshPtr > mMeshs;


		Material&  getMaterial(int idx)
		{ 
			if ( mMaterials[idx] )
				return *mMaterials[idx]; 
			return mEmptyMaterial;
		}
		Texture2DRHI& getTexture(int idx) 
		{ 
			if( mTextures[idx] )
				return *mTextures[idx];
			return mEmptyTexture;
		}
		Mesh&      getMesh(int idx) 
		{ 
			if( mMeshs[idx] )
				return *mMeshs[idx];
			return mEmptyMesh;
		}

		bool  mLineMode;
		ShadowDepthTech mShadowTech;
		DefferredLightingTech mDefferredLightingTech;
		GBufferParamData mGBufferParamData;
		bool   mbShowBuffer;
		
		Mesh   mSkyMesh;
		TextureCubeRHI mTexSky;

		Material  mEmptyMaterial;
		Texture2DRHI mEmptyTexture;
		Mesh      mEmptyMesh;
		
		Texture2DRHI mTexBase;
		Texture2DRHI mTexNormal;

		bool   mPause;
		float  mTime;
		float  mRealTime;
		int    mIdxChioce;

		bool initFrameBuffer( Vec2i const& size );


		Texture2DRHI&  getRenderFrameTexture() { return mFrameTextures[mIdxRenderFrameTexture]; }
		Texture2DRHI&  getPrevRednerFrameTexture() { return mFrameTextures[1 - mIdxRenderFrameTexture]; }
		void swapFrameBufferTexture()
		{
			mIdxRenderFrameTexture = 1 - mIdxRenderFrameTexture;
			mFrameBuffer.setTexture(0, getRenderFrameTexture());
		}
		Texture2DRHI   mFrameTextures[2];
		int         mIdxRenderFrameTexture;
		FrameBuffer mFrameBuffer;


		bool bInitialized = false;

	};





}//namespace RenderGL


#endif // RenderGLStage_H_A0BE1A79_C2DE_4703_A032_404ED3032080
