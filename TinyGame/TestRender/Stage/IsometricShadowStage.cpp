#include "Stage/TestRenderStageBase.h"

#include "RHI/RHIGraphics2D.h"
#include "Editor.h"

namespace Render
{
	class IsometricWorldPosProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(IsometricWorldPosProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricWorldPosition"; }
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
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
	};

	class ClearShadowMapCS : public GlobalShader
	{
	public:
		DECLARE_SHADER(ClearShadowMapCS, Global);
		static char const* GetShaderFileName() { return "Shader/Game/IsometricWorldPosition"; }

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
		static char const* GetShaderFileName() { return "Shader/Game/IsometricWorldPosition"; }

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, WorldPositionTexture);
			BIND_SHADER_PARAM(parameterMap, ShadowMapRW);
			BIND_SHADER_PARAM(parameterMap, ShadowDebugRW);
			BIND_SHADER_PARAM(parameterMap, WorldPosTransform);
			BIND_SHADER_PARAM(parameterMap, WorldToShadowClip);
			BIND_SHADER_PARAM(parameterMap, ShadowTextureSize);
		}

		DEFINE_TEXTURE_PARAM(WorldPositionTexture);
		DEFINE_SHADER_PARAM(ShadowMapRW);
		DEFINE_SHADER_PARAM(ShadowDebugRW);
		DEFINE_SHADER_PARAM(WorldPosTransform);
		DEFINE_SHADER_PARAM(WorldToShadowClip);
		DEFINE_SHADER_PARAM(ShadowTextureSize);
	};

	class ShadowMapVisualizeProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(ShadowMapVisualizeProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricWorldPosition"; }
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

	class WorldPosVisualizeProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(WorldPosVisualizeProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricWorldPosition"; }
		static TArrayView<ShaderEntryInfo const> GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex, SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel , SHADER_ENTRY(VisualizeWorldPosPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, SourceWorldPosTexture);
			BIND_SHADER_PARAM(parameterMap, WorldMin);
			BIND_SHADER_PARAM(parameterMap, WorldMax);
			BIND_SHADER_PARAM(parameterMap, UVRect);
		}

		DEFINE_TEXTURE_PARAM(SourceWorldPosTexture);
		DEFINE_SHADER_PARAM(WorldMin);
		DEFINE_SHADER_PARAM(WorldMax);
		DEFINE_SHADER_PARAM(UVRect);
	};

	class WorldPosShadeProgram : public GlobalShaderProgram
	{
	public:
		DECLARE_SHADER_PROGRAM(WorldPosShadeProgram, Global);

		static char const* GetShaderFileName() { return "Shader/Game/IsometricWorldPosition"; }
		static TArrayView<ShaderEntryInfo const> GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex, SHADER_ENTRY(ScreenVS) },
				{ EShader::Pixel , SHADER_ENTRY(ShadeWorldPosPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_TEXTURE_PARAM(parameterMap, SourceWorldPosTexture);
			BIND_TEXTURE_PARAM(parameterMap, SourceShadowTexture);
			BIND_SHADER_PARAM(parameterMap, UVRect);
			BIND_SHADER_PARAM(parameterMap, WorldPosTransform);
			BIND_SHADER_PARAM(parameterMap, WorldToShadowClip);
			BIND_SHADER_PARAM(parameterMap, BaseColor);
			BIND_SHADER_PARAM(parameterMap, ShadowBias);
			BIND_SHADER_PARAM(parameterMap, ShadowRejectThreshold);
		}

		DEFINE_TEXTURE_PARAM(SourceWorldPosTexture);
		DEFINE_TEXTURE_PARAM(SourceShadowTexture);
		DEFINE_SHADER_PARAM(UVRect);
		DEFINE_SHADER_PARAM(WorldPosTransform);
		DEFINE_SHADER_PARAM(WorldToShadowClip);
		DEFINE_SHADER_PARAM(BaseColor);
		DEFINE_SHADER_PARAM(ShadowBias);
		DEFINE_SHADER_PARAM(ShadowRejectThreshold);
	};

	IMPLEMENT_SHADER_PROGRAM(IsometricWorldPosProgram);
	IMPLEMENT_SHADER(ClearShadowMapCS, EShader::Compute, SHADER_ENTRY(ClearShadowMapCS));
	IMPLEMENT_SHADER(ProjectWorldToShadowCS, EShader::Compute, SHADER_ENTRY(ProjectWorldToShadowCS));
	IMPLEMENT_SHADER_PROGRAM(ShadowMapVisualizeProgram);
	IMPLEMENT_SHADER_PROGRAM(WorldPosVisualizeProgram);
	IMPLEMENT_SHADER_PROGRAM(WorldPosShadeProgram);

	class IsometricShadowStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
		using BoundBox = Math::TAABBox<Vector3>;

		struct RenderTargetBundle
		{
			IntVector2       size = IntVector2(0, 0);
			RHITexture2DRef   colorTexture;
			RHITexture2DRef   depthTexture;
			RHIFrameBufferRef frameBuffer;
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

			VERIFY_RETURN_FALSE(mWorldPosProgram = ShaderManager::Get().getGlobalShaderT<IsometricWorldPosProgram>(true));
			VERIFY_RETURN_FALSE(mClearShadowMapCS = ShaderManager::Get().getGlobalShaderT<ClearShadowMapCS>(true));
			VERIFY_RETURN_FALSE(mProjectWorldToShadowCS = ShaderManager::Get().getGlobalShaderT<ProjectWorldToShadowCS>(true));
			VERIFY_RETURN_FALSE(mShadowVisualizeProgram = ShaderManager::Get().getGlobalShaderT<ShadowMapVisualizeProgram>(true));
			VERIFY_RETURN_FALSE(mWorldPosVisualizeProgram = ShaderManager::Get().getGlobalShaderT<WorldPosVisualizeProgram>(true));
			VERIFY_RETURN_FALSE(mWorldPosShadeProgram = ShaderManager::Get().getGlobalShaderT<WorldPosShadeProgram>(true));

			setupScene();
			calcWorldPosTargetSizes();
			VERIFY_RETURN_FALSE(createWorldPosTarget(mPlaneWorldPosTarget));
			VERIFY_RETURN_FALSE(createWorldPosTarget(mDoughnutWorldPosTarget));

			VERIFY_RETURN_FALSE(mShadowMapTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::R32U, ShadowTextureSize, ShadowTextureSize)
				.Flags(TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue)));
			VERIFY_RETURN_FALSE(mShadowDebugTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::RGBA8, ShadowTextureSize, ShadowTextureSize)
				.Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_CreateUAV | TCF_DefalutValue)));
			VERIFY_RETURN_FALSE(mShadowDebugFrameBuffer = RHICreateFrameBuffer());
			mShadowDebugFrameBuffer->setTexture(0, *mShadowDebugTexture);

			mCaptureCamera.lookAt(Vector3(14, 14, 14), Vector3(0, 0, 0), Vector3(0, 0, 1));
			updateLightCamera();

			if (::Global::Editor())
			{
				DetailViewConfig config;
				config.name = "Isometric Shadow";
				mDetailView = ::Global::Editor()->createDetailView(config);
				auto rotationHandle = mDetailView->addValue(mLightRotation, "LightRotation");
				mDetailView->addCallback(rotationHandle, [this](char const*)
				{
					updateLightCamera();
				});
				mDetailView->addValue(mShadowRejectThreshold, "ShadowRejectThreshold");
				mDetailView->addValue(mShadowBias, "ShadowBias");
			}

			GTextureShowManager.registerTexture("PlaneWorldPos", mPlaneWorldPosTarget.colorTexture);
			GTextureShowManager.registerTexture("DoughnutWorldPos", mDoughnutWorldPosTarget.colorTexture);
			GTextureShowManager.registerTexture("IsometricShadowMap", mShadowMapTexture);
			GTextureShowManager.registerTexture("IsometricShadowDebug", mShadowDebugTexture);
			return true;
		}

		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mCaptureView.releaseRHIResource();
			mLightView.releaseRHIResource();
			releaseTarget(mPlaneWorldPosTarget);
			releaseTarget(mDoughnutWorldPosTarget);
			mShadowDebugFrameBuffer.release();
			mShadowDebugTexture.release();
			mShadowMapTexture.release();
			mWorldPosProgram = nullptr;
			mClearShadowMapCS = nullptr;
			mProjectWorldToShadowCS = nullptr;
			mShadowVisualizeProgram = nullptr;
			mWorldPosVisualizeProgram = nullptr;
			mWorldPosShadeProgram = nullptr;

			BaseClass::preShutdownRenderSystem(bReInit);
		}

		void onRender(float dFrame) override
		{
			(void)dFrame;

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			IntVector2 screenSize = ::Global::GetScreenSize();

			initializeRenderState();
			renderPlaneWorldPos(commandList);
			renderDoughnutWorldPos(commandList);
			renderDirectionalShadowMap(commandList);
			renderPreviewPass(commandList, screenSize);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			RenderUtility::SetFont(g, FONT_S12);
			g.setTextColor(Color3f(1, 1, 1));
			g.drawText(Vector2(18, 18), "Shadowed scene from PlaneWorldPos + DoughnutWorldPos");
			g.endRender();
		}

	private:
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

		bool createWorldPosTarget(RenderTargetBundle& outTarget)
		{
			CHECK(outTarget.size.x > 0 && outTarget.size.y > 0);
			VERIFY_RETURN_FALSE(outTarget.colorTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::RGBA16F, outTarget.size.x, outTarget.size.y)
				.Flags(TCF_RenderTarget | TCF_CreateSRV | TCF_DefalutValue)));
			VERIFY_RETURN_FALSE(outTarget.depthTexture = RHICreateTexture2D(
				TextureDesc::Type2D(ETexture::D24S8, outTarget.size.x, outTarget.size.y).Flags(TCF_None)));
			VERIFY_RETURN_FALSE(outTarget.frameBuffer = RHICreateFrameBuffer());
			outTarget.frameBuffer->setTexture(0, *outTarget.colorTexture);
			outTarget.frameBuffer->setDepth(*outTarget.depthTexture);
			return true;
		}

		void releaseTarget(RenderTargetBundle& target)
		{
			target.frameBuffer.release();
			target.depthTexture.release();
			target.colorTexture.release();
		}

		void setupScene()
		{
			mPlaneTransform = Matrix4::Scale(10.0f, 10.0f, 1.0f);
			mPlaneBounds = BoundBox(Vector3(-1, -1, 0), Vector3(1, 1, 0));

			mDoughnutTextureTransform = Matrix4::Translate(Vector3(0, 0, 1.0f));
			mDoughnutBounds = BoundBox(Vector3(-3, -3, -1), Vector3(3, 3, 1));

			mDoughnutTransforms.clear();
			mDoughnutTransforms.push_back(Matrix4::Scale(1.0f, 1.0f, 1.0f) * Matrix4::Translate(Vector3(0.0f, 0.0f, 1.0f)));

			mSceneBounds.invalidate();
			accumulateBounds(mPlaneBounds, mPlaneTransform);
			for (Matrix4 const& xform : mDoughnutTransforms)
			{
				accumulateBounds(mDoughnutBounds, xform);
			}
			mSceneBounds.expand(Vector3(0.5f, 0.5f, 0.5f));
			calcPlaneUVRect();
			calcDoughnutUVRect();
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
			Matrix4 projection = OrthoMatrix(-viewExtent, viewExtent, viewExtent, -viewExtent, -30.0f, 30.0f);

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
				Vector3 viewPos = (Vector4(corner, 1.0f) * xform * viewMatrix).dividedVector();
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
			Matrix4 projection = OrthoMatrix(center.x - extentX, center.x + extentX, center.y + extentY, center.y - extentY, zCenter - zExtent, zCenter + zExtent);

			mCaptureView.rectOffset = IntVector2(0, 0);
			mCaptureView.rectSize = rectSize;
			mCaptureView.setupTransform(viewMatrix, projection);
		}

		void setupLightView()
		{
			Vector3 sceneSize = mSceneBounds.getSize();
			float viewExtent = 0.6f * Math::Max(sceneSize.x, sceneSize.y);
			Matrix4 projection = OrthoMatrix(-viewExtent, viewExtent, viewExtent, -viewExtent, -40.0f, 40.0f);

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
				Vector4 clip = Vector4((Vector4(corner, 1.0f) * xform).dividedVector(), 1.0f) * mCaptureView.worldToClip;
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

		void calcPlaneUVRect()
		{
			mPlaneUVRect = Vector4(0, 0, 1, 1);
		}

		void calcDoughnutUVRect()
		{
			mDoughnutUVRect = Vector4(0, 0, 1, 1);
		}

		IntVector2 calcTargetSize(Matrix4 const& xform, BoundBox const& bounds)
		{
			Vector4 uvRect;
			calcTextureUVRect(xform, bounds, uvRect);
			float extentU = Math::Max(uvRect.z - uvRect.x, 1.0f / BaseWorldPosTextureSize);
			float extentV = Math::Max(uvRect.w - uvRect.y, 1.0f / BaseWorldPosTextureSize);
			int width = Math::Clamp<int>(Math::CeilToInt(extentU * BaseWorldPosTextureSize), MinWorldPosTextureSize, BaseWorldPosTextureSize);
			int height = Math::Clamp<int>(Math::CeilToInt(extentV * BaseWorldPosTextureSize), MinWorldPosTextureSize, BaseWorldPosTextureSize);
			return IntVector2(width, height);
		}

		void calcWorldPosTargetSizes()
		{
			setupCaptureView();
			mPlaneWorldPosTarget.size = calcTargetSize(Matrix4::Identity(), mPlaneBounds);
			mDoughnutWorldPosTarget.size = calcTargetSize(mDoughnutTextureTransform, mDoughnutBounds);
		}

		bool calcInstanceRect(BoundBox const& bounds, Matrix4 const& xform, int width, int height, int outRect[4])
		{
			float minX = 1e6f, minY = 1e6f, maxX = -1e6f, maxY = -1e6f;
			Vector3 corners[8] =
			{
				{ bounds.min.x, bounds.min.y, bounds.min.z }, { bounds.max.x, bounds.min.y, bounds.min.z },
				{ bounds.min.x, bounds.max.y, bounds.min.z }, { bounds.max.x, bounds.max.y, bounds.min.z },
				{ bounds.min.x, bounds.min.y, bounds.max.z }, { bounds.max.x, bounds.min.y, bounds.max.z },
				{ bounds.min.x, bounds.max.y, bounds.max.z }, { bounds.max.x, bounds.max.y, bounds.max.z },
			};

			for (Vector3 const& corner : corners)
			{
				Vector4 clip = Vector4((Vector4(corner, 1.0f) * xform).dividedVector(), 1.0f) * mCaptureView.worldToClip;
				if (Math::Abs(clip.w) < 1e-6)
					continue;
				Vector3 ndc = clip.dividedVector();
				float px = width * (0.5f * ndc.x + 0.5f);
				float py = height * (0.5f - 0.5f * ndc.y);
				minX = Math::Min(minX, px);
				minY = Math::Min(minY, py);
				maxX = Math::Max(maxX, px);
				maxY = Math::Max(maxY, py);
			}

			if (minX >= maxX || minY >= maxY)
				return false;

			outRect[0] = Math::Clamp<int>(int(minX), 0, width - 1);
			outRect[1] = Math::Clamp<int>(int(minY), 0, height - 1);
			outRect[2] = Math::Clamp<int>(int(maxX), outRect[0] + 1, width);
			outRect[3] = Math::Clamp<int>(int(maxY), outRect[1] + 1, height);
			return true;
		}

		void beginWorldPosPass(RHICommandList& commandList, RenderTargetBundle& target, Matrix4 const& xform, BoundBox const& bounds)
		{
			setupCaptureViewForBounds(xform, bounds, target.size);
			mCaptureView.updateRHIResource();

			RHISetFrameBuffer(commandList, target.frameBuffer);
			RHISetViewport(commandList, 0, 0, target.size.x, target.size.y);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0, 0, 0, 0), 1, 1.0f, 0);
			RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetShaderProgram(commandList, mWorldPosProgram->getRHI());
			mCaptureView.setupShader(commandList, *mWorldPosProgram);
		}

		void renderPlaneWorldPos(RHICommandList& commandList)
		{
			beginWorldPosPass(commandList, mPlaneWorldPosTarget, Matrix4::Identity(), mPlaneBounds);
			mWorldPosProgram->setParam(commandList, SHADER_PARAM(LocalToWorld), Matrix4::Identity());
			getMesh(SimpleMeshId::Plane).draw(commandList);
		}

		void renderDoughnutWorldPos(RHICommandList& commandList)
		{
			beginWorldPosPass(commandList, mDoughnutWorldPosTarget, mDoughnutTextureTransform, mDoughnutBounds);
			mWorldPosProgram->setParam(commandList, SHADER_PARAM(LocalToWorld), mDoughnutTextureTransform);
			getMesh(SimpleMeshId::Doughnut).draw(commandList);
		}

		void projectWorldTextureToShadow(RHICommandList& commandList, RHITexture2D& texture, Matrix4 const& transform)
		{
			RHISetComputeShader(commandList, mProjectWorldToShadowCS->getRHI());
			mProjectWorldToShadowCS->setTexture(commandList, SHADER_PARAM(WorldPositionTexture), texture,
				SHADER_SAMPLER(WorldPositionTexture),
				TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
			mProjectWorldToShadowCS->setRWTexture(commandList, SHADER_PARAM(ShadowMapRW), *mShadowMapTexture, 0, EAccessOp::ReadAndWrite);
			mProjectWorldToShadowCS->setRWTexture(commandList, SHADER_PARAM(ShadowDebugRW), *mShadowDebugTexture, 0, EAccessOp::ReadAndWrite);
			mProjectWorldToShadowCS->setParam(commandList, SHADER_PARAM(WorldPosTransform), transform);
			mProjectWorldToShadowCS->setParam(commandList, SHADER_PARAM(WorldToShadowClip), mLightView.worldToClip);
			mProjectWorldToShadowCS->setParam(commandList, SHADER_PARAM(ShadowTextureSize), IntVector2(ShadowTextureSize, ShadowTextureSize));
			RHIDispatchCompute(commandList, (texture.getSizeX() + 7) / 8, (texture.getSizeY() + 7) / 8, 1);
			mProjectWorldToShadowCS->clearRWTexture(commandList, SHADER_PARAM(ShadowMapRW));
			mProjectWorldToShadowCS->clearRWTexture(commandList, SHADER_PARAM(ShadowDebugRW));
			mProjectWorldToShadowCS->clearTexture(commandList, SHADER_PARAM(WorldPositionTexture));
		}

		void renderDirectionalShadowMap(RHICommandList& commandList)
		{
			GPU_PROFILE("ShadowMap");

			updateLightCamera();
			setupLightView();
			mLightView.updateRHIResource();

			RHISetFrameBuffer(commandList, mShadowDebugFrameBuffer);
			RHISetViewport(commandList, 0, 0, ShadowTextureSize, ShadowTextureSize);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);
			RHIResourceTransition(commandList, { mShadowDebugTexture, mShadowMapTexture }, EResourceTransition::UAV);

			RHISetComputeShader(commandList, mClearShadowMapCS->getRHI());
			mClearShadowMapCS->setRWTexture(commandList, SHADER_PARAM(ShadowMapRW), *mShadowMapTexture, 0, EAccessOp::WriteOnly);
			mClearShadowMapCS->setParam(commandList, SHADER_PARAM(ClearDepthValue), int32(PackFloatDepth(1.0f)));
			mClearShadowMapCS->setParam(commandList, SHADER_PARAM(ShadowTextureSize), IntVector2(ShadowTextureSize, ShadowTextureSize));
			RHIDispatchCompute(commandList, (ShadowTextureSize + 7) / 8, (ShadowTextureSize + 7) / 8, 1);
			mClearShadowMapCS->clearRWTexture(commandList, SHADER_PARAM(ShadowMapRW));

			RHIResourceTransition(commandList, { mShadowDebugTexture, mShadowMapTexture }, EResourceTransition::UAVBarrier);
			projectWorldTextureToShadow(commandList, *mPlaneWorldPosTarget.colorTexture, mPlaneTransform);
			for (Matrix4 const& xform : mDoughnutTransforms)
			{
				RHIResourceTransition(commandList, { mShadowDebugTexture, mShadowMapTexture }, EResourceTransition::UAVBarrier);
				projectWorldTextureToShadow(commandList, *mDoughnutWorldPosTarget.colorTexture, xform);
			}
			RHIResourceTransition(commandList, { mShadowDebugTexture, mShadowMapTexture }, EResourceTransition::SRV);
		}

		void renderPreviewPass(RHICommandList& commandList, IntVector2 const& screenSize)
		{
			setupCaptureView();
			RHISetFrameBuffer(commandList, nullptr);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.02f, 0.02f, 0.025f, 1.0f), 1);
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());

			RHISetShaderProgram(commandList, mWorldPosShadeProgram->getRHI());
			{
				int rect[4];
				if (calcInstanceRect(mPlaneBounds, mPlaneTransform, screenSize.x, screenSize.y, rect))
				{
					RHISetViewport(commandList, rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1]);
					mWorldPosShadeProgram->setTexture(commandList, SHADER_PARAM(SourceWorldPosTexture), *mPlaneWorldPosTarget.colorTexture,
						SHADER_SAMPLER(SourceWorldPosTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
					mWorldPosShadeProgram->setTexture(commandList, SHADER_PARAM(SourceShadowTexture), *mShadowMapTexture,
						SHADER_SAMPLER(SourceShadowTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
					mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(UVRect), mPlaneUVRect);
					mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(WorldPosTransform), mPlaneTransform);
					mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(WorldToShadowClip), mLightView.worldToClip);
					mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(BaseColor), Vector4(0.72f, 0.66f, 0.48f, 1.0f));
					mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(ShadowBias), mShadowBias);
					mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(ShadowRejectThreshold), mShadowRejectThreshold);
					DrawUtility::ScreenRect(commandList);
				}
			}

			RHISetBlendState(commandList, TStaticBlendState<CWM_RGBA, EBlend::SrcAlpha, EBlend::OneMinusSrcAlpha>::GetRHI());
			for (Matrix4 const& xform : mDoughnutTransforms)
			{
				int rect[4];
				if (!calcInstanceRect(mDoughnutBounds, xform, screenSize.x, screenSize.y, rect))
					continue;

				RHISetViewport(commandList, rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1]);
				mWorldPosShadeProgram->setTexture(commandList, SHADER_PARAM(SourceWorldPosTexture), *mDoughnutWorldPosTarget.colorTexture,
					SHADER_SAMPLER(SourceWorldPosTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
				mWorldPosShadeProgram->setTexture(commandList, SHADER_PARAM(SourceShadowTexture), *mShadowMapTexture,
					SHADER_SAMPLER(SourceShadowTexture), TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
				mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(UVRect), mDoughnutUVRect);
				mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(WorldPosTransform), xform);
				mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(WorldToShadowClip), mLightView.worldToClip);
				mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(BaseColor), Vector4(0.74f, 0.79f, 0.96f, 1.0f));
				mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(ShadowBias), mShadowBias);
				mWorldPosShadeProgram->setParam(commandList, SHADER_PARAM(ShadowRejectThreshold), mShadowRejectThreshold);
				DrawUtility::ScreenRect(commandList);
			}

			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
		}

	private:
		static int constexpr BaseWorldPosTextureSize = 1024;
		static int constexpr MinWorldPosTextureSize = 128;
		static int constexpr ShadowTextureSize = 1024;

		Vector3    mLightDir = Vector3(1.0f, -1.0f, -2.0f).getNormal();
		Quaternion mLightRotation = Quaternion::EulerZYX(Vector3(Math::DegToRad(35.264f), 0.0f, Math::DegToRad(45.0f)));
		float      mShadowBias = 0.0025f;
		float      mShadowRejectThreshold = 0.01f;

		SimpleCamera mCaptureCamera;
		SimpleCamera mLightCamera;
		ViewInfo     mCaptureView;
		ViewInfo     mLightView;

		Matrix4 mPlaneTransform;
		BoundBox mPlaneBounds;
		Matrix4 mDoughnutTextureTransform;
		TArray<Matrix4> mDoughnutTransforms;
		BoundBox mDoughnutBounds;
		BoundBox mSceneBounds;
		Vector4  mPlaneUVRect = Vector4(0, 0, 1, 1);
		Vector4  mDoughnutUVRect = Vector4(0, 0, 1, 1);

		IsometricWorldPosProgram*  mWorldPosProgram = nullptr;
		ClearShadowMapCS*          mClearShadowMapCS = nullptr;
		ProjectWorldToShadowCS*    mProjectWorldToShadowCS = nullptr;
		ShadowMapVisualizeProgram* mShadowVisualizeProgram = nullptr;
		WorldPosVisualizeProgram*  mWorldPosVisualizeProgram = nullptr;
		WorldPosShadeProgram*      mWorldPosShadeProgram = nullptr;
		IEditorDetailView*         mDetailView = nullptr;

		RenderTargetBundle mPlaneWorldPosTarget;
		RenderTargetBundle mDoughnutWorldPosTarget;

		RHITexture2DRef   mShadowMapTexture;
		RHITexture2DRef   mShadowDebugTexture;
		RHIFrameBufferRef mShadowDebugFrameBuffer;
	};

	REGISTER_STAGE_ENTRY("Isometric Shadow", IsometricShadowStage, EExecGroup::FeatureDev, "Render|Test");
}
