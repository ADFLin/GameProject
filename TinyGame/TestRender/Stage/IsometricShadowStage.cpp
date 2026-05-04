#include "Stage/TestRenderStageBase.h"

#include "RHI/RHIGraphics2D.h"
#include "Editor.h"

namespace Render
{
	class MeshCaptureProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(MeshCaptureProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }
		static TArrayView<ShaderEntryInfo const> GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex, SHADER_ENTRY(MainVS) },
				{ EShader::Pixel , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);
			BIND_SHADER_PARAM(parameterMap, HeightRange);
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(HeightRange);
	};

	class ClearShadowMapCS : public GlobalShader
	{
	public:
		DECLARE_SHADER(ClearShadowMapCS, Global);
		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, ShadowMapRW);
			BIND_SHADER_PARAM(parameterMap, ClearDepthValue);
			BIND_SHADER_PARAM(parameterMap, ShadowTextureSize);
		}

		DEFINE_SHADER_PARAM(ShadowMapRW);
		DEFINE_SHADER_PARAM(ClearDepthValue);
		DEFINE_SHADER_PARAM(ShadowTextureSize);
	};

	class ProjectWorldToShadowCS : public GlobalShader
	{
	public:
		DECLARE_SHADER(ProjectWorldToShadowCS, Global);
		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, WorldPositionTexture);
			BIND_SHADER_PARAM(parameterMap, ShadowMapRW);
			BIND_SHADER_PARAM(parameterMap, ShadowDebugRW);
			BIND_SHADER_PARAM(parameterMap, WorldPosTransform);
			BIND_SHADER_PARAM(parameterMap, WorldToShadowClip);
			BIND_SHADER_PARAM(parameterMap, CaptureClipToWorld);
			BIND_SHADER_PARAM(parameterMap, HeightRange);
			BIND_SHADER_PARAM(parameterMap, ShadowTextureSize);
			BIND_SHADER_PARAM(parameterMap, ShadowTriangleRejectThreshold);
		}

		DEFINE_TEXTURE_PARAM(WorldPositionTexture);
		DEFINE_SHADER_PARAM(ShadowMapRW);
		DEFINE_SHADER_PARAM(ShadowDebugRW);
		DEFINE_SHADER_PARAM(WorldPosTransform);
		DEFINE_SHADER_PARAM(WorldToShadowClip);
		DEFINE_SHADER_PARAM(CaptureClipToWorld);
		DEFINE_SHADER_PARAM(HeightRange);
		DEFINE_SHADER_PARAM(ShadowTextureSize);
		DEFINE_SHADER_PARAM(ShadowTriangleRejectThreshold);
	};

	class FixBackFaceTextureCS : public GlobalShader
	{
	public:
		DECLARE_SHADER(FixBackFaceTextureCS, Global);
		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, FrontFaceTexture);
			BIND_SHADER_PARAM(parameterMap, BackFaceTextureRW);
			BIND_SHADER_PARAM(parameterMap, BackFaceTextureSize);
			BIND_SHADER_PARAM(parameterMap, BackFaceHeightSnapThreshold);
		}

		DEFINE_TEXTURE_PARAM(FrontFaceTexture);
		DEFINE_SHADER_PARAM(BackFaceTextureRW);
		DEFINE_SHADER_PARAM(BackFaceTextureSize);
		DEFINE_SHADER_PARAM(BackFaceHeightSnapThreshold);
	};

	class ShadowMapVisualizeProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(ShadowMapVisualizeProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }
		static TArrayView<ShaderEntryInfo const> GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex, SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel , SHADER_ENTRY(VisualizeShadowPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, SourceShadowTexture);
		}

		DEFINE_TEXTURE_PARAM(SourceShadowTexture);
	};

	class ProceduralShadowDepthProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(ProceduralShadowDepthProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }
		static TArrayView<ShaderEntryInfo const> GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex, SHADER_ENTRY(ProceduralShadowDepthVS) },
				{ EShader::Pixel , SHADER_ENTRY(ProceduralShadowDepthPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, SourceWorldPosTexture);
			BIND_SHADER_PARAM(parameterMap, WorldPosTransform);
			BIND_SHADER_PARAM(parameterMap, WorldToShadowClip);
			BIND_SHADER_PARAM(parameterMap, CaptureClipToWorld);
			BIND_SHADER_PARAM(parameterMap, HeightRange);
			BIND_SHADER_PARAM(parameterMap, ProceduralTextureSize);
		}

		DEFINE_TEXTURE_PARAM(SourceWorldPosTexture);
		DEFINE_SHADER_PARAM(WorldPosTransform);
		DEFINE_SHADER_PARAM(WorldToShadowClip);
		DEFINE_SHADER_PARAM(CaptureClipToWorld);
		DEFINE_SHADER_PARAM(HeightRange);
		DEFINE_SHADER_PARAM(ProceduralTextureSize);
	};

	struct SceneRenderParams
	{
		DECLARE_UNIFORM_BUFFER_STRUCT(SceneRenderParamsBlock);

		Matrix4 WorldToShadowClip;
		Matrix4 WorldToClip;
		Matrix4 XForm;
		Vector4 ShadowParams;
	};

	class SceneRenderProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(SceneRenderProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricRender"; }
		static TArrayView<ShaderEntryInfo const> GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex, SHADER_ENTRY(RectVS) },
				{ EShader::Pixel , SHADER_ENTRY(SceneRenderPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, SourceWorldPosTexture);
			BIND_TEXTURE_PARAM(parameterMap, SourceShadowTexture);
			BIND_TEXTURE_PARAM(parameterMap, SourceShadowDepthTexture);
			BIND_SHADER_PARAM(parameterMap, UVRect);
			BIND_SHADER_PARAM(parameterMap, WorldPosTransform);
			BIND_SHADER_PARAM(parameterMap, CaptureClipToWorld);
			BIND_SHADER_PARAM(parameterMap, HeightRange);
			BIND_SHADER_PARAM(parameterMap, BaseColor);
		}

		DEFINE_TEXTURE_PARAM(SourceWorldPosTexture);
		DEFINE_TEXTURE_PARAM(SourceShadowTexture);
		DEFINE_TEXTURE_PARAM(SourceShadowDepthTexture);
		DEFINE_SHADER_PARAM(UVRect);
		DEFINE_SHADER_PARAM(WorldPosTransform);
		DEFINE_SHADER_PARAM(CaptureClipToWorld);
		DEFINE_SHADER_PARAM(HeightRange);
		DEFINE_SHADER_PARAM(BaseColor);
	};

	IMPLEMENT_SHADER_PROGRAM(MeshCaptureProgram);
	IMPLEMENT_SHADER(ClearShadowMapCS, EShader::Compute, SHADER_ENTRY(ClearShadowMapCS));
	IMPLEMENT_SHADER(ProjectWorldToShadowCS, EShader::Compute, SHADER_ENTRY(ProjectWorldToShadowCS));
	IMPLEMENT_SHADER(FixBackFaceTextureCS, EShader::Compute, SHADER_ENTRY(FixBackFaceTextureCS));
	IMPLEMENT_SHADER_PROGRAM(ShadowMapVisualizeProgram);
	IMPLEMENT_SHADER_PROGRAM(ProceduralShadowDepthProgram);
	IMPLEMENT_SHADER_PROGRAM(SceneRenderProgram);

	class IsometricShadowStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
		using BoundBox = Math::TAABBox<Vector3>;

		struct RenderTargetBundle
		{
			IntVector2        size = IntVector2(0, 0);
			RHITexture2DRef   colorTextures[2];
			RHITexture2DRef   depthTexture;
			RHIFrameBufferRef frameBuffer;
			Matrix4           captureClipToWorld;
			Vector2           heightRange = Vector2(0, 1);
			Vector4           uvRect;
		};

	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			return true;
		}

		void onEnd() override
		{
			if (mDetailView)
			{
				mDetailView->release();
				mDetailView = nullptr;
			}
			BaseClass::onEnd();
		}

		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(createSimpleMesh());

			VERIFY_RETURN_FALSE(mProgMeshCapture = ShaderManager::Get().getGlobalShaderT<MeshCaptureProgram>(true));
			VERIFY_RETURN_FALSE(mClearShadowMapCS = ShaderManager::Get().getGlobalShaderT<ClearShadowMapCS>(true));
			VERIFY_RETURN_FALSE(mProjectWorldToShadowCS = ShaderManager::Get().getGlobalShaderT<ProjectWorldToShadowCS>(true));
			VERIFY_RETURN_FALSE(mFixBackFaceTextureCS = ShaderManager::Get().getGlobalShaderT<FixBackFaceTextureCS>(true));
			VERIFY_RETURN_FALSE(mShadowVisualizeProgram = ShaderManager::Get().getGlobalShaderT<ShadowMapVisualizeProgram>(true));
			VERIFY_RETURN_FALSE(mProceduralShadowDepthProgram = ShaderManager::Get().getGlobalShaderT<ProceduralShadowDepthProgram>(true));
			VERIFY_RETURN_FALSE(mProgSceneRender = ShaderManager::Get().getGlobalShaderT<SceneRenderProgram>(true));

			mCaptureCamera.lookAt(Vector3(14, 14, 14), Vector3(0, 0, 0), Vector3(0, 0, 1));
			updateLightCamera();


			mPlaneObjectTransform.pos = Vector3(0.0f, 0.0f, 0.0f);
			mPlaneObjectTransform.scale = 10.0f;
			mPlaneTransform = mPlaneObjectTransform.toMatrix();
			mPlaneBounds = BoundBox(Vector3(-1, -1, 0), Vector3(1, 1, 0));
			mDoughnutTextureTransform = Matrix4::Translate(Vector3(0, 0, 0.0f));
			mDoughnutBounds = BoundBox(Vector3(-1, -1, -1), Vector3(1, 1, 1));

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			captureMeshObject(commandList, mPlaneTarget, Matrix4::Identity(), mPlaneBounds, false, [&](RHICommandList& commandList)
			{
				getMesh(SimpleMeshId::Plane).draw(commandList);
			});

			captureMeshObject(commandList, mDoughnutTarget, mDoughnutTextureTransform, mDoughnutBounds, true, [&](RHICommandList& commandList)
			{
				getMesh(SimpleMeshId::Box).draw(commandList);
			});

			setupScene();
			setupCaptureView();


			VERIFY_RETURN_FALSE(mShadowMapTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::R32U, ShadowTextureSize, ShadowTextureSize)
				.Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue)));
			VERIFY_RETURN_FALSE(mShadowDebugTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::RGBA8, ShadowTextureSize, ShadowTextureSize)
				.Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue)));
			VERIFY_RETURN_FALSE(mShadowDebugFrameBuffer = RHICreateFrameBuffer());
			mShadowDebugFrameBuffer->setTexture(0, *mShadowDebugTexture);

			VERIFY_RETURN_FALSE(mProceduralShadowDepthTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::Depth32F, ShadowTextureSize, ShadowTextureSize)
				.Flags(TCF_CreateSRV | TCF_DefalutValue)));
			VERIFY_RETURN_FALSE(mProceduralShadowDepthFrameBuffer = RHICreateFrameBuffer());
			mProceduralShadowDepthFrameBuffer->setDepth(*mProceduralShadowDepthTexture);

			if (::Global::Editor())
			{
				DetailViewConfig config;
				config.name = "Isometric Shadow";
				mDetailView = ::Global::Editor()->createDetailView(config);
				auto rotationHandle = mDetailView->addValue(mLightRotation, "LightRotation");
				mDetailView->addCallback(rotationHandle, [this](char const*)
				{
					updateLightCamera();
					invalidateShadowMap();
				});
				mDetailView->addValue(mShadowRejectThreshold, "ShadowRejectThreshold");
				mDetailView->addValue(mShadowBias, "ShadowBias");
				mDetailView->addValue(mShadowSlopeBias, "ShadowSlopeBias");
				auto shadowTriRejectHandle = mDetailView->addValue(mShadowTriangleRejectThreshold, "ShadowTriRejectThreshold");
				mDetailView->addCallback(shadowTriRejectHandle, [this](char const*)
				{
					invalidateShadowMap();
				});
				mDetailView->addValue(mBackFaceHeightSnapThreshold, "BackFaceSnapThreshold");
				auto proceduralShadowHandle = mDetailView->addValue(mUseProceduralShadowDepth, "UseProceduralShadowDepth");
				mDetailView->addCallback(proceduralShadowHandle, [this](char const*)
				{
					invalidateShadowMap();
				});
			}

			GTextureShowManager.registerTexture("PlaneHeight", mPlaneTarget.colorTextures[0]);
			GTextureShowManager.registerTexture("DoughnutHeight", mDoughnutTarget.colorTextures[0]);
			GTextureShowManager.registerTexture("DoughnutHeight_Back", mDoughnutTarget.colorTextures[1]);
			GTextureShowManager.registerTexture("IsometricShadowMap", mShadowMapTexture);
			GTextureShowManager.registerTexture("IsometricShadowDepth", mProceduralShadowDepthTexture);
			GTextureShowManager.registerTexture("IsometricShadowDebug", mShadowDebugTexture);
			return true;
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mCaptureView.releaseRHIResource();
			mLightView.releaseRHIResource();
			releaseTarget(mPlaneTarget);
			releaseTarget(mDoughnutTarget);
			mShadowDebugFrameBuffer.release();
			mShadowDebugTexture.release();
			mShadowMapTexture.release();
			mProceduralShadowDepthFrameBuffer.release();
			mProceduralShadowDepthTexture.release();
			mSceneRenderParamsBuffer.releaseResource();
			mProgMeshCapture = nullptr;
			mClearShadowMapCS = nullptr;
			mProjectWorldToShadowCS = nullptr;
			mFixBackFaceTextureCS = nullptr;
			mShadowVisualizeProgram = nullptr;
			mProceduralShadowDepthProgram = nullptr;
			mProgSceneRender = nullptr;

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onRender(float dFrame) override
		{
			(void)dFrame;

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			IntVector2 screenSize = ::Global::GetScreenSize();

			initializeRenderState();

			mShadowMapDirty = true;
			if (mShadowMapDirty)
			{
				mShadowMapDirty = false;
				renderShadowMap(commandList);
			}
			renderScenePass(commandList, screenSize);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			RenderUtility::SetFont(g, FONT_S12);
			g.setTextColor(Color3f(1, 1, 1));
			//g.drawText(Vector2(18, 18), "Shadowed scene from PlaneHeight + DoughnutHeight");
			g.endRender();
		}

	private:
		struct ObjectTransform
		{
			Vector3 pos;
			float   scale;

			Matrix4 toMatrix() const
			{
				return Matrix4::Scale(scale) * Matrix4::Translate(pos);
			}
		};

		static uint32 PackFloatDepth(float value)
		{
			union { float f; uint32 u; } data;
			data.f = value;
			return data.u;
		}

		static Vector3 GetLightUpDir(Vector3 const& dir)
		{
			Vector3 up = dir.cross(Vector3(0, 0, 1)).cross(dir);
			if (up.length2() < 1e-4)
				return Vector3(1, 0, 0);
			return up.getNormal();
		}

		void updateLightCamera()
		{
			Vector3 lightDir = mLightRotation.rotate(Vector3(1, 0, 0));
			if (lightDir.length2() < 1e-4f)
				lightDir = Vector3(1.0f, -1.0f, -2.0f);
			lightDir.normalize();
			mLightDir = lightDir;
			Vector3 lightPos = -16.0f * mLightDir;
			mLightCamera.lookAt(lightPos, Vector3(0, 0, 0), GetLightUpDir(mLightDir));
		}

		void invalidateShadowMap()
		{
			mShadowMapDirty = true;
		}

		bool createRenderTarget(IntVector2 const& size, bool bHaveBackFace, RenderTargetBundle& outTarget)
		{
			CHECK(size.x > 0 && size.y > 0);

			outTarget.size = size;

			VERIFY_RETURN_FALSE(outTarget.colorTextures[0] = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::RGBA16F, outTarget.size.x, outTarget.size.y)
				.Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_DefalutValue)));

			if (bHaveBackFace)
			{
				VERIFY_RETURN_FALSE(outTarget.colorTextures[1] = RHICreateTexture2D(
					TextureDesc::Type2D(ETexture::RGBA16F, outTarget.size.x, outTarget.size.y)
					.Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue)));
			}
			VERIFY_RETURN_FALSE(outTarget.depthTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::D24S8, outTarget.size.x, outTarget.size.y).Flags(TCF_None)));
			VERIFY_RETURN_FALSE(outTarget.frameBuffer = RHICreateFrameBuffer());
			outTarget.frameBuffer->setTexture(0, *outTarget.colorTextures[0]);
			outTarget.frameBuffer->setDepth(*outTarget.depthTexture);
			return true;
		}

		void releaseTarget(RenderTargetBundle& target)
		{
			target.frameBuffer.release();
			target.depthTexture.release();
			target.colorTextures[0].release();
			target.colorTextures[1].release();
		}

		void setupScene()
		{
			mDoughnutTransforms.clear();
			float len = 6;
			addDoughnutTransform(Vector3(0.0f, 0.0f, 2.0f), 2.0f);
			addDoughnutTransform(Vector3(len, 0.0f, 1.0f), 1.0f);
			addDoughnutTransform(Vector3(-len, 0.0f, 1.0f), 1.0f);
			addDoughnutTransform(Vector3(len, len, 1.0f), 1.0f);
			addDoughnutTransform(Vector3(len, -len, 1.0f), 1.0f);
			addDoughnutTransform(Vector3(0.0f, -len, 1.0f), 1.0f);

			mSceneBounds.invalidate();
			accumulateBounds(mPlaneBounds, mPlaneTransform);
			for (ObjectTransform const& transform : mDoughnutTransforms)
			{
				accumulateBounds(mDoughnutBounds, transform.toMatrix());
			}
			mSceneBounds.expand(Vector3(0.5f, 0.5f, 0.5f));
		}

		void addDoughnutTransform(Vector3 const& pos, float scale)
		{
			ObjectTransform transform;
			transform.pos = pos;
			transform.scale = scale;
			mDoughnutTransforms.push_back(transform);
		}

		void accumulateBounds(BoundBox const& localBounds, Matrix4 const& localToWorld)
		{
			Vector3 corners[8] =
			{
				{ localBounds.min.x, localBounds.min.y, localBounds.min.z },
				{ localBounds.max.x, localBounds.min.y, localBounds.min.z },
				{ localBounds.min.x, localBounds.max.y, localBounds.min.z },
				{ localBounds.max.x, localBounds.max.y, localBounds.min.z },
				{ localBounds.min.x, localBounds.min.y, localBounds.max.z },
				{ localBounds.max.x, localBounds.min.y, localBounds.max.z },
				{ localBounds.min.x, localBounds.max.y, localBounds.max.z },
				{ localBounds.max.x, localBounds.max.y, localBounds.max.z },
			};

			for (Vector3 const& corner : corners)
			{
				mSceneBounds += (Vector4(corner, 1.0f) * localToWorld).dividedVector();
			}
		}

		void setupCaptureView(IntVector2 const& rectSize = IntVector2(BaseWorldPosTextureSize, BaseWorldPosTextureSize))
		{
			Vector3 sceneSize = mSceneBounds.getSize();
			float viewExtent = 0.6f * Math::Max(sceneSize.x, sceneSize.y);
			float aspect = float(rectSize.x) / float(Math::Max(rectSize.y, 1));
			float extentX = viewExtent * Math::Max(aspect, 1.0f);
			float extentY = viewExtent / Math::Min(aspect, 1.0f);
			Matrix4 projection = OrthoMatrix(-extentX, extentX, -extentY, extentY, -30.0f, 30.0f);

			mCaptureView.rectOffset = IntVector2(0, 0);
			mCaptureView.rectSize = rectSize;
			mCaptureView.setupTransform(mCaptureCamera.getPos(), mCaptureCamera.getRotation(), projection);
		}

		void setupCaptureViewForBounds(Matrix4 const& xform, BoundBox const& bounds, IntVector2 const& rectSize)
		{
			Matrix4 viewMatrix = mCaptureCamera.getViewMatrix();
			Vector3 minView(1e6f, 1e6f, 1e6f);
			Vector3 maxView(-1e6f, -1e6f, -1e6f);
			Vector3 corners[8] =
			{
				{ bounds.min.x, bounds.min.y, bounds.min.z }, { bounds.max.x, bounds.min.y, bounds.min.z },
				{ bounds.min.x, bounds.max.y, bounds.min.z }, { bounds.max.x, bounds.max.y, bounds.min.z },
				{ bounds.min.x, bounds.min.y, bounds.max.z }, { bounds.max.x, bounds.min.y, bounds.max.z },
				{ bounds.min.x, bounds.max.y, bounds.max.z }, { bounds.max.x, bounds.max.y, bounds.max.z },
			};

			for (Vector3 const& corner : corners)
			{
				Vector3 viewPos = Math::TransformPosition(Math::TransformPosition(corner, xform), viewMatrix);
				minView.x = Math::Min(minView.x, viewPos.x);
				minView.y = Math::Min(minView.y, viewPos.y);
				minView.z = Math::Min(minView.z, viewPos.z);
				maxView.x = Math::Max(maxView.x, viewPos.x);
				maxView.y = Math::Max(maxView.y, viewPos.y);
				maxView.z = Math::Max(maxView.z, viewPos.z);
			}

			Vector2 center(0.5f * (minView.x + maxView.x), 0.5f * (minView.y + maxView.y));
			float extentX = 0.5f * Math::Max(maxView.x - minView.x, 0.01f);
			float extentY = 0.5f * Math::Max(maxView.y - minView.y, 0.01f);
			float aspect = float(rectSize.x) / float(Math::Max(rectSize.y, 1));
			if (extentX / extentY < aspect)
				extentX = extentY * aspect;
			else
				extentY = extentX / aspect;

			extentX *= 1.05f;
			extentY *= 1.05f;

			float zCenter = 0.5f * (minView.z + maxView.z);
			float zExtent = 0.5f * Math::Max(maxView.z - minView.z, 0.01f) + 2.0f;
			Matrix4 projection = OrthoMatrix(center.x - extentX, center.x + extentX, center.y - extentY, center.y + extentY, zCenter - zExtent, zCenter + zExtent);

			mCaptureView.rectOffset = IntVector2(0, 0);
			mCaptureView.rectSize = rectSize;
			mCaptureView.setupTransform(viewMatrix, projection);
		}

		void setupLightView()
		{
			Vector3 sceneSize = mSceneBounds.getSize();
			float viewExtent = 0.6f * Math::Max(sceneSize.x, sceneSize.y);
			Matrix4 projection = OrthoMatrix(-viewExtent, viewExtent, -viewExtent, viewExtent, -40.0f, 40.0f);

			mLightView.rectOffset = IntVector2(0, 0);
			mLightView.rectSize = IntVector2(ShadowTextureSize, ShadowTextureSize);
			mLightView.setupTransform(mLightCamera.getPos(), mLightCamera.getRotation(), projection);
		}

		void calcTextureUVRect(Matrix4 const& xform, BoundBox const& bounds, Vector4& outUVRect)
		{
			float minX = 1e6f;
			float minY = 1e6f;
			float maxX = -1e6f;
			float maxY = -1e6f;
			Vector3 corners[8] =
			{
				{ bounds.min.x, bounds.min.y, bounds.min.z }, { bounds.max.x, bounds.min.y, bounds.min.z },
				{ bounds.min.x, bounds.max.y, bounds.min.z }, { bounds.max.x, bounds.max.y, bounds.min.z },
				{ bounds.min.x, bounds.min.y, bounds.max.z }, { bounds.max.x, bounds.min.y, bounds.max.z },
				{ bounds.min.x, bounds.max.y, bounds.max.z }, { bounds.max.x, bounds.max.y, bounds.max.z },
			};

			for (Vector3 const& corner : corners)
			{
				Vector4 clip = Vector4(Math::TransformPosition(corner, xform), 1.0f) * mCaptureView.worldToClip;
				if (Math::Abs(clip.w) < 1e-6)
					continue;
				Vector3 ndc = clip.dividedVector();
				float u = 0.5f * ndc.x + 0.5f;
				float v = 0.5f - 0.5f * ndc.y;
				minX = Math::Min(minX, u);
				minY = Math::Min(minY, v);
				maxX = Math::Max(maxX, u);
				maxY = Math::Max(maxY, v);
			}

			outUVRect = Vector4(
				Math::Clamp(minX, 0.0f, 1.0f),
				Math::Clamp(minY, 0.0f, 1.0f),
				Math::Clamp(maxX, 0.0f, 1.0f),
				Math::Clamp(maxY, 0.0f, 1.0f));
		}


		IntVector2 calcTargetSize(Matrix4 const& xform, BoundBox const& bounds)
		{
			Vector4 uvRect;
			calcTextureUVRect(xform, bounds, uvRect);
			float extentU = Math::Max(uvRect.z - uvRect.x, 1.0f / BaseWorldPosTextureSize);
			float extentV = Math::Max(uvRect.w - uvRect.y, 1.0f / BaseWorldPosTextureSize);
			int width = Math::Max<int>(Math::CeilToInt(extentU * BaseWorldPosTextureSize), MinWorldPosTextureSize);
			int height = Math::Max<int>(Math::CeilToInt(extentV * BaseWorldPosTextureSize), MinWorldPosTextureSize);
			return IntVector2(width, height);
		}


		bool calcInstanceRect(RenderTargetBundle& meshTarget, ObjectTransform const& xform, float zoom, float outRect[4])
		{
			Vector2 uvMin(meshTarget.uvRect.x, meshTarget.uvRect.y);
			Vector2 uvMax(meshTarget.uvRect.z, meshTarget.uvRect.w);
			Vector2 uvCenter = 0.5f * (uvMin + uvMax);
			Vector2 uvHalfExtent = 0.5f * (uvMax - uvMin);
			if (uvHalfExtent.x <= 0.0f || uvHalfExtent.y <= 0.0f)
				return false;

			auto captureUVToSceneView = [this, &meshTarget](Vector2 const& uv)
			{
				Vector2 ndcXY(2.0f * uv.x - 1.0f, 1.0f - 2.0f * uv.y);
				Vector3 captureWorldPos = (Vector4(ndcXY.x, ndcXY.y, 0.0f, 1.0f) * meshTarget.captureClipToWorld).dividedVector();
				return Math::TransformPosition(captureWorldPos, mCaptureView.worldToView);
			};

			Vector3 localCenterView = captureUVToSceneView(uvCenter);
			Vector3 localRightView = captureUVToSceneView(Vector2(uvCenter.x + uvHalfExtent.x, uvCenter.y));
			Vector3 localDownView = captureUVToSceneView(Vector2(uvCenter.x, uvCenter.y + uvHalfExtent.y));
			Vector3 instanceViewPos = Math::TransformPosition(xform.pos, mCaptureView.worldToView);

			float viewScale = xform.scale * zoom;
			Vector3 centerView(
				instanceViewPos.x + localCenterView.x * viewScale,
				instanceViewPos.y + localCenterView.y * viewScale,
				instanceViewPos.z + localCenterView.z * viewScale);
			float halfViewX = Math::Abs(localRightView.x - localCenterView.x) * viewScale;
			float halfViewY = Math::Abs(localDownView.y - localCenterView.y) * viewScale;

			Vector4 centerClip = Vector4(centerView, 1.0f) * mCaptureView.viewToClip;
			if (Math::Abs(centerClip.w) < 1e-6)
				return false;

			Vector3 centerNDC = centerClip.dividedVector();
			Vector2 screenCenter(
				(0.5f * centerNDC.x + 0.5f) * mCaptureView.rectSize.x,
				(0.5f - 0.5f * centerNDC.y) * mCaptureView.rectSize.y);
			Vector2 screenHalfExtent(
				0.5f * mCaptureView.rectSize.x * Math::Abs(mCaptureView.viewToClip(0, 0)) * halfViewX,
				0.5f * mCaptureView.rectSize.y * Math::Abs(mCaptureView.viewToClip(1, 1)) * halfViewY);

			Vector2 rectMin = screenCenter - screenHalfExtent;
			Vector2 rectMax = screenCenter + screenHalfExtent;

			if (rectMax.x <= 0.0f || rectMax.y <= 0.0f || rectMin.x >= mCaptureView.rectSize.x || rectMin.y >= mCaptureView.rectSize.y)
				return false;

			if (rectMin.x >= rectMax.x || rectMin.y >= rectMax.y)
				return false;

			outRect[0] = rectMin.x;
			outRect[1] = rectMin.y;
			outRect[2] = rectMax.x;
			outRect[3] = rectMax.y;
			return true;
		}

		void beginWorldPosPass(RHICommandList& commandList, RenderTargetBundle& target, Matrix4 const& xform, BoundBox const& bounds, int colorIndex, ECullMode cullMode)
		{
			setupCaptureViewForBounds(xform, bounds, target.size);
			mCaptureView.updateRHIResource();
			target.captureClipToWorld = mCaptureView.clipToWorld;
			target.heightRange = Vector2(bounds.min.z, bounds.max.z);
			target.frameBuffer->setTexture(0, *target.colorTextures[colorIndex]);
			RHISetFrameBuffer(commandList, target.frameBuffer);
			RHISetViewport(commandList, 0, 0, target.size.x, target.size.y);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 0), 1, 1.0f, 0);
			RHISetRasterizerState(commandList, GetStaticRasterizerState(cullMode, EFillMode::Solid));
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetShaderProgram(commandList, mProgMeshCapture->getRHI());
			mCaptureView.setupShader(commandList, *mProgMeshCapture);
			SET_SHADER_PARAM(commandList, *mProgMeshCapture, HeightRange, target.heightRange);
		}

		template< typename MeshRenderFunc >
		void captureMeshObject(RHICommandList& commandList, RenderTargetBundle& meshTarget, Matrix4 const& xform, BoundBox const& bounds, bool bHadBackFace, MeshRenderFunc&& func)
		{
			auto size = calcTargetSize(xform, bounds);
			createRenderTarget(size, bHadBackFace, meshTarget);

			setupCaptureViewForBounds(xform, bounds, meshTarget.size);
			calcTextureUVRect(xform, bounds, meshTarget.uvRect);

			beginWorldPosPass(commandList, meshTarget, xform, bounds, 0, ECullMode::Back);
			SET_SHADER_PARAM(commandList, *mProgMeshCapture, LocalToWorld, xform);
			func(commandList);
			if (bHadBackFace)
			{
				beginWorldPosPass(commandList, meshTarget, xform, bounds, 1, ECullMode::Front);
				SET_SHADER_PARAM(commandList, *mProgMeshCapture, LocalToWorld, xform);
				func(commandList);

				fixBackFaceTexture(commandList, meshTarget);
			}

		}

		void fixBackFaceTexture(RHICommandList& commandList, RenderTargetBundle& meshTarget)
		{
			RHIResourceTransition(commandList, { meshTarget.colorTextures[1] }, EResourceTransition::UAV);

			RHISetComputeShader(commandList, mFixBackFaceTextureCS->getRHI());
			mFixBackFaceTextureCS->setTexture(commandList, SHADER_PARAM(FrontFaceTexture), *meshTarget.colorTextures[0]);
			mFixBackFaceTextureCS->setRWTexture(commandList, SHADER_PARAM(BackFaceTextureRW), *meshTarget.colorTextures[1], 0, EAccessOp::ReadAndWrite);
			SET_SHADER_PARAM(commandList, *mFixBackFaceTextureCS, BackFaceTextureSize, meshTarget.size);
			SET_SHADER_PARAM(commandList, *mFixBackFaceTextureCS, BackFaceHeightSnapThreshold, mBackFaceHeightSnapThreshold);
			RHIDispatchCompute(commandList, (meshTarget.size.x + 7) / 8, (meshTarget.size.y + 7) / 8, 1);
			mFixBackFaceTextureCS->clearRWTexture(commandList, SHADER_PARAM(BackFaceTextureRW));
			mFixBackFaceTextureCS->clearTexture(commandList, SHADER_PARAM(FrontFaceTexture));

			RHIResourceTransition(commandList, { meshTarget.colorTextures[1] }, EResourceTransition::SRV);
		}

		void projectWorldTextureToShadow(RHICommandList& commandList, RHITexture2D& texture, Matrix4 const& transform, Matrix4 const& captureClipToWorld, Vector2 const& heightRange)
		{
			RHISetComputeShader(commandList, mProjectWorldToShadowCS->getRHI());
			mProjectWorldToShadowCS->setTexture(commandList, SHADER_PARAM(WorldPositionTexture), texture);
			mProjectWorldToShadowCS->setRWTexture(commandList, SHADER_PARAM(ShadowMapRW), *mShadowMapTexture, 0, EAccessOp::ReadAndWrite);
			SET_SHADER_PARAM(commandList, *mProjectWorldToShadowCS, WorldPosTransform, transform);
			SET_SHADER_PARAM(commandList, *mProjectWorldToShadowCS, WorldToShadowClip, mLightView.worldToClip);
			SET_SHADER_PARAM(commandList, *mProjectWorldToShadowCS, CaptureClipToWorld, captureClipToWorld);
			SET_SHADER_PARAM(commandList, *mProjectWorldToShadowCS, HeightRange, heightRange);
			SET_SHADER_PARAM(commandList, *mProjectWorldToShadowCS, ShadowTextureSize, IntVector2(ShadowTextureSize, ShadowTextureSize));
			SET_SHADER_PARAM(commandList, *mProjectWorldToShadowCS, ShadowTriangleRejectThreshold, mShadowTriangleRejectThreshold);
			RHIDispatchCompute(commandList, (texture.getSizeX() + 7) / 8, (texture.getSizeY() + 7) / 8, 1);
			mProjectWorldToShadowCS->clearRWTexture(commandList, SHADER_PARAM(ShadowMapRW));
			mProjectWorldToShadowCS->clearTexture(commandList, SHADER_PARAM(WorldPositionTexture));
		}

		void drawProceduralShadowTexture(RHICommandList& commandList, RHITexture2D& texture, Matrix4 const& transform, Matrix4 const& captureClipToWorld, Vector2 const& heightRange)
		{
			int const sizeX = texture.getSizeX();
			int const sizeY = texture.getSizeY();
			if (sizeX < 2 || sizeY < 2)
				return;

			mProceduralShadowDepthProgram->setTexture(commandList, SHADER_PARAM(SourceWorldPosTexture), texture,
				SHADER_SAMPLER(SourceWorldPosTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
			SET_SHADER_PARAM(commandList, *mProceduralShadowDepthProgram, WorldPosTransform, transform);
			SET_SHADER_PARAM(commandList, *mProceduralShadowDepthProgram, WorldToShadowClip, mLightView.worldToClip);
			SET_SHADER_PARAM(commandList, *mProceduralShadowDepthProgram, CaptureClipToWorld, captureClipToWorld);
			SET_SHADER_PARAM(commandList, *mProceduralShadowDepthProgram, HeightRange, heightRange);
			SET_SHADER_PARAM(commandList, *mProceduralShadowDepthProgram, ProceduralTextureSize, IntVector2(sizeX, sizeY));
			RHIDrawPrimitive(commandList, EPrimitive::TriangleList, 0, (sizeX - 1) * (sizeY - 1) * 6);
		}

		void renderProceduralShadowDepthMap(RHICommandList& commandList)
		{
			updateLightCamera();
			setupLightView();
			mLightView.updateRHIResource();

			RHIResourceTransition(commandList, { mProceduralShadowDepthTexture }, EResourceTransition::RenderTarget);
			RHISetFrameBuffer(commandList, mProceduralShadowDepthFrameBuffer);
			RHISetViewport(commandList, 0, 0, ShadowTextureSize, ShadowTextureSize);
			RHIClearRenderTargets(commandList, EClearBits::Depth, nullptr, 0, 1.0f, 0);
			RHISetRasterizerState(commandList, GetStaticRasterizerState(ECullMode::None, EFillMode::Solid));
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<CWM_None>::GetRHI());
			RHISetShaderProgram(commandList, mProceduralShadowDepthProgram->getRHI());
			RHISetInputStream(commandList, nullptr, nullptr, 0);

			drawProceduralShadowTexture(commandList, *mPlaneTarget.colorTextures[0], mPlaneTransform, mPlaneTarget.captureClipToWorld, mPlaneTarget.heightRange);

			for (ObjectTransform const& transform : mDoughnutTransforms)
			{
				Matrix4 const xform = transform.toMatrix();
				drawProceduralShadowTexture(commandList, *mDoughnutTarget.colorTextures[0], xform, mDoughnutTarget.captureClipToWorld, mDoughnutTarget.heightRange);
				drawProceduralShadowTexture(commandList, *mDoughnutTarget.colorTextures[1], xform, mDoughnutTarget.captureClipToWorld, mDoughnutTarget.heightRange);
			}

			mProceduralShadowDepthProgram->clearTexture(commandList, SHADER_PARAM(SourceWorldPosTexture));
			RHIResourceTransition(commandList, { mProceduralShadowDepthTexture }, EResourceTransition::SRV);
		}

		void renderComputeShadowMap(RHICommandList& commandList)
		{
			updateLightCamera();
			setupLightView();
			mLightView.updateRHIResource();

			RHISetFrameBuffer(commandList, nullptr);
			RHISetViewport(commandList, 0, 0, ShadowTextureSize, ShadowTextureSize);
			RHIResourceTransition(commandList, { mShadowMapTexture }, EResourceTransition::UAV);

			RHISetComputeShader(commandList, mClearShadowMapCS->getRHI());
			mClearShadowMapCS->setRWTexture(commandList, SHADER_PARAM(ShadowMapRW), *mShadowMapTexture, 0, EAccessOp::WriteOnly);
			SET_SHADER_PARAM(commandList, *mClearShadowMapCS, ClearDepthValue, int32(PackFloatDepth(1.0f)));
			SET_SHADER_PARAM(commandList, *mClearShadowMapCS, ShadowTextureSize, IntVector2(ShadowTextureSize, ShadowTextureSize));
			RHIDispatchCompute(commandList, (ShadowTextureSize + 7) / 8, (ShadowTextureSize + 7) / 8, 1);
			mClearShadowMapCS->clearRWTexture(commandList, SHADER_PARAM(ShadowMapRW));

			RHIResourceTransition(commandList, { mShadowMapTexture }, EResourceTransition::UAVBarrier);
			projectWorldTextureToShadow(commandList, *mPlaneTarget.colorTextures[0], mPlaneTransform, mPlaneTarget.captureClipToWorld, mPlaneTarget.heightRange);

			for (ObjectTransform const& transform : mDoughnutTransforms)
			{
				Matrix4 const xform = transform.toMatrix();
				RHIResourceTransition(commandList, { mShadowMapTexture }, EResourceTransition::UAVBarrier);
				projectWorldTextureToShadow(commandList, *mDoughnutTarget.colorTextures[0], xform, mDoughnutTarget.captureClipToWorld, mDoughnutTarget.heightRange);

				RHIResourceTransition(commandList, { mShadowMapTexture }, EResourceTransition::UAVBarrier);
				projectWorldTextureToShadow(commandList, *mDoughnutTarget.colorTextures[1], xform, mDoughnutTarget.captureClipToWorld, mDoughnutTarget.heightRange);
			}
			RHIResourceTransition(commandList, { mShadowMapTexture }, EResourceTransition::SRV);
		}

		void renderShadowMap(RHICommandList& commandList)
		{
			GPU_PROFILE("ShadowMap");

			if (mUseProceduralShadowDepth)
			{
				renderProceduralShadowDepthMap(commandList);
				return;
			}

			renderComputeShadowMap(commandList);
		}

		void setSceneShadowTextures(RHICommandList& commandList)
		{
			mProgSceneRender->setTexture(commandList, SHADER_PARAM(SourceShadowTexture), *mShadowMapTexture,
				SHADER_SAMPLER(SourceShadowTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
			mProgSceneRender->setTexture(commandList, SHADER_PARAM(SourceShadowDepthTexture), *mProceduralShadowDepthTexture,
				SHADER_SAMPLER(SourceShadowDepthTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
		}

		void setupSceneRenderParams(RHICommandList& commandList, Matrix4 const& screenXForm)
		{
			if (!mSceneRenderParamsBuffer.isValid())
			{
				mSceneRenderParamsBuffer.initializeResource(1, EStructuredBufferType::Const);
			}

			SceneRenderParams params;
			params.WorldToShadowClip = mLightView.worldToClip;
			params.WorldToClip = mCaptureView.worldToClip;
			params.XForm = screenXForm;
			params.ShadowParams = Vector4(mShadowBias, mShadowSlopeBias, 0.35f, mUseProceduralShadowDepth ? 1.0f : 0.0f);
			mSceneRenderParamsBuffer.updateBuffer(params);
			mProgSceneRender->setStructuredUniformBufferT<SceneRenderParams>(commandList, *mSceneRenderParamsBuffer.getRHI());
		}

		void renderScenePass(RHICommandList& commandList, IntVector2 const& screenSize)
		{
			GPU_PROFILE("RenderScene");

			setupCaptureView(screenSize);
			Matrix4 screenXForm = AdjustProjectionMatrixForRHI(OrthoMatrix(0, screenSize.x, screenSize.y, 0, -1, 1));
			RHISetFrameBuffer(commandList, nullptr);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Depth, &LinearColor(0.02f, 0.02f, 0.025f, 1.0f), 1, 1.0f, 0);
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mProgSceneRender->getRHI());
			setupSceneRenderParams(commandList, screenXForm);
			setSceneShadowTextures(commandList);
			{
				float rect[4];
				if (calcInstanceRect(mPlaneTarget, mPlaneObjectTransform, 1.0f, rect))
				{
					mProgSceneRender->setTexture(commandList, SHADER_PARAM(SourceWorldPosTexture), *mPlaneTarget.colorTextures[0],
						SHADER_SAMPLER(SourceWorldPosTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
					SET_SHADER_PARAM(commandList, *mProgSceneRender, UVRect, mPlaneTarget.uvRect);
					SET_SHADER_PARAM(commandList, *mProgSceneRender, WorldPosTransform, mPlaneTransform);
					SET_SHADER_PARAM(commandList, *mProgSceneRender, CaptureClipToWorld, mPlaneTarget.captureClipToWorld);
					SET_SHADER_PARAM(commandList, *mProgSceneRender, HeightRange, mPlaneTarget.heightRange);
					SET_SHADER_PARAM(commandList, *mProgSceneRender, BaseColor, Vector4(0.72f, 0.66f, 0.48f, 1.0f));
					DrawUtility::Rect(commandList, rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1]);
				}
			}

			for (ObjectTransform const& transform : mDoughnutTransforms)
			{
				float rect[4];
				if (!calcInstanceRect(mDoughnutTarget, transform, 1.0f, rect))
					continue;

				mProgSceneRender->setTexture(commandList, SHADER_PARAM(SourceWorldPosTexture), *mDoughnutTarget.colorTextures[0],
					SHADER_SAMPLER(SourceWorldPosTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
				Matrix4 const xform = transform.toMatrix();
				SET_SHADER_PARAM(commandList, *mProgSceneRender, UVRect, mDoughnutTarget.uvRect);
				SET_SHADER_PARAM(commandList, *mProgSceneRender, WorldPosTransform, xform);
				SET_SHADER_PARAM(commandList, *mProgSceneRender, CaptureClipToWorld, mDoughnutTarget.captureClipToWorld);
				SET_SHADER_PARAM(commandList, *mProgSceneRender, HeightRange, mDoughnutTarget.heightRange);
				SET_SHADER_PARAM(commandList, *mProgSceneRender, BaseColor, Vector4(0.74f, 0.79f, 0.96f, 1.0f));
				DrawUtility::Rect(commandList, rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1]);
			}
		}

	private:
		static int constexpr BaseWorldPosTextureSize = 128;
		static int constexpr MinWorldPosTextureSize = 32;
		static int constexpr ShadowTextureSize = 1024;

		Vector3    mLightDir = Vector3(1.0f, -1.0f, -2.0f).getNormal();
		Quaternion mLightRotation = Quaternion::EulerZYX(Vector3(0.0f, Math::DegToRad(35.264f), Math::DegToRad(45.0f)));
		float      mShadowBias = 0.0045f;
		float      mShadowSlopeBias = 1.5f;
		float      mShadowRejectThreshold = 0.01f;
		float      mShadowTriangleRejectThreshold = 1.5f;
		float      mBackFaceHeightSnapThreshold = 0.03f;
		bool       mUseProceduralShadowDepth = true;

		SimpleCamera mCaptureCamera;
		SimpleCamera mLightCamera;
		ViewInfo     mCaptureView;
		ViewInfo     mLightView;

		Matrix4 mPlaneTransform;
		ObjectTransform mPlaneObjectTransform;
		BoundBox mPlaneBounds;
		Matrix4 mDoughnutTextureTransform;
		TArray<ObjectTransform> mDoughnutTransforms;
		BoundBox mDoughnutBounds;
		BoundBox mSceneBounds;

		MeshCaptureProgram*        mProgMeshCapture = nullptr;
		ClearShadowMapCS*          mClearShadowMapCS = nullptr;
		ProjectWorldToShadowCS*    mProjectWorldToShadowCS = nullptr;
		FixBackFaceTextureCS*      mFixBackFaceTextureCS = nullptr;
		ShadowMapVisualizeProgram* mShadowVisualizeProgram = nullptr;
		ProceduralShadowDepthProgram* mProceduralShadowDepthProgram = nullptr;
		SceneRenderProgram*        mProgSceneRender = nullptr;
		IEditorDetailView*         mDetailView = nullptr;

		RenderTargetBundle mPlaneTarget;
		RenderTargetBundle mDoughnutTarget;

		bool mShadowMapDirty = true;
		RHITexture2DRef   mShadowMapTexture;
		RHITexture2DRef   mShadowDebugTexture;
		RHIFrameBufferRef mShadowDebugFrameBuffer;
		RHITexture2DRef   mProceduralShadowDepthTexture;
		RHIFrameBufferRef mProceduralShadowDepthFrameBuffer;
		TStructuredBuffer<SceneRenderParams> mSceneRenderParamsBuffer;
	};

	REGISTER_STAGE_ENTRY("Isometric Shadow", IsometricShadowStage, EExecGroup::FeatureDev, "Render|Test");
}
