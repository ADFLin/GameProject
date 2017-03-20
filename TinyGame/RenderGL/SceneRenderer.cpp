#include "SceneRenderer.h"

#include "GpuProfiler.h"

#include "GLDrawUtility.h"
#include "ShaderCompiler.h"

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
	}

	void ViewInfo::setupShader(ShaderProgram& program)
	{
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
		shader.setParam(SHADER_PARAM(GLight.dir), normalize(dir));

		Vector3 spotParam;
		float angleInner = Math::Min(spotAngle.x, spotAngle.y);
		spotParam.x = Math::Cos(Math::Deg2Rad(Math::Min(89.9, angleInner)));
		spotParam.y = Math::Cos(Math::Deg2Rad(Math::Min(89.9, spotAngle.y)));
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

		mShadowMap2 = new RHITexture2D;
		if( !mShadowMap2->create(Texture::eFloatRGBA, ShadowTextureSize, ShadowTextureSize) )
			return false;

		mCascadeTexture = new RHITexture2D;
		if( !mCascadeTexture->create(Texture::eFloatRGBA, CascadeTextureSize * CascadedShadowNum, CascadeTextureSize) )
			return false;

		int sizeX = Math::Max(CascadedShadowNum * CascadeTextureSize, ShadowTextureSize);
		int sizeY = Math::Max(CascadeTextureSize, ShadowTextureSize);

		if( !depthBuffer1.create(sizeX, sizeY, Texture::eDepth32F) )
			return false;
		if( !depthBuffer2.create(ShadowTextureSize, ShadowTextureSize, Texture::eDepth32F) )
			return false;

		mShadowMap2->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		mShadowMap2->unbind();

		mShadowBuffer.setDepth(depthBuffer1);
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

	void ShadowDepthTech::drawShadowTexture(LightType type)
	{
		ViewportSaveScope vpScope;

		Matrix4 porjectMatrix = OrthoMatrix(0, vpScope[2], 0, vpScope[3], -1, 1);

		MatrixSaveScope matrixScopt(porjectMatrix);

		int length = 200;
		switch( type )
		{
		case LightType::Spot:
			ShaderHelper::drawTexture(*mShadowMap2, Vec2i(0, 0), Vec2i(length, length));
			break;
		case LightType::Directional:
			ShaderHelper::drawTexture(*mCascadeTexture, Vec2i(0, 0), Vec2i(length * CascadedShadowNum, length));
			break;
		case LightType::Point:
			ShaderHelper::drawCubeTexture(*mShadowMap, Vec2i(0, 0), length / 2);
			//ShaderHelper::drawCubeTexture(GWhiteTextureCube, Vec2i(0, 0), length / 2);
		default:
			break;
		}
	}

	void ShadowDepthTech::reload()
	{
		for( int i = 0; i < 3; ++i )
		{
			ShaderManager::getInstance().reloadShader(mProgShadowDepth[i]);
		}
		ShaderManager::getInstance().reloadShader(mProgLighting);
	}

	void ShadowDepthTech::renderLighting(ViewInfo& view, SceneRender& scene, LightInfo const& light, bool bMultiple)
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
		mProgLighting.setParam(SHADER_PARAM(DepthParam), depthParam[0], depthParam[1]);

		mEffectCur = &mProgLighting;
		if( bMultiple )
		{
			glDepthFunc(GL_EQUAL);
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			scene.render(view, *this);
			glDisable(GL_BLEND);
			glDepthFunc(GL_LESS);
		}
		else
		{
			scene.render(view, *this);
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

	void ShadowDepthTech::renderShadowDepth(ViewInfo& view, SceneRender& scene, ShadowProjectParam& shadowProjectParam)
	{
		//glDisable( GL_CULL_FACE );
		LightInfo const& light = *shadowProjectParam.light;

		if( !light.bCastShadow )
			return;

		shadowParam = shadowProjectParam.shadowParam;

		ViewInfo lightView;
		lightView = view;

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
			mShadowBuffer.setDepth(depthBuffer1);
			mShadowBuffer.setTexture(0, *mCascadeTexture);
			mShadowBuffer.bind();

			glClearDepth(1);
			glClearColor(1, 1, 1, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
				glViewport(i*CascadeTextureSize, 0, CascadeTextureSize, CascadeTextureSize);

				lightView.setupTransform(worldToLight, shadowProject);

				Matrix4 corpMatrx = Matrix4(
					delata, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					i * delata, 0, 0, 1);
				mShadowMatrix = worldToLight * shadowProject * biasMatrix;

				shadowProjectParam.shadowMatrix[i] = mShadowMatrix * corpMatrx;

				glLoadMatrixf(worldToLight);
				scene.render(lightView, *this);
			}
			mShadowBuffer.unbind();
		}
		else if( light.type == LightType::Spot )
		{
			GPU_PROFILE("Shadow-Spot");
			shadowProject = PerspectiveMatrix(Math::Deg2Rad(2.0 * Math::Min(89.99, light.spotAngle.y)), 1.0, 0.01, light.radius);
			MatrixSaveScope matScope(shadowProject);

			worldToLight = LookAtMatrix(light.pos, light.dir, GetUpDir(light.dir));
			mShadowMatrix = worldToLight * shadowProject * biasMatrix;
			shadowProjectParam.shadowMatrix[0] = mShadowMatrix;

			shadowProjectParam.shadowTexture = mShadowMap2;
			mShadowBuffer.setDepth(depthBuffer2);
			mShadowBuffer.setTexture(0, *mShadowMap2);
			GL_BIND_LOCK_OBJECT(mShadowBuffer);

			ViewportSaveScope vpScope;
			glViewport(0, 0, ShadowTextureSize, ShadowTextureSize);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1);
			glClearColor(1, 1, 1, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			lightView.setupTransform(worldToLight, shadowProject);
			glLoadMatrixf(worldToLight);
			scene.render(lightView, *this);

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
			glViewport(0, 0, ShadowTextureSize, ShadowTextureSize);
			mShadowBuffer.setDepth(depthBuffer2);

			glEnable(GL_DEPTH_TEST);
			for( int i = 0; i < 6; ++i )
			{
				GPU_PROFILE("Face");
				mShadowBuffer.setTexture(0, *mShadowMap, Texture::Face(i));
				GL_BIND_LOCK_OBJECT(mShadowBuffer);
				{
					GPU_PROFILE("Clear");
					glClearDepth(1);
					glClearColor(1, 1, 1, 1.0);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				}

				worldToLight = LookAtMatrix(light.pos, CubeFaceDir[i], CubeUpDir[i]);
				mShadowMatrix = worldToLight * shadowProject * biasMatrix;
				shadowProjectParam.shadowMatrix[i] = mShadowMatrix;

				{
					GPU_PROFILE("DrawMesh");
					lightView.setupTransform(worldToLight, shadowProject);
					glLoadMatrixf(worldToLight);
					scene.render(lightView, *this);
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
		Vector3 zAxis = normalize(-light.dir);
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

	bool DefferredLightingTech::init( SceneRenderTargets& sceneRenderTargets )
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


		if( !MeshUtility::createLightSphere(mSphereMesh) ||
		    !MeshUtility::createLightCone(mConeMesh) )
			return false;

		if( !ShaderManager::getInstance().loadFile( 
			 mProgLightingScreenRect[(int)LightType::Point] ,
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT") )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgLightingScreenRect[(int)LightType::Spot] ,
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_SPOT \n") )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgLightingScreenRect[(int)LightType::Directional] ,
			"Shader/DeferredLighting",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_DIRECTIONAL\n") )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgLighting[(int)LightType::Point],
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_POINT \n"
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n" , -1 ) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgLighting[(int)LightType::Spot],
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_SPOT \n"
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n", -1) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgLighting[(int)LightType::Directional],
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(LightingPassPS),
			"#define DEFERRED_LIGHT_TYPE LIGHTTYPE_DIRECTIONAL \n"
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n", -1) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mProgLightingShowBound,
			"Shader/DeferredLighting",
			SHADER_ENTRY(LightingPassVS), SHADER_ENTRY(ShowBoundPS),
			"#define DEFERRED_SHADING_USE_BOUND_SHAPE 1 \n", -1) )
			return false;

		return true;
	}

	void DefferredLightingTech::renderBassPass(ViewInfo& view, SceneRender& scene)
	{
		glEnable(GL_DEPTH_TEST);
		mBassPassBuffer.setTexture(0, mSceneRenderTargets->getRenderFrameTexture());
		GL_BIND_LOCK_OBJECT(mBassPassBuffer);
		float const depthValue = 1.0;
		{
			GPU_PROFILE("Clear Buffer");
			mBassPassBuffer.clearBuffer(&Vector4(0, 0, 0, 1), &depthValue);
		}
		scene.render(view, *this);
		
	}
	

	void DefferredLightingTech::prevRenderLights(ViewInfo& view)
	{
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
		//SetupShaderParam(mProgLightingShowBound);
	}

	void DefferredLightingTech::renderLight(ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam)
	{

		auto const BindShaderParam = [this , &view , &light , &shadowProjectParam](ShaderProgram& program)
		{
			//mSceneRenderTargets->setupShaderGBuffer(program, true);
			shadowProjectParam.setupShader(program);
			//view.setupShader(program);
			light.setupShaderGlobalParam(program);
		};

		if( boundMethod != LBM_SCREEN_RECT && light.type != LightType::Directional )
		{
			ShaderProgram& lightShader = (debugMode == DebugMode::eNone) ? mProgLighting[ (int)light.type ] : mProgLightingShowBound;

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

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);

			glDepthMask(false);


			if( boundMethod == LBM_GEMO_BOUND_SHAPE_WITH_STENCIL )
			{
				glEnable(GL_STENCIL_TEST);

				mLightBuffer.bindDepthOnly();
				glClearStencil(1);
				glClear(GL_STENCIL_BUFFER_BIT);
				mLightBuffer.unbind();

				glColorMask(false, false, false, false);

				//if ( debugMode != DebugMode::eShowVolume )
				{
					
					glCullFace(GL_BACK);
					glDepthFunc(GL_GREATER);
					glStencilFunc(GL_ALWAYS, 0, 0);
					glStencilOp(GL_KEEP, GL_KEEP , GL_DECR);

					mLightBuffer.bindDepthOnly();
					DrawBoundShape(false);
					mLightBuffer.unbind();

				}

				glColorMask(true, true, true, true);
				glStencilFunc(GL_EQUAL, 1, 1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			}

			if( debugMode == DebugMode::eShowVolume )
				glDisable(GL_DEPTH_TEST);

			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);

			glCullFace(GL_FRONT);
			glDepthFunc(GL_GEQUAL);

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

			glDepthFunc(GL_LESS);
			glCullFace(GL_BACK);

			glDisable(GL_BLEND);

			if( debugMode == DebugMode::eShowVolume )
				glEnable(GL_DEPTH_TEST);

			if( boundMethod == LBM_GEMO_BOUND_SHAPE_WITH_STENCIL )
			{
				glDisable(GL_STENCIL_TEST);
			}

			glDepthMask(true);
		}
		else
		{
			glBlendFunc(GL_ONE, GL_ONE);
			glEnable(GL_BLEND);
			GL_BIND_LOCK_OBJECT(mSceneRenderTargets->getFrameBuffer());
			glDisable(GL_DEPTH_TEST);
			{
				//MatrixSaveScope matrixScope(Matrix4::Identity());
				ShaderProgram& program = mProgLightingScreenRect[(int)light.type];
				GL_BIND_LOCK_OBJECT(program);
				BindShaderParam(program);
				DrawUtiltiy::ScreenRect();

				glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}
	}

	void DefferredLightingTech::reload()
	{
		for( int i = 0; i < 3; ++i )
		{
			ShaderManager::getInstance().reloadShader(mProgLightingScreenRect[i]);
			ShaderManager::getInstance().reloadShader(mProgLighting[i]);
		}
		ShaderManager::getInstance().reloadShader(mProgLightingShowBound);
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
			glViewport(1 * width + gapX, 0 * height + gapY, drawWidth, drawHeight);
			float colorBias[2] = { 0.5 , 0.5 };
			ShaderHelper::getInstance().copyTextureBiasToBuffer(*textures[BufferB], colorBias);

		}
		//drawTexture(1 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, BufferB);
		drawTexture(2 * width + gapX, 0 * height + gapY, drawWidth, drawHeight, BufferC);
		drawTexture(3 * width + gapX, 3 * height + gapY, drawWidth, drawHeight, BufferD, Vector4(1, 0, 0, 0));
		drawTexture(3 * width + gapX, 2 * height + gapY, drawWidth, drawHeight, BufferD, Vector4(0, 1, 0, 0));
		drawTexture(3 * width + gapX, 1 * height + gapY, drawWidth, drawHeight, BufferD, Vector4(0, 0, 1, 0));
		{
			ViewportSaveScope vpScope;
			glViewport(3 * width + gapX, 0 * height + gapY, drawWidth, drawHeight);
			float valueFactor[2] = { 255 , 0 };
			ShaderHelper::getInstance().mapTextureColorToBuffer(*textures[BufferD], Vector4(0, 0, 0, 1), valueFactor);
		}
		//renderDepthTexture(width , 3 * height, width, height);
	}

	void GBufferParamData::drawTexture(int x, int y, int width, int height, int idxBuffer)
	{
		ShaderHelper::drawTexture(*textures[idxBuffer], Vec2i(x, y), Vec2i(width, height));
	}

	void GBufferParamData::drawTexture(int x, int y, int width, int height, int idxBuffer, Vector4 const& colorMask)
	{
		ViewportSaveScope vpScope;
		glViewport(x, y, width, height);
		ShaderHelper::getInstance().copyTextureMaskToBuffer(*textures[idxBuffer], colorMask);
	}


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

		if( !ShaderManager::getInstance().loadFile(
			mSSAOShader ,
			"Shader/SSAO",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(GeneratePS)) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mSSAOBlurShader ,
			"Shader/SSAO",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(BlurPS)) )
			return false;

		if( !ShaderManager::getInstance().loadFile(
			mAmbientShader ,
			"Shader/SSAO",
			SHADER_ENTRY(ScreenVS), SHADER_ENTRY(AmbientPS)) )
			return false;

		generateKernelVectors(NumDefaultKernel);
		return true;
	}

	void PostProcessSSAO::render(ViewInfo& view, SceneRenderTargets& sceneRenderTargets)
	{
		glDisable(GL_DEPTH_TEST);

		{
			GPU_PROFILE("SSAO-Generate");
			mFrameBuffer.setTexture(0, *mSSAOTexture);
			GL_BIND_LOCK_OBJECT(mFrameBuffer);
			GL_BIND_LOCK_OBJECT(mSSAOShader);
			view.setupShader(mSSAOShader);
			sceneRenderTargets.setupShaderGBuffer(mSSAOShader, true);
			mSSAOShader.setParam(SHADER_PARAM(KernelNum), (int)mKernelVectors.size());
			mSSAOShader.setParam(SHADER_PARAM(KernelVectors), &mKernelVectors[0], mKernelVectors.size());
			mSSAOShader.setParam(SHADER_PARAM(OcclusionRadius), 0.5f);
			DrawUtiltiy::ScreenRect();
		}


		{
			GPU_PROFILE("SSAO-Blur");
			mFrameBuffer.setTexture(0, *mSSAOTextureBlur);
			GL_BIND_LOCK_OBJECT(mFrameBuffer);
			GL_BIND_LOCK_OBJECT(mSSAOBlurShader);
			mSSAOBlurShader.setTexture(SHADER_PARAM(TextureSSAO), *mSSAOTexture);
			DrawUtiltiy::ScreenRect();
		}

		{
			GPU_PROFILE("SSAO-Ambient");
				//sceneRenderTargets.swapFrameBufferTexture();
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
			GL_BIND_LOCK_OBJECT(sceneRenderTargets.getFrameBuffer());
			GL_BIND_LOCK_OBJECT(mAmbientShader);
			mAmbientShader.setTexture(SHADER_PARAM(TextureSSAO), *mSSAOTextureBlur);
			sceneRenderTargets.setupShaderGBuffer(mAmbientShader, false);
			DrawUtiltiy::ScreenRect();
			glDisable(GL_BLEND);
		}
		glEnable(GL_DEPTH_TEST);
	}

	void PostProcessSSAO::reload()
	{
		ShaderManager::getInstance().reloadShader(mSSAOShader);
		ShaderManager::getInstance().reloadShader(mSSAOBlurShader);
		ShaderManager::getInstance().reloadShader(mAmbientShader);
	}

	void ShadowProjectParam::setupShader(ShaderProgram& program) const
	{
		program.setParam(SHADER_PARAM(ShadowParam), shadowParam.x, shadowParam.y);
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
			if( !mFrameTextures[i]->create(Texture::eRGBA16F, size.x, size.y) )
				return false;
		}

		if( !mGBuffer.init(size) )
			return false;

		mDepthTexture = new RHITextureDepth;
		if( !mDepthTexture->create(Texture::eD24S8, size.x, size.y) )
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
		DrawUtiltiy::Rect(x, y, width, height);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}

}//namespace RenderGL
