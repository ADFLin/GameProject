#include "TestRenderStageBase.h"
#include "RHI/SceneRenderer.h"
#include "Renderer/RenderTargetPool.h"
#include "RHI/RHIGlobalResource.h"
#include "RHI/Material.h"

#include "Renderer/BasePassRendering.h"
#include "Renderer/ShadowDepthRendering.h"
#include "ProfileSystem.h"
#include "Math/Curve.h"

#include "Editor.h"

namespace Render
{

	class DeferredRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		DeferredRenderingStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			mCamera.lookAt(Vector3(20, 20, 4), Vector3(10, 10, 0), Vector3(0, 0, 1));
			IntVector2 screenSize = ::Global::GetScreenSize();
			::Global::GUI().cleanupWidget();

			auto frameWidget = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "bShowGBuffer"), bShowGBuffer);
			FWidgetProperty::Bind(frameWidget->addCheckBox(UI_ANY, "bPause"), mbPause);
			return true;
		}


		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1280;
			systemConfigs.screenHeight = 720;
			systemConfigs.bDebugMode = true;
		}


		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));

			VERIFY_RETURN_FALSE(SharedAssetData::createSimpleMesh());
			VERIFY_RETURN_FALSE(SharedAssetData::loadCommonShader());


			char const* MatFiles[] = { "Simple1" , "Simple2" };
			for (int i = 0; i < ARRAY_SIZE(mMaterials); ++i)
			{
				VERIFY_RETURN_FALSE(mMaterials[i].loadFile(MatFiles[i]));
			}
			

			VERIFY_RETURN_FALSE(mShadowFrameBuffer = RHICreateFrameBuffer());
			VERIFY_RETURN_FALSE(mGBufferFrameBuffer = RHICreateFrameBuffer());

			VERIFY_RETURN_FALSE(mSceneRenderTargets.initializeRHI());
			VERIFY_RETURN_FALSE(mDebugPrimitives.initializeRHI());

			LightInfo light;

			light.type = ELightType::Spot;
			light.bCastShadow = true;
			light.dir = Vector3(1, 0, -5).getNormal();
			light.intensity = 100;
			light.radius = 20;
			light.spotAngle = Vector3(45, 45, 0);

			light.pos = Vector3(10, 10, 10);
			light.color = Vector3(0, 1, 0);
			addLight(light);
			light.pos = Vector3(10, 5, 10);
			light.color = Vector3(1, 0, 0);
			light.dir = Vector3(1, 0, -2).getNormal();
			addLight(light);

			light.type = ELightType::Directional;
			light.intensity = 10;
			light.pos = Vector3(10, 10, 10);
			light.color = Vector3(0.1,0.1, 0.1);
			light.dir = Vector3(1, 0, -2).getNormal();
			addLight(light);

			CycleTrack& track = mTrack;
			track.center = Vector3(12.5, 12.5 , 8);
			track.radius = 8;
			track.period = 10;
			track.planeNormal = Vector3(0.2, 0, 1);

			if (::Global::Editor())
			{
				DetailViewConfig config;
				config.name = "LightInfo";
				IEditorDetailView* detailView = ::Global::Editor()->createDetailView(config);
				detailView->addStruct(mLights[0].light);
			}
			return true;
		}

		bool mbPause = false;
		CycleTrack mTrack;

		void addLight(LightInfo const& light)
		{
			LightSceneInfo* info = mLights.addDefault(1);
			info->light = light;
		}

		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mSceneRenderTargets.releaseRHI();
			mShadowFrameBuffer.release();
			mGBufferFrameBuffer.release();
			GRenderTargetPool.releaseRHI();
			mDebugPrimitives.releaseRHI();
			for( int i = 0 ; i < ARRAY_SIZE(mMaterials); ++i)
				mMaterials[i].releaseRHI();

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		MaterialMaster mMaterials[2];
		bool bShowGBuffer = false;
		bool bShowShadowFrustum = true;
		bool bShowLight = true;

		FrameRenderTargets mSceneRenderTargets;
		RHIFrameBufferRef  mShadowFrameBuffer;
		RHIFrameBufferRef  mGBufferFrameBuffer;
		PrimitivesCollection mDebugPrimitives;

		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void tick() override
		{

		}

		void updateFrame(int frame) override
		{
			if (mbPause)
				return;

			mLights[0].light.pos = mTrack.getValue(mView.gameTime);
			mLights[0].light.dir = (Vector3(12.5, 12.5, 0) - mLights[0].light.pos ).getNormal();
		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);
		}



		struct MeshProcesser
		{

			void setupMaterial(Material& material)
			{


			}
			void process(Mesh& mesh, Matrix4 const& localToWorld)
			{



			}
		};

		struct MeshProcesserBasePass
		{
			MeshProcesserBasePass(RHICommandList& commandList, ViewInfo& view)
				:commandList(commandList)
				,view(view)
			{
			}

			void setupMaterial(MaterialMaster& material)
			{
				shaderProgram = material.getShaderT< DeferredBasePassProgram >(nullptr);
				if (shaderProgram == nullptr)
				{
					return;
				}
				RHISetShaderProgram(commandList, shaderProgram->getRHI());
				view.setupShader(commandList, *shaderProgram);
			}

			void process(Mesh& mesh, Matrix4 const& localToWorld)
			{
				Matrix4 worldToLocal;
				float det;
				localToWorld.inverseAffine(worldToLocal, det);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), localToWorld);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);
				mesh.draw(commandList, LinearColor(1, 1, 1, 1));
			}

			ShaderProgram*  shaderProgram;
			ViewInfo&       view;
			RHICommandList& commandList;

		};

		struct MeshProcesserShadowDepth
		{
			MeshProcesserShadowDepth(RHICommandList& commandList, ViewInfo& view)
				:commandList(commandList)
				, view(view)
			{
			}

			void setupMaterial(MaterialMaster& material)
			{
				bUsePositionOnly = material.isUseShadowPositionOnly();
				if ( bUsePositionOnly )
				{
					shaderProgram = material.getShaderT< ShadowDepthPositionOnlyProgram >(nullptr);
				}
				else
				{
					shaderProgram = material.getShaderT< ShadowDepthProgram >(nullptr);
				}
				if (shaderProgram == nullptr)
				{
					return;
				}
				RHISetShaderProgram(commandList, shaderProgram->getRHI());
				shaderProgram->setParam(commandList, SHADER_PARAM(DepthShadowOffset), shadowOffset);
				shaderProgram->setParam(commandList, SHADER_PARAM(DepthShadowMatrix), shadowMatrix);
				view.setupShader(commandList, *shaderProgram);

			}

			void process(Mesh& mesh, Matrix4 const& localToWorld)
			{
				Matrix4 worldToLocal;
				float det;
				localToWorld.inverseAffine(worldToLocal, det);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.localToWorld), localToWorld);
				shaderProgram->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), worldToLocal);
				if (bUsePositionOnly)
				{
					mesh.drawPositionOnly(commandList);
				}
				else
				{
					mesh.draw(commandList, LinearColor(1, 1, 1, 1));
				}
			}

			bool            bUsePositionOnly = true;
			Vector3         shadowOffset;
			Matrix4         shadowMatrix;

			ShaderProgram*  shaderProgram;
			ViewInfo&       view;
			RHICommandList& commandList;

		};

		template< typename TMeshProcesser >
		void renderScene(TMeshProcesser& processer)
		{
			PROFILE_ENTRY("RenderScene");

			{
				//SetupTransform(Matrix4::Identity());
				//DrawUtility::AixsLine(commandList);
			}

			processer.setupMaterial(mMaterials[0]);
			//for( int layer = 0 ; layer < 1; ++layer )
			{
				for (int i = 0; i < 100; ++i)
				{
					int x = i / 10;
					int y = i % 10;
					Vector3 pos = Vector3(2.5 * (i / 10), 2.5 * (i % 10), 1);
					if ((x + y) % 2)
						pos.z += 2.5;
					//pos.z += 2.5 * layer;

					processer.process(mSimpleMeshs[SimpleMeshId::Doughnut], Matrix4::Scale(0.4) * Matrix4::Translate(pos));
				}
			}

			processer.setupMaterial(mMaterials[1]);
			processer.process(mSimpleMeshs[SimpleMeshId::Plane], Matrix4::Scale(20));
		}

		static Vector3 GetUpDir(Vector3 const& dir)
		{
			Vector3 result = dir.cross(Vector3(0, 0, 1));
			if (dir.y * dir.y > 0.99998)
				return Vector3(1, 0, 0);
			return dir.cross(Vector3(0, 0, 1)).cross(dir);
		}


		struct LightSceneInfo
		{
			LightInfo light;
			ShadowProjectParam shadowProjectParam;
		};

		TArray< LightSceneInfo > mLights;

		int mIndex = 0;
		void renderSpotShadowTexture(RHICommandList& commandList, LightInfo const& light, ShadowProjectParam& projectParam)
		{

			int shadowTexSize = 1024;

			RenderTargetDesc desc;
			desc.format = ETexture::ShadowDepth;
			desc.size = IntVector2(shadowTexSize, shadowTexSize);
			desc.numSamples = 1;
			desc.creationFlags = TCF_CreateSRV;
			desc.debugName = InlineString<>::Make("ShadowTex%d", mIndex);


			PooledRenderTargetRef shadowRT = GRenderTargetPool.fetchElement(desc);
			mShadowFrameBuffer->setDepth(*shadowRT->texture);

#if 0
			//RenderTargetDesc desc;
			desc.format = ETexture::RGBA32F;
			desc.size = IntVector2(shadowTexSize, shadowTexSize);
			desc.numSamples = 1;
			desc.creationFlags = TCF_CreateSRV;
			desc.debugName = InlineString<>::Make("ShadowTexDebug%d", mIndex);
			PooledRenderTargetRef shadowRTDebug = GRenderTargetPool.fetchElement(desc);
			mShadowFrameBuffer->setTexture(0, *shadowRTDebug->texture);
#endif

			RHISetViewport(commandList, 0, 0, shadowTexSize, shadowTexSize);
			RHISetFrameBuffer(commandList, mShadowFrameBuffer);
			RHIClearRenderTargets(commandList, EClearBits::Depth, nullptr, 0);

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

			Matrix4 shadowProject;
			if (FRHIZBuffer::IsInverted)
			{
				shadowProject = ReversedZPerspectiveMatrix(Math::DegToRad(2.0 * Math::Min<float>(89.99, light.spotAngle.y)), 1.0, 0.01, light.radius);
			}
			else
			{
				shadowProject = PerspectiveMatrix(Math::DegToRad(2.0 * Math::Min<float>(89.99, light.spotAngle.y)), 1.0, 0.01, light.radius);
			}

			Matrix4 worldToLight = LookAtMatrix(Vector3::Zero(), light.dir, GetUpDir(light.dir));
			Matrix4 shadowMatrix = worldToLight * AdjProjectionMatrixForRHI(shadowProject);


			MeshProcesserShadowDepth processer(commandList, mView);
			processer.shadowOffset = light.pos;
			processer.shadowMatrix = shadowMatrix;

			Matrix4 biasMatrix(
				0.5, 0, 0, 0,
				0, 0.5, 0, 0,
				0, 0, 1, 0,
				0.5, 0.5, 0, 1);

			renderScene(processer);

			projectParam.setupLight(light);
			projectParam.shadowMatrix[0] = shadowMatrix * biasMatrix;
			projectParam.shadowTexture = shadowRT->texture;
			projectParam.shadowOffset = light.pos;
			projectParam.shadowMatrixTest = worldToLight * shadowProject;
		}

		static void DetermineCascadeSplitDist(float nearDist, float farDist, int numCascade, float lambda, float outDist[])
		{
			for (int i = 1; i <= numCascade; ++i)
			{
				float Clog = nearDist * Math::Pow(farDist / nearDist, float(i) / numCascade);
				float Cuni = nearDist + (farDist - nearDist) * (float(i) / numCascade);
				outDist[i - 1] = Math::Lerp(Clog, Cuni, lambda);
			}
		}
		void renderCascadeShadowTexture(RHICommandList& commandList, LightInfo const& light, ShadowProjectParam& projectParam)
		{
			projectParam.setupLight(light);
			return;

			int shadowTexSize = 1024;

			RenderTargetDesc desc;
			desc.format = ETexture::ShadowDepth;
			desc.size = IntVector2(shadowTexSize, shadowTexSize);
			desc.numSamples = 1;
			desc.creationFlags = TCF_CreateSRV;
			desc.debugName = InlineString<>::Make("ShadowTex%d", mIndex);


			PooledRenderTargetRef shadowRT = GRenderTargetPool.fetchElement(desc);
			mShadowFrameBuffer->setDepth(*shadowRT->texture);

#if 0
			//RenderTargetDesc desc;
			desc.format = ETexture::RGBA32F;
			desc.size = IntVector2(shadowTexSize, shadowTexSize);
			desc.numSamples = 1;
			desc.creationFlags = TCF_CreateSRV;
			desc.debugName = InlineString<>::Make("ShadowTexDebug%d", mIndex);
			PooledRenderTargetRef shadowRTDebug = GRenderTargetPool.fetchElement(desc);
			mShadowFrameBuffer->setTexture(0, *shadowRTDebug->texture);
#endif

			RHISetViewport(commandList, 0, 0, shadowTexSize, shadowTexSize);
			RHISetFrameBuffer(commandList, mShadowFrameBuffer);
			RHIClearRenderTargets(commandList, EClearBits::Depth, nullptr, 0);

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

			Matrix4 shadowProject = PerspectiveMatrix(Math::DegToRad(2.0 * Math::Min<float>(89.99, light.spotAngle.y)), 1.0, 0.01, light.radius);
			Matrix4 worldToLight = LookAtMatrix(Vector3::Zero(), light.dir, GetUpDir(light.dir));
			Matrix4 shadowMatrix = worldToLight * AdjProjectionMatrixForRHI(shadowProject);

			MeshProcesserShadowDepth processer(commandList, mView);
			processer.shadowOffset = light.pos;
			processer.shadowMatrix = shadowMatrix;
			Matrix4 biasMatrix(
				0.5, 0, 0, 0,
				0, 0.5, 0, 0,
				0, 0, 1, 0,
				0.5, 0.5, 0, 1);

			renderScene(processer);

			projectParam.setupLight(light);
			projectParam.shadowMatrix[0] = shadowMatrix * biasMatrix;
			projectParam.shadowTexture = shadowRT->texture;
			projectParam.shadowOffset = light.pos;

		}
		void renderShadowTexture(RHICommandList& commandList, LightInfo const& light, ShadowProjectParam& projectParam)
		{
			CHECK(light.bCastShadow);

			switch (light.type)
			{
			case ELightType::Spot:
				renderSpotShadowTexture(commandList, light, projectParam);
				break;
			case ELightType::Point:
				break;
			case ELightType::Directional:
				renderCascadeShadowTexture(commandList, light, projectParam);
				break;
			}
		}


		template< class TFunc >
		void visitLights(TFunc& func)
		{
			int index = 0;
			for (auto& lightScene : mLights)
			{
				func(index, lightScene.light);
			}
		}

		void showLights(RHICommandList& commandList, ViewInfo& view)
		{
			RHISetShaderProgram(commandList, mProgSphere->getRHI());

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());

			float radius = 0.15f;
			view.setupShader(commandList, *mProgSphere);
			mProgSphere->setParam(commandList, SHADER_PARAM(Primitive.worldToLocal), Matrix4::Identity());

			visitLights([this, &view, radius, &commandList](int index, LightInfo const& light)
			{
#if 0
				if (!light.testVisible(view))
					return;
#endif

				mProgSphere->setParameters(commandList, light.pos, radius, light.color);
				mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);
			});
		}
		void onRender(float dFrame) override
		{
			initializeRenderState();

			GRenderTargetPool.freeAllUsedElements();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			Vec2i screenSize = ::Global::GetScreenSize();

			mSceneRenderTargets.prepare(screenSize, 1);
			{
				PROFILE_ENTRY("Base Pass");


				mGBufferFrameBuffer->setTexture(0, mSceneRenderTargets.getFrameTexture());
				mSceneRenderTargets.getGBuffer().attachToBuffer(*mGBufferFrameBuffer);
				mGBufferFrameBuffer->setDepth(mSceneRenderTargets.getDepthTexture());

				{
					GPU_PROFILE("Clear");
					RHISetFrameBuffer(commandList, mGBufferFrameBuffer);
					LinearColor clearColors[EGBuffer::Count + 1] = { LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0),LinearColor(0, 0, 0, 0) };
					RHIClearRenderTargets(commandList, EClearBits::All, clearColors, EGBuffer::Count + 1);
					RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				}

				{
					GPU_PROFILE("Base Pass");
					MeshProcesserBasePass processer(commandList, mView);
					renderScene(processer);
				}
			}

			{
				PROFILE_ENTRY("Render Shadow");
				GPU_PROFILE("Render Shadow");
				mIndex = 0;
				for (auto& lightInfo : mLights)
				{
					if (lightInfo.light.bCastShadow)
					{
						renderShadowTexture(commandList, lightInfo.light, lightInfo.shadowProjectParam);
					}
					++mIndex;
				}
			}



			{
				PROFILE_ENTRY("Lighting");
				GPU_PROFILE("Lighting");

				mSceneRenderTargets.detachDepthTexture();
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

				RHISetBlendState(commandList, TStaticBlendState< CWM_RGBA, EBlend::One, EBlend::One >::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState< ECullMode::None >::GetRHI());


				auto RenderLighting = [&](LightInfo const& light, ShadowProjectParam const& projectParam)
				{
					DeferredLightingProgram::PermutationDomain permutationVector;
					permutationVector.set<DeferredLightingProgram::UseLightType>((int)light.type);
					permutationVector.set<DeferredLightingProgram::HaveBoundShape>(false);
					DeferredLightingProgram* progLiginting = ShaderManager::Get().getGlobalShaderT<DeferredLightingProgram>(permutationVector);

					RHISetShaderProgram(commandList, progLiginting->getRHI());

					projectParam.setupShader(commandList, *progLiginting);
					mView.setupShader(commandList, *progLiginting);
					progLiginting->setParamters(commandList, mSceneRenderTargets);
					light.setupShaderGlobalParam(commandList, *progLiginting);

					DrawUtility::ScreenRect(commandList);

					projectParam.clearShaderResource(commandList, *progLiginting);
					progLiginting->clearTextures(commandList);

				};

				for (auto const& lightInfo : mLights)
				{
					RenderLighting(lightInfo.light, lightInfo.shadowProjectParam);
				}

				//mProgLiginting->clearTextures(commandList);
			}

			if (bShowLight)
			{
				GPU_PROFILE("ShowLight");
				mSceneRenderTargets.attachDepthTexture();
				{
					RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
					RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
					RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
					showLights(commandList, mView);
				}
				mSceneRenderTargets.detachDepthTexture();
			}

			if (bShowShadowFrustum)
			{
				mDebugPrimitives.clear();
				for (int i = 0; i < mLights.size(); ++i)
				{
					if (!mLights[i].light.bCastShadow)
						continue;
					ShadowProjectParam const& projecParam = mLights[i].shadowProjectParam;

					Matrix4 projectMatrixInv;
					float det;
					projecParam.shadowMatrixTest.inverse(projectMatrixInv, det);
					Vector3 vertices[8];
					ViewInfo::GetFrustumVertices(projectMatrixInv, vertices, true);
					for (int i = 0; i < 8; ++i)
					{
						vertices[i] += projecParam.shadowOffset;
					}

					for (int i = 0; i < 4; ++i)
					{
						mDebugPrimitives.addLine(vertices[i], vertices[4 + i], Color3f(1,0,0));
						mDebugPrimitives.addLine(vertices[i], vertices[(i + 1) % 4], Color3f(1, 0, 0));
						mDebugPrimitives.addLine(vertices[i + 4], vertices[4 + (i + 1) % 4], Color3f(1, 0, 0));
					}
				}

#if 1
				mSceneRenderTargets.attachDepthTexture();		
				RHISetFrameBuffer(commandList, mSceneRenderTargets.getFrameBuffer());
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<false>::GetRHI());
				mDebugPrimitives.drawDynamic(commandList, mView);
				mSceneRenderTargets.detachDepthTexture();
#endif
			}



			bitBltToBackBuffer(commandList, mSceneRenderTargets.getFrameTexture());

			if (bShowGBuffer)
			{
				GPU_PROFILE("Draw GBuffer");
				RHISetShaderProgram(commandList, nullptr);
				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				Matrix4 porjectMatrix = AdjProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0,  -1, 1));
				mSceneRenderTargets.getGBuffer().drawTextures(commandList, porjectMatrix, screenSize / 4, IntVector2(2, 2));
			}


			GTextureShowManager.registerRenderTarget(GRenderTargetPool);
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				default:
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

	protected:
	};

	REGISTER_STAGE_ENTRY("Deferred Rendering", DeferredRenderingStage, EExecGroup::FeatureDev, "Render");

}