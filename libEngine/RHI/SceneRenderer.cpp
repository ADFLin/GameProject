#include "SceneRenderer.h"

#include "GpuProfiler.h"

#include "DrawUtility.h"
#include "ShaderCompiler.h"
#include "RHICommand.h"

#include "Scene.h"

#include <algorithm>

namespace RenderGL
{
	void ViewInfo::setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix)
	{
		worldToView = inViewMatrix;
		float det;
		worldToView.inverse(viewToWorld, det);
		worldPos = TransformPosition(Vector3(0, 0, 0), viewToWorld);
		viewToClip = inProjectMatrix;
		worldToClip = worldToView * viewToClip;

		viewToClip.inverse(clipToView, det);
		clipToWorld = clipToView * viewToWorld;

		direction = TransformVector(Vector3(0, 0, -1), viewToWorld);

		updateFrustumPlanes();

		mbDataDirty = true;
	}

	Vec2i ViewInfo::getViewportSize() const
	{
		int values[4];
		glGetIntegerv(GL_VIEWPORT, values);
		return Vec2i(values[2], values[3]);
	}

	void ViewInfo::setupShader(ShaderProgram& program)
	{
		//ref ViewParam.glsl
#if 1
		struct ViewBufferData
		{
			DECLARE_UNIFORM_STRUCT(ViewBlock);

			Matrix4  worldToView;
			Matrix4  worldToClip;
			Matrix4  viewToWorld;
			Matrix4  viewToClip;
			Matrix4  clipToView;
			Matrix4  clipToWorld;
			Vector4  rectPosAndSizeInv;
			Vector3 worldPos;
			float  realTime;
			Vector3 direction;
			float  gameTime;

		};

		if( !mUniformBuffer.isValid() )
		{
			mUniformBuffer = RHICreateUniformBuffer(sizeof(ViewBufferData));
		}

		if ( mbDataDirty )
		{
			mbDataDirty = false;

			void* ptr = mUniformBuffer->lock(ELockAccess::WriteOnly);
			ViewBufferData& data = *(ViewBufferData*)ptr;
			data.worldPos = worldPos;
			data.direction = direction;
			data.worldToView = worldToView;
			data.worldToClip = worldToClip;
			data.viewToWorld = viewToWorld;
			data.viewToClip = viewToClip;
			data.clipToView = clipToView;
			data.clipToWorld = clipToWorld;
			data.gameTime = gameTime;
			data.realTime = realTime;
			data.rectPosAndSizeInv.x = rectOffset.x;
			data.rectPosAndSizeInv.y = rectOffset.y;
			data.rectPosAndSizeInv.z = 1.0 / float(rectSize.x);
			data.rectPosAndSizeInv.w = 1.0 / float(rectSize.y);
			mUniformBuffer->unlock();
		}

		program.setUniformBufferT<ViewBufferData>(*mUniformBuffer);

#else
		program.setParam(SHADER_PARAM(View.worldPos), worldPos);
		program.setParam(SHADER_PARAM(View.direction), direction);
		program.setParam(SHADER_PARAM(View.worldToView), worldToView);
		program.setParam(SHADER_PARAM(View.worldToClip), worldToClip);
		program.setParam(SHADER_PARAM(View.viewToWorld), viewToWorld);
		program.setParam(SHADER_PARAM(View.viewToClip), viewToClip);
		program.setParam(SHADER_PARAM(View.clipToView), clipToView);
		program.setParam(SHADER_PARAM(View.clipToWorld), clipToWorld);
		program.setParam(SHADER_PARAM(View.gameTime), gameTime);
		program.setParam(SHADER_PARAM(View.realTime), realTime);
		int values[4];
		glGetIntegerv(GL_VIEWPORT, values);
		Vector4 viewportParam;
		viewportParam.x = values[0];
		viewportParam.y = values[1];
		viewportParam.z = 1.0 / values[2];
		viewportParam.w = 1.0 / values[3];
		program.setParam(SHADER_PARAM(View.viewportPosAndSizeInv), viewportParam);
#endif
	}

	void ViewInfo::updateFrustumPlanes()
	{
		//#NOTE: Dependent RHI
		Vector3 centerNearPos = (Vector4(0, 0, -1, 1) * clipToWorld).dividedVector();
		Vector3 centerFarPos = (Vector4(0, 0, 1, 1) * clipToWorld).dividedVector();

		Vector3 posRT = (Vector4(1, 1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posLB = (Vector4(-1, -1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posRB = (Vector4(1, -1, 1, 1) * clipToWorld).dividedVector();
		Vector3 posLT = (Vector4(-1, 1, 1, 1) * clipToWorld).dividedVector();

		frustumPlanes[0] = Plane(-direction, centerNearPos); //ZFar;
		frustumPlanes[1] = Plane(direction, centerFarPos); //ZNear
		frustumPlanes[2] = Plane(worldPos, posRT, posLT); //top
		frustumPlanes[3] = Plane(worldPos, posLB, posRB); //bottom
		frustumPlanes[4] = Plane(worldPos, posLT, posLB); //left
		frustumPlanes[5] = Plane(worldPos, posRB, posRT); //right
	}

	void LightInfo::setupShaderGlobalParam(ShaderProgram& shader) const
	{
		shader.setParam(SHADER_PARAM(GLight.worldPosAndRadius), Vector4(pos, radius));
		shader.setParam(SHADER_PARAM(GLight.color), intensity * color);
		shader.setParam(SHADER_PARAM(GLight.type), int(type));
		shader.setParam(SHADER_PARAM(GLight.bCastShadow), int(bCastShadow));
		shader.setParam(SHADER_PARAM(GLight.dir), Math::GetNormal(dir));

		Vector3 spotParam;
		float angleInner = Math::Min(spotAngle.x, spotAngle.y);
		spotParam.x = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, angleInner)));
		spotParam.y = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, spotAngle.y)));
		shader.setParam(SHADER_PARAM(GLight.spotParam), spotParam);
	}

	bool ShadowDepthTech::init()
	{
		if( !mShadowBuffer.create() )
			return false;

		mShadowMap = new RHITextureCube;
		if( !mShadowMap->create(Texture::eFloatRGBA, ShadowTextureSize, ShadowTextureSize) )
			return false;


		mShadowMap->bind();
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mShadowMap->unbind();

		mShadowMap2 = RHICreateTexture2D(Texture::eFloatRGBA, ShadowTextureSize, ShadowTextureSize);
		if( !mShadowMap2.isValid() )
			return false;

		mCascadeTexture = RHICreateTexture2D(Texture::eFloatRGBA, CascadeTextureSize * CascadedShadowNum, CascadeTextureSize);
		if( !mCascadeTexture.isValid() )
			return false;

		int sizeX = Math::Max(CascadedShadowNum * CascadeTextureSize, ShadowTextureSize);
		int sizeY = Math::Max(CascadeTextureSize, ShadowTextureSize);

		depthBuffer1 = new RHIDepthRenderBuffer;
		if( !depthBuffer1->create(sizeX, sizeY, Texture::eDepth32F) )
			return false;
		depthBuffer2 = new RHIDepthRenderBuffer;
		if( !depthBuffer2->create(ShadowTextureSize, ShadowTextureSize, Texture::eDepth32F) )
			return false;

		mShadowMap2->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		mShadowMap2->unbind();

		mShadowBuffer.setDepth(*depthBuffer1);
		mShadowBuffer.addTexture(*mShadowMap, Texture::eFaceX);
		//mBuffer.addTexture( mShadowMap2 );
		//mBuffer.addTexture( mShadowMap , Texture::eFaceX , false );

#if 0
		if( !ShaderManager::getInstance().loadFile( mProgShadowDepth[0],"Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgShadowDepth[1],"Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgShadowDepth[2],"Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgLighting , "Shader/ShadowLighting") )
			return false;
#endif

		depthParam[0] = 0.001;
		depthParam[1] = 100;

		return true;
	}

	void ShadowDepthTech::drawShadowTexture(LightType type , Vec2i const& pos , int length )
	{
		ViewportSaveScope vpScope;

		Matrix4 porjectMatrix = OrthoMatrix(0, vpScope[2], 0, vpScope[3], -1, 1);

		MatrixSaveScope matrixScopt(porjectMatrix);

		switch( type )
		{
		case LightType::Spot:
			DrawUtility::DrawTexture(*mShadowMap2, pos, Vec2i(length, length));
			break;
		case LightType::Directional:
			DrawUtility::DrawTexture(*mCascadeTexture, pos, Vec2i(length * CascadedShadowNum, length));
			break;
		case LightType::Point:
			DrawUtility::DrawCubeTexture(*mShadowMap, pos, length / 2);
			//ShaderHelper::drawCubeTexture(GWhiteTextureCube, Vec2i(0, 0), length / 2);
		default:
			break;
		}
	}

	void ShadowDepthTech::reload()
	{
		for( int i = 0; i < 3; ++i )
		{
			ShaderManager::Get().reloadShader(mProgShadowDepth[i]);
		}
		ShaderManager::Get().reloadShader(mProgLighting);
	}

	void ShadowDepthTech::renderLighting(ViewInfo& view, SceneInterface& scene, LightInfo const& light, bool bMultiple)
	{

		ShadowProjectParam shadowPrjectParam;
		shadowPrjectParam.setupLight(light);
		renderShadowDepth(view, scene, shadowPrjectParam);

		GL_BIND_LOCK_OBJECT(mProgLighting);
		//mProgLighting.setTexture(SHADER_PARAM(texSM) , mShadowMap2 , 0 );

		view.setupShader(mProgLighting);
		light.setupShaderGlobalParam(mProgLighting);

		//mProgLighting.setParam(SHADER_PARAM(worldToLightView) , worldToLightView );
		mProgLighting.setTexture(SHADER_PARAM(ShadowTextureCube), *mShadowMap);
		mProgLighting.setParam(SHADER_PARAM(DepthParam), Vector2( depthParam[0], depthParam[1] ));

		mEffectCur = &mProgLighting;

		if( bMultiple )
		{
			//glDepthFunc(GL_EQUAL);
			RHISetDepthStencilState(TStaticDepthStencilState< true , ECompareFun::Equal >::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA , Blend::eOne, Blend::eOne >::GetRHI());
			RenderContext context(view, *this);
			scene.render( context);
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState< true, ECompareFun::Less >::GetRHI());
			//glDepthFunc(GL_LESS);
		}
		else
		{
			RenderContext context(view, *this);
			scene.render( context);
		}
		mEffectCur = nullptr;
	}

	static Vector3 GetUpDir(Vector3 const& dir)
	{
		Vector3 result = dir.cross(Vector3(0, 0, 1));
		if( dir.y * dir.y > 0.99998 )
			return Vector3(1, 0, 0);
		return dir.cross(Vector3(0, 0, 1)).cross(dir);
	}

	void ShadowDepthTech::renderShadowDepth(ViewInfo& view, SceneInterface& scene, ShadowProjectParam& shadowProjectParam)
	{
		//RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
		LightInfo const& light = *shadowProjectParam.light;

		if( !light.bCastShadow )
			return;

		shadowParam = shadowProjectParam.shadowParam;

		ViewInfo lightView;
		lightView = view;
		lightView.mUniformBuffer = nullptr;

#if !USE_MATERIAL_SHADOW
		GL_BIND_LOCK_OBJECT(mProgShadowDepth);
		mEffectCur = &mProgShadowDepth[LIGHTTYPE_POINT];
		mEffectCur->setParam(SHADER_PARAM(DepthParam), depthParam[0], depthParam[1]);
#endif
		Matrix4 biasMatrix(
			0.5, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 0.5, 0,
			0.5, 0.5, 0.5, 1);
		//baisMatrix = Matrix4::Identity();
		Matrix4  worldToLight;
		Matrix4 shadowProject;

		RHISetBlendState(TStaticBlendState<>::GetRHI());

		if( light.type == LightType::Directional )
		{
			GPU_PROFILE("Shadow - Directional");
			
			shadowProjectParam.numCascade = CascadedShadowNum;

			Vector3 farViewPos = (Vector4(0, 0, 1, 1) * view.clipToView).dividedVector();
			Vector3 nearViewPos = (Vector4(0, 0, -1, 1) * view.clipToView).dividedVector();
			float renderMaxDist = std::max(-mCascadeMaxDist - nearViewPos.z, farViewPos.z);

			determineCascadeSplitDist(nearViewPos.z, renderMaxDist, CascadedShadowNum, 0.5, shadowProjectParam.cacadeDepth);

			auto GetDepth = [&view](float value) ->float
			{
				Vector3 pos = (Vector4(0, 0, value, 1) * view.viewToClip).dividedVector();
				return pos.z;
			};

			shadowProjectParam.shadowTexture = mCascadeTexture;
			mShadowBuffer.setDepth(*depthBuffer1);
			mShadowBuffer.setTexture(0, *mCascadeTexture);

			GL_BIND_LOCK_OBJECT(mShadowBuffer);

			glClearDepth(1);
			glClearColor(1, 1, 1, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());

			for( int i = 0; i < CascadedShadowNum; ++i )
			{
				float delata = 1.0f / CascadedShadowNum;
				float depthParam[2];

				if( i == 0 )
					depthParam[0] = GetDepth(nearViewPos.z);
				else
					depthParam[0] = GetDepth(shadowProjectParam.cacadeDepth[i - 1]);
				depthParam[1] = GetDepth(shadowProjectParam.cacadeDepth[i]);

				calcCascadeShadowProjectInfo(view, light, depthParam, worldToLight, shadowProject);

				MatrixSaveScope matScope(shadowProject);

				ViewportSaveScope vpScope;
				RHISetViewport(i*CascadeTextureSize, 0, CascadeTextureSize, CascadeTextureSize);

				lightView.setupTransform(worldToLight, shadowProject);

				Matrix4 corpMatrix = Matrix4(
					delata, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					i * delata, 0, 0, 1);
				mShadowMatrix = worldToLight * shadowProject * biasMatrix;

				shadowProjectParam.shadowMatrix[i] = mShadowMatrix * corpMatrix;
				RenderContext context(lightView, *this);
				scene.render( context );
			}
		}
		else if( light.type == LightType::Spot )
		{
			GPU_PROFILE("Shadow-Spot");
			shadowProject = PerspectiveMatrix(Math::Deg2Rad(2.0 * Math::Min<float>(89.99, light.spotAngle.y)), 1.0, 0.01, light.radius);
			MatrixSaveScope matScope(shadowProject);

			worldToLight = LookAtMatrix(light.pos, light.dir, GetUpDir(light.dir));
			mShadowMatrix = worldToLight * shadowProject * biasMatrix;
			shadowProjectParam.shadowMatrix[0] = mShadowMatrix;

			shadowProjectParam.shadowTexture = mShadowMap2;
			mShadowBuffer.setDepth(*depthBuffer2);
			mShadowBuffer.setTexture(0, *mShadowMap2);
			GL_BIND_LOCK_OBJECT(mShadowBuffer);

			ViewportSaveScope vpScope;
			RHISetViewport(0, 0, ShadowTextureSize, ShadowTextureSize);
			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			glClearDepth(1);
			glClearColor(1, 1, 1, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lightView.setupTransform(worldToLight, shadowProject);
			RenderContext context(lightView, *this);
			scene.render( context );

		}
		else if( light.type == LightType::Point )
		{
			GPU_PROFILE("Shadow-Point");
			shadowProject = PerspectiveMatrix(Math::Deg2Rad(90), 1.0, 0.01, light.radius);
			MatrixSaveScope matScope(shadowProject);

			Matrix4 biasMatrix(
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 0.5, 0,
				0, 0, 0.5, 1);
			Vector3 CubeFaceDir[] =
			{
				Vector3(1,0,0),Vector3(-1,0,0),
				Vector3(0,1,0),Vector3(0,-1,0),
				Vector3(0,0,1),Vector3(0,0,-1),
			};

			Vector3 CubeUpDir[] =
			{
				Vector3(0,-1,0),Vector3(0,-1,0),
				Vector3(0,0,1),Vector3(0,0,-1),
				Vector3(0,-1,0),Vector3(0,-1,0),
			};

			shadowProjectParam.shadowTexture = mShadowMap;

			ViewportSaveScope vpScope;
			RHISetViewport(0, 0, ShadowTextureSize, ShadowTextureSize);
			mShadowBuffer.setDepth(*depthBuffer2);

			RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
			for( int i = 0; i < 6; ++i )
			{
				//GPU_PROFILE("Face");
				mShadowBuffer.setTexture(0, *mShadowMap, Texture::Face(i));
				GL_BIND_LOCK_OBJECT(mShadowBuffer);
				{
					//GPU_PROFILE("Clear");
					glClearDepth(1);
					glClearColor(1, 1, 1, 1.0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				}

				worldToLight = LookAtMatrix(light.pos, CubeFaceDir[i], CubeUpDir[i]);
				mShadowMatrix = worldToLight * shadowProject * biasMatrix;
				shadowProjectParam.shadowMatrix[i] = mShadowMatrix;

				{
					//GPU_PROFILE("DrawMesh");
					lightView.setupTransform(worldToLight, shadowProject);
					RenderContext context(lightView, *this);
					scene.render( context );
				}

			}
		}
	}


	void ShadowDepthTech::determineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[])
	{
		for( int i = 1; i <= numCascade; ++i )
		{
			float Clog = nearDist * Math::Pow(farDist / nearDist, float(i) / numCascade);
			float Cuni = nearDist + (farDist - nearDist) * (float(i) / numCascade);
			outDist[i - 1] = Math::Lerp(Clog, Cuni, lambda);
		}
	}


	void ShadowDepthTech::calcCascadeShadowProjectInfo(ViewInfo &view, LightInfo const &light, float depthParam[2], Matrix4 &worldToLight, Matrix4 &shadowProject)
	{
		Vector3 boundPos[8];
		int idx = 0;
		for( int i = -1; i <= 1; i += 2 )
		{
			for( int j = -1; j <= 1; j += 2 )
			{
				for( int k = 0; k < 2; ++k )
				{
					Vector4 v(i, j, depthParam[k], 1);
					v = v * view.clipToWorld;
					boundPos[idx] = v.dividedVector();
					++idx;
				}
			}
		}
		Vector3 zAxis = Math::GetNormal(-light.dir);
		Vector3 viewDir = view.getViewForwardDir();
		Vector3 xAxis = viewDir.cross(zAxis);
		if( xAxis.length2() < 1e-4 )
		{
			xAxis = view.getViewRightDir();
		}
		xAxis.normalize();
		Vector3 yAxis = zAxis.cross(xAxis);
		yAxis.normalize();
		float MaxLen = 10000000;
		Vector3 Vmin = Vector3(MaxLen, MaxLen, MaxLen);
		Vector3 Vmax = -Vmin;
		for( int i = 0; i < 8; ++i )
		{
			Vector3 v;
			v.x = xAxis.dot(boundPos[i]);
			v.y = yAxis.dot(boundPos[i]);
			v.z = zAxis.dot(boundPos[i]);
			Vmin.min(v);
			Vmax.max(v);
		}

		Vector3 Vmid = 0.5 * (Vmax + Vmin);
		Vector3 viewPos = Vmid.x * xAxis + Vmid.y * yAxis;
		Vector3 fBoundVolume = Vmax - Vmin;

		viewPos = Vector3::Zero();
		worldToLight = LookAtMatrix(viewPos, light.dir, yAxis);
		shadowProject = OrthoMatrix(Vmin.x, Vmax.x, Vmin.y, Vmax.y, -mCascadeMaxDist / 2, Vmax.z);
	}

	bool DefferredShadingTech::init( SceneRenderTargets& sceneRenderTargets )
	{
		mSceneRenderTargets = &sceneRenderTargets;

		GBufferParamData& GBuffer = sceneRenderTargets.getGBuffer();

		if( !mBassPassBuffer.create() )
			return false;
		if( !mLightBuffer.create() )
			return false;

		mBassPassBuffer.addTexture(*GBuffer.textures[0]);
		for( int i = 0; i < GBufferParamData::NumBuffer; ++i )
		{
			mBassPassBuffer.addTexture(*GBuffer.textures[i]);
		}

		mBassPassBuffer.setDepth(sceneRenderTargets.getDepthTexture());

		mLightBuffer.addTexture( sceneRenderTargets.getRenderFrameTexture());
		mLightBuffer.setDepth( sceneRenderTargets.getDepthTexture() );


		if( !MeshBuild::LightSphere(mSphereMesh) ||
		    !MeshBuild::LightCone(mConeMesh) )
			return false;

		if( !ShaderManager::Get().loadFile( 
			 mProgLightingScreenRect[(int)LightType::Point] ,
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT") )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgLightingScreenRect[(int)LightType::Spot] ,
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_SPOT \n") )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgLightingScreenRect[(int)LightType::Directional] ,
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_DIRECTIONAL\n") )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgLighting[(int)LightType::Point],
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT \n"
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n") )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgLighting[(int)LightType::Spot],
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_SPOT \n"
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n") )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgLighting[(int)LightType::Directional],
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_DIRECTIONAL \n"
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n") )
			return false;

		if( !ShaderManager::Get().loadFile(
			mProgLightingShowBound,
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(ShowBoundPS),
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n") )
			return false;

		return true;
	}

	void DefferredShadingTech::renderBassPass(ViewInfo& view, SceneInterface& scene)
	{
		mBassPassBuffer.setTexture(0, mSceneRenderTargets->getRenderFrameTexture());
		GL_BIND_LOCK_OBJECT(mBassPassBuffer);
		float const depthValue = 1.0;
		{
			GPU_PROFILE("Clear Buffer");
			mBassPassBuffer.clearBuffer(&Vector4(0, 0, 0, 1), &depthValue);
		}
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
		RenderContext context(view, *this);
		scene.render( context );
	}
	

	void DefferredShadingTech::prevRenderLights(ViewInfo& view)
	{
		//#PROB GL Can't Save Uniform Params
#if 0
		auto const SetupShaderParam = [this, &view](ShaderProgram& program)
		{
			program.bind();
			mSceneRenderTargets->setupShaderGBuffer(program, true);
			view.setupShader(program);
			program.unbind();
		};

		for( int i = 0; i < 3; ++i )
		{
			SetupShaderParam(mProgLightingScreenRect[i]);
			SetupShaderParam(mProgLighting[i]);
		}
		SetupShaderParam(mProgLightingShowBound);
#endif
	}

	void DefferredShadingTech::renderLight(ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam)
	{

		auto const BindShaderParam = [this , &view , &light , &shadowProjectParam](DefferredLightingProgram& program)
		{
			program.setParamters(*mSceneRenderTargets);
			shadowProjectParam.setupShader(program);
			view.setupShader(program);
			light.setupShaderGlobalParam(program);
		};

		if( boundMethod != LBM_SCREEN_RECT && light.type != LightType::Directional )
		{
			DefferredLightingProgram& lightShader = (debugMode == DebugMode::eNone) ? mProgLighting[ (int)light.type ] : mProgLightingShowBound;

			mLightBuffer.setTexture(0, mSceneRenderTargets->getRenderFrameTexture());

			Mesh* boundMesh;
			Matrix4 lightXForm;
			switch( light.type )
			{
			case LightType::Point:
				lightXForm = Matrix4::Scale(light.radius) * Matrix4::Translate(light.pos) * view.worldToView;
				boundMesh = &mSphereMesh;
				break;
			case LightType::Spot:
				{
					float factor = Math::Tan( Math::Deg2Rad( light.spotAngle.y ) );
					lightXForm = Matrix4::Scale(light.radius * Vector3( factor , factor , 1 ) ) * BasisMaterix::FromZ(light.dir) * Matrix4::Translate(light.pos) * view.worldToView;
					boundMesh = &mConeMesh;
				}
				break;
			}

			assert(boundMesh);

			auto const DrawBoundShape = [this,boundMesh,&lightXForm]( bool bUseShader )
			{
				glPushMatrix();
				glLoadMatrixf(lightXForm);
				boundMesh->draw();
				glPopMatrix();
			};

			constexpr bool bWriteDepth = false;

			if( boundMethod == LBM_GEMO_BOUND_SHAPE_WITH_STENCIL )
			{
				constexpr bool bEnableStencilTest = true;
				mLightBuffer.bindDepthOnly();
				glClearStencil(1);
				glClear(GL_STENCIL_BUFFER_BIT);
				mLightBuffer.unbind();

				RHISetBlendState(TStaticBlendState< CWM_NONE >::GetRHI());
				//if ( debugMode != DebugMode::eShowVolume )
				{
					RHISetRasterizerState(TStaticRasterizerState< ECullMode::Back >::GetRHI());
					RHISetDepthStencilState(
						TStaticDepthStencilState<
							bWriteDepth, ECompareFun::Greater,
							bEnableStencilTest, ECompareFun::Always,
							Stencil::eKeep, Stencil::eKeep, Stencil::eDecr, 0x0 
						>::GetRHI(), 0x0);

					mLightBuffer.bindDepthOnly();
					DrawBoundShape(false);
					mLightBuffer.unbind();

				}

				RHISetBlendState(TStaticBlendState< CWM_RGBA >::GetRHI());
				if( debugMode == DebugMode::eShowVolume )
				{
					RHISetDepthStencilState(
						TStaticDepthStencilState<
							bWriteDepth, ECompareFun::Always,
							bEnableStencilTest, ECompareFun::Equal,
							Stencil::eKeep, Stencil::eKeep, Stencil::eKeep, 0x1
						>::GetRHI(), 0x1);
				}
				else
				{
					RHISetDepthStencilState(
						TStaticDepthStencilState<
							bWriteDepth, ECompareFun::GeraterEqual,
							bEnableStencilTest, ECompareFun::Equal,
							Stencil::eKeep, Stencil::eKeep, Stencil::eKeep, 0x1
						>::GetRHI(), 0x1);
				}
			}
			else
			{
				if( debugMode == DebugMode::eShowVolume )
				{
					RHISetDepthStencilState(TStaticDepthStencilState<bWriteDepth, ECompareFun::Always >::GetRHI());

				}
				else
				{
					RHISetDepthStencilState(TStaticDepthStencilState<bWriteDepth, ECompareFun::GeraterEqual >::GetRHI());
				}
			}

			RHISetBlendState(TStaticBlendState< CWM_RGBA , Blend::eOne, Blend::eOne >::GetRHI());
			RHISetRasterizerState(TStaticRasterizerState< ECullMode::Front >::GetRHI());
			{
				GL_BIND_LOCK_OBJECT(mLightBuffer);
				GL_BIND_LOCK_OBJECT(lightShader);
				//if( debugMode == DebugMode::eNone )
				{
					BindShaderParam(lightShader);
				}
				lightShader.setParam(SHADER_PARAM(BoundTransform), lightXForm);
				DrawBoundShape(true);
			}
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
		}
		else
		{
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			RHISetBlendState(TStaticBlendState< CWM_RGBA, Blend::eOne, Blend::eOne >::GetRHI());
			{
				GL_BIND_LOCK_OBJECT(mSceneRenderTargets->getFrameBuffer());
				//MatrixSaveScope matrixScope(Matrix4::Identity());
				DefferredLightingProgram& program = mProgLightingScreenRect[(int)light.type];
				GL_BIND_LOCK_OBJECT(program);
				BindShaderParam(program);
				DrawUtility::ScreenRectShader();

				//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}

		}

	}

	void DefferredShadingTech::reload()
	{
		for( int i = 0; i < 3; ++i )
		{
			ShaderManager::Get().reloadShader(mProgLightingScreenRect[i]);
			ShaderManager::Get().reloadShader(mProgLighting[i]);
		}
		ShaderManager::Get().reloadShader(mProgLightingShowBound);
	}

	bool GBufferParamData::init(Vec2i const& size)
	{
		for( int i = 0; i < NumBuffer; ++i )
		{
			textures[i] = new RHITexture2D;

			if( !textures[i]->create(Texture::eFloatRGBA, size.x, size.y) )
				return false;

			textures[i]->bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			textures[i]->unbind();
		}


		return true;
	}

	void GBufferParamData::setupShader(ShaderProgram& program)
	{
		program.setTexture(SHADER_PARAM(GBufferTextureA), *textures[BufferA]);
		program.setTexture(SHADER_PARAM(GBufferTextureB), *textures[BufferB]);
		program.setTexture(SHADER_PARAM(GBufferTextureC), *textures[BufferC]);
		program.setTexture(SHADER_PARAM(GBufferTextureD), *textures[BufferD]);

	}

	void GBufferParamData::drawTextures( Vec2i const& size , Vec2i const& gapSize )
	{
		int width = size.x;
		int height = size.y;
		int gapX = gapSize.x;
		int gapY = gapSize.y;
		int drawWidth = width - 2 * gapX;
		int drawHeight = height - 2 * gapY;

		drawTexture(0 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, BufferA);
		{
			ViewportSaveScope vpScope;
			RHISetViewport(1 * width + gapX, 0 * height + gapY, drawWidth, drawHeight);
			float colorBias[2] = { 0.5 , 0.5 };
			ShaderHelper::Get().copyTextureBiasToBuffer(*textures[BufferB], colorBias);

		}
		//drawTexture(1 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, BufferB);
		drawTexture(2 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, BufferC);
		drawTexture(3 * width + gapX, 3 * height + gapY, drawWidth, drawHeight, BufferD, Vector4(1, 0, 0, 0));
		drawTexture(3 * width + gapX, 2 * height + gapY, drawWidth, drawHeight, BufferD, Vector4(0, 1, 0, 0));
		drawTexture(3 * width + gapX, 1 * height + gapY, drawWidth, drawHeight, BufferD, Vector4(0, 0, 1, 0));
		{
			ViewportSaveScope vpScope;
			RHISetViewport(3 * width + gapX, 0 * height + gapY, drawWidth, drawHeight);
			float valueFactor[2] = { 255 , 0 };
			ShaderHelper::Get().mapTextureColorToBuffer(*textures[BufferD], Vector4(0, 0, 0, 1), valueFactor);
		}
		//renderDepthTexture(width , 3 * height, width, height);
	}

	void GBufferParamData::drawTexture(int x, int y, int width, int height, int idxBuffer)
	{
		DrawUtility::DrawTexture(*textures[idxBuffer], Vec2i(x, y), Vec2i(width, height));
	}

	void GBufferParamData::drawTexture(int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask)
	{
		ViewportSaveScope vpScope;
		RHISetViewport(x, y, width, height);
		ShaderHelper::Get().copyTextureMaskToBuffer(*textures[idxBuffer], colorMask);
	}

	class SSAOGenerateProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(SSAOGenerateProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SSAO";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(GeneratePS) },
				{ Shader::eEmpty  , nullptr },
			};
			return entries;
		}
	public:
		void bindParameters();
		void setParameters(SceneRenderTargets& sceneRenderTargets, Vector3 kernelVectors[], int numKernelVector);

		GBufferShaderParameters mParamGBuffer;
		ShaderParameter mParamKernelNum;
		ShaderParameter mParamKernelVectors;
		ShaderParameter mParamOcclusionRadius;
	};

	IMPLEMENT_GLOBAL_SHADER(SSAOGenerateProgram)

	class SSAOBlurProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(SSAOBlurProgram)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SSAO";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(BlurPS) },
				{ Shader::eEmpty  , nullptr },
			};
			return entries;
		}
	public:
		void bindParameters();
		void setParameters(RHITexture2D& SSAOTexture);

		ShaderParameter mParamTextureSSAO;
	};

	IMPLEMENT_GLOBAL_SHADER(SSAOBlurProgram)

	class SSAOAmbientProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(SSAOAmbientProgram)
		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SSAO";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(AmbientPS) },
				{ Shader::eEmpty  , nullptr },
			};
			return entries;
		}
	public:
		void bindParameters();
		void setParameters(SceneRenderTargets& sceneRenderTargets, RHITexture2D& SSAOTexture);

		GBufferShaderParameters mParamGBuffer;
		ShaderParameter mParamTextureSSAO;
	};

	IMPLEMENT_GLOBAL_SHADER(SSAOAmbientProgram)

	bool PostProcessSSAO::init(Vec2i const& size)
	{
		if( !mFrameBuffer.create() )
			return false;
		
		mSSAOTexture = new RHITexture2D;
		if( !mSSAOTexture->create(Texture::eFloatRGBA, size.x, size.y) )
			return false;

		mSSAOTexture->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mSSAOTexture->unbind();

		mSSAOTextureBlur = new RHITexture2D;
		if( !mSSAOTextureBlur->create(Texture::eFloatRGBA, size.x, size.y) )
			return false;

		mSSAOTextureBlur->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		mSSAOTextureBlur->unbind();

		mFrameBuffer.addTexture(*mSSAOTexture);

		mProgSSAOGenerate = ShaderManager::Get().getGlobalShaderT< SSAOGenerateProgram >(true);
		if( mProgSSAOGenerate == nullptr )
			return false;
		mProgSSAOBlur = ShaderManager::Get().getGlobalShaderT< SSAOBlurProgram >(true);
		if( mProgSSAOBlur == nullptr )
			return false;
		mProgAmbient = ShaderManager::Get().getGlobalShaderT< SSAOAmbientProgram >(true);
		if( mProgAmbient == nullptr )
			return false;

		generateKernelVectors(NumDefaultKernel);
		return true;
	}

	void PostProcessSSAO::render(ViewInfo& view, SceneRenderTargets& sceneRenderTargets)
	{
		RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
		{
			GPU_PROFILE("SSAO-Generate");
			mFrameBuffer.setTexture(0, *mSSAOTexture);
			GL_BIND_LOCK_OBJECT(mFrameBuffer);
			GL_BIND_LOCK_OBJECT(mProgSSAOGenerate);
			view.setupShader(*mProgSSAOGenerate);
			mProgSSAOGenerate->setParameters( sceneRenderTargets , &mKernelVectors[0], mKernelVectors.size());
			DrawUtility::ScreenRectShader();
		}


		{
			GPU_PROFILE("SSAO-Blur");
			mFrameBuffer.setTexture(0, *mSSAOTextureBlur);
			GL_BIND_LOCK_OBJECT(mFrameBuffer);
			GL_BIND_LOCK_OBJECT(mProgSSAOBlur);
			mProgSSAOBlur->setParameters(*mSSAOTexture);
			DrawUtility::ScreenRectShader();
		}

		{
			GPU_PROFILE("SSAO-Ambient");
				//sceneRenderTargets.swapFrameBufferTexture();

			RHISetBlendState(TStaticBlendState< CWM_RGBA , Blend::eOne, Blend::eOne >::GetRHI());
			GL_BIND_LOCK_OBJECT(sceneRenderTargets.getFrameBuffer());
			GL_BIND_LOCK_OBJECT(mProgAmbient);
			mProgAmbient->setParameters(sceneRenderTargets, *mSSAOTextureBlur);
			DrawUtility::ScreenRectShader();
			
		}
	}

	void PostProcessSSAO::drawSSAOTexture(Vec2i const& pos, Vec2i const& size)
	{
		DrawUtility::DrawTexture(*mSSAOTextureBlur, pos, size);
	}

	void PostProcessSSAO::reload()
	{
		ShaderManager::Get().reloadShader(*mProgSSAOGenerate);
		ShaderManager::Get().reloadShader(*mProgSSAOBlur);
		ShaderManager::Get().reloadShader(*mProgAmbient);
	}

	void ShadowProjectParam::setupLight(LightInfo const& inLight)
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

	void ShadowProjectParam::setupShader(ShaderProgram& program) const
	{
		program.setParam(SHADER_PARAM(ShadowParam), Vector2( shadowParam.x, shadowParam.y ));
		switch( light->type )
		{
		case LightType::Spot:
			program.setParam(SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 1);
			if( light->bCastShadow )
				program.setTexture(SHADER_PARAM(ShadowTexture2D), *(RHITexture2D*)shadowTexture);
			else
				program.setTexture(SHADER_PARAM(ShadowTexture2D), *GWhiteTexture2D);
			break;
		case LightType::Point:
			program.setParam(SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 6);
			if( light->bCastShadow )
				program.setTexture(SHADER_PARAM(ShadowTextureCube), *(RHITextureCube*)shadowTexture);
			else
				program.setTexture(SHADER_PARAM(ShadowTextureCube), *GWhiteTextureCube);
			break;
		case LightType::Directional:
			program.setParam(SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, numCascade);
			if( light->bCastShadow )
				program.setTexture(SHADER_PARAM(ShadowTexture2D), *(RHITexture2D*)shadowTexture);
			else
				program.setTexture(SHADER_PARAM(ShadowTexture2D), *GWhiteTexture2D);
			program.setParam(SHADER_PARAM(NumCascade), numCascade);
			program.setParam(SHADER_PARAM(CacadeDepth), cacadeDepth, numCascade);
			break;
		}
	}

	bool SceneRenderTargets::init(Vec2i const& size)
	{
		mIdxRenderFrameTexture = 0;
		for( int i = 0; i < 2; ++i )
		{
			mFrameTextures[i] = new RHITexture2D;
			if( !mFrameTextures[i]->create(Texture::eFloatRGBA, size.x, size.y) )
				return false;
		}

		if( !mGBuffer.init(size) )
			return false;
		
		mDepthTexture = new RHITextureDepth;
		if( !mDepthTexture->create(Texture::eD32FS8, size.x, size.y) )
			return false;

		mDepthTexture->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		mDepthTexture->unbind();

		if( !mFrameBuffer.create() )
			return false;
		mFrameBuffer.addTexture(getRenderFrameTexture());
		return true;
	}

	void SceneRenderTargets::drawDepthTexture(int x, int y, int width, int height)
	{
		//PROB
		glLoadIdentity();
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mDepthTexture->mHandle);
		glColor3f(1, 1, 1);
		DrawUtility::Rect(x, y, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

	bool OITTechnique::init(Vec2i const& size)
	{
		mColorStorageTexture = RHICreateTexture2D();
		if( !mColorStorageTexture->create(Texture::eRGBA16F, OIT_StorageSize, OIT_StorageSize) )
			return false;
		mNodeAndDepthStorageTexture = RHICreateTexture2D();
		if( !mNodeAndDepthStorageTexture->create(Texture::eRGBA32I, OIT_StorageSize, OIT_StorageSize) )
			return false;
		mNodeHeadTexture = RHICreateTexture2D();
		if( !mNodeHeadTexture->create(Texture::eR32U, size.x, size.y) )
			return false;

		if( !mStorageUsageCounter.create() )
			return false;

		mMeshScreenTri.mDecl.addElement(Vertex::ATTRIBUTE_POSITION, Vertex::eFloat4);
		Vector4 v[] =
		{
			Vector4(0,0,0,1) , Vector4(0.5,0,0,1) , Vector4(0.25,0.5,0,1) ,
		};
		mMeshScreenTri.createBuffer(v, 3);
		mMeshScreenTri.mType = PrimitiveType::TriangleList;


		{
			ShaderCompileOption option;
			option.version = 430;
			option.addDefine(SHADER_PARAM(OIT_STORAGE_SIZE), OIT_StorageSize);
			if( !ShaderManager::Get().loadFile(
				mShaderBassPassTest, "Shader/OITRender",
				SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BassPassPS),
				option, nullptr) )
				return false;
			if( !ShaderManager::Get().loadFile(
				mShaderResolve, "Shader/OITRender",
				SHADER_ENTRY(ScreenVS), SHADER_ENTRY(ResolvePS),
				option, nullptr) )
				return false;
		}

		BMA_InternalValMin[NumBMALevel - 1] = 1;
		for( int i = 0; i < NumBMALevel; ++i )
		{
			if ( i != NumBMALevel - 1 )
				BMA_InternalValMin[i] = BMA_MaxPixelCounts[i + 1] + 1;

			ShaderCompileOption option;
			option.version = 430;
			option.addDefine(SHADER_PARAM(OIT_STORAGE_SIZE), OIT_StorageSize);
			option.addDefine(SHADER_PARAM(OIT_MAX_PIXEL_COUNT) , BMA_MaxPixelCounts[i]);
			if( !ShaderManager::Get().loadFile(
				mShaderBMAResolves[i], "Shader/OITRender",
				SHADER_ENTRY(ScreenVS), SHADER_ENTRY(ResolvePS),
				option , nullptr) )
				return false;
		}

		{

			Vector4 v[] =
			{
				Vector4(1,1,0,1) , Vector4(-1,1,0,1) , Vector4(-1,-1,0,1) , Vector4(1,-1,0,1)
			};
			int indices[] = { 0 , 1 , 2 , 0 , 2 , 3 };
			mScreenMesh.mDecl.addElement(Vertex::ATTRIBUTE_POSITION, Vertex::eFloat4);
			if( !mScreenMesh.createBuffer(v, 4, indices, 6, true) )
				return false;
		}

		mFrameBuffer.create();
		mFrameBuffer.addTexture(*mColorStorageTexture);
		mFrameBuffer.addTexture(*mNodeAndDepthStorageTexture);

		return true;
	}

	void OITTechnique::render(ViewInfo& view, SceneInterface& scnenRender, SceneRenderTargets* sceneRenderTargets)
	{

		auto DrawFun = [this, &view, &scnenRender]()
		{
			RenderContext context(view, *this);
			scnenRender.renderTranslucent(context);
		};

		renderInternal( view, DrawFun , sceneRenderTargets );

		if( 1 )
		{
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			ViewportSaveScope vpScope;
			OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			DrawUtility::DrawTexture(*mColorStorageTexture, Vec2i(0, 0), Vec2i(200, 200));
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}
	}

	void OITTechnique::renderTest(ViewInfo& view, SceneRenderTargets& sceneRenderTargets, Mesh& mesh, Material* material)
	{
		auto DrawFun = [this , &view , &mesh , material ]()
		{
			//GL_BIND_LOCK_OBJECT(sceneRenderTargets.getFrameBuffer());

			Vector4 color[] =
			{
				Vector4(1,0,0,0.1),
				Vector4(0,1,0,0.1),
				Vector4(0,0,1,0.1),
				Vector4(1,1,0,0.1),
				Vector4(1,0,1,0.1),
				Vector4(0,1,1,0.1),
				Vector4(1,1,1,0.1),
			};

			GL_BIND_LOCK_OBJECT(mShaderBassPassTest);
			view.setupShader(mShaderBassPassTest);
			mShaderBassPassTest.setRWTexture(SHADER_PARAM(ColorStorageRWTexture), *mColorStorageTexture, AO_WRITE_ONLY);
			mShaderBassPassTest.setRWTexture(SHADER_PARAM(NodeAndDepthStorageRWTexture), *mNodeAndDepthStorageTexture, AO_READ_AND_WRITE);
			mShaderBassPassTest.setRWTexture(SHADER_PARAM(NodeHeadRWTexture), *mNodeHeadTexture, AO_READ_AND_WRITE);


			//
			mShaderBassPassTest.setParam(SHADER_PARAM(BaseColor), color[0]);

			mShaderBassPassTest.setParam(SHADER_PARAM(WorldTransform), Matrix4::Identity());
			mesh.drawShader();
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			mShaderBassPassTest.setParam(SHADER_PARAM(WorldTransform), Matrix4::Translate(Vector3(10,0,0)));
			mesh.drawShader();
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			mShaderBassPassTest.setParam(SHADER_PARAM(WorldTransform), Matrix4::Translate(Vector3(-10, 0, 0)));
			mesh.drawShader();
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			
		};

		renderInternal(view, DrawFun);


		if (1)
		{
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			ViewportSaveScope vpScope;
			OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());
			DrawUtility::DrawTexture(*mColorStorageTexture, Vec2i(0, 0), Vec2i(200,200));
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}

	}

	void OITTechnique::reload()
	{
		ShaderManager::Get().reloadShader(mShaderBassPassTest);
		ShaderManager::Get().reloadShader(mShaderResolve);
		for( int i = 0; i < NumBMALevel; ++i )
			ShaderManager::Get().reloadShader(mShaderBMAResolves[i]);
	}

	void OITTechnique::renderInternal(ViewInfo& view, std::function< void() > drawFuncion , SceneRenderTargets* sceneRenderTargets )
	{
		GPU_PROFILE("OIT");
		
		RHISetRasterizerState(TStaticRasterizerState< ECullMode::None >::GetRHI());
		constexpr bool bWriteDepth = false;

		if ( 1 )
		{
			GPU_PROFILE("ClearStorage");
			GLfloat clearValueA[] = { 0 ,0, 0, 1 };
			GLint clearValueB[] = { 0 ,0, 0, 0 };
			GLuint clearValueC[] = { 0 ,0, 0, 0 };

			if ( 0 )
			{
				if( 1 )
				{
					ShaderHelper::Get().clearBuffer(*mColorStorageTexture, clearValueA);
					ShaderHelper::Get().clearBuffer(*mNodeAndDepthStorageTexture, clearValueB);
				}
				else
				{
					GL_BIND_LOCK_OBJECT(mFrameBuffer);
					ViewportSaveScope vpScope;
					RHISetViewport(0, 0, OIT_StorageSize, OIT_StorageSize);
					glClearBufferfv(GL_COLOR, 0, clearValueA);
					glClearBufferiv(GL_COLOR, 1, clearValueB);
					//glClearBufferuiv(GL_COLOR, 2, clearValueC);
				}
			}

			ShaderHelper::Get().clearBuffer(*mNodeHeadTexture, clearValueC);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		if( sceneRenderTargets )
			sceneRenderTargets->getFrameBuffer().bind();

		if( bUseBMA )
		{
			glClearStencil(0);
			glClear(GL_STENCIL_BUFFER_BIT);

			RHISetDepthStencilState(
				TStaticDepthStencilState< 
					bWriteDepth , ECompareFun::Less , 
					true , ECompareFun::Always , Stencil::eKeep , Stencil::eKeep , Stencil::eIncr , 0xff
				>::GetRHI(), 0 );

		}
		else
		{
			RHISetDepthStencilState( TStaticDepthStencilState< bWriteDepth >::GetRHI());
		}
		if( 1 )
		{
			GPU_PROFILE("BasePass");
			RHISetBlendState(TStaticBlendState<CWM_NONE>::GetRHI());
			mStorageUsageCounter.setValue(0);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			mStorageUsageCounter.bind();

			//GL_BIND_LOCK_OBJECT(sceneRenderTargets.getFrameBuffer());
			drawFuncion();

			mStorageUsageCounter.unbind();
			glFlush();
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}

		//if(0 )
		{
			GPU_PROFILE("Resolve");

			RHISetBlendState(TStaticBlendState<CWM_RGBA, Blend::eSrcAlpha, Blend::eOneMinusSrcAlpha >::GetRHI());
			ViewportSaveScope vpScope;
			OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			if( bUseBMA )
			{
				
				for( int i = 0; i < NumBMALevel; ++i )
				{
					GPU_PROFILE_VA("BMA=%d", BMA_MaxPixelCounts[i]);
					RHISetDepthStencilState(
						TStaticDepthStencilState< bWriteDepth, ECompareFun::Always ,
							true , ECompareFun::LessEqual , Stencil::eKeep , Stencil::eZero , Stencil::eZero , 0xff 
						>::GetRHI() , BMA_InternalValMin[i]);

					BMAResolveProgram& shaderprogram = mShaderBMAResolves[i];
					GL_BIND_LOCK_OBJECT(shaderprogram);
					shaderprogram.setParameters(*mColorStorageTexture, *mNodeAndDepthStorageTexture, *mNodeHeadTexture);
					mScreenMesh.drawShader();
				}

			}
			else
			{
				RHISetDepthStencilState(TStaticDepthStencilState< bWriteDepth , ECompareFun::Always >::GetRHI());

				BMAResolveProgram& shaderprogram = mShaderBMAResolves[0];
				GL_BIND_LOCK_OBJECT(shaderprogram);
				shaderprogram.setParameters(*mColorStorageTexture, *mNodeAndDepthStorageTexture, *mNodeHeadTexture);
				mScreenMesh.drawShader();
			}

			
		}

		if( sceneRenderTargets )
			sceneRenderTargets->getFrameBuffer().unbind();

		RHISetBlendState(TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<>::GetRHI());
		RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	}

	void OITTechnique::setupShader(ShaderProgram& program)
	{
		program.setRWTexture(SHADER_PARAM(ColorStorageRWTexture), *mColorStorageTexture, AO_WRITE_ONLY);
		program.setRWTexture(SHADER_PARAM(NodeAndDepthStorageRWTexture), *mNodeAndDepthStorageTexture, AO_READ_AND_WRITE);
		program.setRWTexture(SHADER_PARAM(NodeHeadRWTexture), *mNodeHeadTexture, AO_READ_AND_WRITE);
	}

	MaterialShaderProgram* OITTechnique::getMaterialShader(RenderContext& context, MaterialMaster& material, VertexFactory* vertexFactory)
	{
		return material.getShader(RenderTechiqueUsage::OIT , vertexFactory);
	}

	void OITTechnique::setupMaterialShader(RenderContext& context, ShaderProgram& program)
	{
		setupShader(program);
	}

	void BMAResolveProgram::bindParameters()
	{
		mParamColorStorageTexture.bind(*this, SHADER_PARAM(ColorStorageTexture));
		mParamNodeAndDepthStorageTexture.bind(*this, SHADER_PARAM(NodeAndDepthStorageTexture));
		mParamNodeHeadTexture.bind(*this, SHADER_PARAM(NodeHeadTexture));
	}

	void BMAResolveProgram::setParameters(RHITexture2D& ColorStorageTexture, RHITexture2D& NodeAndDepthStorageTexture, RHITexture2D& NodeHeadTexture)
	{
		setRWTexture(mParamColorStorageTexture, ColorStorageTexture, AO_READ_AND_WRITE);
		setRWTexture(mParamNodeAndDepthStorageTexture, NodeAndDepthStorageTexture, AO_READ_AND_WRITE);
		setRWTexture(mParamNodeHeadTexture, NodeHeadTexture, AO_READ_AND_WRITE);
	}

	void SSAOGenerateProgram::bindParameters()
	{
		mParamGBuffer.bindParameters(*this , true);
		mParamKernelNum.bind(*this, SHADER_PARAM(KernelNum));
		mParamKernelVectors.bind(*this, SHADER_PARAM(KernelVectors));
		mParamOcclusionRadius.bind(*this, SHADER_PARAM(OcclusionRadius));
	}

	void SSAOGenerateProgram::setParameters(SceneRenderTargets& sceneRenderTargets, Vector3 kernelVectors[], int numKernelVector)
	{
		mParamGBuffer.setParameters(*this, sceneRenderTargets);
		setParam(mParamKernelNum, (int)numKernelVector);
		setParam(mParamKernelVectors, &kernelVectors[0], numKernelVector);
		setParam(mParamOcclusionRadius, 0.5f);
	}

	void SSAOBlurProgram::bindParameters()
	{
		mParamTextureSSAO.bind(*this, SHADER_PARAM(TextureSSAO));
	}

	void SSAOBlurProgram::setParameters(RHITexture2D& SSAOTexture)
	{
		setTexture(mParamTextureSSAO, SSAOTexture);
	}

	void SSAOAmbientProgram::bindParameters()
	{
		mParamGBuffer.bindParameters(*this);
		mParamTextureSSAO.bind(*this, SHADER_PARAM(TextureSSAO));
	}

	void SSAOAmbientProgram::setParameters(SceneRenderTargets& sceneRenderTargets, RHITexture2D& SSAOTexture)
	{
		mParamGBuffer.setParameters(*this, sceneRenderTargets.getGBuffer());
		setTexture(mParamTextureSSAO, SSAOTexture);
	}

	void GBufferShaderParameters::bindParameters(ShaderProgram& program, bool bUseDepth /*= false */)
	{
		mParamGBufferTextureA.bind(program, SHADER_PARAM(GBufferTextureA));
		mParamGBufferTextureB.bind(program, SHADER_PARAM(GBufferTextureB));
		mParamGBufferTextureC.bind(program, SHADER_PARAM(GBufferTextureC));
		mParamGBufferTextureD.bind(program, SHADER_PARAM(GBufferTextureD));
		if( bUseDepth )
		{
			mParamFrameDepthTexture.bind(program, SHADER_PARAM(FrameDepthTexture));
		}
	}

	void GBufferShaderParameters::setParameters(ShaderProgram& program, GBufferParamData& GBufferData)
	{
		if( mParamGBufferTextureA.isBound() )
			program.setTexture(mParamGBufferTextureA, *GBufferData.textures[GBufferParamData::BufferA]);
		if( mParamGBufferTextureB.isBound() )
			program.setTexture(mParamGBufferTextureB, *GBufferData.textures[GBufferParamData::BufferB]);
		if( mParamGBufferTextureC.isBound() )
			program.setTexture(mParamGBufferTextureC, *GBufferData.textures[GBufferParamData::BufferC]);
		if( mParamGBufferTextureD.isBound() )
			program.setTexture(mParamGBufferTextureD, *GBufferData.textures[GBufferParamData::BufferD]);
	}

	void GBufferShaderParameters::setParameters(ShaderProgram& program, SceneRenderTargets& sceneRenderTargets)
	{
		setParameters(program, sceneRenderTargets.getGBuffer());
		if( mParamFrameDepthTexture.isBound() )
		{
			program.setTexture(mParamFrameDepthTexture, sceneRenderTargets.getDepthTexture());
		}
	}

	class DOFGenerateCoC : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(DOFGenerateCoC);


	};

	IMPLEMENT_GLOBAL_SHADER(DOFGenerateCoC);

	class DOFBlurBaseProgram : public GlobalShaderProgram
	{
	public:
		static Vector2 GetFliter(float x, float a, float b)
		{
			float x2 = x * x;
			float e, c, s;
			e = Math::Exp(a * x2);
			c = Math::Cos(b * x2);
			s = Math::Sin(b * x2);
			return Vector2(e * c, e * s);
		}

		static int const SampleRaidusNum = 8;

		static float GenerateFilterValues( Vector4 values[] , Vector2 scaleAndBias[2] )
		{
			float a[] = { -0.886528, -1.960518 };
			float b[] = { 5.268909, 1.558213 };
			float A[] = { 0.411259, 0.513282 };
			float B[] = { -0.548794 , 4.561110 };


			float min[2] = { Math::MaxFloat , Math::MaxFloat };
			float max[2] = { Math::MinFloat , Math::MinFloat };
			for( int x = -SampleRaidusNum; x <= SampleRaidusNum; ++x )
			{
				for( int c = 0; c < 2; ++c )
				{
					Vector2 f = GetFliter(float(x) / SampleRaidusNum, a[c], b[c]);
					values[x + SampleRaidusNum][2 * c] = f.x;
					values[x + SampleRaidusNum][2 * c + 1] = f.y;

					max[c] = Math::Max(max[c], f.x);
					max[c] = Math::Max(max[c], f.y);
					min[c] = Math::Min(min[c], f.x);
					min[c] = Math::Min(min[c], f.y);
				}
			}
		
			float factor = 0;

			for( int c = 0; c < 2; ++c )
			{
				for( int x = -SampleRaidusNum; x <= SampleRaidusNum; ++x )
				{
					Vector4 vx = values[x + SampleRaidusNum];
					
					for( int y = -SampleRaidusNum; y <= SampleRaidusNum; ++y )
					{
						Vector4 vy = values[y + SampleRaidusNum];

						Vector2 fx = Vector2(vx[2 * c], vx[2 * c + 1]);
						Vector2 fy = Vector2(vy[2 * c], vy[2 * c + 1]);
						factor += A[c] * (fx.x * fy.x - fx.y * fy.y) + B[c] * (fx.x * fy.y + fx.y * fy.x);
					}
				}
			}
			factor = 1.0 / Math::Sqrt(factor);
			for( int c = 0; c < 2 ;++c)
			{
				max[c] *= factor;
				min[c] *= factor;
				scaleAndBias[c].x = (max[c] - min[c]);
				scaleAndBias[c].y = min[c];
			}

			for( int x = -SampleRaidusNum; x <= SampleRaidusNum; ++x )
			{
				values[x + SampleRaidusNum] *= factor;
			}

			return factor;
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			struct ParamConstructor
			{
				ParamConstructor()
				{
					Vector4 values[2 * SampleRaidusNum + 1];
					Vector2 scaleAndBias[2];
					factor = GenerateFilterValues(values , scaleAndBias);

					code += "const float4 ";
					code += SHADER_PARAM(FliterWidgets);
					code += "[]= {";

					FixString<256> str;
					for( int i = 0; i < 2 * SampleRaidusNum + 1; ++i )
					{
						if( i != 0 )
							code += ",";
						
						str.format("float4( %f , %f , %f , %f )" , values[i].x , values[i].y , values[i].z , values[i].w);
						code += str;
					}
					code += "};\n";

					str.format("const float4 %s = float4( %f , %f , %f , %f );", SHADER_PARAM(FliterScaleBias) , scaleAndBias[0].x, scaleAndBias[0].y, scaleAndBias[1].x, scaleAndBias[1].y);
					code += str;
				}
				float factor;
				std::string code;
			};
			static ParamConstructor sParamConstructor;
			option.addDefine(SHADER_PARAM( SAMPLE_RADIUS_NUM ), SampleRaidusNum);
			option.addDefine(SHADER_PARAM( FLITER_NFACTOR ), sParamConstructor.factor);
			option.addInclude("Common");
			option.addCode(sParamConstructor.code.c_str());
		}

		static char const* GetShaderFileName()
		{
			return "Shader/DOF";
		}

	};



	class DOFBlurVProgram : public DOFBlurBaseProgram
	{

		DECLARE_GLOBAL_SHADER(DOFBlurVProgram);
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			DOFBlurBaseProgram::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM( DOF_PASS_STEP ), 1);
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainBlurV) },
				{ Shader::eEmpty  , nullptr },
			};
			return entries;
		}
	};


	class DOFBlurHAndCombineProgram : public DOFBlurBaseProgram
	{
		DECLARE_GLOBAL_SHADER(DOFBlurHAndCombineProgram);
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			DOFBlurBaseProgram::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(DOF_PASS_STEP), 2);
		}

		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ Shader::eVertex , SHADER_ENTRY(ScreenVS) },
				{ Shader::ePixel  , SHADER_ENTRY(MainBlurHAndCombine) },
				{ Shader::eEmpty  , nullptr },
			};
			return entries;
		}
		void bindParameters()
		{
			mParamTextureR.bind(*this, SHADER_PARAM(TextureR));
			mParamTextureG.bind(*this, SHADER_PARAM(TextureG));
			mParamTextureB.bind(*this, SHADER_PARAM(TextureB));
		}

		void setParameters(RHITexture2D& textureR, RHITexture2D& textureG, RHITexture2D& textureB)
		{
			auto& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp, Sampler::eClamp >::GetRHI();
			if ( mParamTextureR.isBound() )
			setTexture(mParamTextureR, textureR, sampler);
			if( mParamTextureG.isBound() )
			setTexture(mParamTextureG, textureG, sampler);
			if( mParamTextureB.isBound() )
			setTexture(mParamTextureB, textureB, sampler);
		}

		ShaderParameter mParamTextureR;
		ShaderParameter mParamTextureG;
		ShaderParameter mParamTextureB;
	};

	IMPLEMENT_GLOBAL_SHADER(DOFBlurVProgram);
	IMPLEMENT_GLOBAL_SHADER(DOFBlurHAndCombineProgram);

	bool PostProcessDOF::init(Vec2i const& size)
	{
#if 0
		mProgGenCoc = ShaderManager::Get().getGlobalShaderT< DOFGenerateCoC >(true);
		if( mProgGenCoc == nullptr )
			return false;
#endif

		mProgBlurV = ShaderManager::Get().getGlobalShaderT< DOFBlurVProgram >(true);
		if( mProgBlurV == nullptr )
			return false;
		mProgBlurHAndCombine = ShaderManager::Get().getGlobalShaderT< DOFBlurHAndCombineProgram >(true);
		if( mProgBlurHAndCombine == nullptr )
			return false;

		mTextureNear = RHICreateTexture2D(Texture::eRGBA32F, size.x, size.y);
		if( !mTextureNear.isValid() )
			return false;
		mTextureFar = RHICreateTexture2D(Texture::eRGBA32F, size.x, size.y);
		if( !mTextureFar.isValid() )
			return false;


		if( !mFrameBufferGen.create() )
			return false;

		mFrameBufferGen.addTexture(*mTextureNear);
		mFrameBufferGen.addTexture(*mTextureFar);

		mTextureBlurR = RHICreateTexture2D(Texture::eRGBA32F, size.x, size.y);
		if( !mTextureBlurR.isValid() )
			return false;
		mTextureBlurG = RHICreateTexture2D(Texture::eRGBA32F, size.x, size.y);
		if( !mTextureBlurG.isValid() )
			return false;
		mTextureBlurB = RHICreateTexture2D(Texture::eRGBA32F, size.x, size.y);
		if( !mTextureBlurB.isValid() )
			return false;

		if( !mFrameBufferBlur.create() )
			return false;

		mFrameBufferBlur.addTexture(*mTextureBlurR);
		mFrameBufferBlur.addTexture(*mTextureBlurG);
		mFrameBufferBlur.addTexture(*mTextureBlurB);
	}


	void PostProcessDOF::render(ViewInfo& view, SceneRenderTargets& sceneRenderTargets)
	{

		auto& sampler = TStaticSamplerState< Sampler::eBilinear, Sampler::eClamp, Sampler::eClamp , Sampler::eClamp >::GetRHI();
		ViewportSaveScope vpScope;

		{
			GL_BIND_LOCK_OBJECT(mFrameBufferBlur);
			RHITexture2D& frameTexture = sceneRenderTargets.getRenderFrameTexture();

			RHISetViewport(0, 0, frameTexture.getSizeX(), frameTexture.getSizeY());
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());

			GL_BIND_LOCK_OBJECT(*mProgBlurV);
			mProgBlurV->setTexture(SHADER_PARAM(Texture), frameTexture , sampler);

			DrawUtility::ScreenRectShader();
		}

		{
			//sceneRenderTargets.swapFrameBufferTexture();
			GL_BIND_LOCK_OBJECT(sceneRenderTargets.getFrameBuffer());

			RHITexture2D& frameTexture = sceneRenderTargets.getPrevRenderFrameTexture();
			RHISetViewport(0, 0, frameTexture.getSizeX(), frameTexture.getSizeY());
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(StaticDepthDisableState::GetRHI());

			GL_BIND_LOCK_OBJECT(*mProgBlurHAndCombine);
			mProgBlurHAndCombine->setParameters(*mTextureBlurR, *mTextureBlurG, *mTextureBlurB);
			//mProgBlurHAndCombine->setTexture(SHADER_PARAM(Texture), frameTexture, sampler);
			DrawUtility::ScreenRectShader();
		}	
	}

	class ClearBufferProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(ClearBufferProgram);

		static int constexpr SizeX = 8;
		static int constexpr SizeY = 8;
		static int constexpr SizeZ = 8;


		void bindParameters()
		{
			mParamBufferRW.bind(*this, SHADER_PARAM(TargetRWTexture));
			mParamClearValue.bind(*this, SHADER_PARAM(ClearValue));
		}

		void setParameters(RHITexture3D& Buffer , Vector4 const& clearValue)
		{
			setRWTexture(mParamBufferRW, Buffer, AO_WRITE_ONLY);
			setParam(mParamClearValue, clearValue);
		}
		
		void clearTexture(RHITexture3D& buffer , Vector4 const& clearValue)
		{
			int nx = (buffer.getSizeX() + SizeX - 1) / SizeX;
			int ny = (buffer.getSizeY() + SizeY - 1) / SizeY;
			int nz = (buffer.getSizeZ() + SizeZ - 1) / SizeZ;
			GL_BIND_LOCK_OBJECT(*this)
			setParameters(buffer, clearValue);
			glDispatchCompute(nx, ny, nz);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(SIZE_X), SizeX);
			option.addDefine(SHADER_PARAM(SIZE_Y), SizeY);
			option.addDefine(SHADER_PARAM(SIZE_Z), SizeZ);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/BufferUtility";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(BufferClearCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamBufferRW;
		ShaderParameter mParamClearValue;
	};
	struct VolumetricLightingParameter
	{
		RHITexture3D*     volumeBuffer[2];
		RHITexture3D*     scatteringBuffer[2];
		RHIUniformBuffer* lightBuffer;
		int               numLights;


	};
	class LightScatteringProgram : public GlobalShaderProgram
	{
		DECLARE_GLOBAL_SHADER(LightScatteringProgram);

		static int constexpr GroupSizeX = 8;
		static int constexpr GroupSizeY = 8;

		void bindParameters()
		{
			mParamVolumeBufferA.bind(*this, SHADER_PARAM(VolumeBufferA));
			mParamVolumeBufferB.bind(*this, SHADER_PARAM(VolumeBufferB));
			mParamScatteringRWBuffer.bind(*this, SHADER_PARAM(ScatteringRWBuffer));
			mParamTitledLightNum.bind(*this, SHADER_PARAM(TitledLightNum));
		}

		void setParameters(ViewInfo& view , VolumetricLightingParameter& parameter )
		{
			setTexture(mParamVolumeBufferA, *parameter.volumeBuffer[0]);
			setTexture(mParamVolumeBufferB, *parameter.volumeBuffer[1]);
			setRWTexture(mParamScatteringRWBuffer, *parameter.scatteringBuffer[0], AO_WRITE_ONLY);
			setUniformBufferT<TitledLightInfo>(*parameter.lightBuffer);
			view.setupShader(*this);
			setParam(mParamTitledLightNum, parameter.numLights);
		}

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(GROUP_SIZE_X), GroupSizeX);
			option.addDefine(SHADER_PARAM(GROUP_SIZE_Y), GroupSizeY);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/VolumetricLighting";
		}
		static ShaderEntryInfo const* GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ Shader::eCompute , SHADER_ENTRY(LightScatteringCS) },
				{ Shader::eEmpty , nullptr },
			};
			return entries;
		}

		ShaderParameter mParamVolumeBufferA;
		ShaderParameter mParamVolumeBufferB;
		ShaderParameter mParamScatteringRWBuffer;
		ShaderParameter mParamTitledLightNum;
	};

	IMPLEMENT_GLOBAL_SHADER(ClearBufferProgram)
	IMPLEMENT_GLOBAL_SHADER(LightScatteringProgram)

	bool VolumetricLightingTech::init(Vec2i const& screenSize)
	{
		mProgClearBuffer = ShaderManager::Get().getGlobalShaderT< ClearBufferProgram >(true);
		if( mProgClearBuffer == nullptr )
			return false;
		mProgLightScattering = ShaderManager::Get().getGlobalShaderT< LightScatteringProgram >(true);
		if( mProgLightScattering == nullptr )
			return false;


		if( !setupBuffer(screenSize, 10, 32) )
			return false;
		return true;
	}


	bool VolumetricLightingTech::setupBuffer(Vec2i const& screenSize, int sizeFactor, int depthSlices)
	{
		int nx = (screenSize.x + sizeFactor - 1) / sizeFactor;
		int ny = (screenSize.y + sizeFactor - 1) / sizeFactor;

		mVolumeBufferA = RHICreateTexture3D(Texture::eRGBA16F, nx, ny, depthSlices);
		mVolumeBufferB = RHICreateTexture3D(Texture::eRGBA16F, nx, ny, depthSlices);
		mScatteringBuffer = RHICreateTexture3D(Texture::eRGBA16F, nx, ny, depthSlices);
		mTiledLightBuffer = RHICreateUniformBuffer(sizeof(TitledLightInfo) * MaxTiledLightNum);

		return mVolumeBufferA.isValid() && mVolumeBufferB.isValid() && mScatteringBuffer.isValid() && mTiledLightBuffer.isValid();
	}

	void VolumetricLightingTech::render(ViewInfo& view, std::vector< LightInfo > const& lights)
	{
		{
			GPU_PROFILE("ClearBuffer");
			mProgClearBuffer->clearTexture(*mVolumeBufferA, Vector4(0, 0, 0, 0));
			mProgClearBuffer->clearTexture(*mVolumeBufferB, Vector4(0, 0, 0, 0));
			mProgClearBuffer->clearTexture(*mScatteringBuffer, Vector4(0, 0, 0, 0));
		}

		TitledLightInfo* pInfo = (TitledLightInfo*)mTiledLightBuffer->lock(ELockAccess::WriteOnly, 0, sizeof(TitledLightInfo) * lights.size());
		for( auto const& light : lights )
		{
			pInfo->pos = light.pos;
			pInfo->type = (int)light.type;
			pInfo->color = light.color;
			pInfo->intensity = light.intensity;
			pInfo->dir = light.dir;
			pInfo->radius = light.radius;
			Vector3 spotParam;
			float angleInner = Math::Min(light.spotAngle.x, light.spotAngle.y);
			spotParam.x = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, angleInner)));
			spotParam.y = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, light.spotAngle.y)));
			pInfo->param = Vector4(spotParam.x,spotParam.y, light.bCastShadow ? 1.0 : 0.0 , 0.0);
			++pInfo;
		}
		mTiledLightBuffer->unlock();

		VolumetricLightingParameter parameter;
		parameter.volumeBuffer[0] = mVolumeBufferA;
		parameter.volumeBuffer[1] = mVolumeBufferB;
		parameter.scatteringBuffer[0] = mScatteringBuffer;
		parameter.numLights = lights.size();


		{
			GPU_PROFILE("LightScattering");
			GL_BIND_LOCK_OBJECT(*mProgLightScattering);
			mProgLightScattering->setParameters(view , parameter);
		}
	}

}//namespace RenderGL