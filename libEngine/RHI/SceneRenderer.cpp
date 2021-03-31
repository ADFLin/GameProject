#include "SceneRenderer.h"

#include "GpuProfiler.h"
#include "RHICommand.h"
#include "OpenGLCommon.h"

#include "ShaderManager.h"
#include "VertexFactory.h"
#include "MaterialShader.h"
#include "DrawUtility.h"
#include "Scene.h"

#include "Renderer/BasePassRendering.h"
#include "Renderer/ShadowDepthRendering.h"
#include "Renderer/SceneLighting.h"
#include "Renderer/MeshBuild.h"

#include "FixString.h"
#include "CoreShare.h"




#include <algorithm>


namespace Render
{
	int const OIT_StorageSize = 4096;

	void TiledLightInfo::setValue(LightInfo const& light)
	{
		pos = light.pos;
		type = (int32)light.type;
		color = light.color;
		intensity = light.intensity;
		dir = light.dir;
		radius = light.radius;
		float angleInner = Math::Min(light.spotAngle.x, light.spotAngle.y);
		param.x = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, angleInner)));
		param.y = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, light.spotAngle.y)));
	}

	void LightInfo::setupShaderGlobalParam(RHICommandList& commandList, ShaderProgram& shader) const
	{
		shader.setParam(commandList, SHADER_PARAM(GLight.worldPosAndRadius), Vector4(pos, radius));
		shader.setParam(commandList, SHADER_PARAM(GLight.color), intensity * color);
		shader.setParam(commandList, SHADER_PARAM(GLight.type), int(type));
		shader.setParam(commandList, SHADER_PARAM(GLight.bCastShadow), int(bCastShadow));
		shader.setParam(commandList, SHADER_PARAM(GLight.dir), Math::GetNormal(dir));

		Vector3 spotParam;
		float angleInner = Math::Min(spotAngle.x, spotAngle.y);
		spotParam.x = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, angleInner)));
		spotParam.y = Math::Cos(Math::Deg2Rad(Math::Min<float>(89.9, spotAngle.y)));
		shader.setParam(commandList, SHADER_PARAM(GLight.spotParam), spotParam);
	}

	bool ShadowDepthTech::init()
	{
		VERIFY_RETURN_FALSE(mShadowBuffer = RHICreateFrameBuffer());

		mShadowMap = RHICreateTextureCube(ETexture::FloatRGBA, ShadowTextureSize);
		if( !mShadowMap.isValid())
			return false;
		mShadowMap2 = RHICreateTexture2D(ETexture::FloatRGBA, ShadowTextureSize, ShadowTextureSize);
		if( !mShadowMap2.isValid() )
			return false;

		mCascadeTexture = RHICreateTexture2D(ETexture::FloatRGBA, CascadeTextureSize * CascadedShadowNum, CascadeTextureSize);
		if( !mCascadeTexture.isValid() )
			return false;

		int sizeX = Math::Max(CascadedShadowNum * CascadeTextureSize, ShadowTextureSize);
		int sizeY = Math::Max(CascadeTextureSize, ShadowTextureSize);

#if USE_DepthRenderBuffer
		depthBuffer1 = new RHIDepthRenderBuffer;
		if( !depthBuffer1->create(sizeX, sizeY, ETexture::Depth32F) )
			return false;
		depthBuffer2 = new RHIDepthRenderBuffer;
		if( !depthBuffer2->create(ShadowTextureSize, ShadowTextureSize, ETexture::Depth32F) )
			return false;
#endif

		OpenGLCast::To(mShadowMap2)->bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		OpenGLCast::To(mShadowMap2)->unbind();
#if USE_DepthRenderBuffer
		mShadowBuffer->setDepth(*depthBuffer1);
#endif
		mShadowBuffer->addTexture(*mShadowMap, ETexture::FaceX);
		//mBuffer.addTexture( mShadowMap2 );
		//mBuffer.addTexture( mShadowMap , ETexture::FaceX , false );

#if 0
		if( !ShaderManager::getInstance().loadFile(mProgShadowDepthList[0],"Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgShadowDepthList[1],"Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgShadowDepthList[2],"Shader/ShadowMap", "#define SHADOW_LIGHT_TYPE LIGHTTYPE_SPOT") )
			return false;
		if( !ShaderManager::getInstance().loadFile(mProgLighting , "Shader/ShadowLighting") )
			return false;
#endif

		depthParam[0] = 0.001;
		depthParam[1] = 100;

		return true;
	}

	void ShadowDepthTech::drawShadowTexture(RHICommandList& commandList, LightType type , Matrix4 const& porjectMatrix , IntVector2 const& pos , int length )
	{

		switch( type )
		{
		case LightType::Spot:
			DrawUtility::DrawTexture(commandList, porjectMatrix, *mShadowMap2, pos, IntVector2(length, length));
			break;
		case LightType::Directional:
			DrawUtility::DrawTexture(commandList, porjectMatrix, *mCascadeTexture, pos, IntVector2(length * CascadedShadowNum, length));
			break;
		case LightType::Point:
			DrawUtility::DrawCubeTexture(commandList, porjectMatrix, *mShadowMap, pos, length / 2);
			//ShaderHelper::drawCubeTexture(commandList, GWhiteTextureCube, Vec2i(0, 0), length / 2);
		default:
			break;
		}
	}

	void ShadowDepthTech::reload()
	{
		for(auto& shader : mProgShadowDepthList)
		{
			ShaderManager::Get().reloadShader(shader);
		}
		ShaderManager::Get().reloadShader(mProgLighting);
	}


	MaterialShaderProgram* ShadowDepthTech::getMaterialShader(RenderContext& context, MaterialMaster& material, VertexFactory* vertexFactory)
	{
#if USE_MATERIAL_SHADOW
		if( mEffectCur == &mProgLighting )
			return nullptr;
		return material.getShaderT< ShadowDepthProgram >(vertexFactory);
#else
		return nullptr;
#endif
	}

	void ShadowDepthTech::renderLighting(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene, LightInfo const& light, bool bMultiple)
	{

		ShadowProjectParam shadowPrjectParam;
		shadowPrjectParam.setupLight(light);
		renderShadowDepth(commandList, view, scene, shadowPrjectParam);

		RHISetShaderProgram(commandList, mProgLighting.getRHIResource());
		//mProgLighting.setTexture(SHADER_PARAM(texSM) , mShadowMap2 , 0 );

		view.setupShader(commandList, mProgLighting);
		light.setupShaderGlobalParam(commandList, mProgLighting);

		//mProgLighting.setParam(SHADER_PARAM(worldToLightView) , worldToLightView );
		mProgLighting.setTexture(commandList, SHADER_PARAM(ShadowTextureCube), *mShadowMap, SHADER_PARAM(ShadowTextureCubeSampler) ,
								 TStaticSamplerState<ESampler::Bilinear , ESampler::Clamp , ESampler::Clamp , ESampler::Clamp >::GetRHI() );
		mProgLighting.setParam(commandList, SHADER_PARAM(DepthParam), Vector2( depthParam[0], depthParam[1] ));

		mEffectCur = &mProgLighting;

		if( bMultiple )
		{
			//glDepthFunc(GL_EQUAL);
			RHISetDepthStencilState(commandList, TStaticDepthStencilState< true , ECompareFunc::Equal >::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , EBlend::One, EBlend::One >::GetRHI());
			RenderContext context(commandList, view, *this);
			scene.render(context);
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState< true, ECompareFunc::Less >::GetRHI());
			//glDepthFunc(GL_LESS);
		}
		else
		{
			RenderContext context(commandList, view, *this);
			scene.render(context);
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

	void ShadowDepthTech::renderShadowDepth(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene, ShadowProjectParam& shadowProjectParam)
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
		RHISetShaderProgram(commandList, mProgShadowDepthList->getRHIResource());
		mEffectCur = &mProgShadowDepthList[LIGHTTYPE_POINT];
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

		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

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
#if USE_DepthRenderBuffer
			mShadowBuffer.setDepth(*depthBuffer1);
#endif
			mShadowBuffer->setTexture(0, *mCascadeTexture);


			RHISetFrameBuffer(commandList, mShadowBuffer);
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(1, 1, 1, 1), 1, 1);

			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

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

				ViewportSaveScope vpScope(commandList);
				RHISetViewport(commandList, i*CascadeTextureSize, 0, CascadeTextureSize, CascadeTextureSize);

				lightView.setupTransform(worldToLight, shadowProject);

				Matrix4 corpMatrix = Matrix4(
					delata, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					i * delata, 0, 0, 1);
				mShadowMatrix = worldToLight * shadowProject * biasMatrix;

				shadowProjectParam.shadowMatrix[i] = mShadowMatrix * corpMatrix;
				RenderContext context(commandList, lightView, *this);
				scene.render( context );
			}

			RHISetFrameBuffer(commandList, nullptr);
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

#if USE_DepthRenderBuffer
			mShadowBuffer.setDepth(*depthBuffer2);
#endif
			mShadowBuffer->setTexture(0, *mShadowMap2);
			RHISetFrameBuffer(commandList, mShadowBuffer);

			ViewportSaveScope vpScope(commandList);
			RHISetViewport(commandList, 0, 0, ShadowTextureSize, ShadowTextureSize);
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(1, 1, 1, 1), 1, 1);

			lightView.setupTransform(worldToLight, shadowProject);
			RenderContext context(commandList, lightView, *this);
			scene.render( context );
			RHISetFrameBuffer(commandList, nullptr);
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

			ViewportSaveScope vpScope(commandList);
			RHISetViewport(commandList, 0, 0, ShadowTextureSize, ShadowTextureSize);
#if USE_DepthRenderBuffer
			mShadowBuffer->setDepth(*depthBuffer2);
#endif
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			for( int i = 0; i < 6; ++i )
			{
				//GPU_PROFILE("Face");
				mShadowBuffer->setTexture(0, *mShadowMap, ETexture::Face(i));

				RHISetFrameBuffer(commandList, mShadowBuffer);
				{
					//GPU_PROFILE("Clear");
					RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(1, 1, 1, 1), 1, 1);
				}

				worldToLight = LookAtMatrix(light.pos, ETexture::GetFaceDir(ETexture::Face(i)), ETexture::GetFaceUpDir(ETexture::Face(i)));
				mShadowMatrix = worldToLight * shadowProject * biasMatrix;
				shadowProjectParam.shadowMatrix[i] = mShadowMatrix;

				{
					//GPU_PROFILE("DrawMesh");
					lightView.setupTransform(worldToLight, shadowProject);
					RenderContext context(commandList, lightView, *this);
					scene.render( context );
				}

				RHISetFrameBuffer(commandList, nullptr);
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
		for(auto const& pos : boundPos)
		{
			Vector3 v;
			v.x = xAxis.dot(pos);
			v.y = yAxis.dot(pos);
			v.z = zAxis.dot(pos);
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



	bool DeferredShadingTech::init( FrameRenderTargets& sceneRenderTargets )
	{
		mSceneRenderTargets = &sceneRenderTargets;

		GBufferResource& GBuffer = sceneRenderTargets.getGBuffer();

		VERIFY_RETURN_FALSE(mBassPassBuffer = RHICreateFrameBuffer());
		VERIFY_RETURN_FALSE(mLightingBuffer = RHICreateFrameBuffer());
		VERIFY_RETURN_FALSE(mLightingDepthBuffer = RHICreateFrameBuffer());

		mBassPassBuffer->setTexture(0,sceneRenderTargets.getFrameTexture());
		GBuffer.attachToBuffer(*mBassPassBuffer);

		mBassPassBuffer->setDepth( sceneRenderTargets.getDepthTexture());

		mLightingBuffer->addTexture( sceneRenderTargets.getFrameTexture() );
		mLightingBuffer->setDepth( sceneRenderTargets.getDepthTexture() );
		mLightingDepthBuffer->setDepth(sceneRenderTargets.getDepthTexture());

		VERIFY_RETURN_FALSE(MeshBuild::LightSphere(mSphereMesh));
		VERIFY_RETURN_FALSE(MeshBuild::LightCone(mConeMesh));

#define GET_LIGHTING_SHADER( LIGHT_TYPE , NAME )\
		VERIFY_RETURN_FALSE( mProgLightingScreenRect[(int)LIGHT_TYPE] = ShaderManager::Get().getGlobalShaderT< TDeferredLightingProgram< LIGHT_TYPE > >(true) );\
		VERIFY_RETURN_FALSE( mProgLighting[(int)LIGHT_TYPE] = ShaderManager::Get().getGlobalShaderT< DeferredLightingProgram##NAME >(true) );

		GET_LIGHTING_SHADER(LightType::Spot, Spot);
		GET_LIGHTING_SHADER(LightType::Point , Point);
		GET_LIGHTING_SHADER(LightType::Directional , Directional);
		

#undef GET_LIGHTING_SHADER

		VERIFY_RETURN_FALSE(mProgLightingShowBound = ShaderManager::Get().getGlobalShaderT< LightingShowBoundProgram >(true));
		return true;
	}

	void DeferredShadingTech::renderBassPass(RHICommandList& commandList, ViewInfo& view, SceneInterface& scene)
	{
		mBassPassBuffer->setTexture(0, mSceneRenderTargets->getFrameTexture());
		RHISetFrameBuffer(commandList, mBassPassBuffer);
		float const depthValue = 1.0;
		{
			GPU_PROFILE("Clear Buffer");
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0, 0, 0, 0), 1, depthValue);
		}
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RenderContext context(commandList, view, *this);
		scene.render(context);
	}
	

	void DeferredShadingTech::prevRenderLights(RHICommandList& commandList, ViewInfo& view)
	{

	}

	void DeferredShadingTech::renderLight(RHICommandList& commandList, ViewInfo& view, LightInfo const& light, ShadowProjectParam const& shadowProjectParam)
	{

		auto const BindShaderParam = [this , &view , &light , &shadowProjectParam](RHICommandList& commandList, DeferredLightingProgram& program)
		{
			program.setParamters(commandList, *mSceneRenderTargets);
			shadowProjectParam.setupShader(commandList, program);
			view.setupShader(commandList, program);
			light.setupShaderGlobalParam(commandList, program);
		};

		if( boundMethod != LBM_SCREEN_RECT && light.type != LightType::Directional )
		{
			DeferredLightingProgram* lightShader = (debugMode == DebugMode::eNone) ? mProgLighting[ (int)light.type ] : mProgLightingShowBound;

			mLightingBuffer->setTexture(0, mSceneRenderTargets->getFrameTexture());

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

			auto const DrawBoundShape = [this,boundMesh,&lightXForm](RHICommandList& commandList, bool bUseShader )
			{
				glPushMatrix();
				glLoadMatrixf(lightXForm);
				boundMesh->draw(commandList);
				glPopMatrix();
			};

			constexpr bool bWriteDepth = false;

			if( boundMethod == LBM_GEMO_BOUND_SHAPE_WITH_STENCIL )
			{
				constexpr bool bEnableStencilTest = true;

				RHISetFrameBuffer(commandList, mLightingDepthBuffer);
				RHIClearRenderTargets(commandList, EClearBits::Stencil, nullptr, 0, 0, 1 );
				RHISetFrameBuffer(commandList, nullptr);

				RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());
				//if ( debugMode != DebugMode::eShowVolume )
				{
					RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::Back >::GetRHI());
					RHISetDepthStencilState(commandList,
						TStaticDepthStencilState<
							bWriteDepth, ECompareFunc::Greater,
							bEnableStencilTest, ECompareFunc::Always,
							EStencil::Keep, EStencil::Keep, EStencil::Decr, 0x0 
						>::GetRHI(), 0x0);

					RHISetFrameBuffer(commandList, mLightingDepthBuffer);
					DrawBoundShape(commandList, false);
					RHISetFrameBuffer(commandList, nullptr);

				}

				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA >::GetRHI());
				if( debugMode == DebugMode::eShowVolume )
				{
					RHISetDepthStencilState(commandList,
						TStaticDepthStencilState<
							bWriteDepth, ECompareFunc::Always,
							bEnableStencilTest, ECompareFunc::Equal,
							EStencil::Keep, EStencil::Keep, EStencil::Keep, 0x1
						>::GetRHI(), 0x1);
				}
				else
				{
					RHISetDepthStencilState(commandList,
						TStaticDepthStencilState<
							bWriteDepth, ECompareFunc::GeraterEqual,
							bEnableStencilTest, ECompareFunc::Equal,
							EStencil::Keep, EStencil::Keep, EStencil::Keep, 0x1
						>::GetRHI(), 0x1);
				}
			}
			else
			{
				if( debugMode == DebugMode::eShowVolume )
				{
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<bWriteDepth, ECompareFunc::Always >::GetRHI());

				}
				else
				{
					RHISetDepthStencilState(commandList, TStaticDepthStencilState<bWriteDepth, ECompareFunc::GeraterEqual >::GetRHI());
				}
			}

			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , EBlend::One, EBlend::One >::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::Front >::GetRHI());
			{
				RHISetFrameBuffer(commandList, mLightingBuffer);
				RHISetShaderProgram(commandList, lightShader->getRHIResource());
				//if( debugMode == DebugMode::eNone )
				{
					BindShaderParam(commandList, *lightShader);
				}
				lightShader->setParam(commandList, SHADER_PARAM(BoundTransform), lightXForm);
				DrawBoundShape(commandList, true);
			}
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
		}
		else
		{
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
			{
				RHISetFrameBuffer(commandList, mSceneRenderTargets->getFrameBuffer());
				//MatrixSaveScope matrixScope(Matrix4::Identity());
				DeferredLightingProgram* program = mProgLightingScreenRect[(int)light.type];
				RHISetShaderProgram(commandList, program->getRHIResource());
				BindShaderParam(commandList, *program);
				DrawUtility::ScreenRect(commandList);
				RHISetFrameBuffer(commandList, nullptr);
				//glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
			}

		}

	}

	MaterialShaderProgram* DeferredShadingTech::getMaterialShader(RenderContext& context, MaterialMaster& material, VertexFactory* vertexFactory)
	{
		//return &GSimpleBasePass;
		return material.getShaderT<DeferredBasePassProgram>(vertexFactory);
	}

	void DeferredShadingTech::reload()
	{
		for( int i = 0; i < 3; ++i )
		{
			ShaderManager::Get().reloadShader(*mProgLightingScreenRect[i]);
			ShaderManager::Get().reloadShader(*mProgLighting[i]);
		}
		ShaderManager::Get().reloadShader(*mProgLightingShowBound);
	}




	class SSAOGenerateProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SSAOGenerateProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SSAO";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(GeneratePS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override;
		void setParameters(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, Vector3 kernelVectors[], int numKernelVector);

		GBufferShaderParameters mParamGBuffer;
		ShaderParameter mParamKernelNum;
		ShaderParameter mParamKernelVectors;
		ShaderParameter mParamOcclusionRadius;
	};

	IMPLEMENT_SHADER_PROGRAM(SSAOGenerateProgram);

	class SSAOBlurProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SSAOBlurProgram, Global)

		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SSAO";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BlurPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override;
		void setParameters(RHICommandList& commandList, RHITexture2D& SSAOTexture);

		ShaderParameter mParamTextureSSAO;
		ShaderParameter mParamTextureSamplerSSAO;
	};

	IMPLEMENT_SHADER_PROGRAM(SSAOBlurProgram);

	class SSAOAmbientProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(SSAOAmbientProgram, Global)
		static void SetupShaderCompileOption(ShaderCompileOption&) {}
		static char const* GetShaderFileName()
		{
			return "Shader/SSAO";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(AmbientPS) },
			};
			return entries;
		}
	public:
		void bindParameters(ShaderParameterMap const& parameterMap) override;
		void setParameters(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, RHITexture2D& SSAOTexture);

		GBufferShaderParameters mParamGBuffer;
		ShaderParameter mParamTextureSSAO;
		ShaderParameter mParamTextureSamplerSSAO;
	};

	IMPLEMENT_SHADER_PROGRAM(SSAOAmbientProgram);

	bool PostProcessSSAO::init(IntVector2 const& size)
	{
		VERIFY_RETURN_FALSE( mSSAOTexture = RHICreateTexture2D(ETexture::FloatRGBA, size.x, size.y) );
		VERIFY_RETURN_FALSE( mSSAOTextureBlur = RHICreateTexture2D(ETexture::FloatRGBA, size.x, size.y) );
		VERIFY_RETURN_FALSE( mFrameBuffer = RHICreateFrameBuffer());

		mFrameBuffer->addTexture(*mSSAOTexture);

		VERIFY_RETURN_FALSE(mProgSSAOGenerate = ShaderManager::Get().getGlobalShaderT< SSAOGenerateProgram >(true) );
		VERIFY_RETURN_FALSE(mProgSSAOBlur = ShaderManager::Get().getGlobalShaderT< SSAOBlurProgram >(true) );
		VERIFY_RETURN_FALSE(mProgAmbient = ShaderManager::Get().getGlobalShaderT< SSAOAmbientProgram >(true));

		generateKernelVectors(NumDefaultKernel);
		return true;
	}

	void PostProcessSSAO::render(RHICommandList& commandList, ViewInfo& view, FrameRenderTargets& sceneRenderTargets)
	{
		RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
		{
			GPU_PROFILE("SSAO-Generate");
			mFrameBuffer->setTexture(0, *mSSAOTexture);

			RHISetFrameBuffer(commandList, mFrameBuffer);
			RHISetShaderProgram(commandList, mProgSSAOGenerate->getRHIResource());
			view.setupShader(commandList, *mProgSSAOGenerate);
			mProgSSAOGenerate->setParameters(commandList, sceneRenderTargets , &mKernelVectors[0], mKernelVectors.size());
			DrawUtility::ScreenRect(commandList);
			RHISetFrameBuffer(commandList, nullptr);
		}


		{
			GPU_PROFILE("SSAO-Blur");
			mFrameBuffer->setTexture(0, *mSSAOTextureBlur);
			RHISetFrameBuffer(commandList, mFrameBuffer);
			RHISetShaderProgram(commandList, mProgSSAOBlur->getRHIResource());
			mProgSSAOBlur->setParameters(commandList, *mSSAOTexture);
			DrawUtility::ScreenRect(commandList);
			RHISetFrameBuffer(commandList, nullptr);
		}


		{
			GPU_PROFILE("SSAO-Ambient");
			//sceneRenderTargets.swapFrameBufferTexture();
			RHISetFrameBuffer(commandList, sceneRenderTargets.getFrameBuffer());

			RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA , EBlend::One, EBlend::One >::GetRHI());	
			RHISetShaderProgram(commandList, mProgAmbient->getRHIResource());
			mProgAmbient->setParameters(commandList, sceneRenderTargets, *mSSAOTextureBlur);
			DrawUtility::ScreenRect(commandList);		
		}
		return;
	}

	void PostProcessSSAO::drawSSAOTexture(RHICommandList& commandList, Matrix4 const& XForm, IntVector2 const& pos, IntVector2 const& size)
	{
		DrawUtility::DrawTexture(commandList, XForm, *mSSAOTextureBlur, pos, size);
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

	void ShadowProjectParam::setupShader(RHICommandList& commandList, ShaderProgram& program) const
	{
		program.setParam(commandList, SHADER_PARAM(ShadowParam), Vector2( shadowParam.x, shadowParam.y ));
		switch( light->type )
		{
		case LightType::Spot:
			program.setParam(commandList, SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 1);
			if( light->bCastShadow )
				program.setTexture(commandList, SHADER_PARAM(ShadowTexture2D), *(RHITexture2D*)shadowTexture);
			else
				program.setTexture(commandList, SHADER_PARAM(ShadowTexture2D), *GWhiteTexture2D);
			break;
		case LightType::Point:
			program.setParam(commandList, SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 6);
			if( light->bCastShadow )
				program.setTexture(commandList, SHADER_PARAM(ShadowTextureCube), *(RHITextureCube*)shadowTexture);
			else
				program.setTexture(commandList, SHADER_PARAM(ShadowTextureCube), *GWhiteTextureCube);
			break;
		case LightType::Directional:
			program.setParam(commandList, SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, numCascade);
			if( light->bCastShadow )
				program.setTexture(commandList, SHADER_PARAM(ShadowTexture2D), *(RHITexture2D*)shadowTexture);
			else
				program.setTexture(commandList, SHADER_PARAM(ShadowTexture2D), *GWhiteTexture2D);
			program.setParam(commandList, SHADER_PARAM(NumCascade), numCascade);
			program.setParam(commandList, SHADER_PARAM(CacadeDepth), cacadeDepth, numCascade);
			break;
		}
	}

	bool OITShaderData::init(int storageSize, IntVector2 const& screenSize)
	{
		colorStorageTexture = RHICreateTexture2D(ETexture::RGBA16F, storageSize, storageSize);
		VERIFY_RETURN_FALSE(colorStorageTexture.isValid());

		nodeAndDepthStorageTexture = RHICreateTexture2D(ETexture::RGBA32I, storageSize, storageSize);
		VERIFY_RETURN_FALSE(nodeAndDepthStorageTexture.isValid());

		nodeHeadTexture = RHICreateTexture2D(ETexture::R32U, screenSize.x, screenSize.y);
		VERIFY_RETURN_FALSE(nodeHeadTexture.isValid());

		storageUsageCounter = RHICreateVertexBuffer(sizeof(uint32) , 1 , BCF_DefalutValue | BCF_CpuAccessWrite );
		VERIFY_RETURN_FALSE(storageUsageCounter.isValid());

		return true;
	}


	void OITShaderData::releaseRHI()
	{
		colorStorageTexture.release();
		nodeAndDepthStorageTexture.release();
		nodeHeadTexture.release();
		storageUsageCounter.release();
	}

	class BMAResolveProgram : public GlobalShaderProgram
	{
	public:

		DECLARE_SHADER_PROGRAM(BMAResolveProgram, Global);

		void bindParameters(ShaderParameterMap const& parameterMap) override;

		void setParameters(RHICommandList& commandList, OITShaderData& data);

		ShaderParameter mParamColorStorageTexture;
		ShaderParameter mParamNodeAndDepthStorageTexture;
		ShaderParameter mParamNodeHeadTexture;

		static void SetupShaderCompileOption(ShaderCompileOption& option) 
		{
			option.addDefine(SHADER_PARAM(OIT_STORAGE_SIZE), OIT_StorageSize);
		}
		static char const* GetShaderFileName()
		{
			return "Shader/OITResolve";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(ResolvePS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(BMAResolveProgram);

	bool OITTechnique::init(IntVector2 const& size)
	{
		VERIFY_RETURN_FALSE(mShaderData.init(OIT_StorageSize, size));

		{
			ShaderCompileOption option;
			option.addDefine(SHADER_PARAM(OIT_STORAGE_SIZE), OIT_StorageSize);
			option.bShowComplieInfo = true;
			if( !ShaderManager::Get().loadFile(
				mShaderBassPassTest, "Shader/OITRender",
				SHADER_ENTRY(BassPassVS), SHADER_ENTRY(BassPassPS),
				option, nullptr) )
				return false;
			VERIFY_RETURN_FALSE( mShaderResolve = ShaderManager::Get().loadGlobalShaderT< BMAResolveProgram >(option) );
		}

		BMA_InternalValMin[NumBMALevel - 1] = 1;
		for( int i = 0; i < NumBMALevel; ++i )
		{
			if ( i != NumBMALevel - 1 )
				BMA_InternalValMin[i] = BMA_MaxPixelCounts[i + 1] + 1;

			ShaderCompileOption option;
			option.addDefine(SHADER_PARAM(OIT_MAX_PIXEL_COUNT) , BMA_MaxPixelCounts[i]);
			VERIFY_RETURN_FALSE(mShaderBMAResolves[i] = ShaderManager::Get().loadGlobalShaderT< BMAResolveProgram >(option));
		}

		{

			Vector4 v[] =
			{
				Vector4(1,1,0,1) , Vector4(-1,1,0,1) , Vector4(-1,-1,0,1) , Vector4(1,-1,0,1)
			};
			int indices[] = { 0 , 1 , 2 , 0 , 2 , 3 };
			mScreenMesh.mInputLayoutDesc.addElement(0, Vertex::ATTRIBUTE_POSITION, Vertex::eFloat4);
			if( !mScreenMesh.createRHIResource(v, 4, indices, 6, true) )
				return false;
		}

		VERIFY_RETURN_FALSE( mFrameBuffer = RHICreateFrameBuffer());
		mFrameBuffer->addTexture(*mShaderData.colorStorageTexture);
		mFrameBuffer->addTexture(*mShaderData.nodeAndDepthStorageTexture);

		return true;
	}

	void OITTechnique::render(RHICommandList& commandList, ViewInfo& view, SceneInterface& scnenRender, FrameRenderTargets* sceneRenderTargets)
	{

		auto DrawFun = [this, &view, &scnenRender](RHICommandList& commandList)
		{
			RenderContext context(commandList, view, *this);
			scnenRender.renderTranslucent(context);
		};

		renderInternal(commandList, view, DrawFun , sceneRenderTargets );

		if( 1 )
		{
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			//#TODO : Remove ViewportSaveScope, MatrixSaveScope
			ViewportSaveScope vpScope(commandList);
			Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, vpScope[2], 0, vpScope[3], -1, 1));
			MatrixSaveScope matrixScopt(porjectMatrix);

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
			DrawUtility::DrawTexture(commandList, porjectMatrix, *mShaderData.colorStorageTexture, IntVector2(0, 0), IntVector2(200, 200));
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}
	}

	void OITTechnique::renderTest(RHICommandList& commandList, ViewInfo& view, FrameRenderTargets& sceneRenderTargets, Mesh& mesh, Material* material)
	{
		auto DrawFun = [this , &view , &mesh , material ](RHICommandList& commandList)
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

			RHISetShaderProgram(commandList, mShaderBassPassTest.getRHIResource());
			view.setupShader(commandList, mShaderBassPassTest);
			mShaderBassPassTest.setRWTexture(commandList, SHADER_PARAM(ColorStorageRWTexture), *mShaderData.colorStorageTexture, AO_WRITE_ONLY);
			mShaderBassPassTest.setRWTexture(commandList, SHADER_PARAM(NodeAndDepthStorageRWTexture), *mShaderData.nodeAndDepthStorageTexture, AO_READ_AND_WRITE);
			mShaderBassPassTest.setRWTexture(commandList, SHADER_PARAM(NodeHeadRWTexture), *mShaderData.nodeHeadTexture, AO_READ_AND_WRITE);
			mShaderBassPassTest.setAtomicCounterBuffer(commandList, SHADER_PARAM(NextIndex), *mShaderData.storageUsageCounter);


			//
			mShaderBassPassTest.setParam(commandList, SHADER_PARAM(BaseColor), color[0]);

			mShaderBassPassTest.setParam(commandList, SHADER_PARAM(WorldTransform), Matrix4::Identity());
			mesh.draw(commandList);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			mShaderBassPassTest.setParam(commandList, SHADER_PARAM(WorldTransform), Matrix4::Translate(Vector3(10,0,0)));
			mesh.draw(commandList);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			mShaderBassPassTest.setParam(commandList, SHADER_PARAM(WorldTransform), Matrix4::Translate(Vector3(-10, 0, 0)));
			mesh.draw(commandList);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			
		};

		renderInternal(commandList, view, DrawFun);


		if (1)
		{
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			//#TODO : Remove ViewportSaveScope, MatrixSaveScope
			ViewportSaveScope vpScope(commandList);
			Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, vpScope[2], 0, vpScope[3], -1, 1));
			MatrixSaveScope matrixScopt(porjectMatrix);

			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			DrawUtility::DrawTexture(commandList, porjectMatrix, *mShaderData.colorStorageTexture, IntVector2(0, 0), IntVector2(200,200));
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}

	}

	void OITTechnique::reload()
	{
		ShaderManager::Get().reloadShader(mShaderBassPassTest);
		ShaderManager::Get().reloadShader(*mShaderResolve);
		for(auto shader : mShaderBMAResolves)
			ShaderManager::Get().reloadShader(*shader);
	}

	void OITTechnique::renderInternal(RHICommandList& commandList, ViewInfo& view, std::function< void(RHICommandList&) > drawFuncion , FrameRenderTargets* sceneRenderTargets )
	{
		GPU_PROFILE("OIT");
		
		RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());
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
					ShaderHelper::Get().clearBuffer(commandList, *mShaderData.colorStorageTexture, clearValueA);
					ShaderHelper::Get().clearBuffer(commandList, *mShaderData.nodeAndDepthStorageTexture, clearValueB);
				}
				else
				{
					RHISetFrameBuffer(commandList, mFrameBuffer);
					ViewportSaveScope vpScope(commandList);
					RHISetViewport(commandList, 0, 0, OIT_StorageSize, OIT_StorageSize);
					glClearBufferfv(GL_COLOR, 0, clearValueA);
					glClearBufferiv(GL_COLOR, 1, clearValueB);
					//glClearBufferuiv(GL_COLOR, 2, clearValueC);
				}
			}

			ShaderHelper::Get().clearBuffer(commandList, *mShaderData.nodeHeadTexture, clearValueC);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}

		if( sceneRenderTargets )
		{
			RHISetFrameBuffer(commandList, sceneRenderTargets->getFrameBuffer());
		}
		else
		{
			RHISetFrameBuffer(commandList, nullptr);
		}

		if( bUseBMA )
		{
			RHIClearRenderTargets(commandList, EClearBits::Stencil, nullptr, 0, 0, 0);

			RHISetDepthStencilState(commandList,
				TStaticDepthStencilState< 
					bWriteDepth , ECompareFunc::Less , 
					true , ECompareFunc::Always , EStencil::Keep , EStencil::Keep , EStencil::Incr , 0xff
				>::GetRHI(), 0 );

		}
		else
		{
			RHISetDepthStencilState(commandList, TStaticDepthStencilState< bWriteDepth >::GetRHI());
		}

		if( 1 )
		{
			GPU_PROFILE("BasePass");
			RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());

			uint32* value = (uint32*)RHILockBuffer(mShaderData.storageUsageCounter , ELockAccess::WriteOnly);
			*value = 0;
			RHIUnlockBuffer(mShaderData.storageUsageCounter);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			//GL_BIND_LOCK_OBJECT(sceneRenderTargets.getFrameBuffer());
			drawFuncion(commandList);

			RHIFlushCommand(commandList);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

		}

		if( 1 )
		{
			GPU_PROFILE("Resolve");

			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha >::GetRHI());
			ViewportSaveScope vpScope(commandList);
			OrthoMatrix matProj(0, vpScope[2], 0, vpScope[3], -1, 1);
			MatrixSaveScope matScope(matProj);

			if( bUseBMA )
			{
				
				for( int i = 0; i < NumBMALevel; ++i )
				{
					GPU_PROFILE("BMA=%d", BMA_MaxPixelCounts[i]);
					RHISetDepthStencilState(commandList,
						TStaticDepthStencilState< bWriteDepth, ECompareFunc::Always ,
							true , ECompareFunc::LessEqual , EStencil::Keep , EStencil::Zero , EStencil::Zero , 0xff 
						>::GetRHI() , BMA_InternalValMin[i]);

					BMAResolveProgram* shaderprogram = mShaderBMAResolves[i];
					RHISetShaderProgram(commandList, shaderprogram->getRHIResource());
					shaderprogram->setParameters(commandList, mShaderData );
					mScreenMesh.draw(commandList);
				}

			}
			else
			{
				RHISetDepthStencilState(commandList, TStaticDepthStencilState< bWriteDepth , ECompareFunc::Always >::GetRHI());

				BMAResolveProgram* shaderprogram = mShaderBMAResolves[0];
				RHISetShaderProgram(commandList, shaderprogram->getRHIResource());
				shaderprogram->setParameters(commandList, mShaderData);
				mScreenMesh.draw(commandList);
			}

			
		}

		if( sceneRenderTargets )
		{
			RHISetFrameBuffer(commandList, nullptr);
		}
		RHISetShaderProgram(commandList, nullptr);
		RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
		RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
	}


	class OITBBasePassProgram : public MaterialShaderProgram
	{
		using BaseClass = MaterialShaderProgram;
		DECLARE_EXPORTED_SHADER_PROGRAM(OITBBasePassProgram , Material, CORE_API);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			BaseClass::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(OIT_STORAGE_SIZE), OIT_StorageSize);
		}

		static char const* GetShaderFileName()
		{
			return "Shader/OITRender";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(BassPassVS) },
				{ EShader::Pixel  , SHADER_ENTRY(BassPassPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			BaseClass::bindParameters(parameterMap);
			mParamOITCommon.bindParameters(parameterMap);
		}

		void setParameters(RHICommandList& commandList, OITShaderData& data )
		{
			mParamOITCommon.setParameters(commandList, *this, data);
		}

		OITCommonParameter mParamOITCommon;
	};

	void OITTechnique::setupShader(RHICommandList& commandList, ShaderProgram& program)
	{
		program.setRWTexture(commandList, SHADER_PARAM(ColorStorageRWTexture), *mShaderData.colorStorageTexture, AO_WRITE_ONLY);
		program.setRWTexture(commandList, SHADER_PARAM(NodeAndDepthStorageRWTexture), *mShaderData.nodeAndDepthStorageTexture, AO_READ_AND_WRITE);
		program.setRWTexture(commandList, SHADER_PARAM(NodeHeadRWTexture), *mShaderData.nodeHeadTexture, AO_READ_AND_WRITE);
		program.setAtomicCounterBuffer(commandList, SHADER_PARAM(NextIndex), *mShaderData.storageUsageCounter);
	}

	MaterialShaderProgram* OITTechnique::getMaterialShader(RenderContext& context, MaterialMaster& material, VertexFactory* vertexFactory)
	{
		return material.getShaderT< OITBBasePassProgram >(vertexFactory);
	}

	void OITTechnique::setupMaterialShader(RenderContext& context, MaterialShaderProgram& program)
	{
		static_cast<OITBBasePassProgram&>(program).setParameters(context.getCommnadList(), mShaderData);
	}

	void BMAResolveProgram::bindParameters(ShaderParameterMap const& parameterMap)
{
		mParamColorStorageTexture.bind(parameterMap, SHADER_PARAM(ColorStorageTexture));
		mParamNodeAndDepthStorageTexture.bind(parameterMap, SHADER_PARAM(NodeAndDepthStorageTexture));
		mParamNodeHeadTexture.bind(parameterMap, SHADER_PARAM(NodeHeadTexture));
	}

	void BMAResolveProgram::setParameters(RHICommandList& commandList, OITShaderData& data )
	{
		setRWTexture(commandList, mParamColorStorageTexture, *data.colorStorageTexture, AO_READ_AND_WRITE);
		setRWTexture(commandList, mParamNodeAndDepthStorageTexture, *data.nodeAndDepthStorageTexture, AO_READ_AND_WRITE);
		setRWTexture(commandList, mParamNodeHeadTexture, *data.nodeHeadTexture, AO_READ_AND_WRITE);
	}

	void SSAOGenerateProgram::bindParameters(ShaderParameterMap const& parameterMap)
{
		mParamGBuffer.bindParameters(parameterMap , true);
		mParamKernelNum.bind(parameterMap, SHADER_PARAM(KernelNum));
		mParamKernelVectors.bind(parameterMap, SHADER_PARAM(KernelVectors));
		mParamOcclusionRadius.bind(parameterMap, SHADER_PARAM(OcclusionRadius));
	}

	void SSAOGenerateProgram::setParameters(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, Vector3 kernelVectors[], int numKernelVector)
	{
		mParamGBuffer.setParameters(commandList, *this, sceneRenderTargets);
		setParam(commandList, mParamKernelNum, (int)numKernelVector);
		setParam(commandList, mParamKernelVectors, &kernelVectors[0], numKernelVector);
		setParam(commandList, mParamOcclusionRadius, 0.5f);
	}

	void SSAOBlurProgram::bindParameters(ShaderParameterMap const& parameterMap)
{
		mParamTextureSSAO.bind(parameterMap, SHADER_PARAM(TextureSSAO));
		mParamTextureSamplerSSAO.bind(parameterMap, SHADER_PARAM(TextureSamplerSSAO));
	}

	void SSAOBlurProgram::setParameters(RHICommandList& commandList, RHITexture2D& SSAOTexture)
	{
		setTexture( commandList, mParamTextureSSAO, SSAOTexture , mParamTextureSamplerSSAO,
				   TStaticSamplerState<ESampler::Bilinear , ESampler::Clamp , ESampler::Clamp , ESampler::Clamp >::GetRHI() );
	}

	void SSAOAmbientProgram::bindParameters(ShaderParameterMap const& parameterMap)
{
		mParamGBuffer.bindParameters(parameterMap);
		mParamTextureSSAO.bind(parameterMap, SHADER_PARAM(TextureSSAO));
		mParamTextureSamplerSSAO.bind(parameterMap, SHADER_PARAM(TextureSamplerSSAO));
	}

	void SSAOAmbientProgram::setParameters(RHICommandList& commandList, FrameRenderTargets& sceneRenderTargets, RHITexture2D& SSAOTexture)
	{
		auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI();
		mParamGBuffer.setParameters(commandList, *this, sceneRenderTargets.getGBuffer(), sampler);
		setTexture(commandList, mParamTextureSSAO, SSAOTexture , mParamTextureSamplerSSAO ,
				   TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI());
	}

	void GBufferShaderParameters::bindParameters(ShaderParameterMap const& parameterMap, bool bUseDepth /*= false */)
	{
		BIND_TEXTURE_PARAM(parameterMap, GBufferTextureA);
		BIND_TEXTURE_PARAM(parameterMap, GBufferTextureB);
		BIND_TEXTURE_PARAM(parameterMap, GBufferTextureC);
		BIND_TEXTURE_PARAM(parameterMap, GBufferTextureD);
		
		if( bUseDepth )
		{
			BIND_TEXTURE_PARAM(parameterMap, FrameDepthTexture);
		}
	}

	void GBufferShaderParameters::setParameters(RHICommandList& commandList, ShaderProgram& program, GBufferResource& GBufferData, RHISamplerState& sampler )
	{
		if (mParamGBufferTextureA.isBound())
		{
			SetShaderTextureT(commandList, program, mParamGBufferTextureA, GBufferData.getResolvedTexture(EGBuffer::A), mParamGBufferTextureASampler, sampler);
		}
		if (mParamGBufferTextureB.isBound())
		{
			SetShaderTextureT(commandList, program, mParamGBufferTextureB, GBufferData.getResolvedTexture(EGBuffer::B), mParamGBufferTextureBSampler, sampler);
		}
		if (mParamGBufferTextureC.isBound())
		{
			SetShaderTextureT(commandList, program, mParamGBufferTextureC, GBufferData.getResolvedTexture(EGBuffer::C), mParamGBufferTextureCSampler, sampler);
		}
		if (mParamGBufferTextureD.isBound())
		{
			SetShaderTextureT(commandList, program, mParamGBufferTextureD, GBufferData.getResolvedTexture(EGBuffer::D), mParamGBufferTextureDSampler, sampler);
		}
	}

	void GBufferShaderParameters::setParameters(RHICommandList& commandList, ShaderProgram& program, FrameRenderTargets& sceneRenderTargets)
	{
		auto& sampler = TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI();
		setParameters(commandList, program, sceneRenderTargets.getGBuffer(), sampler);
		if( mParamFrameDepthTexture.isBound() )
		{
			program.setTexture(commandList, mParamFrameDepthTexture, sceneRenderTargets.getDepthTexture(), mParamFrameDepthTextureSampler , sampler);
		}
	}

	class DOFGenerateCoC : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(DOFGenerateCoC, Global);


	};

	IMPLEMENT_SHADER_PROGRAM(DOFGenerateCoC);

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
			option.addCode(sParamConstructor.code.c_str());
		}

		static char const* GetShaderFileName()
		{
			return "Shader/DOF";
		}

	};



	class DOFBlurVProgram : public DOFBlurBaseProgram
	{

		DECLARE_SHADER_PROGRAM(DOFBlurVProgram, Global);
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			DOFBlurBaseProgram::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM( DOF_PASS_STEP ), 1);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainBlurV) },
			};
			return entries;
		}
	};


	class DOFBlurHAndCombineProgram : public DOFBlurBaseProgram
	{
		DECLARE_SHADER_PROGRAM(DOFBlurHAndCombineProgram, Global);
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			DOFBlurBaseProgram::SetupShaderCompileOption(option);
			option.addDefine(SHADER_PARAM(DOF_PASS_STEP), 2);
		}

		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainBlurHAndCombine) },
			};
			return entries;
		}
		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamTextureR.bind(parameterMap, SHADER_PARAM(TextureR));
			mParamTextureG.bind(parameterMap, SHADER_PARAM(TextureG));
			mParamTextureB.bind(parameterMap, SHADER_PARAM(TextureB));
		}

		void setParameters(RHICommandList& commandList, RHITexture2D& textureR, RHITexture2D& textureG, RHITexture2D& textureB)
		{
			auto& sampler = TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp >::GetRHI();
			if ( mParamTextureR.isBound() )
				setTexture(commandList, mParamTextureR, textureR, mParamTextureR, sampler);
			if( mParamTextureG.isBound() )
				setTexture(commandList, mParamTextureG, textureG, mParamTextureG, sampler);
			if( mParamTextureB.isBound() )
				setTexture(commandList, mParamTextureB, textureB, mParamTextureB, sampler);
		}

		ShaderParameter mParamTextureR;
		ShaderParameter mParamTextureG;
		ShaderParameter mParamTextureB;
	};

	IMPLEMENT_SHADER_PROGRAM(DOFBlurVProgram);
	IMPLEMENT_SHADER_PROGRAM(DOFBlurHAndCombineProgram);

	bool PostProcessDOF::init(IntVector2 const& size)
	{
#if 0
		mProgGenCoc = ShaderManager::Get().getGlobalShaderT< DOFGenerateCoC >(true);
		if( mProgGenCoc == nullptr )
			return false;
#endif

		VERIFY_RETURN_FALSE(mProgBlurV = ShaderManager::Get().getGlobalShaderT< DOFBlurVProgram >(true));
		VERIFY_RETURN_FALSE(mProgBlurHAndCombine = ShaderManager::Get().getGlobalShaderT< DOFBlurHAndCombineProgram >(true));

		VERIFY_RETURN_FALSE(mTextureNear = RHICreateTexture2D(ETexture::RGBA32F, size.x, size.y));
		VERIFY_RETURN_FALSE(mTextureFar = RHICreateTexture2D(ETexture::RGBA32F, size.x, size.y));

		VERIFY_RETURN_FALSE(mFrameBufferGen = RHICreateFrameBuffer());

		mFrameBufferGen->addTexture(*mTextureNear);
		mFrameBufferGen->addTexture(*mTextureFar);

		VERIFY_RETURN_FALSE(mTextureBlurR = RHICreateTexture2D(ETexture::RGBA32F, size.x, size.y));
		VERIFY_RETURN_FALSE(mTextureBlurG = RHICreateTexture2D(ETexture::RGBA32F, size.x, size.y));
		VERIFY_RETURN_FALSE(mTextureBlurB = RHICreateTexture2D(ETexture::RGBA32F, size.x, size.y));

		VERIFY_RETURN_FALSE(mFrameBufferBlur = RHICreateFrameBuffer());
		mFrameBufferBlur->addTexture(*mTextureBlurR);
		mFrameBufferBlur->addTexture(*mTextureBlurG);
		mFrameBufferBlur->addTexture(*mTextureBlurB);

		return true;
	}


	void PostProcessDOF::render(RHICommandList& commandList, ViewInfo& view, FrameRenderTargets& sceneRenderTargets)
	{

		auto& sampler = TStaticSamplerState< ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp , ESampler::Clamp >::GetRHI();
		ViewportSaveScope vpScope(commandList);

		{
			RHISetFrameBuffer(commandList, mFrameBufferBlur);
			RHITexture2D& frameTexture = sceneRenderTargets.getFrameTexture();

			RHISetViewport(commandList, 0, 0, frameTexture.getSizeX(), frameTexture.getSizeY());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());


			RHISetShaderProgram(commandList, mProgBlurV->getRHIResource());
			mProgBlurV->setTexture(commandList, SHADER_PARAM(Texture), frameTexture , SHADER_PARAM(TextureSampler), sampler);

			DrawUtility::ScreenRect(commandList);
		}

		{
			RHISetFrameBuffer(commandList, sceneRenderTargets.getFrameBuffer());

			RHITexture2D& frameTexture = sceneRenderTargets.getPrevFrameTexture();
			RHISetViewport(commandList, 0, 0, frameTexture.getSizeX(), frameTexture.getSizeY());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());

			RHISetShaderProgram(commandList, mProgBlurHAndCombine->getRHIResource());
			mProgBlurHAndCombine->setParameters(commandList, *mTextureBlurR, *mTextureBlurG, *mTextureBlurB);
			//mProgBlurHAndCombine->setTexture(SHADER_PARAM(Texture), frameTexture, sampler);
			DrawUtility::ScreenRect(commandList);
		}	
	}

	class ClearBufferProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(ClearBufferProgram, Global);

		static int constexpr SizeX = 8;
		static int constexpr SizeY = 8;
		static int constexpr SizeZ = 8;


		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamBufferRW.bind(parameterMap, SHADER_PARAM(TargetRWTexture));
			mParamClearValue.bind(parameterMap, SHADER_PARAM(ClearValue));
		}

		void setParameters(RHICommandList& commandList, RHITexture3D& Buffer , Vector4 const& clearValue)
		{
			setRWTexture(commandList, mParamBufferRW, Buffer, AO_WRITE_ONLY);
			setParam(commandList, mParamClearValue, clearValue);
		}
		
		void clearTexture(RHICommandList& commandList, RHITexture3D& buffer , Vector4 const& clearValue)
		{
			int nx = (buffer.getSizeX() + SizeX - 1) / SizeX;
			int ny = (buffer.getSizeY() + SizeY - 1) / SizeY;
			int nz = (buffer.getSizeZ() + SizeZ - 1) / SizeZ;
			RHISetShaderProgram(commandList, getRHIResource());
			setParameters(commandList, buffer, clearValue);
			RHIDispatchCompute(commandList, nx, ny, nz);
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
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(BufferClearCS) },
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
		RHIVertexBuffer*  lightBuffer;
		int               numLights;
	};

	class LightScatteringProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(LightScatteringProgram, Global);

		static int constexpr GroupSizeX = 8;
		static int constexpr GroupSizeY = 8;

		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamVolumeBufferA.bind(parameterMap, SHADER_PARAM(VolumeBufferA));
			mParamVolumeBufferB.bind(parameterMap, SHADER_PARAM(VolumeBufferB));
			mParamScatteringRWBuffer.bind(parameterMap, SHADER_PARAM(ScatteringRWBuffer));
			mParamTiledLightNum.bind(parameterMap, SHADER_PARAM(TiledLightNum));
		}

		void setParameters(RHICommandList& commandList, ViewInfo& view , VolumetricLightingParameter& parameter )
		{
			if (mParamVolumeBufferA.isBound())
				setTexture(commandList, mParamVolumeBufferA, *parameter.volumeBuffer[0]);
			if (mParamVolumeBufferB.isBound())
				setTexture(commandList, mParamVolumeBufferB, *parameter.volumeBuffer[1]);
			if (mParamScatteringRWBuffer.isBound())
				setRWTexture(commandList, mParamScatteringRWBuffer, *parameter.scatteringBuffer[0], AO_WRITE_ONLY);

			setStructuredStorageBufferT<TiledLightInfo>(commandList, *parameter.lightBuffer);
			view.setupShader(commandList, *this);
			if (mParamTiledLightNum.isBound())
				setParam(commandList, mParamTiledLightNum, parameter.numLights);
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
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo entries[] =
			{
				{ EShader::Compute , SHADER_ENTRY(LightScatteringCS) },
			};
			return entries;
		}

		ShaderParameter mParamVolumeBufferA;
		ShaderParameter mParamVolumeBufferB;
		ShaderParameter mParamScatteringRWBuffer;
		ShaderParameter mParamTiledLightNum;
	};

	IMPLEMENT_SHADER_PROGRAM(ClearBufferProgram);
	IMPLEMENT_SHADER_PROGRAM(LightScatteringProgram);

	bool VolumetricLightingTech::init(IntVector2 const& screenSize)
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


	bool VolumetricLightingTech::setupBuffer(IntVector2 const& screenSize, int sizeFactor, int depthSlices)
	{
		int nx = (screenSize.x + sizeFactor - 1) / sizeFactor;
		int ny = (screenSize.y + sizeFactor - 1) / sizeFactor;

		mVolumeBufferA = RHICreateTexture3D(ETexture::RGBA16F, nx, ny, depthSlices);
		mVolumeBufferB = RHICreateTexture3D(ETexture::RGBA16F, nx, ny, depthSlices);
		mScatteringBuffer = RHICreateTexture3D(ETexture::RGBA16F, nx, ny, depthSlices);
		mTiledLightBuffer = RHICreateVertexBuffer(sizeof(TiledLightInfo) , MaxTiledLightNum);

		return mVolumeBufferA.isValid() && mVolumeBufferB.isValid() && mScatteringBuffer.isValid() && mTiledLightBuffer.isValid();
	}

	void VolumetricLightingTech::render(RHICommandList& commandList, ViewInfo& view, std::vector< LightInfo > const& lights)
	{
		{
			GPU_PROFILE("ClearBuffer");
			mProgClearBuffer->clearTexture(commandList, *mVolumeBufferA, Vector4(0, 0, 0, 0));
			mProgClearBuffer->clearTexture(commandList, *mVolumeBufferB, Vector4(0, 0, 0, 0));
			mProgClearBuffer->clearTexture(commandList, *mScatteringBuffer, Vector4(0, 0, 0, 0));
		}

		TiledLightInfo* pInfo = (TiledLightInfo*)RHILockBuffer( mTiledLightBuffer , ELockAccess::WriteOnly, 0, sizeof(TiledLightInfo) * lights.size());
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
		RHIUnlockBuffer(mTiledLightBuffer);

		VolumetricLightingParameter parameter;
		parameter.volumeBuffer[0] = mVolumeBufferA;
		parameter.volumeBuffer[1] = mVolumeBufferB;
		parameter.scatteringBuffer[0] = mScatteringBuffer;
		parameter.lightBuffer = mTiledLightBuffer;
		parameter.numLights = lights.size();


		{
			GPU_PROFILE("LightScattering");
			RHISetShaderProgram(commandList, mProgLightScattering->getRHIResource());
			mProgLightScattering->setParameters(commandList, view , parameter);
		}
	}

	
#if CORE_SHARE_CODE
	IMPLEMENT_SHADER_PROGRAM(OITBBasePassProgram);
#endif



}//namespace Render
